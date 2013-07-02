//=============================================================================

// File       : fc8050_wrapper.c
// Description: 
//
// Revision History:
// Version   Date         Author       Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2009             jaksal
//  1.1.0       2010/12/06       yschoi         add tsif, i2c drivers
//=============================================================================

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>

#include <linux/mutex.h>
#include <linux/spinlock.h>

#include "../../dmb_hw.h"
#include "../../dmb_interface.h"

#include "../tdmb_comdef.h"
#include "../tdmb_dev.h"
#include "../tdmb_test.h"

#ifdef FEATURE_DMB_PMIC_POWER
#include <mach/vreg.h>
#include <mach/mpp.h>
#endif /* FEATURE_DMB_PMIC_POWER */

#ifdef FEATURE_DMB_SET_ANT_PATH
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/pmic8058-regulator.h>
#endif /* FEATURE_DMB_SET_ANT_PATH */

#include "fc8050_wrapper.h"
#include "fc8050_demux.h"
#include "fc8050_isr.h"
#include "fci_types.h"


/*==================================================================*/
/*                      DEFINITION                                  */
/*==================================================================*/
//#define FC8050_IF_RW_TEST
//#define FC8050_READ_ALL_FIC // play시 tsif로 들어온 FIC DATA를 전부 read할 경우
//#define FC8050_VERIFY_TEST //EBI2 (android) FC8050 I/F 검증용

//#define FC8050_TS_TEST
#define FC8050_DOUBLE_BUFFERING
#define FC8050_MSC_BUFFERS        16
#define FC8050_MSC_BUFFER_SIZE    (CH0_BUF_THR + 1)



/*==================================================================*/
/*                      TYPE DEFINITION                             */
/*==================================================================*/
typedef struct {
  uint32 freq;
  uint8  subChId;
  uint8  servicetype;
  uint16 bitrate;
} REGS;

tdmb_bb_int_type g_Int_type;

typedef struct _INT_TYPE
{
  uint8   valid;
  uint32  address;
  uint32  length;
  uint8   subch_id;
  uint8   svc_type;
}INT_TYPE;

INT_TYPE fic_type;

#ifdef FC8050_DOUBLE_BUFFERING
INT_TYPE msc_type[FC8050_MSC_BUFFERS];
int gInputBuffer = 0;
int gOutputBuffer = 0;
#else
INT_TYPE msc_type;
#endif

REGS fci_subch_info;


/*==================================================================*/
/*                      VARIABLE                                    */
/*==================================================================*/
//static int tuner_i2c_addr = 0x61;
//static int bbm_i2c_flag;
static int subChId;
//static uint8 SyncStatus=0;
u8 msc_data[FC8050_MSC_BUFFER_SIZE * FC8050_MSC_BUFFERS];
u8 fc8050_fic_data[188*8*8];

tdmb_bb_service_type serviceType;
//extern rex_crit_sect_type tdmb_tcc_crit_sect;
//extern boolean tdmb_mngr_get_weak_area(void);
uint8 tune_res;
#ifdef FC8050_READ_ALL_FIC
static boolean air_play = FALSE;
#endif /* FC8050_READ_ALL_FIC */
#ifdef FC8050_VERIFY_TEST
int test_cnt;
#endif /* FC8050_VERIFY_TEST */
u16 g_mfIntStatus;
extern tdmb_mode_type dmb_mode;
extern ts_data_type ts_data;

#ifdef FC8050_USE_TSIF
static int g_fci_ch_start = 0;
#ifdef FEATURE_FIT_FRAME_SIZE
extern u32 audio_frame_size;
extern u32 remain_audio_frame;
#endif
#endif

//static DEFINE_MUTEX(tdmb_mutex_lock);
//static DEFINE_SPINLOCK(tdmb_spin_lock);

#ifdef FC8050_TS_TEST
//extern void create_tspacket_anal();
//extern int put_ts_packet(int no, unsigned char* packet, int sz);
/*
 * Max Demux Count
 */
#define MAX_DEMUX           2

/*
 * Sync Byte 0xb8
 */
#define SYNC_BYTE_INVERSION

//#define byte        unsigned char
//#define stream      unsigned char*

struct pid_info {
  //unsigned long count;
  //unsigned long discontinuity;
  short         continuity;
};

struct demux_info {
  struct pid_info  pids[8192];

  unsigned long    ts_packet_c;
  unsigned long    malformed_packet_c;
  unsigned long    tot_scraped_sz;
  unsigned long    packet_no;
};

static struct demux_info demux[MAX_DEMUX];

int g_packet_read_cnt = 0;


static int is_sync(unsigned char* p) {
  int syncword = p[0];
#ifdef SYNC_BYTE_INVERSION
  if(0x47 == syncword || 0xb8 == syncword)
    return 1;
#else
  if(0x47 == syncword)
    return 1;
#endif
  return 0;
}


int put_ts_packet(int no, unsigned char* packet, int sz) {
  unsigned char* p;
  int transport_error_indicator, pid, payload_unit_start_indicator, continuity_counter, last_continuity_counter;
  int i;

  if((sz % 188))
  {
    //PRINTF(NULL, "L");
    //TDMB_MSG_FCI_BB("[%s]\n", __func__);
  }
  else
  {
    for(i = 0; i < sz; i += 188)
    {
      p = packet + i;

      pid = ((p[1] & 0x1f) << 8) + p[2];

      // NULL Packet이면 버림
      if(0x1fff == pid)
      {
        continue;
      }

      if(!is_sync(packet + i))
      {
        //PRINTF(NULL, "Sync Drop\n");
        TDMB_MSG_FCI_BB("[%s] [Sync Drop] Video %d Invalid sync: 0x%02x, Offset: %d, Frame No: %d\n", __func__, no, *(packet + i), i, i / 188);
        //PRINTF(NULL, "   Video %d Invalid sync: 0x%02x, Offset: %d, Frame No: %d\n\r", no, *(packet + i), i, i / 188);
        break;
      }

      // Error Indicator가 설정되면 Packet을 버림
      transport_error_indicator = (p[1] & 0x80) >> 7;
      if(1 == transport_error_indicator)
      {
        //PRINTF(NULL, "Error Indicator\n");
        TDMB_MSG_FCI_BB("[%s] [Error Indicator] Video %d PID 0x%04x: err_ind, Offset: %d, Frame No: %d\n", __func__, no, pid, i, i / 188);
        //PRINTF(NULL, "   Video %d PID 0x%04x: err_ind, Offset: %d, Frame No: %d\n\r", no, pid, i, i / 188);
        continue;
      }

      payload_unit_start_indicator = (p[1] & 0x40) >> 6;

      // Continuity Counter Check
      continuity_counter = p[3] & 0x0f;

      if(demux[no].pids[pid].continuity == -1)
      {
        demux[no].pids[pid].continuity = continuity_counter;
      }
      else
      {
        last_continuity_counter = demux[no].pids[pid].continuity;

        demux[no].pids[pid].continuity = continuity_counter;

        if(((last_continuity_counter + 1) & 0x0f) != continuity_counter)
        {
          //PRINTF(NULL, "Discontinuity\n");
          TDMB_MSG_FCI_BB("[%s] [Discontinuity] Video %d PID 0x%04x: last counter %x ,current %x, Offset: %d,Frame No: %d, start_ind: %d\n", __func__, no, pid, last_continuity_counter, continuity_counter, i, i / 188, payload_unit_start_indicator);
          //PRINTF(NULL, "   Video %d PID 0x%04x: last counter %x ,current %x, Offset: %d,Frame No: %d, start_ind: %d\n\r", no, pid, last_continuity_counter, continuity_counter, i, i / 188, payload_unit_start_indicator);
        }
      }
    }
  }

  return 0;
}

void create_tspacket_anal(void) {
  int n, i;

  for(n = 0; n < MAX_DEMUX; n++)
  {
    memset(&demux[n], 0, sizeof(demux[n]));

    for(i = 0; i < 8192; i++)
    {
      demux[n].pids[i].continuity = -1;
    }
  }
}

