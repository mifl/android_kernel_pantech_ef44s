//=============================================================================
// File       : TCC3170_bb.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       
//  1.1.0       2012/03/07       yschoi         porting New SDK
//=============================================================================

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <mach/gpio.h>

#include <asm/irq.h>

#include "../../dmb_hw.h"
#include "../../dmb_interface.h"
#include "../../dmb_test.h"

#include "../tdmb_comdef.h"
#include "../tdmb_dev.h"
#include "../tdmb_test.h"

#include "tcpal_os.h"

#include "tcbd_feature.h"
#include "tcbd_api_common.h"
#include "tcbd_drv_io.h"
#include "tcbd_drv_ip.h"

#include "tcbd_hal.h"
//#include "tcbd.h"
#include "tcbd_stream_parser.h"



/*================================================================== */
/*=================       TCC3170 BB Function       ================== */
/*================================================================== */

#define TMDB_SPI_CTRL_ID TCC3170_CID
#define TC3170_MSC_BUF_SIZE 16 // temp value
#define TC3170_FIC_SIZE 384
#define TC3170_DATA_SIZE ((TCBD_MAX_FIFO_SIZE>>1)+(188*4))

struct tcc3170_private_data {
  struct tcbd_device device;
  unsigned char subch_id;
  unsigned char data_mode;
};

#ifdef FEATURE_DMB_SPI_IF
typedef struct 
{
  uint8 tcc_fic_buf[TC3170_FIC_SIZE];
  uint16 tcc_fic_size;
  //uint8 tcc_msc_buf[TC3170_DATA_SIZE*TC3170_MSC_BUF_SIZE];
  uint8 tcc_msc_buf[TC3170_DATA_SIZE];
  uint16 tcc_msc_size;
} tcc_data_type;

tcc_data_type tcc_data;

extern void tcpal_split_stream(struct tcbd_irq_data *irq_data);
#endif

#ifdef FEATURE_DMB_SPI_IF
enum tcbd_peri_type gCmdInterface = PERI_TYPE_SPI_ONLY;
#else
enum tcbd_peri_type gCmdInterface = PERI_TYPE_STS;
#endif

#if 0 //def FEATURE_DMB_SPI_IF
//#define MSC_BUFFER_FOR_TCC_SDK
#ifdef MSC_BUFFER_FOR_TCC_SDK
int msc_put_cnt;
#else
int msc_put_count = 0;
int msc_get_count = 0;
#endif
#endif

#ifdef FEATURE_DMB_TSIF_IF
ts_data_type ts_tmp_data[2];

static int msc_buf_idx = 0;
static int tsif_read_flag = 0;

static DEFINE_MUTEX(lock);
#endif

static int first_getber_flag = 0;

uint8 sync_lock;
int start_tune = 0;


extern tdmb_mode_type dmb_mode;
extern ts_data_type ts_data;
extern fic_data_type fic_data;

//extern irqreturn_t tdmb_interrupt_tcc(void);

static boolean tcc3170_function_register(tdmb_bb_function_table_type *);
struct tcc3170_private_data tcc3170_dev_data;



/*====================================================================
FUNCTION       tdmb_bb_tcc3170_init  
DESCRIPTION    matching tcc3170 Function at TDMB BB Function
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean tdmb_bb_tcc3170_init(tdmb_bb_function_table_type *function_table_ptr)
{
  boolean bb_initialized;

#if defined(FEATURE_DMB_I2C_CMD)
  tcpal_set_i2c_io_function();
#elif defined(FEATURE_DMB_SPI_IF)
  tcpal_set_cspi_io_function();
#endif /*FEATURE_DMB_SPI_IF*/

  bb_initialized = tcc3170_function_register(function_table_ptr);
  return bb_initialized;
}


/*====================================================================
FUNCTION       tcc3170_function_register  
DESCRIPTION    
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static boolean tcc3170_function_register(tdmb_bb_function_table_type *ftable_ptr)
{
  ftable_ptr->tdmb_bb_drv_init          = tcc3170_init;
  ftable_ptr->tdmb_bb_power_on          = tcc3170_power_on;
  ftable_ptr->tdmb_bb_power_off         = tcc3170_power_off;
  ftable_ptr->tdmb_bb_ch_scan_start     = tcc3170_ch_scan_start;
  ftable_ptr->tdmb_bb_resync            = tcc3170_bb_resync;
  ftable_ptr->tdmb_bb_subch_start       = tcc3170_subch_start;
  ftable_ptr->tdmb_bb_read_int          = tcc3170_read_int;
  ftable_ptr->tdmb_bb_get_sync_status   = tcc3170_get_sync_status;
  ftable_ptr->tdmb_bb_read_fib          = tcc3170_read_fib;
  ftable_ptr->tdmb_bb_set_subchannel_info = tcc3170_set_subchannel_info;
  ftable_ptr->tdmb_bb_read_msc          = tcc3170_read_msc;
  ftable_ptr->tdmb_bb_get_ber           = tcc3170_get_ber;
  ftable_ptr->tdmb_bb_ch_stop           = tcc3170_stop;
  ftable_ptr->tdmb_bb_powersave_mode    = tcc3170_set_powersave_mode;
  ftable_ptr->tdmb_bb_ch_test           = tcc3170_test;

  return TRUE;
}


/*====================================================================
FUNCTION       tcc3170_i2c_write_word
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 tcc3170_i2c_write_word(uint8 chipid, uint8 reg, uint8 data)
{
  uint8 ret = 0;

#ifdef FEATURE_DMB_I2C_CMD
  ret = dmb_i2c_write(chipid, reg, sizeof(uint8), data, sizeof(uint8));
#endif /* FEATURE_DMB_I2C_CMD */

  return ret;
}


