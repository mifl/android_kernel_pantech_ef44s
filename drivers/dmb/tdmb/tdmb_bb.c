//=============================================================================
// File       : tdmb_bb.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2007/08/08       hkyu           drafty
//  1.1.0       2009/04/28       yschoi         Android Porting
//=============================================================================

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <mach/gpio.h>
#include <asm/irq.h>

#include "tdmb_comdef.h"
#include "tdmb_dev.h"
#include "tdmb_chip.h"
#include "tdmb_bb.h"
#include "tdmb_test.h"

/*================================================================== */
/*================        TDMB BB Definition       ================= */
/*================================================================== */

#if 0 // test
//cys for ts data dump
//#define FEATURE_TEST_TS_DUMP

#ifdef FEATURE_TEST_TS_DUMP // cys for test
#define MAX_TS_SIZE1 (INC_INTERRUPT_SIZE * 300)
uint8   aTsBuff1[MAX_TS_SIZE1];
uint32  uiCnt1 = 0;
#endif
#endif

#if 0 // android not used
#define DMB_SYNC_BYTE 0x47
#define DAB_SYNC_BYTE 0xff
#define TS_RESYNC_CNT 3
#define AUTOSCAN_TRY_MAX_CNT 50
#define AUTOSCAN_RESYNC_MAX_CNT 10
#endif // 0

#if defined(FEATURE_TDMB_USE_INC_T3700)
#define TDMB_BB_DRIVE_INIT(x) \
      tdmb_bb_t3700_init(x); \
      TDMB_MSG_BB("TDMB BB ---> [T3700]");
#elif defined(FEATURE_TDMB_USE_INC_T3900)
#define TDMB_BB_DRIVE_INIT(x)  \
      tdmb_bb_t3900_init(x); \
      TDMB_MSG_BB("TDMB BB ---> [T3900]");
#elif defined(FEATURE_TDMB_USE_FCI_FC8050)
#define TDMB_BB_DRIVE_INIT(x)  \
      tdmb_bb_fc8050_init(x); \
      TDMB_MSG_BB("TDMB BB ---> [FC8050]");
#elif defined(FEATURE_TDMB_USE_RTV_MTV350)
#define TDMB_BB_DRIVE_INIT(x)  \
      tdmb_bb_mtv350_init(x); \
      TDMB_MSG_BB("TDMB BB ---> [MTV350]");
#elif defined(FEATURE_TDMB_USE_TCC_TCC3170)
#define TDMB_BB_DRIVE_INIT(x)  \
      tdmb_bb_tcc3170_init(x); \
      TDMB_MSG_BB("TDMB BB ---> [TCC3170]");
#else
#error code "no tdmb baseband"
#endif

/*================================================================== */
/*====================== jaksal add BB Function =================== */
/*================================================================== */
tdmb_bb_function_table_type tdmb_bb_function_table;
static boolean tdmb_bb_initialized = FALSE;
tBBSYNCStage bb_sync_stage;
tSignalQuality g_tSigQual;

tdmb_mode_type dmb_mode = TDMB_MODE_AIR_PLAY;
tdmb_autoscan_state autoscan_state = AUTOSCAN_EXIT_STATE;

const uint32 KOREA_BAND_TABLE[] = {
  175280,177008,178736,
  181280,183008,184736,
  187280,189008,190736,
  193280,195008,196736,
  199280,201008,202736,
  205280,207008,208736,
  211280,213008,214736
};

const uint16 BAND_TABLE_MAX[]= {21, 38, 23, 23, 31};//{KOREA, BAND-3, L-BAND, CANADA, CHINA_BAND_III}

typedef struct {
  unsigned int snr;
  unsigned int pcber;
  unsigned int rsber;
  unsigned char rssi;
} temp_debug_type;

uint8 gFrequencyBand;
uint8 gFreqTableNum;

tBBStatus ch_verify_result = BB_SUCCESS;

#ifdef FEATURE_DMB_EBI_IF
void *ebi2_tdmb_base;
#endif