void release_tspacket_anal(void) {
  //PRINTF(NULL, "Finish\n\r");
  TDMB_MSG_FCI_BB("[%s] Finish \n", __func__);
}
#endif


#if 0
#define MSB(X) 				(((X) >>8) & 0Xff)
#define LSB(X)  			((X) & 0Xff)
#define BYTESWAP(X) 			((LSB(X)<<8) | MSB(X))


static unsigned short crc16(unsigned char *fibBuffer, int len)
{
  int i, j, k;
  unsigned int sta, din;
  unsigned int crc_tmp=0x0; 
  int crc_buf[16];
  int crc_coff[16] = { // CRC16 CCITT REVERSED
                        0, 0, 0, 0, // 0x0
                        1, 0, 0, 0, // 0x8
                        0, 0, 0, 1, // 0x1
                        0, 0, 0, 1 // 0x1
                      };

  for(j=0; j<16; j++) 
    crc_buf[j] = 0x1;

  for(i=0; i<len; i++)
  {
    sta = fibBuffer[i] & 0xff;

    for(k=7; k>=0; k--)
    {
      din = ((sta >> k) & 0x1) ^ (crc_buf[15] & 0x1);

      for(j=15; j>0; j--) 
        crc_buf[j] = (crc_buf[j-1] & 0x1) ^ ((crc_coff[j-1] * din) & 0x1);

      crc_buf[0] = din;
    }
  }

  crc_tmp = 0;

  for(j=15; j>=0; j--) 
    crc_tmp = (crc_tmp << 1) | (crc_buf[j] & 0x1);

  return ~crc_tmp & 0xffff;
}


int fic_chk_crc(u8 *pFic, u16 length)
{
  int result = 0;
  int i;
  u8 *fic_ptr;
  u16 crc;

  fic_ptr = pFic;

  for(i=0; i<length/32; i++)
  {
    crc = (u16)(*(u16*)(fic_ptr+30));

    if(crc16(fic_ptr,30) == BYTESWAP(crc))
    {
      //crc_good_cnt++;
    }
    else
    {
      TDMB_MSG_FCI_BB("[%s] CRC ERROR!!!!!!!!!!!!!!!!!!!!!!!!: FIB %d\n", __func__, i);
    }

    fic_ptr+=32;
  }

  return result;
  }
#endif



/*==================================================================*/
/*                      FUNCTION PROTOTYPE                          */
/*==================================================================*/

static boolean fc8050_function_register(tdmb_bb_function_table_type *ftable_ptr)
{
  ftable_ptr->tdmb_bb_drv_init          = fc8050_drv_init;
  ftable_ptr->tdmb_bb_power_on          = fc8050_power_on;
  ftable_ptr->tdmb_bb_power_off         = fc8050_power_off;
  ftable_ptr->tdmb_bb_ch_scan_start     = fc8050_ch_scan_start;
  ftable_ptr->tdmb_bb_resync            = fc8050_bb_resync;
  ftable_ptr->tdmb_bb_subch_start       = fc8050_subch_start;
  ftable_ptr->tdmb_bb_read_int          = fc8050_read_int;
  ftable_ptr->tdmb_bb_get_sync_status   = fc8050_get_sync_status;
  ftable_ptr->tdmb_bb_read_fib          = fc8050_read_fib;
  ftable_ptr->tdmb_bb_set_subchannel_info = fc8050_set_subchannel_info;
  ftable_ptr->tdmb_bb_read_msc          = fc8050_read_msc;
  ftable_ptr->tdmb_bb_get_ber           = fc8050_get_ber;
  ftable_ptr->tdmb_bb_ch_stop           = fc8050_stop;
  ftable_ptr->tdmb_bb_ch_test           = fc8050_test;

  return TRUE;
}


int TDMB_FIC_CALLBACK(u32 userdata, u8 *data, int length)
{
  g_Int_type = TDMB_BB_INT_FIC;

  if((fic_type.address==0)&&(fic_type.length==0))
    fic_type.address = (uint32)&fc8050_fic_data[0];

  //fic_chk_crc(data, length);

  memcpy((void*)&fc8050_fic_data[fic_type.length], data, length);


  fic_type.length = length;
  //TDMB_MSG_FCI_BB("[%s] len[%d] total_len[%d] [%x]\n", __func__, length, (int)fic_type.length,(int)fic_type.address);
  //tdmb_set_fic_intr();//tdmb_bb_fic_process();/

  return TRUE;
}


#ifdef FC8050_DOUBLE_BUFFERING
int TDMB_MSC_CALLBACK(u32 userdata, u8 subChId, u8 *data, int length)
{
  //u16 mfoverStatus;

#ifdef FC8050_TS_TEST
  put_ts_packet(0, data, length);
#endif
  g_Int_type = TDMB_BB_INT_MSC;

  if(((gInputBuffer + 1) % FC8050_MSC_BUFFERS) == gOutputBuffer)
  {
    TDMB_MSG_FCI_BB("[%s] BUFFER ALLOCATION ERRROR\n", __func__);
    return 0;
  }

  memcpy((void*)&msc_data[gInputBuffer * FC8050_MSC_BUFFER_SIZE], data, length);

  msc_type[gInputBuffer].length = length;
  msc_type[gInputBuffer].subch_id = subChId;
  msc_type[gInputBuffer].address = (uint32)&msc_data[gInputBuffer * FC8050_MSC_BUFFER_SIZE];

  gInputBuffer = (gInputBuffer + 1) % FC8050_MSC_BUFFERS;

  //TDMB_MSG_FCI_BB("[%s] cnt[%d] total_len[%d] data0[%x] data1[%x]\n", __func__, test_cnt, length, *data, *(data+1));

#if 1//def FC8050_VERIFY_TEST
  if(serviceType == TDMB_BB_SVC_DMB)
  {  
    int i;
    u16 veri_val=0;

    for(i=0;i<length;i+=188)
    {
      if((*(data+i) !=0x47) && !((*(data+i+1))&0x80))
      {
        //TDMB_MSG_FCI_BB("[%s] Buffer reset             !!!\n", __func__);
        TDMB_MSG_FCI_BB("[%s] DATA error !!!  idx[%d]  [%x] [%x]\n", __func__, i/188, *(data+i), *(data+i+1));
        BBM_WORD_READ(NULL, BBM_BUF_ENABLE, &veri_val);
        veri_val &= ~0x01;
        BBM_WORD_WRITE(NULL, BBM_BUF_ENABLE, veri_val);
        veri_val |= 0x01;
        //msleep(1);  
        BBM_WORD_WRITE(NULL, BBM_BUF_ENABLE, veri_val);
        //fc8050_isr_interruptclear();

        break;
      }
    }
  }
#endif /* FC8050_VERIFY_TEST */

  return TRUE;
}

#else