/*====================================================================
FUNCTION       tcc3170_i2c_write_len
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 tcc3170_i2c_write_len(uint8 chipid, uint8 reg, uint8 *data, uint32 data_size)
{
  uint8 ret = 0;

#ifdef FEATURE_DMB_I2C_CMD
  ret = dmb_i2c_write_len(chipid, reg, sizeof(uint8), data, (uint16)data_size);
#endif /* FEATURE_DMB_I2C_CMD */

  return ret;
}


/*====================================================================
FUNCTION       tcc3170_i2c_read_word
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 tcc3170_i2c_read_word(uint8 chipid, uint8 reg, uint8 *data)
{
#ifdef FEATURE_DMB_I2C_CMD
  dmb_i2c_read(chipid, reg, sizeof(uint8), (uint16 *)data, sizeof(uint8));
#endif /* FEATURE_DMB_I2C_CMD */

  return TRUE;
}


/*====================================================================
FUNCTION       tcc3170_i2c_read_len
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_i2c_read_len(uint8 chipid, uint8 uiAddr, uint8 *read_buf, uint32 read_buf_len)
{
#ifdef FEATURE_DMB_I2C_CMD
  dmb_i2c_read_len(chipid, uiAddr, read_buf, (uint16)read_buf_len, sizeof(uint8));
#endif /* FEATURE_DMB_I2C_CMD */

  return;
}


#if 0
/*====================================================================
FUNCTION       tcc3170_spi_write
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_spi_write(uint8 chipid, uint16 reg, uint8 data)
{
  int ret = 0;

  ret = tcc3170_IO_CSPI_Reg_Write(0, chipid, reg, data);

  return ret;
}

/*====================================================================
FUNCTION       tcc3170_spi_write_len
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_spi_write_len(uint8 chipid, uint16 reg, uint8 *data, uint16 length)
{
  int  ret = 0;
  
  ret = tcc3170_IO_CSPI_Reg_WriteEx(0, chipid, reg, data, length);

  return ret;
}

/*====================================================================
FUNCTION       tcc3170_i2c_write_word
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 tcc3170_spi_read(uint8 chipid, uint16 reg, uint8 data)
{
  data = tcc3170_IO_CSPI_Reg_Read(0, chipid, reg, 0);

  return data;
}

/*====================================================================
FUNCTION       tcc3170_spi_read_len
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 tcc3170_spi_read_len(uint8 chipid, uint16 reg, uint8 *data, uint16 length)
{
  int ret=0;

  ret = tcc3170_IO_CSPI_Reg_ReadEx(0, chipid, reg, data, length);
  
  return ret;
}
#endif


/*====================================================================
FUNCTION       tcc3170_set_powersave_mode
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_set_powersave_mode(void)
{
  // if necessary

  //TDMB_MSG_TCC_BB("[%s] \n", __func__);
}


/*====================================================================
FUNCTION       tcc3170_power_on
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void tcc3170_check_chipid(void)
{
  u8 chip_id = 0;
  struct tcbd_device *dev = &tcc3170_dev_data.device;

  /* check chip id */
  tcbd_reg_read(dev, TCBD_CHIPID, &chip_id);
  TDMB_MSG_TCC_BB("[%s] chip id : 0x%X \n", __func__, chip_id);
  if (chip_id != TCBD_CHIPID_VALUE)
  {
    TDMB_MSG_TCC_BB("[%s] invalid chip id !!! exit!\n", __func__);
  }
}


/*====================================================================
FUNCTION       tcc3170_power_on
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_power_on(void)
{
  //power on sequency
  
  TDMB_MSG_TCC_BB("[%s] start!!!\n", __func__);

  dmb_power_on();

  dmb_set_gpio(DMB_RESET, 0);
  msleep (10);

  dmb_power_on_chip();

  msleep(10);

  dmb_set_gpio(DMB_RESET, 1);

  msleep(10);
}


/*====================================================================
FUNCTION       tcc3170_power_off
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_power_off(void)
{
  struct tcbd_device *dev = &tcc3170_dev_data.device;

  TDMB_MSG_TCC_BB("[%s] start!!!\n", __func__);

  tcbd_io_close(dev);

  dmb_set_gpio(DMB_RESET, 0);
  msleep(1);

  dmb_power_off();
}


/*====================================================================
FUNCTION       tcc3170_ch_scan_start
DESCRIPTION  
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_ch_scan_start(int freq, int band, unsigned char for_air)
{
  int ret = 0;
  u8 status;
  struct tcbd_device *dev = &tcc3170_dev_data.device;
 // freq. setting use for start autoscan &  air play
  TDMB_MSG_TCC_BB("[%s] Channel Frequency[%d]\n", __func__, freq);

  tcbd_disable_irq(dev, 0);
  ret = tcbd_tune_frequency(dev, freq, 1500);
  if (ret < 0)
  {
    TDMB_MSG_TCC_BB("[%s] failed to tune frequency! [%d]\n", __func__, ret);
    return;
  }
  TDMB_MSG_TCC_BB("[%s] SetFreq  ret[%d]\n", __func__,ret);

  if(dmb_mode == TDMB_MODE_AUTOSCAN)
  {
    ret = tcbd_wait_tune(dev, &status);
    sync_lock = ret==0?1:0;
  }
  else
  {
    sync_lock = TRUE;
  }

  start_tune = 1;

#if defined(FEATURE_DMB_SPI_IF)
  tcbd_enable_irq(dev, TCBD_IRQ_EN_DATAINT);
#elif defined(FEATURE_DMB_I2C_CMD)
  tcbd_enable_irq(dev, TCBD_IRQ_EN_FIFOAINIT);
#endif /* FEATURE_DMB_I2C_CMD */

  TDMB_MSG_TCC_BB("[%s] sync lock %s  sync_lock[%d] %d \n", __func__, ret==0?"OK~":"Fail!!", sync_lock, ret);
}