#ifdef FEATURE_TDMB_USE_FCI
tdmb_bb_int_type fci_int_type;
#endif /* FEATURE_TDMB_USE_FCI */



/*================================================================== */
/*====================== jaksal add BB Function =================== */
/*================================================================== */

static uint8 tdmb_baseband_drv_init(void);
static void tdmb_baseband_power_on(void);
static void tdmb_baseband_power_off(void);
static void tdmb_baseband_ch_scan_start(int, int, unsigned char);
static void tdmb_baseband_resync(unsigned char);
static int tdmb_baseband_subch_start(uint8 *, uint32);
static tdmb_bb_int_type tdmb_baseband_read_int(void);
static tBBStatus tdmb_baseband_get_sync_status(void);
static int tdmb_baseband_read_fib(uint8 *);
static void tdmb_baseband_set_subchannel_info(void *);
static int tdmb_baseband_read_msc(uint8 *);
static void tdmb_baseband_get_ber(tdmb_bb_sig_type *);
static uint8 tdmb_baseband_ch_stop(void);
static void tdmb_baseband_powersave_mode(void);
static void tdmb_baseband_ch_test(int ch);
static int tdmb_bb_set_freq_band(int);

/*====================================================================
FUNCTION       tdmb_bb_func_tbl_init  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean tdmb_bb_func_tbl_init(void)
{
  TDMB_MSG_BB("[%s]~!!!\n", __func__);

  if(tdmb_bb_initialized)
    return TRUE;    

  tdmb_bb_function_table.tdmb_bb_drv_init       = tdmb_baseband_drv_init;
  tdmb_bb_function_table.tdmb_bb_power_on       = tdmb_baseband_power_on;
  tdmb_bb_function_table.tdmb_bb_power_off      = tdmb_baseband_power_off;
  tdmb_bb_function_table.tdmb_bb_ch_scan_start  = tdmb_baseband_ch_scan_start;
  tdmb_bb_function_table.tdmb_bb_resync         = tdmb_baseband_resync;
  tdmb_bb_function_table.tdmb_bb_subch_start    = tdmb_baseband_subch_start;
  tdmb_bb_function_table.tdmb_bb_read_int       = tdmb_baseband_read_int;
  tdmb_bb_function_table.tdmb_bb_get_sync_status= tdmb_baseband_get_sync_status;
  tdmb_bb_function_table.tdmb_bb_read_fib       = tdmb_baseband_read_fib;
  tdmb_bb_function_table.tdmb_bb_set_subchannel_info = tdmb_baseband_set_subchannel_info;
  tdmb_bb_function_table.tdmb_bb_read_msc       = tdmb_baseband_read_msc;
  tdmb_bb_function_table.tdmb_bb_get_ber        = tdmb_baseband_get_ber;
  tdmb_bb_function_table.tdmb_bb_ch_stop        = tdmb_baseband_ch_stop;
  tdmb_bb_function_table.tdmb_bb_powersave_mode = tdmb_baseband_powersave_mode;
  tdmb_bb_function_table.tdmb_bb_ch_test        = tdmb_baseband_ch_test;

  tdmb_bb_initialized = TDMB_BB_DRIVE_INIT(&tdmb_bb_function_table);

  return tdmb_bb_initialized;
}

/*====================================================================
FUNCTION       tdmb_bb_set_freq_band
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int tdmb_bb_set_freq_band(int which_Band)
{
  int ret = 1;

  //TDMB_MSG_BB("[%s]~!!!\n", __func__);
  
  switch(which_Band)
  {
    case KOREA_BAND:
      gFrequencyBand = KOREA_BAND;
      gFreqTableNum = BAND_TABLE_MAX[(KOREA_BAND-1)]; 
    break;
    
    case BAND_III:  
    case L_BAND:   
    case CANADA_BAND:
    default:
      ret = 0;
    break;
  }

  //TDMB_MSG_BB("[%s] band [%d]  freq num [%d]\n", __func__, gFrequencyBand, gFreqTableNum);

  return ret;
}

/*====================================================================
FUNCTION       tdmb_bb_drv_init
DESCRIPTION  
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tdmb_bb_drv_init(void)
{
  int ret = 0;

  TDMB_MSG_BB("[%s]~!!!\n", __func__);

  ret = tdmb_bb_set_freq_band(KOREA_BAND);
 
  tdmb_bb_power_on(); 
  tdmb_bb_init();

  return ret;
}

/*====================================================================
FUNCTION       tdmb_bb_init  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_init(void)
{
  TDMB_MSG_BB("[%s]~!!!\n", __func__);

  tdmb_bb_function_table.tdmb_bb_drv_init();
}

/*====================================================================
FUNCTION       tdmb_bb_powersave_mode  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_powersave_mode(void)
{
  TDMB_MSG_BB("[%s]!!!\n", __func__);

  tdmb_bb_function_table.tdmb_bb_powersave_mode();
}

EXPORT_SYMBOL(tdmb_bb_powersave_mode);

/*====================================================================
FUNCTION       tdmb_bb_power_on  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_power_on(void)
{
  TDMB_MSG_BB("[%s]~!!!\n", __func__);

  if(!tdmb_bb_initialized)
  {
    if(tdmb_bb_func_tbl_init() == FALSE)
      return;
  }

  tdmb_bb_function_table.tdmb_bb_power_on();

#ifdef CONFIG_EF10_BOARD // hdmi powersave_mode
  // EF10은 hdmi와 전원 공유하므로 hdmi on시 TDMB powersave_mode 필요.
#endif /* CONFIG_EF10_BOARD */
}