int TDMB_MSC_CALLBACK(u32 userdata, u8 subChId, u8 *data, int length)
{
  //g_Int_type = TDMB_BB_INT_MSC;
  u16 mfoverStatus;
  unsigned long flag;
#ifdef FC8050_VERIFY_TEST
  u8 probe=0;
  u16 veri_val=0;
  u16 verify[5] = {0xf000, 0x0f00, 0x00f0, 0x000f, 0xffff};
  static u8 cnt=0;
#endif /* FC8050_VERIFY_TEST */

  test_cnt++;
#ifdef FC8050_VERIFY_TEST  
  probe = BBM_PROBE(NULL);

  BBM_WORD_WRITE(NULL, 0x04, verify[test_cnt%5]);
  BBM_WORD_READ(NULL, 0x04, &veri_val);

  if(probe ||(verify[test_cnt%5]!=veri_val))
  {
    TDMB_MSG_FCI_BB("[%s] MSC verify error probe[%d] verify[%x] veri_val[%x] \n", __func__, probe, verify[test_cnt%5], veri_val);
  }
  BBM_WORD_READ(NULL, BBM_BUF_OVERRUN, &mfoverStatus);
  BBM_WORD_WRITE(NULL, BBM_BUF_OVERRUN, mfoverStatus);
  BBM_WORD_WRITE(NULL, BBM_BUF_OVERRUN, 0x0000);

  if(mfoverStatus& 0x01)
  {
    TDMB_MSG_FCI_BB("[%s]                            overun[%x]  \n", __func__, mfoverStatus);
  }
#endif /* FC8050_VERIFY_TEST */

  if((*data !=0x47)&&(!(*(data+1)&0x80)))
  {
    if(++cnt>3)
    {
      TDMB_MSG_FCI_BB("[%s] DATA error !!!   [%x]  [%x]\n", __func__, *data, *(data+1));
      BBM_WORD_READ(NULL, BBM_BUF_ENABLE, &veri_val);
      veri_val &= ~0x01;
      BBM_WORD_WRITE(NULL, BBM_BUF_ENABLE, veri_val);
      veri_val |= 0x01;
      //msleep(1);  
      BBM_WORD_WRITE(NULL, BBM_BUF_ENABLE, veri_val);
      cnt=0;
    }
  }

  //local_irq_save(flag);


  if((msc_type.address==0)&&(msc_type.length==0))
    msc_type.address = (uint32)&msc_data[0];
  memcpy((void*)&msc_data[msc_type.length], data, length);
  //local_irq_restore(flag);

  msc_type.length = length;
  //TDMB_MSG_FCI_BB("[%s] len[%d] total_len[%d] data0[%x] data1[%x]\n", __func__, test_cnt, msc_type.length, msc_data[msc_type.length-length], msc_data[msc_type.length-length+1]);
  return TRUE;
}
#endif /* FC8050_DOUBLE_BUFFERING */

/*====================================================================
FUNCTION       tdmb_bb_fc8050_init  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean tdmb_bb_fc8050_init(tdmb_bb_function_table_type *function_table_ptr)
{
    boolean bb_initialized;

    bb_initialized = fc8050_function_register(function_table_ptr);

    return bb_initialized;
}

/*====================================================================
FUNCTION       msWait  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void msWait(unsigned long ms)
{
  msleep(ms);
}


/*====================================================================
FUNCTION       FC8050_OVERRUN_CHECK  
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int FC8050_OVERRUN_CHECK(u8 mask)
{
  u16 mfoverStatus;

  BBM_WORD_READ(NULL, BBM_BUF_OVERRUN, &mfoverStatus);

  if(mfoverStatus&mask)
  {
    BBM_WORD_WRITE(NULL, BBM_BUF_OVERRUN, mfoverStatus);
    BBM_WORD_WRITE(NULL, BBM_BUF_OVERRUN, 0x0000);
    return TRUE;
  }

  return FALSE;	//is not overrun
}


/*====================================================================
FUNCTION       fc8050_isr_interruptclear  
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_isr_interruptclear(void)
{
  u8	extIntStatus = 0;
  HANDLE hDevice = NULL;

  BBM_BYTE_READ(hDevice, BBM_COM_INT_STATUS, &extIntStatus);
  BBM_BYTE_WRITE(hDevice, BBM_COM_INT_STATUS, extIntStatus);
  BBM_BYTE_WRITE(hDevice, BBM_COM_INT_STATUS, 0x00);
}


/*====================================================================
FUNCTION       FC8050_BUFFER_RESET  
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void FC8050_BUFFER_RESET(u8 mask)
{
  //TDMB_MSG_FCI_BB("[%s]\n", __func__);

  if(FC8050_OVERRUN_CHECK(mask))
  {
    TDMB_MSG_FCI_BB("[%s] FC8050 Overrun~ !!!\n", __func__);

#if 0
    {
    u16 veri_val = 0;

    BBM_WORD_READ(NULL, BBM_BUF_ENABLE, &veri_val);
    veri_val &= ~mask;
    BBM_WORD_WRITE(NULL, BBM_BUF_ENABLE, veri_val);
    veri_val |= mask; 
    BBM_WORD_WRITE(NULL, BBM_BUF_ENABLE, veri_val);
    }
#endif

    fc8050_isr_interruptclear();
  }
}


/*====================================================================
FUNCTION       fc8050_i2c_write_byte  
DESCRIPTION  8bit reg. / 8bit data
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_i2c_write_byte(uint8 chipid, uint8 reg, uint8 *data, uint16 length)
{
  uint8 ret = 0;

  //TDMB_MSG_FCI_BB("[%s]\n", __func__);

#ifdef FEATURE_DMB_I2C_CMD
  ret = dmb_i2c_write(chipid, reg, sizeof(uint8), *data, length);
  //ret = dmb_i2c_write_len(chipid, reg, 1, data, length);
#endif

  return ret;
}

/*====================================================================
FUNCTION       fc8050_i2c_write_word  
DESCRIPTION  16bit reg. / 16bit data
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_i2c_write_word(uint8 chipid, uint16 reg, uint16 *data, uint16 length)
{
  uint8 ret = 0;

  //TDMB_MSG_FCI_BB("[%s]\n", __func__);

#ifdef FEATURE_DMB_I2C_CMD
  ret = dmb_i2c_write(chipid, reg, sizeof(uint16), *data, length);
  //ret = dmb_i2c_write_word(chipid, reg, 2, data, length);
#endif

  return ret;
}

/*====================================================================
FUNCTION       fc8050_i2c_write_len  
DESCRIPTION  8bit reg. / N-bit data
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_i2c_write_len(uint8 chipid, uint8 reg, uint8 *data, uint16 length)
{
  uint8 ret = 0;
  uint16 w_data;

  w_data = ((*(data+1)<<8) |*data);
#ifdef FEATURE_DMB_I2C_CMD
  ret = dmb_i2c_write(chipid, reg, sizeof(uint8), w_data, length);
  //ret = dmb_i2c_write_len(chipid, reg, 1, data, length);
#endif

  return ret;
}

/*====================================================================
FUNCTION       fc8050_i2c_read_byte
DESCRIPTION  8bit reg. / 8bit data
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_i2c_read_byte(uint8 chipid, uint8 reg, uint8 *data, uint16 length)
{
  uint8 ret = 0;

  //TDMB_MSG_FCI_BB("[%s]\n", __func__);

#ifdef FEATURE_DMB_I2C_CMD
  ret = dmb_i2c_read(chipid, reg, sizeof(uint8), (uint16*)data, length);
  //ret = dmb_i2c_read_len(chipid, reg, 1, data, length);
#endif

  return ret;
}

/*====================================================================
FUNCTION       fc8050_i2c_read_word  
DESCRIPTION  16bit reg. / 16bit data
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_i2c_read_word(uint8 chipid, uint16 reg, uint16 *data, uint16 length)
{
  uint8 ret = 0;

  //TDMB_MSG_FCI_BB("[%s]\n", __func__);

#ifdef FEATURE_DMB_I2C_CMD
  ret = dmb_i2c_read(chipid, reg, sizeof(uint16), data, length);
  //ret = dmb_i2c_read_word(chipid, reg, 2, data, length);
#endif

  return ret;
}

/*====================================================================
FUNCTION       fc8050_i2c_read_len
DESCRIPTION  8bit reg. / N-bit data
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_i2c_read_len(uint8 chipid, uint8 reg, uint8 *buf_ptr, uint16 length)
{
  uint8 ret = 0;

  //TDMB_MSG_FCI_BB("[%s]\n", __func__);

#ifdef FEATURE_DMB_I2C_CMD
  dmb_i2c_read_len(chipid, reg, buf_ptr, length, sizeof(uint8));
  //ret = dmb_i2c_read_len(chipid, reg, 1, buf_ptr, length);
#endif

  return ret;
}

#if defined(FC8050_IF_RW_TEST) || defined(FEATURE_COMMAND_TEST_ON_BOOT)
/*====================================================================
FUNCTION       fc8050_if_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_if_test(uint8 test_id)
{
  uint8 data, temp;
  uint16 wdata,i = 0;
  unsigned int ldata;

  TDMB_MSG_FCI_BB("[%s] start!!!\n", __func__);

#ifdef FEATURE_DMB_I2C_CMD
  if(BBM_HOSTIF_SELECT(NULL, BBM_I2C))
#elif defined(FEATURE_DMB_SPI_IF)
  if(BBM_HOSTIF_SELECT(NULL, BBM_SPI))
#endif
  {
    return;
  }

  switch(test_id)
  {
    case 1:
      TDMB_MSG_FCI_BB("[%s] byte R/W test\n", __func__);
      //while(1)
      for(i=0;i<100;i++)
      {
        BBM_WRITE(NULL, 0x05, i & 0xff);
        BBM_READ(NULL, 0x05, &data);
        if((i & 0xff) != data)
          TDMB_MSG_FCI_BB("[%s] byte R/W test err !![0x%x,0x%x]\n", __func__, i, data);
        //i++;
      }
    break;
    
    case 2:
      TDMB_MSG_FCI_BB("[%s] word R/W test\n", __func__);
      for(i=0;i<100;i++)
      {
        BBM_WORD_WRITE(NULL, 0x0210, i & 0xffff);//TDMB_MSG_FCI_BB("word W [%x][%x]\n",0x0210,i);
        BBM_WORD_READ(NULL, 0x0210, &wdata); //TDMB_MSG_FCI_BB("word R [%x][%x]\n",0x0210,wdata);
        if((i & 0xffff) != wdata)
          TDMB_MSG_FCI_BB("[%s] word R/W test err !![0x%x,0x%x]\n", __func__, i, wdata);
      }
    break;

    case 3:
      TDMB_MSG_FCI_BB("[%s] long R/W test\n", __func__);
      for(i=0;i<100;i++)
      {
        BBM_LONG_WRITE(NULL, 0x0210, i & 0xffffffff); //TDMB_MSG_FCI_BB("long W [%x][%x]\n",0x0210,i);
        BBM_LONG_READ(NULL, 0x0210, &ldata); //TDMB_MSG_FCI_BB("long R [%x][%x]\n",0x0210,ldata);
        if((i & 0xffffffff) != ldata)
          TDMB_MSG_FCI_BB("[%s] long R/W test err !![0x%x,0x%x]\n", __func__, i, ldata);
      }
    break;

    case 4:
      TDMB_MSG_FCI_BB("[%s] tuner R/W test\n", __func__);
      for(i=0;i<100;i++)
      {
        temp = i&0xff;
        BBM_TUNER_WRITE(NULL, 0x12, 0x01, &temp, 0x01);
        BBM_TUNER_READ(NULL, 0x12, 0x01, &data, 0x01);
        if((i & 0xff) != data)
          TDMB_MSG_FCI_BB("[%s] tuner R/W test err !![0x%x,0x%x]\n", __func__, i, data);
      }
    break;
    
    default:
    break;
  }
}
#endif /* FC8050_I2C_RW_TEST */