/*====================================================================
FUNCTION       tcc3170_init_parser
DESCRIPTION            
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void tcc3170_init_parser(void)
{
#if defined(FEATURE_DMB_TSIF_IF) && defined(FEATURE_TDMB_MULTI_CHANNEL_ENABLE)
  tcbd_init_parser(0, (tcbd_stream_callback)tcc3170_tsif_parser_callback);
#elif defined(FEATURE_DMB_SPI_IF)
  tcbd_init_parser(0, tcpal_irq_stream_callback);
#endif /* FEATURE_DMB_SPI_IF */

}


/*====================================================================
FUNCTION       tcc3170_init
DESCRIPTION            
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 tcc3170_init(void)
{
  //tcc3170 initialization
  int ret;
  struct tcbd_device *dev = &tcc3170_dev_data.device;

  TDMB_MSG_TCC_BB("[%s] start  Cmd[%d] size[%d]\n", __func__, gCmdInterface, TC3170_DATA_SIZE);

  memset(dev, 0, sizeof(struct tcbd_device));

  /* before doing I/O, I/O function pointer must be initialized. */
  /* command io open */
  ret = tcbd_io_open(dev);
  if (ret < 0)
  {
    TDMB_MSG_TCC_BB("[%s] failed io open %d \n", __func__, (int)ret);
  }

  tcc3170_check_chipid();

  // cys RW test
#if 0
  tcc3170_rw_test();
  return 0;
#endif

  tcbd_select_peri(dev, gCmdInterface);
  tcbd_change_irq_mode(dev, INTR_MODE_EDGE_FALLING);

  tcc3170_init_parser();

#ifdef FEATURE_DMB_CLK_19200
  ret = tcbd_device_start(dev, CLOCK_19200KHZ);
#elif defined(FEATURE_DMB_CLK_24576)
  ret = tcbd_device_start(dev, CLOCK_24576KHZ);
#endif
  if(ret == 0)
  {
    TDMB_MSG_TCC_BB("[%s] OK ~~\n", __func__);
  }
  else
  {
    TDMB_MSG_TCC_BB("[%s] device start failed[%d]\n", __func__, ret);
  }

#if 0 //def FEATURE_DMB_SPI_IF
#ifdef MSC_BUFFER_FOR_TCC_SDK
  msc_put_cnt = 0;
#else
  msc_put_count = 0;
  msc_get_count = 0;
#endif
#endif

#ifdef FEATURE_DMB_SPI_IF
  memset((void*)&tcc_data, 0x00, sizeof(tcc_data_type));
#endif

  return 0;
}