/*====================================================================
FUNCTION       tdmb_bb_power_off  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_power_off(void)
{
  TDMB_MSG_BB("[%s]~!!!\n", __func__);

  tdmb_bb_function_table.tdmb_bb_power_off();
}

/*====================================================================
FUNCTION       tdmb_bb_ch_scan_start
DESCRIPTION   single channel scan
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_ch_scan_start(int freq, int band, unsigned char freq_offset)
{
  TDMB_MSG_BB("[%s]~!!!\n", __func__);

  tdmb_bb_function_table.tdmb_bb_ch_scan_start(freq, band, freq_offset);
}

/*====================================================================
FUNCTION       tdmb_bb_get_frequency
DESCRIPTION  해당 Band에 맞는 주파수와 주파수를 얻어온다
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_get_frequency(unsigned long *freq, unsigned char band,unsigned int index)
{
  //TDMB_MSG_BB("[%s]\n", __func__);

  switch(band)
  {
    case KOREA_BAND :
      *freq = KOREA_BAND_TABLE[index];
    break;
    
    case BAND_III :
    break;
    
    case L_BAND :
    break;
    
    case CANADA_BAND :
    break;
    
    default:
    break;
  }
  
  //TDMB_MSG_BB("[%s] freq[%d]  band[%d] index[%d]\n", __func__, (int)*freq, (int)band,index);
  
}

/*====================================================================
FUNCTION       tdmb_bb_fic_process
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_fic_process(void)
{
  TDMB_MSG_BB("[%s]~!!!\n", __func__);
}

/*====================================================================
FUNCTION       tdmb_bb_set_int
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_set_int(boolean on_off)
{
  TDMB_MSG_BB("[%s] on_off[%d]\n", __func__, on_off);

#if(defined(FEATURE_TDMB_USE_FCI) && defined(FEATURE_DMB_TSIF_IF))
  fc8050_set_int(on_off);
#endif

  return;
}

/*====================================================================
FUNCTION       tdmb_bb_set_fic_isr
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean tdmb_bb_set_fic_isr(boolean on_off)
{
  bool ret;

  TDMB_MSG_BB("[%s] on_off[%d]\n", __func__, on_off);

  ret = tdmb_set_isr(on_off);

  return ret;
}

/*====================================================================
FUNCTION       tdmb_bb_chw_IntHandler2
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_chw_IntHandler2(void)
{
  TDMB_MSG_BB("[%s]~!!!\n", __func__);
}

/*====================================================================
FUNCTION       tdmb_bb_ebi2_chw_IntHandler
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint16 tdmb_bb_ebi2_chw_IntHandler(uint8 *ts_stream_buffer)
{
  uint16 chw_size;

  TDMB_MSG_BB("[%s]~!!!\n", __func__);

  chw_size = tdmb_bb_read_msc(ts_stream_buffer);

  //TDMB_MSG_BB("[%s] chw INTR  TS size[%d]= [0x%x] [0x%x] [0x%x] [0x%x] 0x%x\n", __func__, chw_size, ts_stream_buffer[0], ts_stream_buffer[1],ts_stream_buffer[2],size[0],size[1]);

  return chw_size;
}

/*====================================================================
FUNCTION       tdmb_bb_resync
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_resync(unsigned char ucIMR)
{
  TDMB_MSG_BB("[%s]~!!!\n", __func__);

  tdmb_bb_function_table.tdmb_bb_resync(ucIMR);
}

/*====================================================================
FUNCTION       tdmb_bb_subch_start
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tdmb_bb_subch_start (uint8 *Regs, uint32 data_rate)
{
  int ret;

  ret = tdmb_bb_function_table.tdmb_bb_subch_start(Regs, data_rate);

  return ret;
}

/*====================================================================
FUNCTION       tdmb_bb_drv_start
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_drv_start(void)
{
  TDMB_MSG_BB("[%s]~!!!\n", __func__);
}

/*====================================================================
FUNCTION       tdmb_bb_drv_stop
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_drv_stop(void)
{
  TDMB_MSG_BB("[%s]~!!!\n", __func__);
}

/*====================================================================
FUNCTION       tdmb_bb_report_debug_info  
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_report_debug_info(void)
{
  TDMB_MSG_BB("[%s]~!!!\n", __func__);
}

/*====================================================================
FUNCTION       tdmb_bb_get_tuning_status
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tBBSYNCStage tdmb_bb_get_tuning_status(void)
{
  //TDMB_MSG_BB("[%s]~!!!\n", __func__);

  if(tdmb_bb_get_sync_status() == BB_SUCCESS)
  {
      bb_sync_stage = BB_SYNC_STAGE_3;      
  }
  else
  {
      bb_sync_stage = BB_SYNC_STAGE_0;      
    TDMB_MSG_BB("[%s] SYNC_LOCK fail %d\n", __func__, bb_sync_stage);
  }  

  return bb_sync_stage;
}

/*====================================================================
FUNCTION       tdmb_bb_set_fic_ch_result
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_set_fic_ch_result(tBBStatus result)
{
  TDMB_MSG_BB("[%s]~!!!\n", __func__);

  ch_verify_result = result;
}

/*====================================================================
FUNCTION       tdmb_bb_get_fic_ch_result
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tBBStatus tdmb_bb_get_fic_ch_result(void)
{
  TDMB_MSG_BB("[%s]~!!!\n", __func__);

  return ch_verify_result;
}

/*====================================================================
FUNCTION       tdmb_bb_read_int
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tdmb_bb_int_type tdmb_bb_read_int(void)
{
  return tdmb_bb_function_table.tdmb_bb_read_int();
}

/*====================================================================
FUNCTION       tdmb_bb_get_sync_status
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tBBStatus tdmb_bb_get_sync_status(void)
{
  return tdmb_bb_function_table.tdmb_bb_get_sync_status();
}

/*====================================================================
FUNCTION       FIC_read_fib_data
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
dword tdmb_bb_read_fib(byte *fib_buf)
{
  return tdmb_bb_function_table.tdmb_bb_read_fib(fib_buf);
}

/*====================================================================
FUNCTION       tdmb_bb_set_subchannel_info
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_set_subchannel_info(void *sub_ch_info)
{
  tdmb_bb_function_table.tdmb_bb_set_subchannel_info(sub_ch_info);
}

/*====================================================================
FUNCTION       tdmb_bb_read_msc
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tdmb_bb_read_msc(uint8 *msc_buf)
{
  int ret;
  ret = tdmb_bb_function_table.tdmb_bb_read_msc(msc_buf);

#ifdef FEATURE_NETBER_TEST_ON_BOOT
    netber_GetError(ret, msc_buf);
#endif

  return ret;
}

/*====================================================================
FUNCTION       tdmb_bb_get_ber
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_get_ber(void)
{
  tdmb_bb_sig_type signals;

  tdmb_bb_function_table.tdmb_bb_get_ber(&signals);

  g_tSigQual.PCBER = signals.PCBER;
#ifdef FEATURE_TDMB_USE_FCI
  //antenna bar = RSSI = SNR  change..
  g_tSigQual.RSSI = (unsigned char)signals.SNR;  
  g_tSigQual.SNR = (unsigned int)signals.RSSI;
#else
  g_tSigQual.RSSI = signals.RSSI;
  g_tSigQual.SNR = signals.SNR;
#endif /* FEATURE_TDMB_USE_INC */
  g_tSigQual.RSBER = signals.RSBER;

  //TDMB_MSG_BB("[%s] pcber[%d], rssi[%d] snr[%d] rsber[%d]\n", __func__, g_tSigQual.PCBER, g_tSigQual.RSSI, g_tSigQual.SNR, g_tSigQual.RSBER);
}