/*====================================================================
FUNCTION       fc8050_power_on
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_power_on(void)
{
  TDMB_MSG_FCI_BB("[%s] start!!!\n", __func__);

  dmb_power_on();


  dmb_power_on_chip();

  dmb_set_gpio(DMB_RESET, 1);
  msleep(10);

  dmb_set_gpio(DMB_RESET, 0);
  msleep(10);

  dmb_set_gpio(DMB_RESET, 1);
}

/*====================================================================
FUNCTION       fc8050_power_off
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_power_off(void)
{
  TDMB_MSG_FCI_BB("[%s] start!!!\n", __func__);

  //dmb_set_gpio(DMB_RESET, 0);
  //msleep(1);

  dmb_power_off();
}


/*====================================================================
FUNCTION       fc8050_control_fic
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int8 fc8050_control_fic(uint8 enable, uint8 mode) //mode : 0(ts_mode), 1(i2c_mode)
{
  unsigned short mask;
#ifdef FC8050_USE_TSIF
#ifndef FC8050_USE_QUP_I2C
  unsigned char lmode;
#endif
#endif /* FC8050_USE_TSIF */
  
  BBM_WORD_READ(NULL, BBM_BUF_INT, &mask);
  
  if(enable == 1)
  {
#ifdef FC8050_USE_TSIF
    if(mode == 1)
#endif
    {
      BBM_WRITE(NULL, BBM_COM_INT_ENABLE, 1);
      BBM_WRITE(NULL, BBM_COM_STATUS_ENABLE, 1);
    }
    mask |= 0x100;
  }
  else
  {
    BBM_WRITE(NULL, BBM_COM_INT_ENABLE, 0);
    BBM_WRITE(NULL, BBM_COM_STATUS_ENABLE, 0);
    mask &= ~0x100;
  }
  
  BBM_WORD_WRITE(NULL, BBM_BUF_INT, mask);
  BBM_WORD_WRITE(NULL, BBM_BUF_ENABLE, mask);

#ifdef FC8050_USE_TSIF
#ifndef FC8050_USE_QUP_I2C
  BBM_READ(NULL, BBM_TS_SELECT, &lmode);
  
  if(mode == 1)
  {
    lmode &= ~0x40;
    //BBM_WRITE(NULL, BBM_COM_INT_ENABLE, 1);
    //BBM_WRITE(NULL, BBM_COM_STATUS_ENABLE, 1);
  }
  else
  {
    lmode |= 0x40;
    //BBM_WRITE(NULL, BBM_COM_INT_ENABLE, 0);
    //BBM_WRITE(NULL, BBM_COM_STATUS_ENABLE, 0);
  }
  
  BBM_WRITE(NULL, BBM_TS_SELECT, lmode);
#endif
#endif /* FC8050_USE_TSIF */

  return TRUE;
}

/*====================================================================
FUNCTION       fc8050_set_int
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_set_int(uint8 enable)
{
  TDMB_MSG_FCI_BB("[%s] !!!\n", __func__);

  if(enable == 1)
  {
    BBM_WRITE(NULL, BBM_COM_INT_ENABLE, 1);
    BBM_WRITE(NULL, BBM_COM_STATUS_ENABLE, 1);
  }
  else
  {
    BBM_WRITE(NULL, BBM_COM_INT_ENABLE, 0);
    BBM_WRITE(NULL, BBM_COM_STATUS_ENABLE, 0);
  }
}

/*====================================================================
FUNCTION       fc8050_get_fic
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int8 fc8050_get_fic(uint8* buffer, uint32* buffer_size)
{
  HANDLE hDevice = NULL;
  u16      mfIntStatus = 0;
  u16      size;
  int i;
  //unsigned short mask;

  if(buffer==NULL)
    return FALSE;

  for(i=0;i<20;i++)
  {
    BBM_WORD_READ(hDevice, BBM_BUF_STATUS, &mfIntStatus);

    if(mfIntStatus)
    break;

    msWait(5);
  }

  if(mfIntStatus == 0)
    return FALSE;

  BBM_WORD_WRITE(hDevice, BBM_BUF_STATUS, mfIntStatus);
  BBM_WORD_WRITE(hDevice, BBM_BUF_STATUS, 0x0000);

  if(mfIntStatus & 0x0100) 
  {
    BBM_WORD_READ(hDevice, BBM_BUF_FIC_THR, &size);
    size += 1;
    if(size-1) 
    {
      BBM_DATA(hDevice, BBM_COM_FIC_DATA, buffer, size);
      *buffer_size=size;
    }
  }

  if(mfIntStatus & 0x0100)
    return TRUE;
  else
    return FALSE;
}

/*====================================================================
FUNCTION       fc8050_ch_scan_start
DESCRIPTION 
    if for_air is greater than 0, this is for air play.
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_ch_scan_start(int freq, int band, unsigned char for_air)
{

  TDMB_MSG_FCI_BB("[%s] Channel Frequency[%d] \n", __func__, freq);

  //fc8050_control_fic(1, 1);
  fc8050_control_fic(0, 1);

  if(BBM_TUNER_SET_FREQ(NULL, freq)){
    tune_res = BBM_NOK;
    return;
  }

  tune_res = BBM_SCAN_STATUS(NULL);

  //if(!tune_res)
    fc8050_control_fic(1, 1);

  TDMB_MSG_FCI_BB("[%s] tune_res[%d]\n", __func__, tune_res);
}

/*====================================================================
FUNCTION       fc8050_drv_init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_drv_init(void)
{
  int res = BBM_NOK;

  TDMB_MSG_FCI_BB("[%s] %d \n", __func__, FC8050_VIDEO_BUF_NUM);

#if defined(FC8050_USE_I2C)
  if(BBM_HOSTIF_SELECT(NULL, BBM_I2C))
#elif defined(FC8050_USE_EBI2)
  if(BBM_HOSTIF_SELECT(NULL, BBM_PPI))
#elif defined(FC8050_USE_SPI)
  if(BBM_HOSTIF_SELECT(NULL, BBM_SPI))
#endif
  {
    TDMB_MSG_FCI_BB("[%s] hostif select fail!!! \n", __func__);
    return FALSE;
  }

  BBM_FIC_CALLBACK_REGISTER(0, TDMB_FIC_CALLBACK);
  BBM_MSC_CALLBACK_REGISTER(0, TDMB_MSC_CALLBACK);

  res=BBM_INIT(NULL);
  res|=BBM_TUNER_SELECT(NULL, FC8050_TUNER, BAND3_TYPE);

#ifdef FC8050_USE_TSIF
  BBM_WRITE(NULL, BBM_BUF_STS_CTRL, 0xc8);
#endif

#ifdef FC8050_IF_RW_TEST   
  fc8050_if_test(1); // byte
  fc8050_if_test(2); // word
  fc8050_if_test(3); // long
  fc8050_if_test(4); // tuner
#endif /*  FC8050_I2C_RW_TEST */

  if(res)
    return FALSE;