/*====================================================================
FUNCTION       tcc3170_bb_resync
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_bb_resync(unsigned char imr)
{
  // if necessary

  //TDMB_MSG_TCC_BB("[%s] \n", __func__);
}


/*====================================================================
FUNCTION       tcc3170_stop
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 tcc3170_stop(void)
{
  // if necessary
  int ret;
  struct tcc3170_private_data *dev_data = &tcc3170_dev_data;
  
  TDMB_MSG_TCC_BB("[%s] \n", __func__);

  ret = tcbd_unregister_service(&dev_data->device, dev_data->subch_id);

  tcbd_disable_irq(&dev_data->device, 0);

  TDMB_MSG_TCC_BB("[%s] end!\n", __func__);
  return 0;
}


/*====================================================================
FUNCTION       tcc3170_subch_start
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_subch_start(uint8 *regs, uint32 data_rate)
{
  // subch setting  code
  int ret; //, icontrol = 1;
  struct tcc3170_private_data *dev_data = &tcc3170_dev_data;

  TDMB_MSG_TCC_BB("[%s] data mode[%x] subchId[%x]\n", __func__, dev_data->data_mode, dev_data->subch_id);

  ret = tcbd_register_service(&dev_data->device, dev_data->subch_id, dev_data->data_mode);

  tcc3170_init_parser();

#if 0 //def FEATURE_DMB_SPI_IF
#ifdef MSC_BUFFER_FOR_TCC_SDK
  msc_put_cnt = 0;
#else
  msc_put_count = 0;
  msc_get_count = 0;
#endif
#endif

#ifdef FEATURE_DMB_TSIF_IF
  msc_buf_idx = 0;
  tsif_read_flag = 0;
  ts_tmp_data[0].fic_size = 0;
  ts_tmp_data[0].msc_size = 0;
  ts_tmp_data[1].fic_size = 0;
  ts_tmp_data[1].msc_size = 0;
#endif

  first_getber_flag = 1;

  TDMB_MSG_TCC_BB("[%s]  end ! ret[%d]\n", __func__, ret);

  //ret = (ret > 0)? TRUE: FALSE;
  ret = (ret > 0)? FALSE: TRUE;

  return ret;
}


/*====================================================================
FUNCTION       tcc3170_read_int
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tdmb_bb_int_type tcc3170_read_int(void)
{
#ifdef FEATURE_DMB_SPI_IF
  struct tcbd_irq_data irq_data;
#endif

  //TDMB_MSG_TCC_BB("[%s] \n", __func__);

#ifdef FEATURE_DMB_SPI_IF
  irq_data.device = &tcc3170_dev_data.device;

  tcpal_split_stream(&irq_data);
#endif

  return TDMB_BB_INT_MSC;
}


/*====================================================================
FUNCTION       tcc3170_get_sync_status
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tBBStatus tcc3170_get_sync_status(void)
{
  // check sync status
  TDMB_MSG_TCC_BB("[%s] %d\n", __func__, sync_lock);

  //return BB_SUCCESS;
  return sync_lock;
}


//#define DEBUG_DUMP_FILE

#ifdef DEBUG_DUMP_FILE
extern void file_dump(u8 *p, u32 size, char *path);
#endif

/*====================================================================
FUNCTION       tcc3170_read_fib
DESCRIPTION use for read FIC data only autoscan
DEPENDENCIES
RETURN VALUE number of FIB
SIDE EFFECTS
======================================================================*/
int tcc3170_read_fib(uint8 *fibs)
{
  // Read FIC data only autoscan
  uint16 wFicLen = TCBD_FIC_SIZE;
#ifdef FEATURE_DMB_I2C_CMD
  char fic_temp_buf[TCBD_FIC_SIZE];
  struct tcbd_device *dev = &tcc3170_dev_data.device;
  int ret = 0;

  //struct tcbd_status_data _status_data;

  TDMB_MSG_TCC_BB("[%s] size[%d]\n", __func__, wFicLen);

  ret = tcbd_read_fic_data(dev, fic_temp_buf, wFicLen);

  if(ret < 0)
  {
    fic_data.fib_num = 0;
    TDMB_MSG_TCC_BB("[%s] error!!! ret[%d]\n", __func__, ret);
    return 0;
  }

#if 1 // cys
  if(wFicLen > 384)
    wFicLen = 384;
#endif

  if(wFicLen != 0)
  {
    memcpy(fibs, fic_temp_buf, wFicLen);
  }

#ifdef DEBUG_DUMP_FILE
	//file_dump(ts_data.fic_buf, ts_data.fic_size, "/mnt/sdcard/Movies/tcc3170.bin");
	file_dump(fic_temp_buf, wFicLen, "/data/local/tmp/tcc3170.bin");
#endif

#if 0
  tcbd_read_signal_info(dev, &_status_data);
  TDMB_MSG_TCC_BB("[%s] PCBER[%d], SNR[%d], RSSI[%d]\n", __func__, _status_data.pcber, _status_data.snr, _status_data.rssi);
#endif

#elif defined(FEATURE_DMB_SPI_IF)
  wFicLen = tcc_data.tcc_fic_size;

  TDMB_MSG_TCC_BB("[%s] size[%d]\n", __func__, wFicLen);

  tcc_data.tcc_fic_size = 0;

#if 1 // cys
  if(wFicLen > 384)
    wFicLen = 384;
#endif

  if(wFicLen != 0)
  {
    memcpy(fibs, tcc_data.tcc_fic_buf, wFicLen);
  }
#endif

  return wFicLen;
}