/*====================================================================
FUNCTION       tdmb_bb_ch_stop
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_ch_stop(void)
{
  tdmb_bb_function_table.tdmb_bb_ch_stop();
}

/*====================================================================
FUNCTION       tdmb_bb_ch_test
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_ch_test(int ch)
{
  tdmb_bb_function_table.tdmb_bb_ch_test(ch);
}

/*====================================================================
FUNCTION       tdmb_baseband_drv_init
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static uint8 tdmb_baseband_drv_init(void)
{
  return 0;
}

/*====================================================================
FUNCTION       tdmb_baseband_power_on
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void tdmb_baseband_power_on(void)
{
}

/*====================================================================
FUNCTION       tdmb_basebane_power_off
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void tdmb_baseband_power_off(void)
{
}


/*====================================================================
FUNCTION       tdmb_baseband_ch_scan_start
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void tdmb_baseband_ch_scan_start(int freq, int band, unsigned char freq_offset)
{
}

/*====================================================================
FUNCTION       tdmb_baseband_resync
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void tdmb_baseband_resync(unsigned char ucIMR)
{
}

/*====================================================================
FUNCTION       tdmb_baseband_subch_start
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int tdmb_baseband_subch_start(uint8 *regs, uint32 data_rate)
{
  return 0;
}

/*====================================================================
FUNCTION       tdmb_baseband_read_int
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static tdmb_bb_int_type tdmb_baseband_read_int(void)
{
  return 0;
}

/*====================================================================
FUNCTION       tdmb_baseband_get_sync_status
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static tBBStatus tdmb_baseband_get_sync_status(void)
{
  return 0;
}

/*====================================================================
FUNCTION       tdmb_baseband_read_fib
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int tdmb_baseband_read_fib(uint8 *fibs)
{
  return 0;
}

/*====================================================================
FUNCTION       tdmb_baseband_set_subchannel_info
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void tdmb_baseband_set_subchannel_info(void *sub_ch_info)
{
}

/*====================================================================
FUNCTION       tdmb_baseband_read_msc
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int tdmb_baseband_read_msc(uint8 *buffer)
{
  return 0;
}

/*====================================================================
FUNCTION       tdmb_baseband_get_ber
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void tdmb_baseband_get_ber(tdmb_bb_sig_type *signal)
{
}

/*====================================================================
FUNCTION       tdmb_baseband_ch_stop
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static uint8 tdmb_baseband_ch_stop(void)
{
  return 0;
}

/*====================================================================
FUNCTION       tdmb_baseband_power_save_mode
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void tdmb_baseband_powersave_mode(void)
{
}
/*====================================================================
FUNCTION       tdmb_baseband_set_subchannel_info
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void tdmb_baseband_ch_test(int ch)
{
}