#ifdef FC8050_USE_TSIF
  fc8050_demux_init();
  g_fci_ch_start = 0;
#endif

#ifdef FC8050_DOUBLE_BUFFERING
  gInputBuffer = 0;
  gOutputBuffer = 0;

  memset((void*)&msc_type[0], 0x00, sizeof(msc_type));
#else
  memset((void*)&msc_type, 0x00, sizeof(INT_TYPE));
#endif
  memset((void*)&fic_type, 0x00, sizeof(INT_TYPE));

#ifdef FC8050_TS_TEST
  create_tspacket_anal();
#endif

  TDMB_MSG_FCI_BB("[%s] end \n", __func__);

  return TRUE;
}

/*====================================================================
FUNCTION       fc8050_bb_resync
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_bb_resync(unsigned char imr)
{
}

/*====================================================================
FUNCTION       fc8050_stop
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_stop(void)
{
  int res=BBM_NOK;

  TDMB_MSG_FCI_BB("[%s] \n", __func__);

  if(serviceType == TDMB_BB_SVC_DAB)
  {
    res=BBM_AUDIO_DESELECT(NULL, 0, 3);
#ifdef FC8050_USE_TSIF
    fc8050_demux_deselect_channel( subChId, 3);
#endif
  }
  else if(serviceType == TDMB_BB_SVC_DMB)
  {
    res=BBM_VIDEO_DESELECT(NULL, 0, 0, 0);
#ifdef FC8050_USE_TSIF
    fc8050_demux_deselect_video( subChId, 0);
#endif
  }
  else if(serviceType == TDMB_BB_SVC_DATA) 
  {
    res=BBM_DATA_DESELECT(NULL, 0, 2);
#ifdef FC8050_USE_TSIF
    fc8050_demux_deselect_channel( subChId, 2);
#endif
  }
  else 
  {
    return FALSE;
  }

  if(res)
    return FALSE;

#ifdef FC8050_READ_ALL_FIC
  air_play = FALSE;
#endif /* FC8050_READ_ALL_FIC */

#ifdef FC8050_USE_TSIF
  fc8050_demux_init();
  g_fci_ch_start = 0;
#endif

#ifdef FC8050_DOUBLE_BUFFERING
  gInputBuffer = 0;
  gOutputBuffer = 0;

  memset((void*)&msc_type[0], 0x00, sizeof(msc_type));
#else
  memset((void*)&msc_type, 0x00, sizeof(INT_TYPE));
#endif
  memset((void*)&fic_type, 0x00, sizeof(INT_TYPE));

  TDMB_MSG_FCI_BB("[%s] end!\n", __func__);

  return TRUE;
}

/*====================================================================
FUNCTION       fc8050_subch_start
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int fc8050_subch_start(uint8 *regs, uint32 data_rate)
{ 
  uint32 freq=0;
  REGS* p_regs;
  int res=BBM_NOK;
  int fic_mode = 0;
  p_regs = &fci_subch_info;

#ifdef FC8050_USE_TSIF
  g_fci_ch_start = 1;
#endif

  //#ifdef FC8050_USE_I2C
  //  fic_mode = 1;
  //#endif

  fc8050_stop();
  
  if(dmb_mode == TDMB_MODE_NETBER)
  {
    fc8050_control_fic(0, fic_mode); // FIC OFF
  }
  else
  {
#ifdef FEATURE_TDMB_MULTI_CHANNEL_ENABLE
    fc8050_control_fic(1, fic_mode); // FIC ON
    //fc8050_control_fic(0, fic_mode); // FIC OFF
#else
    fc8050_control_fic(0, fic_mode); // FIC OFF
#endif
  }

  freq = p_regs->freq;
  subChId = p_regs->subChId;
  serviceType = p_regs->servicetype;
  TDMB_MSG_FCI_BB("[%s] subchid[%x] type[%x] freq[%d]\n", __func__, subChId, serviceType, (int)freq);
  //BBM_WRITE(0x25d, 0x0);

#if 0//def FC8050_USE_EBI2 //for android , EBI2
  BBM_READ(NULL, BBM_COM_INT_STATUS, &extIntStatus);
  BBM_WRITE(NULL, BBM_COM_INT_STATUS, extIntStatus);
  BBM_WRITE(NULL, BBM_COM_INT_STATUS, 0x00);
#endif /* FC8050_USE_EBI2 */

#ifdef FC8050_VERIFY_TEST
  test_cnt = 0;
#endif /* FC8050_VERIFY_TEST */

  res = BBM_TUNER_SET_FREQ(NULL, freq);

  //if(res)
    //return FALSE;

  if(serviceType == TDMB_BB_SVC_DAB)
  {
    //TDMB_MSG_FCI_BB("[%s] bitrate[%d]", __func__, fci_subch_info.bitrate);
#ifdef FC8050_USE_TSIF
#ifdef FEATURE_FIT_FRAME_SIZE
    BBM_WORD_WRITE(NULL, BBM_BUF_CH3_END, CH3_BUF_START + (184*TSIF_CHUNK_SIZE*2)-1);
    BBM_WORD_WRITE(NULL, BBM_BUF_CH3_THR, (184*TSIF_CHUNK_SIZE)-1);
    remain_audio_frame=0;
    audio_frame_size=fci_subch_info.bitrate*3;
#else
    BBM_WORD_WRITE(NULL, BBM_BUF_CH3_END, CH3_BUF_START + (fci_subch_info.bitrate*12*2)-1);
    BBM_WORD_WRITE(NULL, BBM_BUF_CH3_THR, (fci_subch_info.bitrate*6*2)-1);
#endif
#else  
    BBM_WORD_WRITE(NULL, BBM_BUF_CH3_END, CH3_BUF_START + (fci_subch_info.bitrate*12*2)-1);
    BBM_WORD_WRITE(NULL, BBM_BUF_CH3_THR, (fci_subch_info.bitrate*6*2)-1);
#endif
    res=BBM_AUDIO_SELECT(NULL, subChId, 3);
#ifdef FC8050_USE_TSIF
    fc8050_demux_select_channel(subChId, 3);
#endif
  }
  else if(serviceType == TDMB_BB_SVC_DMB)
  {
    res=BBM_VIDEO_SELECT(NULL, subChId, 0, 0);
#ifdef FC8050_USE_TSIF
    fc8050_demux_select_video(subChId, 0);
#endif
  }
  else if(serviceType == TDMB_BB_SVC_DATA)
  {
    res=BBM_DATA_SELECT(NULL, subChId, 2);
#ifdef FC8050_USE_TSIF
    fc8050_demux_select_channel(subChId, 2);
#endif
  }

  if(res)
    return FALSE;