#if 0 // not used
/*====================================================================
FUNCTION       tcc3170_get_bitrate
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint16 tcc3170_get_bitrate(uint8 type, uint16 bitrate, uint16 subch_size, uint16 pro_level, uint16 option)
{
  uint8 i;
  uint16 tcc_bitrate = 0;
  uint16 UEP_bitrate_table[14] = {32,48,56,64,80,96,112,128,160,192,224,256,320,384};

  if(type == TDMB_BB_SVC_DMB)// EEP
  {
    if(option == 0) // ucOption 0
    {
      switch(pro_level)
      {
        case 0: tcc_bitrate = (subch_size/12); break;
        case 1: tcc_bitrate = (subch_size/8);  break;
        case 2: tcc_bitrate = (subch_size/6);  break;
        case 3: tcc_bitrate = (subch_size/4);  break;
      }
    }
    else if(option == 1)
    {
      switch(pro_level)
      {
        case 0: tcc_bitrate = (subch_size/27); break;
        case 1: tcc_bitrate = (subch_size/21);  break;
        case 2: tcc_bitrate = (subch_size/18);  break;
        case 3: tcc_bitrate = (subch_size/15);  break;
      }
    }
  }
  else if(type == TDMB_BB_SVC_DAB) // UEP (shrot form)
  {
    for (i = 0; i < 14; i++)
    {
      if (bitrate == UEP_bitrate_table[i])
      {
        break;
      }
    }
    if(i > 13)
    {
      TDMB_MSG_TCC_BB("%s  Error DAB  bitrate[%d]\n",__func__, i);
      return FALSE;
    }
    tcc_bitrate = i;
  }
  TDMB_MSG_TCC_BB("%s  type[%d] bitrate[%x]\n",__func__, type, tcc_bitrate);
  return tcc_bitrate;
}
#endif


/*====================================================================
FUNCTION       tcc3170_set_subchannel_info
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_set_subchannel_info(void * sub_ch_info)
{
  static chan_info *tcc_ch_info;
  struct tcc3170_private_data *dev_data = &tcc3170_dev_data;

  tcc_ch_info = (chan_info *)sub_ch_info;

  dev_data->subch_id = tcc_ch_info->uiSubChID;
  dev_data->data_mode = (tcc_ch_info->uiServiceType == TDMB_BB_SVC_DAB)? 0x00 : 0x01;
}


/*====================================================================
FUNCTION       tcc3170_read_msc
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_read_msc(uint8 *msc_buffer)
{
#ifdef FEATURE_DMB_SPI_IF
  uint16 msc_size = 0, fic_size = 0;

  //TDMB_MSG_TCC_BB("[%s] start\n", __func__);
  fic_size = tcc_data.tcc_fic_size;
  if(fic_size != 0)
  {
    memcpy(ts_data.fic_buf, tcc_data.tcc_fic_buf, fic_size);
    ts_data.type = FIC_DATA;
    ts_data.fic_size = fic_size;
    //TDMB_MSG_TCC_BB("  FIC in Read MSC size[%d] \n", fic_size);
  }
  tcc_data.tcc_fic_size = 0;

  msc_size = tcc_data.tcc_msc_size;
  if(msc_size != 0)
  {
    if(ts_data.type == FIC_DATA)
      ts_data.type = FIC_MSC_DATA;
    else
      ts_data.type = MSC_DATA;

    //TDMB_MSG_TCC_BB("  MSC read size[%d]  type[%d]  get cnt[%d]\n", msc_size, ts_data.type,msc_get_count);
#if 0
#ifdef MSC_BUFFER_FOR_TCC_SDK
    memcpy(msc_buffer, (void*)&tcc_data.tcc_msc_buf, msc_size);
#else
    memcpy(msc_buffer, (void*)&tcc_data.tcc_msc_buf[msc_get_count*TC3170_MSC_BUF_SIZE], msc_size);
    msc_get_count = (msc_get_count +1) % TC3170_MSC_BUF_SIZE;
#endif
#else
    memcpy(msc_buffer, (void*)&tcc_data.tcc_msc_buf, msc_size);
#endif
  }
  else
  {
    TDMB_MSG_TCC_BB("MSC NULL !! [%d]\n", msc_size);
  }
  tcc_data.tcc_msc_size = 0;
  memset((void*)&tcc_data, 0x00, sizeof(tcc_data_type));

  return msc_size;
#else
  return 0;
#endif /* FEATURE_DMB_SPI_IF */
}


/*====================================================================
FUNCTION       tcc3170_Ant_Level
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_Ant_Level(uint32 pcber,int rssi)
{
  uint8 level = 0;
  static uint8 save_level = 0;
  uint16 hystery_value[]= {50,50};

//111021 telechips ANT bar patch, different ant table by RSSI

  uint32 ant75[] = {1200, 900, 700, 500, 300,};
  uint32 ant80[] = {1200, 900, 700, 500, 1,};
  uint32 ant85[] = {1200, 900, 700, 1,   1,};
  uint32 ant90[] = {1200, 900, 1,   1,   1,};
  uint32 ant100[]= {1200, 1,   1,   1,   1,};
  uint32 *ant_tbl = ant75;
  
  if(rssi > -75) ant_tbl = ant75;
  else if(rssi > -80) ant_tbl = ant80;
  else if(rssi > -85) ant_tbl = ant85;
  else if(rssi > -90) ant_tbl = ant90;
  else if(rssi > -100) ant_tbl = ant100;

  if(pcber >= ant_tbl[0]) level = 0;
  else if(pcber >= ant_tbl[1]) level = 1;
  else if(pcber >= ant_tbl[2]) level = 2;
  else if(pcber >= ant_tbl[3]) level = 3;
  else if(pcber >= ant_tbl[4]) level = 4;
  else if(pcber < ant_tbl[4]) level = 5;
   
  if((level > 0) && (abs(level - save_level) == 1))
  {
    if (pcber < (ant_tbl[level - 1] - (level < 3 ? hystery_value[0] : hystery_value[1])))
    {
      save_level = level;
    }
  }
  else 
  {
    save_level = level;
  }

#if 0
  if(rssi > -75) {ant_tbl = ant75;TDMB_MSG_TCC_BB("[%d] ant75 [%d] Ant[%d]\n",rssi,(unsigned int)pcber,save_level);}
  else if(rssi > -80) {ant_tbl = ant85;TDMB_MSG_TCC_BB("[%d]   ant80 [%d] Ant[%d]\n",rssi,(unsigned int)pcber,save_level);}
  else if(rssi > -85) {ant_tbl = ant85;TDMB_MSG_TCC_BB("[%d]   ant85 [%d] Ant[%d]\n",rssi,(unsigned int)pcber,save_level);}
  else if(rssi > -90) {ant_tbl = ant85;TDMB_MSG_TCC_BB("[%d]   ant90 [%d] Ant[%d]\n",rssi,(unsigned int)pcber,save_level);}
  else if(rssi > -100) {ant_tbl = ant100;TDMB_MSG_TCC_BB("[%d]     ant100 [%d] Ant[%d]\n",rssi,(unsigned int)pcber,save_level);}
#endif

  return save_level;

}


#if (defined(CONFIG_SKY_EF40S_BOARD) || defined(CONFIG_SKY_EF40K_BOARD))
#if 0
/*====================================================================
FUNCTION       tcc3170_Ant_Level
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_Ant_Level_EF40(uint32 pcber,int rssi)
{
  // set ant level (0 ~ 6 level)
#if 0
  //from TCC SDK
  if(pcber <= 50) ant_level = 0;
  else if(pcber <= 30000) ant_level = 5;
  else if(pcber <= 50000) ant_level = 4;
  else if(pcber <= 70000) ant_level = 3;
  else if(pcber <= 90000) ant_level = 2;
  else if(pcber <= 120000) ant_level = 1;
  else if(pcber > 120000) ant_level = 0;
#endif

  uint8 level = 0;
  static uint8 save_level = 0;
  uint16 hystery_value[]= {5000,5000};
//111021 telechips ANT bar patch, different ant table by RSSI

#if 1
  uint32 ant75[] = {120000, 90000, 70000, 50000, 30000,};
  uint32 ant80[] = {120000, 90000, 70000, 50000, 1,};
  uint32 ant85[] = {120000, 90000, 70000, 1,     1,};
  uint32 ant90[] = {120000, 90000, 1,     1,     1,};
  uint32 ant100[]= {120000, 1,     1,     1,     1,};
  uint32 *ant_tbl = ant75;
  
  if(rssi > -75) ant_tbl = ant75;
  else if(rssi > -80) ant_tbl = ant80;
  else if(rssi > -85) ant_tbl = ant85;
  else if(rssi > -90) ant_tbl = ant90;
  else if(rssi > -100) ant_tbl = ant100;

  if(pcber >= ant_tbl[0] || pcber <= 50) level = 0;
  else if(pcber >= ant_tbl[1] ) level = 1;
  else if(pcber >= ant_tbl[2] ) level = 2;
  else if(pcber >= ant_tbl[3] ) level = 3;
  else if(pcber >= ant_tbl[4] ) level = 4;
  else if(pcber < ant_tbl[4]) level = 5;
#else
    uint32 ant_tbl[] = {1200, // 0 <-> 1
                         900,   // 1 <-> 2
                         700,   // 2 <-> 3
                         500,   // 3 <-> 4
                         300};  // 4 <-> 5
    
    if(pcber >= ant_tbl[0] || pcber <= 50) level = 0;
    else if(pcber >= ant_tbl[1] && pcber < ant_tbl[0]) level = 1;
    else if(pcber >= ant_tbl[2] && pcber < ant_tbl[1]) level = 2;
    else if(pcber >= ant_tbl[3] && pcber < ant_tbl[2]) level = 3;
    else if(pcber >= ant_tbl[4] && pcber < ant_tbl[3]) level = 4;
    else if(pcber < ant_tbl[4]) level = 5;
#endif

   
  if((level > 0) && (abs(level - save_level) == 1))
  {
    if (pcber < (ant_tbl[level - 1] - (level < 3 ? hystery_value[0] : hystery_value[1])))
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
#endif
#endif /* CONFIG_SKY_EF40S_BOARD */


