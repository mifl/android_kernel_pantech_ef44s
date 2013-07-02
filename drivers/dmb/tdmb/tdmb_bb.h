//=============================================================================
// File       : tdmb_bb.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2007/08/08       hkyu           Draft
//  1.1.0       2009/04/28       yschoi         Android Porting
//=============================================================================

#ifndef _TDMB_BB_H_
#define _TDMB_BB_H_

#include "../dmb_type.h"
#include "tdmb_comdef.h"

/* ========== Message ID for TDMB ========== */

#define TDMB_MSG_BB(fmt, arg...) \
  DMB_KERN_MSG_ALERT(fmt, ##arg)


#define uchar unsigned char
#define ulong unsigned long
#define ushort unsigned short
#define uint unsigned int

#define _SIG_AVG_NUM_            32
#define _PCBER_RESOLUTION_      100000
#define _FTO2_UNLOCK_TIMES_     10

typedef enum _tagBBStatus {
  BB_FAIL,
  BB_SUCCESS,
  BB_SYNCKEEP,
  BB_SYNCLOST,
  BB_RECONFIG,
  BB_ANNOUNCEMENT,
  BB_BUILD_FAIL,
  // 2007/02/01 cgpark added for FIC
  BB_FIC_DECODING_DONE,
  BB_FIC_RECONFIG_OCCUR, // reconfiguration occur
  // sky tdmb scheme : ch. setting 이후 FIC data로 ch info의 가/부를 확인한다.
  BB_FIC_CHANNEL_VERIFY,  
  BB_FIC_CHANNEL_NOT_VERIFY,
  BB_FIC_CRC_ERROR
} tBBStatus;

typedef enum _tagBBSYNCStage {
  BB_SYNC_STAGE_0,
  BB_SYNC_STAGE_1,
  BB_SYNC_STAGE_2,
  BB_SYNC_STAGE_3,
  BB_SYNC_NO_SIGNAL
}tBBSYNCStage;

typedef enum {
  TM_MODE1 = 1,
  TM_MODE2, 
  TM_MODE3,
  TM_MODE4
}tm_mode_type;

//]]

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


#define KOREA_BAND  0x01
#define BAND_III    0x02
#define L_BAND      0x03
#define CANADA_BAND 0x04

///////// signal indicator //////////////
#define USE_SNR_INDICATOR

#define FRAME_SYNC_TIMEOUT    20 // 20 * 20 = 400 ms
#define AUTOSCAN_TIMEOUT      80 // 80 * 20 = 1600 ms
#define AUTOSCAN_MAX_TIMEOUT  150 //150 * 20 = 3000 ms

// TCC310 Interrupt Request
#define FIC_INT     0x10
#define FrDONE      0x04
#define BOTH_INT    0x14
#define FULLCAP_INT 0x02

#define BB_FIB_SIZE         32
#define BB_MAX_FIB_NUM      12
#define BB_MAX_FIC_SIZE   (BB_MAX_FIB_NUM * BB_FIB_SIZE) // 384
#define BB_MAX_FRAME_DURATION   96
#define BB_MAX_DATA_SIZE    (1024*12)

#ifdef FEATURE_DMB_TSIF_IF
#define TDMB_TS_PKT_SIZE  188
#define TSIF_CHUNK_SIZE    16
#define TSIF_DATA_SIZE    192
#endif

#if 0
typedef enum {
  TDMB_BB_TCC3100,
  TDMB_BB_T3300,
  TDMB_BB_FC8000,
  TDMB_BB_MAX
}tdmb_bb_id_type;
#endif // 0

typedef enum {
  TDMB_BB_SVC_NONE = 0,
  TDMB_BB_SVC_DAB,
  TDMB_BB_SVC_DMB,
#ifdef FEATURE_TDMB_VISUAL_RADIO_SERVICE
  TDMB_BB_SVC_VISUALRADIO,
#endif
  TDMB_BB_SVC_NETBER,
  TDMB_BB_SVC_DATA,
  TDMB_BB_SVC_TYPE_MAX
} tdmb_bb_service_type;

typedef enum {
  TDMB_BB_INT_FIC,
  TDMB_BB_INT_MSC,
  TDMB_BB_INT_FIC_MSC,
  TDMB_BB_INT_MAX
}tdmb_bb_int_type;

typedef struct {
  unsigned int SNR;
  unsigned int PCBER;
  unsigned int RSBER;
  unsigned char RSSI;
}tSignalQuality;

typedef  tSignalQuality tdmb_bb_sig_type;


////////////////////////////////////////////////////////////
// 요기부터 tdmb_dev.h에서 가져옴. 나중에 정리 필요.

typedef struct {
  unsigned long freq;
  unsigned char band;
  unsigned int index;
} get_frequency_type;

typedef struct {
  int freq;
  int band;
  unsigned char freq_offset;
} ch_scan_type;

typedef enum
{
  TYPE_NONE,
  FIC_DATA,
  MSC_DATA,
  FIC_MSC_DATA,
} tdmb_data_type;

typedef struct {
  tdmb_data_type type;
  int fic_size;
//#ifdef FEATURE_TDMB_MULTI_CHANNEL_ENABLE
  char fic_buf[BB_MAX_FIC_SIZE];
//#else
//  char fic_buf[1];
//#endif
  int msc_size;
  char msc_buf[BB_MAX_DATA_SIZE];
} ts_data_type;

typedef struct {
  int fib_num;
  char fic_buf[BB_MAX_FIC_SIZE];
} fic_data_type;

typedef enum
{
  TDMB_MODE_INIT =0,
  TDMB_MODE_AIR_PLAY,
  TDMB_MODE_LOCAL_PLAY,
  TDMB_MODE_AUTOSCAN,
  TDMB_MODE_NETBER,
  TDMB_MODE_EXIT,
  TDMB_MODE_MAX
} tdmb_mode_type;

#if 0
typedef enum
{
  NONE_TYPE = 0,
  DAB,
  DMB,
  DATA_
} Service_Type_Info;
#endif // 0

typedef enum
{
  G_DMB_MODE,
  G_GFREQUENCYBAND,
  G_GFREQTABLENUM,
  G_AUTOSCAN_STATE,
  G_MICRO_USB_ANT
} g_var_enum_type;

typedef enum
{
  G_READ,
  G_WRITE,
  G_RW
} g_var_rw_type;

typedef struct {
  g_var_rw_type type;
  g_var_enum_type  name;
  int32 data;
} g_var_type;

typedef enum
{
  AUTOSCAN_START_STATE = 0,
  AUTOSCAN_SYNC_STATE,
  //AUTOSCAN_WAIT_FIC_PROC_STATE,
  AUTOSCAN_FIC_PROC_DONE_STATE,
  AUTOSCAN_EXIT_STATE
} tdmb_autoscan_state;

////////////////////////////////////////////////////////////


typedef struct
{
  uint8            (*tdmb_bb_drv_init) (void);
  void             (*tdmb_bb_power_on) (void);
  void             (*tdmb_bb_power_off) (void);  
  void             (*tdmb_bb_ch_scan_start)(int, int, unsigned char);
  void             (*tdmb_bb_resync)(unsigned char);
  int              (*tdmb_bb_subch_start)(uint8 *, uint32);
  tdmb_bb_int_type (*tdmb_bb_read_int)(void);
  tBBStatus        (*tdmb_bb_get_sync_status)(void);
  int              (*tdmb_bb_read_fib)(uint8 *); 
  void             (*tdmb_bb_set_subchannel_info)(void *);
  int              (*tdmb_bb_read_msc)(byte *);
  void             (*tdmb_bb_get_ber)(tdmb_bb_sig_type *);
  uint8            (*tdmb_bb_ch_stop)(void);
  void             (*tdmb_bb_powersave_mode)(void);
  void             (*tdmb_bb_ch_test)(int ch);
} tdmb_bb_function_table_type;

#ifdef FEATURE_DMB_EBI_IF
extern void *ebi2_tdmb_base;
#endif

/*====================================================================
FUNCTION       tdmb_bb_functable_init
DESCRIPTION  
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean tdmb_bb_func_tbl_init(void);

/*====================================================================
FUNCTION       tdmb_bb_drv_init
DESCRIPTION  
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tdmb_bb_drv_init(void);

/*====================================================================
FUNCTION       tdmb_bb_init  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_init(void);

/*====================================================================
FUNCTION       tdmb_bb_power_on  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/

void tdmb_bb_powersave_mode(void);

/*====================================================================
FUNCTION       tdmb_bb_power_off  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/

void tdmb_bb_power_on(void);

/*====================================================================
FUNCTION       tdmb_bb_power_off  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_power_off(void);

/*====================================================================
FUNCTION       tdmb_bb_ch_scan_start
DESCRIPTION   single channel start
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_ch_scan_start(int freq, int band, unsigned char freq_offset);

/*====================================================================
FUNCTION       tdmb_bb_get_frequency
DESCRIPTION  해당 Band에 맞는 주파수와 주파수를 얻어온다
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_get_frequency(unsigned long *freq, unsigned char band,unsigned int index);

/*====================================================================
FUNCTION       tdmb_bb_fic_process
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_fic_process(void);

/*====================================================================
FUNCTION       tdmb_bb_set_int
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_set_int(boolean on_off);

/*====================================================================
FUNCTION       tdmb_bb_set_fic_isr
DESCRIPTION        
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean tdmb_bb_set_fic_isr(boolean on_off);

/*====================================================================
FUNCTION       tdmb_bb_chw_IntHandler2
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_chw_IntHandler2(void);

/*====================================================================
FUNCTION       tdmb_bb_ebi2_chw_IntHandler
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint16 tdmb_bb_ebi2_chw_IntHandler(uint8 *ts_stream_buffer);

/*====================================================================
FUNCTION       tdmb_bb_resync
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_resync(unsigned char ucIMR);

/*====================================================================
FUNCTION       tdmb_bb_subch_start
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tdmb_bb_subch_start(uint8 *Regs, uint32 data_rate);

/*====================================================================
FUNCTION       tdmb_bb_drv_start
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_drv_start(void);

/*====================================================================
FUNCTION       tdmb_bb_drv_stop
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_drv_stop(void);

/*====================================================================
FUNCTION       tdmb_bb_report_debug_info  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_report_debug_info(void);

/*====================================================================
FUNCTION       tdmb_bb_get_tuning_status
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tBBSYNCStage tdmb_bb_get_tuning_status(void);

/*====================================================================
FUNCTION       tdmb_bb_set_fic_ch_result
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_set_fic_ch_result(tBBStatus);

/*====================================================================
FUNCTION       tdmb_bb_get_fic_ch_result
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tBBStatus tdmb_bb_get_fic_ch_result(void);

/*====================================================================
FUNCTION       tdmb_bb_read_int
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tdmb_bb_int_type tdmb_bb_read_int(void);

/*====================================================================
FUNCTION       tdmb_bb_get_sync_status
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tBBStatus tdmb_bb_get_sync_status(void);

/*====================================================================
FUNCTION       tdmb_bb_read_fib
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
dword tdmb_bb_read_fib(byte *);

/*====================================================================
FUNCTION       tdmb_bb_set_subchannel_info
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_set_subchannel_info(void *);

/*====================================================================
FUNCTION       tdmb_bb_read_msc
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tdmb_bb_read_msc(uint8 *);

/*====================================================================
FUNCTION       tdmb_bb_i2c_test
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean tdmb_bb_i2c_test(uint8);

/*====================================================================
FUNCTION       tdmb_bb_get_ber
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_get_ber(void);

/*====================================================================
FUNCTION       tdmb_bb_ch_stop
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_ch_stop(void);

/*====================================================================
FUNCTION       tdmb_bb_ch_test
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tdmb_bb_ch_test(int ch);

#endif /* _TDMB_BB_H_ */