#ifdef FC8050_READ_ALL_FIC
  air_play = TRUE;
#endif /* FC8050_READ_ALL_FIC */

  return TRUE;
}

/*====================================================================
FUNCTION       fc8050_read_int
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
#if 0//def FC8050_USE_TSIF
tdmb_bb_int_type fc8050_read_int(u8* data, u32 length)
#else
tdmb_bb_int_type fc8050_read_int(void)
#endif
{
#if 1//def FC8050_USE_TSIF
  //TDMB_MSG_FCI_BB("[%s]\n", __func__);

#ifdef FC8050_USE_TSIF
  if(g_fci_ch_start)
  {
    TDMB_MSG_FCI_BB("Return ISR [%d]\n",g_fci_ch_start);
    return g_Int_type;
  }
#endif

  fc8050_isr(NULL);
  return g_Int_type;
#else
  tdmb_bb_int_type interrupt = TDMB_BB_INT_MAX;
  uint8 extIntStatus;
  //uint16  mfIntStatus;
  HANDLE hDevice = NULL;

  BBM_WRITE(hDevice, BBM_COM_INT_ENABLE, 0);

  BBM_READ(hDevice, BBM_COM_INT_STATUS, &extIntStatus);
  BBM_WRITE(hDevice, BBM_COM_INT_STATUS, extIntStatus);
  BBM_WRITE(hDevice, BBM_COM_INT_STATUS, 0x00);

  BBM_WORD_READ(hDevice, BBM_BUF_STATUS, &g_mfIntStatus);
  BBM_WORD_WRITE(hDevice, BBM_BUF_STATUS, g_mfIntStatus);
  BBM_WORD_WRITE(hDevice, BBM_BUF_STATUS, 0x0000);

  BBM_WRITE(hDevice, BBM_COM_INT_ENABLE, ENABLE_INT_MASK);

  return interrupt;       
#endif
}

/*====================================================================
FUNCTION       fc8050_get_sync_status
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tBBStatus fc8050_get_sync_status(void)
{
  uint8  sync_status = 0;

  BBM_READ(NULL, BBM_SYNC_STATUS, &sync_status);

  if(sync_status & 0x20)
  {
    TDMB_MSG_FCI_BB("[%s] SYNC SUCESS\n", __func__);
    return BB_SUCCESS;
  }
  else
  {
    TDMB_MSG_FCI_BB("[%s] SYNC FAIL\n", __func__);
    return BB_FAIL;
  }
}

/*====================================================================
FUNCTION       fc8050_read_fib
DESCRIPTION  FOR android EBI2
DEPENDENCIES
RETURN VALUE number of FIB
SIDE EFFECTS
======================================================================*/
int fc8050_read_fib2(uint8 *fibs)
{
  //u8  extIntStatus = 0;  
  u16 size;
  //u16 mfIntStatus = 0;
  HANDLE hDevice = NULL;
  //unsigned long flag;  

  TDMB_MSG_FCI_BB("[%s]\n", __func__);

  if(g_mfIntStatus & 0x0100) 
  {
    BBM_WORD_READ(hDevice, BBM_BUF_FIC_THR, &size);
    size += 1;
    if(size-1) 
    {
      //local_irq_save(flag);
      BBM_DATA(hDevice, BBM_COM_FIC_DATA, &fibs[0], size);
      TDMB_MSG_FCI_BB("[%s] fic data size[%d]\n", __func__, size);
      //local_irq_restore(flag);
    }
  }
  return BB_MAX_FIB_NUM;
}

/*====================================================================
FUNCTION       fc8050_read_fib
DESCRIPTION   For FIC_CALLBACK using
DEPENDENCIES
RETURN VALUE number of FIB
SIDE EFFECTS
======================================================================*/
int fc8050_read_fib(uint8 *fibs)
{
  uint32 size=0;
#ifdef FC8050_READ_ALL_FIC
  static uint16 temp_size=0;
#endif /* FC8050_READ_ALL_FIC */

  size = fic_type.length;

#ifdef FC8050_READ_ALL_FIC
  if(air_play == TRUE)
  {
    if(fic_type.length > BB_MAX_FIC_SIZE)
    {
      if(fic_type.address!=0)
        memcpy(&fibs[0], (void*)(fic_type.address+temp_size), BB_MAX_FIC_SIZE);
      temp_size += BB_MAX_FIC_SIZE;

      TDMB_MSG_FCI_BB("[%s] [%x] size[%d]\n", __func__, fic_type.address, temp_size);
      if((temp_size+BB_MAX_FIC_SIZE) > fic_type.length) 
      {
        fic_type.length = 0;
        fic_type.address=0;
        temp_size = 0;
      }
      else 
      {
        //tdmb_set_fic_intr();
      }
    }
    else
    {
      fic_type.length = 0;
      if(fic_type.address!=0)
        memcpy(&fibs[0], (void*)fic_type.address, size);
      temp_size = 0;
      fic_type.address=0;
    }
    return BB_MAX_FIB_NUM;
  }
  else // autoscan
  {
    fic_type.length=0;
    if(fic_type.address!=0)
      memcpy(&fibs[0], (void*)fic_type.address, size);
    TDMB_MSG_FCI_BB("[%s] [%x] size[%d]\n", __func__, fic_type.address, size);
    temp_size = 0;
    fic_type.address=0;

    return size/32;
  }
#else

  if(fic_type.address!=0)
  {
    memcpy(&fibs[0], (void*)fic_type.address, size);
    TDMB_MSG_FCI_BB("[%s] [%x] size[%d] data[%x]\n", __func__, (int)fic_type.address, (int)size,fibs[0]);
  }
  fic_type.length=0;
  fic_type.address=0;

  return size/32;

#endif /* FC8050_READ_ALL_FIC */
}