/*====================================================================
FUNCTION       tcc3170_reconfig_n_ber
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_get_ber(tdmb_bb_sig_type *psigs)
{
  struct tcbd_status_data status = {0,};
  struct tcbd_device *dev = &tcc3170_dev_data.device;

  if (first_getber_flag) // [cys] tcbd_moving_avg의 array의 size 만큼 더 읽는다.
  {
    tcbd_read_signal_info(dev, &status); // 1
    tcbd_read_signal_info(dev, &status); // 2
    first_getber_flag = 0;

    TDMB_MSG_TCC_BB("[%s] 1st get_ber after subch_start\n",  __func__);
  }

  tcbd_read_signal_info(dev, &status);
  psigs->PCBER = status.pcber;
  psigs->RSBER = status.vber;
  psigs->SNR = status.rssi;
#if (defined(CONFIG_SKY_EF40S_BOARD) || defined(CONFIG_SKY_EF40K_BOARD))
  psigs->RSSI = tcc3170_Ant_Level_EF40(status.pcber, status.rssi);
#else
  psigs->RSSI = tcc3170_Ant_Level(status.pcber, status.rssi);
#endif
  //TDMB_MSG_TCC_BB("[%s] pcber[%d], rssi[%d] snr[%d] rsber[%d]\n",  __func__, psigs->PCBER, psigs->RSSI, psigs->SNR, psigs->RSBER);
}


#ifdef FEATURE_DMB_TSIF_IF
/*====================================================================
FUNCTION       tcc3170_tsif_parser_callback
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_tsif_parser_callback(int dev_idx, unsigned char *_stream, int _size, int _subch_id, int _type)
{
  static int msc_buf_size = 0;

  //TDMB_MSG_TCC_BB("[%s] type[%d], size[%d]\n", __func__, _type, _size);

#if 0
  if(msc_buf_idx)
    TDMB_MSG_TCC_BB("[%s] msc_buf_idx[%d], size[%d]\n", __func__, msc_buf_idx, _size);
#endif

  if(tsif_read_flag)
  {
    msc_buf_size = 0;
    msc_buf_idx = (msc_buf_idx? 0: 1);
    tsif_read_flag = 0;
  }

  switch (_type)
  {
  case 0: // MSC
    //TDMB_MSG_TCC_BB("[%s] msc_buf_idx[%d], msc_buf_change_flag[%d], msc_buf_size[%d]\n", __func__, msc_buf_idx, tsif_read_flag, msc_buf_size);

    if((msc_buf_size + _size) > BB_MAX_DATA_SIZE)
    {
      TDMB_MSG_TCC_BB("[%s] buffer overflow [%d] > [%d]!!\n", __func__, msc_buf_size + _size, BB_MAX_DATA_SIZE);
      return;
    }
    else if(_size > BB_MAX_DATA_SIZE)
    {
      TDMB_MSG_TCC_BB("[%s] MSC size[%d] > buf size [%d]!!\n", __func__, _size, BB_MAX_DATA_SIZE);
      _size = BB_MAX_DATA_SIZE;
    }

    mutex_lock(&lock);

    //dmb_data_dump(_stream, _size, "/data/misc/dmb/raw_msc_afterParsing1.txt");

    memcpy(&ts_tmp_data[msc_buf_idx].msc_buf[msc_buf_size], _stream, _size);
    msc_buf_size = msc_buf_size + _size;
    ts_tmp_data[msc_buf_idx].msc_size = msc_buf_size;

    mutex_unlock(&lock);
    break;

  case 1: // FIC
    if(_size > TC3170_FIC_SIZE)
    {
      //TDMB_MSG_TCC_BB("[%s] FIC size overflow [%d]!!\n", __func__, _size);
      _size = TC3170_FIC_SIZE;
    }

    memcpy(ts_tmp_data[msc_buf_idx].fic_buf, _stream, _size);
    ts_tmp_data[msc_buf_idx].fic_size = _size;
    break;

  case 2: // STATUS
    TDMB_MSG_TCC_BB("[%s] status!!\n", __func__);
    break;

  default:
    TDMB_MSG_TCC_BB("[%s] unknown type!! size[%d] type[%d] subch_id:[%d]\n", __func__, _size, _type, _subch_id);
    break;
  }
}


/*====================================================================
FUNCTION       tcc3170_tsif_data_parser
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_tsif_data_parser(uint8* data_buf, uint32 size)
{
  static int msc_buf_r_idx;

  //TDMB_MSG_TCC_BB("[%s] \n", __func__);

   //dmb_data_dump(ts_data.msc_buf, packet_size, "/data/misc/dmb/raw_msc.txt");

  if(tsif_read_flag)
    TDMB_MSG_TCC_BB("[%s] check!!! msc_buf_change_flag[%d]\n", __func__, tsif_read_flag);

  tsif_read_flag = 1;

  //TDMB_MSG_TCC_BB("[%s] msc_buf_idx[%d], msc_buf_r_idx[%d]\n", __func__, msc_buf_idx, msc_buf_r_idx);

  msc_buf_r_idx = msc_buf_idx;

  tcbd_split_stream(0, data_buf, size);

  ts_data.fic_size = ts_tmp_data[msc_buf_r_idx].fic_size;
  if(ts_data.fic_size)
  {
    ts_tmp_data[msc_buf_r_idx].fic_size = 0;
    if(ts_data.fic_size)
      memcpy(ts_data.fic_buf, ts_tmp_data[msc_buf_r_idx].fic_buf, ts_data.fic_size);
  }

  ts_data.msc_size = ts_tmp_data[msc_buf_r_idx].msc_size;
  if(ts_data.msc_size)
  {
    ts_tmp_data[msc_buf_r_idx].msc_size = 0;
    if(ts_data.msc_size)
    {
      memcpy(ts_data.msc_buf, ts_tmp_data[msc_buf_r_idx].msc_buf, ts_data.msc_size);  
      //dmb_data_dump(ts_data.msc_buf, ts_data.msc_size, "/data/misc/dmb/raw_msc_afterParsing2.txt");
    }
  }

  if(ts_data.msc_size)
  {
    ts_data.type = MSC_DATA;
    if(ts_data.fic_size)
      ts_data.type = FIC_MSC_DATA;
  }
  else if(ts_data.fic_size)
  {
    ts_data.type = FIC_DATA;
  }
  else
  {
    TDMB_MSG_TCC_BB ("[%s] msc_size[%d], fic_size[%d]\n", __func__, ts_data.msc_size, ts_data.fic_size);
  }

  return;
}
#endif


#ifdef FEATURE_DMB_SPI_IF
/*====================================================================
FUNCTION       tcc3170_spi_put_data
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_spi_put_data(uint8* data_buf, uint32 size, uint8 type)
{
  TDMB_MSG_TCC_BB("[%s] type[%d]\n", __func__, type);

#ifdef FEATURE_DMB_SPI_IF
  switch(type)
  {
  case 0: /* MSC */
    tcc3170_spi_put_msc_data(data_buf, size);
    break;

  case 1: /* FIC */
    tcc3170_spi_put_fic_data(data_buf, size);
    break;

  default:
    break;
  }
#endif /* FEATURE_DMB_SPI_IF */
  return 0;
}


/*====================================================================
FUNCTION       tcc3170_spi_put_fic_data
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_spi_put_fic_data(uint8* data_buf, uint32 size)
{
  //TDMB_MSG_TCC_BB("[%s]\n", __func__);

  if(size > TC3170_FIC_SIZE)
  {
    //TDMB_MSG_TCC_BB("[%s] FIC size overflow [%d]!!\n", __func__, _size);
    size = TC3170_FIC_SIZE;
  }

  memcpy(tcc_data.tcc_fic_buf, data_buf, size);
  tcc_data.tcc_fic_size = size;

  return TRUE;
}


/*====================================================================
FUNCTION       tcc3170_spi_put_msc_data
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_spi_put_msc_data(uint8* data_buf, uint32 size)
{
  //TDMB_MSG_TCC_BB("[%s] crc[%d] type[%d]\n", __func__, crc, type);
  
#if 0 //FEATURE_TDMB_MULTI_CHANNEL_ENABLE
#ifdef MSC_BUFFER_FOR_TCC_SDK
  memcpy((void*)&tcc_data.tcc_msc_buf[tcc_data.tcc_msc_size], data_buf, size);
  tcc_data.tcc_msc_size += size;
  msc_put_cnt++;
  TDMB_MSG_TCC_BB("Put  MSC  -- now size[%d] x [%d] = total[%d] \n", (int)size, msc_put_cnt, tcc_data.tcc_msc_size);
#else
#ifdef FEATURE_TS_PKT_MSG
  TDMB_MSG_TCC_BB("--> [%d] Put MSC size[%d]  [%x] [%x]\n",type,(int)size,data_buf[0],data_buf[1]);
#endif
  tcc_data.tcc_msc_size = size;
  memcpy((void*)&tcc_data.tcc_msc_buf[msc_put_count*TC3170_MSC_BUF_SIZE], data_buf, size);
  msc_put_count = (msc_put_count +1) % TC3170_MSC_BUF_SIZE;
#endif
#if 0//def FEATURE_TS_PKT_MSG
  if(type == SRVTYPE_DMB)
  {
    uint16 cnt;
    for(cnt=0; cnt<size;cnt+=188)
    {
      if((data_buf[cnt+0] != 0x47) )//||(data_buf[cnt+1] == 0x80))
      {
        TDMB_MSG_TCC_BB("%s [%x] [%x] [%x] [%x]\n", __func__, data_buf[cnt+0],data_buf[cnt+1], data_buf[cnt+2], data_buf[cnt+3]);
      }
    }
  }
#endif
#else
#if 0
  tcc_data.tcc_msc_size = size;

  if (msc_put_count >= TC3170_MSC_BUF_SIZE)
  {
    msc_put_count = TC3170_MSC_BUF_SIZE;
  }
  else
  {
    memcpy((void*)&tcc_data.tcc_msc_buf[msc_put_count*TC3170_MSC_BUF_SIZE], data_buf, size);
    msc_put_count ++;
  }
#else
  if(size > BB_MAX_DATA_SIZE)
  {
    //TDMB_MSG_TCC_BB("[%s] MSC size overflow [%d]!!\n", __func__, _size);
    size = BB_MAX_DATA_SIZE;
  }

  memcpy((void*)tcc_data.tcc_msc_buf, data_buf, size);
  tcc_data.tcc_msc_size = size;
#endif
#endif

  return TRUE;
}
#endif /* FEATURE_DMB_SPI_IF */