/*====================================================================
FUNCTION       fc8050_set_subchannel_info
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_set_subchannel_info(void * sub_ch_info)
{
  static chan_info *fci_ch_info;

  fci_ch_info = (chan_info *)sub_ch_info;

  fci_subch_info.freq = fci_ch_info->ulRFNum;
  fci_subch_info.servicetype = fci_ch_info->uiServiceType;
  fci_subch_info.subChId = (uint8)fci_ch_info->uiSubChID;
  fci_subch_info.bitrate = fci_ch_info->uiBitRate;
}

/*====================================================================
FUNCTION       fc8050_read_msc
DESCRIPTION  FOR android EBI2
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int fc8050_read_msc2(uint8 *msc_buffer)
{
  uint16 msc_size=0;
//  u16 mfIntStatus = 0;
  u16 mfoverStatus = 0;
  u16 size;
  int   i;
  HANDLE hDevice = NULL;

  //TDMB_MSG_FCI_BB("[%s]\n", __func__);

  BBM_WORD_READ(hDevice, BBM_BUF_OVERRUN, &mfoverStatus);
  BBM_WORD_WRITE(hDevice, BBM_BUF_OVERRUN, mfoverStatus);
  BBM_WORD_WRITE(hDevice, BBM_BUF_OVERRUN, 0x0000);

  if(mfoverStatus& 0x01)
  {
    TDMB_MSG_FCI_BB("[%s]                            overun[%x] \n", __func__, mfoverStatus);
  }

  if(g_mfIntStatus & 0x0100) 
  {
    BBM_WORD_READ(hDevice, BBM_BUF_FIC_THR, &size);
    size += 1;
    if(size-1) 
    {
      BBM_DATA(hDevice, BBM_COM_FIC_DATA, &ts_data.fic_buf[0], size);
      ts_data.type = FIC_DATA;
      ts_data.fic_size = size;
      //TDMB_MSG_FCI_BB("[%s] fic data size[%d]\n", __func__, size);
    }
  }

  for(i=0; i<8; i++) 
  {
    if(g_mfIntStatus & (1<<i)) 
    {
      BBM_WORD_READ(hDevice, BBM_BUF_CH0_THR+i*2, &msc_size); 
      msc_size += 1;
  
      if(msc_size-1) 
      {
        u8  subChId;
  
        BBM_READ(hDevice, BBM_BUF_CH0_SUBCH+i, &subChId);
        subChId = subChId & 0x3f;
  
        BBM_DATA(hDevice, (BBM_COM_CH0_DATA+i), msc_buffer, msc_size);
  
        if(ts_data.type == FIC_DATA)
          ts_data.type = FIC_MSC_DATA;
        else
          ts_data.type = MSC_DATA;
        //TDMB_MSG_FCI_BB("[%s] size[%d]  [%x] [%x] [%x][%x]\n", __func__, msc_size, msc_buffer[0], msc_buffer[1], msc_buffer[2], msc_buffer[3]);
      }
    }
  }

#ifdef FEATURE_TS_PKT_MSG
  if(dmb_mode == TDMB_MODE_AIR_PLAY)
  {
    uint16 cnt;
    for(cnt=0; cnt<msc_size;cnt+=188)
    {
      if((msc_buffer[cnt+0] != 0x47) ||(msc_buffer[cnt+1] == 0x80))
      {
        TDMB_MSG_FCI_BB("[%s] error !!   [%x] [%x] [%x] [%x]\n", __func__, msc_buffer[cnt+0], msc_buffer[cnt+1], msc_buffer[cnt+2], msc_buffer[cnt+3]);
      }
    }
  }
#endif 

  return msc_size;
}

/*====================================================================
FUNCTION       fc8050_read_msc
DESCRIPTION  For MSC_CALLBACK using
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
#ifdef FC8050_DOUBLE_BUFFERING
int fc8050_read_msc(uint8 *msc_buffer)
{
  uint32 msc_size, fic_size=0;

  //TDMB_MSG_FCI_BB("[%s] \n",__func__);

#if 1 // for EBI2 IF, for android, FIC data read during airplay
  fic_size = fic_type.length;
  fic_type.length=0;

  if(fic_size != 0)
  {      
    if(fic_type.address!=0)
    {
      memcpy(&ts_data.fic_buf[0], (void*)fic_type.address, fic_size);        
      ts_data.type = FIC_DATA;
      ts_data.fic_size = fic_size;
      //TDMB_MSG_FCI_BB("[%s] fic data [%x] size[%d]\n", __func__, (unsigned int)fic_type.address, (int)fic_size);
    }
  }
  fic_type.address=0;
#endif

  if(gOutputBuffer == gInputBuffer)
  {
    TDMB_MSG_FCI_BB("[%s] 1 fic_size[%d]\n", __func__, (int)fic_size);
    return 0;
  }

  msc_size = msc_type[gOutputBuffer].length;

  if(!msc_type[gOutputBuffer].address || !msc_size)
  {
    TDMB_MSG_FCI_BB("[%s] 2 fic_size[%d]\n", __func__, (int)fic_size);
    return 0;
  }

  //memcpy(&msc_buffer[0], (void*)msc_type[gOutputBuffer].address, msc_size);
  memcpy(&msc_buffer[0], (void*) &msc_data[gOutputBuffer * FC8050_MSC_BUFFER_SIZE], msc_size);

  if(ts_data.type == FIC_DATA)
    ts_data.type = FIC_MSC_DATA;
  else
    ts_data.type = MSC_DATA;

  msc_type[gOutputBuffer].address = 0;
  msc_type[gOutputBuffer].length = 0;
  msc_type[gOutputBuffer].subch_id = 0;

  gOutputBuffer = (gOutputBuffer + 1) % FC8050_MSC_BUFFERS;

#ifdef FEATURE_TS_PKT_MSG
  if(dmb_mode == TDMB_MODE_AIR_PLAY)
  {
    uint16 i = 0;
    uint cnt = 0;

    for(i=0; i<msc_size;i+=188)
    {
      //if((msc_buffer[i+0] != 0x47) || (msc_buffer[i+1] == 0x80))
      if(msc_buffer[i+0] != 0x47)
      {
        TDMB_MSG_FCI_BB("[%s] error !! idx[%d] [%x] [%x] [%x] [%x]\n", __func__, i/188, msc_buffer[i+0], msc_buffer[i+1], msc_buffer[i+2], msc_buffer[i+3]);
        cnt ++;
      }
    }
    TDMB_MSG_FCI_BB("[%s] end!!! msc_size[%d], fic_size[%d] err_cnt[%d]\n", __func__, (int)msc_size, (int)fic_size, cnt);
  }
#else
  //TDMB_MSG_FCI_BB("[%s] end\n",__func__);
#endif

  return msc_size;
}

#else

int fc8050_read_msc(uint8 *msc_buffer)
{
  uint32 msc_size, fic_size=0;
  //uint8 i, msc_ch;
  //uint8  rsSubChId;

  msc_size = msc_type.length;
  msc_type.length=0;

#if 1 // for EBI2 IF, for android, FIC data read during airplay
  fic_size = fic_type.length;
  fic_type.length=0;

  if(fic_size != 0)
  {      
    if(fic_type.address!=0)
    {
      memcpy(&ts_data.fic_buf[0], (void*)fic_type.address, fic_size);
      ts_data.type = FIC_DATA;
      ts_data.fic_size = fic_size;
    }
    //TDMB_MSG_FCI_BB("[%s] fic data [%x] size[%d]\n", __func__, fic_type.address, fic_size);
  }
  fic_type.address=0;
#endif

  //if(msc_size != 0)
  {
    if(msc_type.address!=0)
    {
#ifdef FC8050_VERIFY_TEST
      test_cnt--;
#endif /* FC8050_VERIFY_TEST */
      memcpy(&msc_buffer[0], (void*)msc_type.address, msc_size);
      if(ts_data.type == FIC_DATA)
        ts_data.type = FIC_MSC_DATA;
      else
        ts_data.type = MSC_DATA;
      //TDMB_MSG_FCI_BB("[%s] [%x] size[%d] type[%d]  [%x][%x]\n", __func__, msc_type.address, msc_size,ts_data.type, msc_buffer[0], msc_buffer[1]);
    }
    else
    {
      //TDMB_MSG_FCI_BB("[%s] fc8050_read_msc msc address is NULL type[%d]  [%x][%x]\n", __func__, ts_data.type, msc_buffer[0], msc_buffer[1]);
    }

#ifdef FEATURE_TS_PKT_MSG
    if(dmb_mode == TDMB_MODE_AIR_PLAY)
    {
      uint16 cnt;
      for(cnt=0; cnt<msc_size;cnt+=188)
      {
        if((msc_buffer[cnt+0] != 0x47) ||(msc_buffer[cnt+1] == 0x80))
        {
          TDMB_MSG_FCI_BB("[%s] error !!   [%x] [%x] [%x] [%x]\n", __func__, msc_buffer[cnt+0],msc_buffer[cnt+1],msc_buffer[cnt+2],msc_buffer[cnt+3]);
        }
      }
    }
#endif    
  }
  msc_type.address=0;

  return msc_size;
}

#endif /* FC8050_DOUBLE_BUFFERING */