/*================================================================== */
/*=================    TCC3170 BB Test Function     ================== */
/*================================================================== */

/*====================================================================
FUNCTION       tcc3170_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_test(int ch)
{
  // Just start TCC3170 & check MSC data
  chan_info tcc_ch_Info;
  uint8 result;

  TDMB_MSG_TCC_BB("[%s] Start\n", __func__);

  tcc3170_init();

  //while(1)
  { 
    tdmb_get_fixed_chan_info((service_t)ch, &tcc_ch_Info);
 
    TDMB_MSG_TCC_BB("[%s]  freq[%d] type[%d] subchid[0x%x]\n", __func__, (int)tcc_ch_Info.ulRFNum, tcc_ch_Info.uiServiceType, tcc_ch_Info.uiSubChID);

    tcc3170_set_subchannel_info(&tcc_ch_Info);
  
    tcc3170_ch_scan_start((int)tcc_ch_Info.ulRFNum, 0,0);
  
    result = tcc3170_subch_start((uint8*)&tcc_ch_Info, (uint32)tcc_ch_Info.uiBitRate);
    
    TDMB_MSG_TCC_BB("[%s] end !![%d]\n", __func__, result);
  
    msleep(500);
    msleep(500);
  }
}


extern void TcbdTestIo(struct tcbd_device *_device, s32 _testItem);

/*====================================================================
FUNCTION       tcc3170_rw_test
DESCRIPTION  
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 tcc3170_rw_test(void) 
{
  u8  data;
  struct tcbd_device *dev = &tcc3170_dev_data.device;

  // chip version read -> default 0x37
  tcbd_reg_read(dev, 0x0C, &data);
  TDMB_MSG_TCC_BB("[%s] Chip ID : [0x%x]\n",__func__, data);

#if 0
#ifdef FEATURE_DMB_CLK_19200
  tcbd_init_pll(dev, CLOCK_19200KHZ);
#elif defined(FEATURE_DMB_CLK_24576)
  tcbd_init_pll(dev, CLOCK_24576KHZ);
#endif
#endif

  TcbdTestIo(dev, 0);
  TDMB_MSG_TCC_BB("[%s] 0 end\n",__func__);
  TcbdTestIo(dev, 1);
  TDMB_MSG_TCC_BB("[%s] 1 end\n",__func__);
  TcbdTestIo(dev, 2);
  TDMB_MSG_TCC_BB("[%s] 2 end\n",__func__);
  TcbdTestIo(dev, 3);
  TDMB_MSG_TCC_BB("[%s] 3 end\n",__func__);
  TcbdTestIo(dev, 4);
  TDMB_MSG_TCC_BB("[%s] 4 end\n",__func__);
  TcbdTestIo(dev, 5);
  TDMB_MSG_TCC_BB("[%s] 5 end\n",__func__);
  
  TDMB_MSG_TCC_BB("[%s] tcc3170 RW Test end\n", __func__);

  return 0;
}