/*====================================================================
FUNCTION       fc8050_RFInputPower
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int fc8050_RFInputPower(void)
{
  int RSSI;

  BBM_TUNER_GET_RSSI(NULL, &RSSI);
  return abs(RSSI);
}

/*====================================================================
FUNCTION       fc8050_reconfig_n_ber
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_get_ber(tdmb_bb_sig_type *psigs)
{
  int RSSI;
  uint8 vt_ctrl=0;
  uint32 temp_pcber=0;
  uint32  ber=0;
  unsigned int esum,bper,tbe;
  uint16 nframe, rserror;
  //float ber=0;

  BBM_READ(NULL, BBM_VT_CONTROL, &vt_ctrl);

  vt_ctrl |= 0x10;
  BBM_WRITE(NULL, BBM_VT_CONTROL, vt_ctrl);

  BBM_LONG_READ(NULL, BBM_VT_RT_BER_PERIOD, &bper);

  BBM_LONG_READ(NULL, BBM_VT_RT_ERROR_SUM, &tbe);

  if(bper == 0)  
  {
    psigs->PCBER = 20000;
    temp_pcber = psigs->PCBER;
  } 
  else
  {
    //psigs->PCBER =(tbe * (4000000000 / bper))/40000;
    //psigs->PCBER =(tbe * (100000 / bper));
    psigs->PCBER =(unsigned int)((tbe * (4000000000UL / bper))/40000);

    temp_pcber = psigs->PCBER;
  }
  //TDMB_MSG_FCI_BB("[%s] vt_ber_per[%x], vt_err_sum[%x]\n", __func__, bper, tbe);
  vt_ctrl &= ~0x10;
  BBM_WRITE(NULL, BBM_VT_CONTROL, vt_ctrl);

  BBM_WORD_READ(NULL, BBM_RS_BER_PERIOD, &nframe);

  BBM_LONG_READ(NULL, BBM_RS_ERR_SUM, &esum);

  BBM_WORD_READ(NULL, BBM_RS_FAIL_COUNT, &rserror);

  if(nframe == 0)
  {
    psigs->RSBER = 20000;
  }
  else
  {
    ber = esum;
    ber += rserror * 9;
    //ber *=  (4000000000 / ((nframe + 1) * 204 * 8))/4000;
    //ber *=  (1000000 / ((nframe + 1) * 204 * 8));
    ber *=  (4000000000UL / ((nframe + 1) * 204 * 8))/4000;

    psigs->RSBER = ber;
  }
  //TDMB_MSG_FCI_BB("[%s] tbe = [%d], bper = [%d], esum = [%d], nframe = [%d]\n", __func__, tbe, bper, esum, nframe);

#if 0
  {
  uint16 en_data, int_data, end_data, th_data;
  uint8 tun_data;

  //Baseband
  BBM_WORD_READ(NULL, BBM_BUF_ENABLE, &en_data);
  BBM_WORD_READ(NULL, BBM_BUF_INT, &int_data);
  BBM_WORD_READ(NULL, BBM_BUF_CH0_END, &end_data);
  BBM_WORD_READ(NULL, BBM_BUF_CH0_THR, &th_data);

  //RF        
  BBM_TUNER_READ(NULL, 0x3B, 0x01, &tun_data, 0x01);

  TDMB_MSG_FCI_BB("[%s] en[%d], int[%d], end[%d], th[%d], tun[%d]\n", __func__, en_data, int_data, end_data, th_data, tun_data);
  }
#endif

  RSSI = 90; //fc8050_RFInputPower();

  psigs->SNR = fc8050_Ant_Level(RSSI, psigs->PCBER);
  psigs->RSSI = RSSI;
  //TDMB_MSG_FCI_BB("pcber[%d] snr[%d] rsber[%d]  rssi[%d]\n", psigs->PCBER, psigs->SNR, psigs->RSBER,psigs->RSSI);
}

/*====================================================================
FUNCTION       fc8050_Ant_Level
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int fc8050_Ant_Level(int RSSI, uint32 pcber)
{
  uint8 level = 0;
  static uint16 save_level=0;
  uint16 ant_tbl[] = {11000, // 0 <-> 1
                       8500,  // 1 <-> 2
                       6000,  // 2 <-> 3
                       3500,  // 3 <-> 4
                       1000}; // 4 <-> 5
  uint16 hystery_value[]= {500, 1000, 1000, 1000, 500};

  if(RSSI > 104)
    return 0;

  if(pcber >= ant_tbl[0])
  {
    save_level = level = 0;
    return save_level;
  }

  if((pcber >= ant_tbl[1] && pcber < ant_tbl[0])) level = 1;
  else if((pcber >= ant_tbl[2] && pcber < ant_tbl[1])) level = 2;
  else if((pcber >= ant_tbl[3] && pcber < ant_tbl[2])) level = 3;
  else if((pcber >= ant_tbl[4] && pcber < ant_tbl[3])) level = 4;
  else if(pcber < ant_tbl[4]) level = 5;

  if(level == save_level + 1) // Level 이 1칸 올라간 경우에만.
  {
    if(pcber < (ant_tbl[level - 1] - hystery_value[level - 1]))
    {
      save_level = level;
    }
  }
  else
  {
    save_level = level;
  }

  return save_level;
}

/*====================================================================
FUNCTION       fc8050_get_demux_buffer
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_get_demux_buffer(u8 **res_ptr, u32 *res_size)
{
  TDMB_MSG_FCI_BB("[%s]\n", __func__);

#ifdef FC8050_DOUBLE_BUFFERING
  if(gOutputBuffer == gInputBuffer)
    return;

  *res_size = msc_type[gOutputBuffer].length;

  if(!msc_type[gOutputBuffer].address || !*res_size)
    return;

  *res_ptr = &msc_data[gOutputBuffer * FC8050_MSC_BUFFER_SIZE];

  if(ts_data.type == FIC_DATA)
    ts_data.type = FIC_MSC_DATA;
  else
    ts_data.type = MSC_DATA;

  msc_type[gOutputBuffer].address = 0;
  msc_type[gOutputBuffer].length = 0;
  msc_type[gOutputBuffer].subch_id = 0;

  gOutputBuffer = (gOutputBuffer + 1) % FC8050_MSC_BUFFERS;
#else
  *res_ptr = (u8*)msc_type.address;
  *res_size = msc_type.length;

  memset((void*)&msc_type, 0x00, sizeof(INT_TYPE));
#endif
}

/*====================================================================
FUNCTION       fc8050_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_test(int ch)
{
  uint32 bitrate;
  int result;
  REGS fci_regs;
  chan_info fci_ch_info;

  TDMB_MSG_FCI_BB("[%s] ch[%d]\n", __func__, ch);

  fc8050_drv_init();
  
#ifdef FC8050_IF_RW_TEST   
    fc8050_if_test(1); // byte
    fc8050_if_test(2); // word
    fc8050_if_test(3); // long
    fc8050_if_test(4); // tuner
#endif /*  FC8050_I2C_RW_TEST */

  tdmb_get_fixed_chan_info((service_t)ch, &fci_ch_info);

  fci_regs.freq = fci_ch_info.ulRFNum;
  fci_regs.servicetype = (fci_ch_info.uiServiceType == TDMB_BB_SVC_DAB) ? 1 : 2;
  fci_ch_info.uiServiceType = fci_regs.servicetype;
  fci_regs.subChId = (uint8)fci_ch_info.uiSubChID;

  bitrate = (uint32)fci_ch_info.uiBitRate;

  TDMB_MSG_FCI_BB("[%s] freq[%d]  type[%d] subchid[0x%x] bitrate[%d]\n", __func__, (int)fci_regs.freq, fci_regs.servicetype, fci_regs.subChId, (int)bitrate);

  fc8050_set_subchannel_info(&fci_ch_info);

  // cys
  fc8050_ch_scan_start((int)fci_regs.freq, 0, 0);

  result = fc8050_subch_start((uint8*)&fci_regs, bitrate);
  //fc8050_power_off();

  if(result == FALSE)
  {
    TDMB_MSG_FCI_BB("[%s] fc8050_subch_start fail [%d]\n", __func__, result);
  }
}
