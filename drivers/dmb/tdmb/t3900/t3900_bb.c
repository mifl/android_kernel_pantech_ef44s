//=============================================================================
// File       : T3900_bb.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2009/02/17       wgyoo          Draft
//  1.1.0       2009/04/28       yschoi         Android Porting
//  1.2.0       2010/07/30       wgyoo          T3900 porting
//=============================================================================

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <mach/gpio.h>

#include "../../dmb_hw.h"
#include "../../dmb_interface.h"

#include "../tdmb_comdef.h"
#include "../tdmb_dev.h"
#include "../tdmb_test.h"

#include "t3900_bb.h"
#include "t3900_includes.h"


#ifdef FEATURE_COMMUNICATION_CHECK
extern ST_SUBCH_INFO          g_SetMultiInfo;
extern INC_UINT8              g_IsChannelStart;
extern INC_UINT8              g_IsSyncChannel;
extern INC_UINT8              g_ReChanTick;
#endif

/*================================================================== */
/*=================       T3700 BB Function       ================== */
/*================================================================== */
//#define USE_FEATURE_PHONE_FIC_PROC_IN_MULTI

#define TMDB_I2C_CTRL_ID T3700_CID

#define DPLL_OUTSIDE  0
#define DPLL_INSIDE   1

#ifdef USE_FEATURE_PHONE_FIC_PROC_IN_MULTI
#ifdef INC_MULTI_CHANNEL_ENABLE
ST_FIFO g_stFifo;
ST_FIFO* g_pStFifo;
#endif
#endif /* USE_FEATURE_PHONE_FIC_PROC_IN_MULTI */

extern tdmb_mode_type dmb_mode;
typedef enum _tagINTERRUPT_CTRL
{
  INC_INTERRUPT_NON = 0,
  INC_FIC_INTERRUPT,
  INC_MSC_INTERRUPT,
}INTERRUPT_CTRL, *PINTERRUPT_CTRL;

static INTERRUPT_CTRL g_InCIntCtrl = INC_INTERRUPT_NON;
st_subch_info  g_stEnsembleInfo;

boolean tdmb_power_on = FALSE;
boolean fic_data_ready = FALSE;


typedef struct{
  uint32 freq;
  uint16 dpll;
  boolean in_air_play;
  boolean fic_started;
  uint32  ulEnsembleSame;
  int16   nBbpStatus;
  uint8   IsEnsembleChange;
  uint8   IsFreqLock;
  uint8   IsChangeEnsemble;
  uint32  ulReConfigTime;
  uint16  uiChipID;

  uint8   byAntLevel;
  uint16  wCIFTimeTick;
  uint16  wCERValue;

  uint16  wDiffErrCnt;

  uint16  uiVtbErr;
  uint16  uiVtbData;
  uint16  uiRSErrBit;
  uint16  uiRSErrTS;
  uint16  uiCER;

  uint16   ucCERCnt;
  uint16   ucRetryCnt;

  uint32  dPreBER;
  uint32  dPstBER;

  uint8   IsReSync;
  uint8   ucRfData[3];
  uint8   ucScanMode;
}ensemble_status_t;

static ensemble_status_t ensemble_status;

extern tdmb_autoscan_state autoscan_state;

static boolean t3700_function_register(tdmb_bb_function_table_type *);

extern CTRL_MODE          m_ucCommandMode;

//INC_UINT8   acStreamBuff[1024*8];

//#define FEATURE_DUMP

#ifdef FEATURE_DUMP // dump test
#define DUMP_CNT 100;
INC_UINT8   acStreamBuff_dump[1024*8*DUMP_CNT];
#endif

extern ts_data_type ts_data;

#ifdef FEATURE_TDMB_KERNEL_MSG_ON
#ifdef INC_MULTI_CHANNEL_ENABLE
char cCHtype = 'M';
#else
char cCHtype = 'S';
#endif

#ifdef FEATURE_DMB_EBI_CMD
char cCMDtype = 'E';
#else
char cCMDtype = 'I';
#endif

#ifndef FEATURE_TS_PKT_MSG
int g_packet_read_cnt = 0;
#endif
#endif /* FEATURE_TDMB_KERNEL_MSG_ON */

#ifdef INC_T3700_USE_SPI
int ch_stop_check = 0;
#endif

/*====================================================================
FUNCTION       tdmb_bb_t3700_init  
DESCRIPTION    matching T3700 Function at TDMB BB Function
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean tdmb_bb_t3900_init(tdmb_bb_function_table_type *function_table_ptr)
{
  boolean bb_initialized;

  bb_initialized = t3700_function_register(function_table_ptr);

  return bb_initialized;
}


static boolean t3700_function_register(tdmb_bb_function_table_type *ftable_ptr)
{
  ftable_ptr->tdmb_bb_drv_init          = t3700_init;
  ftable_ptr->tdmb_bb_power_on          = t3700_power_on;
  ftable_ptr->tdmb_bb_power_off         = t3700_power_off;
  ftable_ptr->tdmb_bb_ch_scan_start     = t3700_ch_scan_start;
  ftable_ptr->tdmb_bb_resync            = t3700_bb_resync;
  ftable_ptr->tdmb_bb_subch_start       = t3700_subch_start;
  ftable_ptr->tdmb_bb_read_int          = t3700_read_int;
  ftable_ptr->tdmb_bb_get_sync_status   = t3700_get_sync_status;
  ftable_ptr->tdmb_bb_read_fib          = t3700_read_fib;
  ftable_ptr->tdmb_bb_set_subchannel_info = t3700_set_subchannel_info;
  ftable_ptr->tdmb_bb_read_msc          = t3700_read_msc;
  ftable_ptr->tdmb_bb_get_ber           = t3700_get_ber;
  ftable_ptr->tdmb_bb_ch_stop           = t3700_stop;
  ftable_ptr->tdmb_bb_powersave_mode    = t3700_set_powersave_mode;
  ftable_ptr->tdmb_bb_ch_test           = t3700_test;

  return TRUE;
}


uint8 t3700_i2c_write_word(uint8 chipid, uint16 reg, uint16 data)
{
  uint8 ret = 0;

#ifdef FEATURE_DMB_I2C_CMD
  ret = dmb_i2c_write(chipid, reg, sizeof(uint16), data, sizeof(uint16));
#endif /* FEATURE_DMB_I2C_CMD */

  return ret;
}

uint8 t3700_i2c_read_word(uint8 chipid, uint16 reg, uint16 *data)
{
#ifdef FEATURE_DMB_I2C_CMD
  dmb_i2c_read(chipid, reg, sizeof(uint16), data, sizeof(uint16));
#endif /* FEATURE_DMB_I2C_CMD */

  return TRUE;
}

void t3700_i2c_read_len(uint8 chipid, unsigned int uiAddr, uint8 ucRecvBuf[], uint16 ucCount)
{
#ifdef FEATURE_DMB_I2C_CMD
  dmb_i2c_read_len(chipid, uiAddr, ucRecvBuf, ucCount, sizeof(uint16));
#endif /* FEATURE_DMB_I2C_CMD */

  return;
}


/*====================================================================
FUNCTION       t3700_set_powersave_mode
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void t3700_set_powersave_mode(void)
{
  dmb_set_gpio(DMB_RESET, 0);
  msleep(1);
}


/*====================================================================
FUNCTION       t3700_power_on
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void t3700_power_on(void)
{
// 1. DMB_RESET : LOW
// 2. DMB_PWR_EN : HIGH
// 3. DMB_RESET : HIGH
// 4. TCXO

  TDMB_MSG_INC_BB("[%s] start!!!\n", __func__);

  dmb_power_on();

  dmb_set_gpio(DMB_RESET, 0);
  msleep(1);

  dmb_power_on_chip();
  msleep(10);

  dmb_set_gpio(DMB_RESET, 1);
  msleep(100);

  tdmb_power_on = TRUE;

  TDMB_MSG_INC_BB("[%s] end!!!\n", __func__);
}


/*====================================================================
FUNCTION       t3700_power_off
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void t3700_power_off(void)
{
// 1. DMB_RESET : LOW
// 2. DMB_PWR_EN : LOW
// 3. TCXO

  TDMB_MSG_INC_BB("[%s] start!!![%d]\n", __func__, tdmb_power_on);

  if(!tdmb_power_on)
  {
    return;
  }

  dmb_set_gpio(DMB_RESET, 0);
  msleep(1);

  dmb_power_off();

#ifdef INC_T3700_USE_SPI
  ch_stop_check = 0;
#endif
  
tdmb_power_on = FALSE;

  TDMB_MSG_INC_BB("[%s] end!!!\n", __func__);
}


/*====================================================================
FUNCTION       t3700_ch_scan_start
DESCRIPTION 
    if for_air is greater than 0, this is for air play.
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void t3700_ch_scan_start(int freq, int band, unsigned char for_air)
{
  TDMB_MSG_INC_BB("[%s] Channel Frequency[%d] Band[%d] Mode[%d]\n", __func__, freq, band, for_air);

  INC_SET_INTERRUPT(TMDB_I2C_CTRL_ID, 0x0000);
  INC_CLEAR_INTERRUPT(TMDB_I2C_CTRL_ID, 0xffff);

  g_InCIntCtrl = INC_INTERRUPT_NON;

  INC_STOP(TMDB_I2C_CTRL_ID); 
  INC_READY(TMDB_I2C_CTRL_ID, freq);

  ensemble_status.freq = freq;

  // 20081111 cys, Autoscan 시작시 RF AGC cap이 충전되는 시간 100ms만큼 기다린다. (7A, 7B scan이 되지 않는 문제)
  if(((autoscan_state == AUTOSCAN_SYNC_STATE) && (freq == 175280)) || dmb_mode == TDMB_MODE_NETBER)
  {
    //rex_sleep(100);
    msleep(100);
    TDMB_MSG_INC_BB("[%s] autoscan delay\n", __func__);
  }

  if(INC_SYNCDETECTOR(TMDB_I2C_CTRL_ID, freq, 0) == INC_SUCCESS)
  {
    TDMB_MSG_INC_BB("[%s] SYNCDETECT SUCCESS\n", __func__);
    ensemble_status.dpll = DPLL_INSIDE;
    if(for_air)
      ensemble_status.fic_started = TRUE;

    g_InCIntCtrl = INC_FIC_INTERRUPT;
    INC_SET_INTERRUPT(TMDB_I2C_CTRL_ID, INC_FIC_INTERRUPT_ENABLE);
    INC_CLEAR_INTERRUPT(TMDB_I2C_CTRL_ID, INC_FIC_INTERRUPT_ENABLE);
  }
  else
  {
    TDMB_MSG_INC_BB("[%s] SYNCDETECT ERROR\n", __func__);

    if(for_air)
    {
      ensemble_status.fic_started = FALSE; 
    }
  }
}

/*====================================================================
FUNCTION       t3700_init
DESCRIPTION            
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 t3700_init(void)
{
  g_InCIntCtrl = INC_INTERRUPT_NON;
  TDMB_MSG_INC_BB("[%s] INC_INIT FUNCITON CALL\n", __func__);

  return INC_INIT(TMDB_I2C_CTRL_ID);
}


/*====================================================================
FUNCTION       t3700_bb_resync
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void t3700_bb_resync(unsigned char imr)
{
  //TDMB_MSG_INC_BB("[%s] RESYNC FUNCITON CALL\n", __func__);
}


/*====================================================================
FUNCTION       t3700_stop
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 t3700_stop(void)
{
  ensemble_status.freq = 0;
  ensemble_status.dpll = DPLL_OUTSIDE;
  ensemble_status.in_air_play = FALSE;
  TDMB_MSG_INC_BB("[%s] T3700 STOP\n", __func__);

#ifdef INC_T3700_USE_SPI
  ch_stop_check = 1;
#endif

  return INC_STOP(TMDB_I2C_CTRL_ID);
}


/*====================================================================
FUNCTION       t3700_start_ctrl
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 t3700_start_ctrl(st_subch_info* pMainInfo)
{
  chan_info*      pSetChInfo;
  ST_SUBCH_INFO *stMultiCh;
  INC_CHANNEL_INFO* pstChInfo;
  INC_UINT8       cLoop;
  INC_UINT8       IsEnsembleSame;

  g_InCIntCtrl = INC_INTERRUPT_NON;

#ifdef INC_T3700_USE_TSIF
  INC_SET_INTERRUPT(TMDB_I2C_CTRL_ID, INC_INTERRUPT_ALL_DISABLE);    
#else
  INC_SET_INTERRUPT(TMDB_I2C_CTRL_ID, 0x0000);
#endif /* INC_T3700_USE_TSIF */
  INC_CLEAR_INTERRUPT(TMDB_I2C_CTRL_ID, 0xFFFF);

#ifdef INC_MULTI_CHANNEL_ENABLE
  INC_MULTI_SORT_INIT();
  fic_data_ready = FALSE;
#ifdef USE_FEATURE_PHONE_FIC_PROC_IN_MULTI  
  g_pStFifo = &g_stFifo;
  INC_QFIFO_INIT(g_pStFifo, BB_MAX_FIC_SIZE*8);
#endif /* USE_FEATURE_PHONE_FIC_PROC_IN_MULTI */
#endif

  stMultiCh = kmalloc(sizeof(ST_SUBCH_INFO), GFP_KERNEL);
  memset(stMultiCh, 0, sizeof(ST_SUBCH_INFO));

  stMultiCh->nSetCnt = pMainInfo->nSetCnt;

  for(cLoop = 0 ; cLoop < pMainInfo->nSetCnt; cLoop++){
    pSetChInfo = &pMainInfo->astSubChInfo[cLoop];
    pstChInfo  = &stMultiCh->astSubChInfo[cLoop];

    pstChInfo->ulRFFreq         = pSetChInfo->ulRFNum;
    pstChInfo->uiEnsembleID     = pSetChInfo->uiEnsumbleID;
    pstChInfo->ucSubChID        = pSetChInfo->uiSubChID;
    pstChInfo->uiStarAddr       = pSetChInfo->uiStarAddr;
    pstChInfo->uiBitRate        = pSetChInfo->uiBitRate;
    pstChInfo->uiTmID           = pSetChInfo->uiTmID;
    pstChInfo->ucSlFlag         = pSetChInfo->uiSlFlag;
    pstChInfo->ucTableIndex     = pSetChInfo->ucTableIndex;
    pstChInfo->ucOption         = pSetChInfo->ucOption;
    pstChInfo->ucProtectionLevel= pSetChInfo->uiProtectionLevel;
    pstChInfo->uiDifferentRate  = pSetChInfo->uiDifferentRate;
    pstChInfo->uiSchSize        = pSetChInfo->uiSchSize;

    if(pSetChInfo->uiServiceType == 0x18)
      pSetChInfo->uiServiceType = TDMB_BB_SVC_DMB;

    if(dmb_mode == TDMB_MODE_NETBER)
    {
#if (defined(INC_MULTI_CHANNEL_FIC_UPLOAD) && defined(INC_MULTI_CHANNEL_NETBER_NOT_FIC_UPLOAD))
      pstChInfo->ucServiceType = pSetChInfo->uiServiceType = TDMB_BB_SVC_NETBER;
#else

#ifdef FEATURE_NETBER_TEST_ON_AIR
      pstChInfo->ucServiceType = pSetChInfo->uiServiceType = TDMB_BB_SVC_NETBER;
#else
      pstChInfo->ucServiceType = pSetChInfo->uiServiceType;
#endif /* FEATURE_NETBER_TEST_ON_AIR */

#endif
    }
    else
    {
      pstChInfo->ucServiceType = (TDMB_BB_SVC_DMB == pSetChInfo->uiServiceType) ? 0x18 : 0;
    }

    TDMB_MSG_INC_BB("[%s] Index[%d] SERVICE TYPE 0x%X\n", __func__, cLoop, pSetChInfo->uiServiceType);
  }

#ifdef FEATURE_COMMUNICATION_CHECK
	memset(&g_SetMultiInfo, 0, sizeof(ST_SUBCH_INFO));
	memcpy(&g_SetMultiInfo, stMultiCh, sizeof(ST_SUBCH_INFO));
	g_IsSyncChannel = g_IsChannelStart = INC_ERROR;
#endif

  IsEnsembleSame = ensemble_status.freq == pMainInfo->astSubChInfo[0].ulRFNum;

  if(!IsEnsembleSame || (ensemble_status.dpll != DPLL_INSIDE))
  {
    g_InCIntCtrl = INC_INTERRUPT_NON;
    if(INC_STOP(TMDB_I2C_CTRL_ID) == INC_ERROR) 
    {
      TDMB_MSG_INC_BB("[%s] INC_STOP ERROR\n", __func__);
      //return INC_ERROR;
      goto exit;
    }

    if(INC_RF500_START(TMDB_I2C_CTRL_ID, pMainInfo->astSubChInfo[0].ulRFNum, KOREA_BAND_ENABLE) == INC_ERROR)
    {
      TDMB_MSG_INC_BB("[%s] INC_RF500_START ERROR\n", __func__);
      //return INC_ERROR;
      goto exit;
    }

    if(INC_SYNCDETECTOR(TMDB_I2C_CTRL_ID, pMainInfo->astSubChInfo[0].ulRFNum, 0) == INC_ERROR) 
    { 
      TDMB_MSG_INC_BB("[%s] INC_SYNCDETECTOR ERROR\n", __func__);
      //return INC_ERROR;
      goto exit;
    }
  ensemble_status.dpll = DPLL_INSIDE;
  }

#ifndef INC_T3700_USE_TSIF
  INC_SET_INTERRUPT(TMDB_I2C_CTRL_ID, INC_MPI_INTERRUPT_ENABLE);
#endif /* INC_T3700_USE_EBI2 */

  INC_AIRPLAY_SETTING(TMDB_I2C_CTRL_ID);

#ifdef INC_MULTI_CHANNEL_ENABLE
  if(INC_MULTI_START(TMDB_I2C_CTRL_ID, stMultiCh, 0)== INC_ERROR)
  {
    TDMB_MSG_INC_BB("[%s] INC_MULTI_START ERROR\n", __func__);
    //return INC_ERROR;
    goto exit;
  }
#else
  if(INC_START(TMDB_I2C_CTRL_ID, stMultiCh, 0) == INC_ERROR) 
  {
    TDMB_MSG_INC_BB("[%s] INC_START ERROR\n", __func__);
    //return INC_ERROR;
    goto exit;
  }
#endif

  //INC_AIRPLAY_SETTING(TMDB_I2C_CTRL_ID);

#ifdef INC_T3700_USE_SPI
  ch_stop_check = 0;
#endif

#ifdef INC_T3700_USE_TSIF
  g_InCIntCtrl = INC_FIC_INTERRUPT;
#else //EBI2
  g_InCIntCtrl = INC_MSC_INTERRUPT;
#endif /* INC_T3700_USE_TSIF */

  ensemble_status.in_air_play = TRUE;
  ensemble_status.freq = pMainInfo->astSubChInfo[0].ulRFNum;

#ifdef FEATURE_COMMUNICATION_CHECK
  g_IsSyncChannel = g_IsChannelStart = INC_SUCCESS;
#endif

  TDMB_MSG_INC_BB("[%s] INC START GOOD\n", __func__); 
  kfree(stMultiCh);
  return INC_SUCCESS;

 exit:
  kfree(stMultiCh);
  return INC_ERROR;
}


/*====================================================================
FUNCTION       t3700_subch_start
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int t3700_subch_start(uint8 *regs, uint32 data_rate)
{ 
  g_stEnsembleInfo.nSetCnt = 1;

  return t3700_start_ctrl(&g_stEnsembleInfo);
}


/*====================================================================
FUNCTION       t3700_read_int
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tdmb_bb_int_type t3700_read_int(void)
{
  if(g_InCIntCtrl == INC_FIC_INTERRUPT)
  {
    TDMB_MSG_INC_BB("[%s] TDMB_BB_INT_FIC\n", __func__);
    return TDMB_BB_INT_FIC;
  }

  TDMB_MSG_INC_BB("[%s] TDMB_BB_INT_MSC\n", __func__);
  return TDMB_BB_INT_MSC;
}


/*====================================================================
FUNCTION       t3700_get_sync_status
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tBBStatus t3700_get_sync_status(void)
{
  INC_UINT16 wOperState;

  wOperState = INC_CMD_READ(TMDB_I2C_CTRL_ID, APB_PHY_BASE+ 0x10);
  wOperState = ((wOperState & 0x7000) >> 12);

  if(wOperState >= 5){
    if(ensemble_status.fic_started == FALSE)
    {
      ensemble_status.dpll = DPLL_INSIDE;
      ensemble_status.fic_started = TRUE;
      INC_SET_INTERRUPT(TMDB_I2C_CTRL_ID, INC_FIC_INTERRUPT_ENABLE);
    }

    TDMB_MSG_INC_BB("[%s] SYNC STATUS GOOD\n", __func__);
    ensemble_status.IsFreqLock = FREQ_LOCK;
    return BB_SUCCESS;
  }

  TDMB_MSG_INC_BB("[%s] SYNC STATUS FAIL\n", __func__);
  ensemble_status.IsFreqLock = FREQ_FREE;
  return BB_SYNCLOST;  
}


/*====================================================================
FUNCTION       t3700_read_fib
DESCRIPTION
DEPENDENCIES
RETURN VALUE number of FIB
SIDE EFFECTS
======================================================================*/
int t3700_read_fib(uint8 *fibs)
{
  INC_UINT16 wFicLen;

#ifdef USE_FEATURE_PHONE_FIC_PROC_IN_MULTI
#ifdef INC_MULTI_CHANNEL_ENABLE
  INC_UINT8   aFicBuff[BB_MAX_FIC_SIZE];
  if(ensemble_status.in_air_play == TRUE)
    {

    //if(fic_data_ready == 0) return 0;
    if(fic_data_ready == TRUE)
    {
      if(INC_QFIFO_GET_SIZE(g_pStFifo) >= BB_MAX_FIC_SIZE)
      {
        INC_QFIFO_BRING(g_pStFifo, aFicBuff, BB_MAX_FIC_SIZE);
        memcpy(fibs, aFicBuff, BB_MAX_FIC_SIZE);

        TDMB_MSG_INC_BB("[%s] REMAIN %d\n", __func__, INC_QFIFO_GET_SIZE(g_pStFifo));

        if(INC_QFIFO_GET_SIZE(g_pStFifo) == 0)
          fic_data_ready = FALSE; 

        return BB_MAX_FIB_NUM;
      }
    }
  }
#endif
#endif /* USE_FEATURE_PHONE_FIC_PROC_IN_MULTI */

  wFicLen = INC_CMD_READ(TMDB_I2C_CTRL_ID, APB_VTB_BASE+ 0x09) + 1;
  if(wFicLen != BB_MAX_FIC_SIZE)
  {
    TDMB_MSG_INC_BB("[%s] error wFicLen[%d]\n", __func__, wFicLen);
    // 20090921 cys wFicLen = 352 로 넘어오는 경우 많아 임시로 수정
    //return INC_ERROR;
  }

  msleep(1);

  INC_CMD_READ_BURST(TMDB_I2C_CTRL_ID, APB_FIC_BASE, fibs, wFicLen);
  TDMB_MSG_INC_BB("[%s] success\n", __func__);

  // 20090921 cys wFicLen = 352 로 넘어오는 경우 많아 임시로 수정
  //return BB_MAX_FIB_NUM;
  return wFicLen;
}


/*====================================================================
FUNCTION       t3700_set_subchannel_info
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void t3700_set_subchannel_info(void * sub_ch_info)
{
  static chan_info *ch_info;

  ch_info = (chan_info *)sub_ch_info;

  g_stEnsembleInfo.astSubChInfo[0].ucOption         = ch_info->ucOption;
  g_stEnsembleInfo.astSubChInfo[0].ucTableIndex     = ch_info->ucTableIndex;
  g_stEnsembleInfo.astSubChInfo[0].uiBitRate        = ch_info->uiBitRate;
  g_stEnsembleInfo.astSubChInfo[0].uiEnsumbleID     = ch_info->uiEnsumbleID;
  g_stEnsembleInfo.astSubChInfo[0].uiProtectionLevel= ch_info->uiProtectionLevel;
  g_stEnsembleInfo.astSubChInfo[0].uiStarAddr       = ch_info->uiStarAddr;
  g_stEnsembleInfo.astSubChInfo[0].uiSchSize        = ch_info->uiSchSize;
  g_stEnsembleInfo.astSubChInfo[0].uiServiceType    = ch_info->uiServiceType;
  g_stEnsembleInfo.astSubChInfo[0].uiSlFlag         = ch_info->uiSlFlag;
  g_stEnsembleInfo.astSubChInfo[0].uiSubChID        = ch_info->uiSubChID;
  g_stEnsembleInfo.astSubChInfo[0].uiTmID           = ch_info->uiTmID;
  g_stEnsembleInfo.astSubChInfo[0].ulRFNum          = ch_info->ulRFNum;
  g_stEnsembleInfo.astSubChInfo[0].uiDifferentRate  = ch_info->uiDifferentRate;

  TDMB_MSG_INC_BB("[%s]\n", __func__);
}


/*====================================================================
FUNCTION       t3700_read_msc
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int t3700_read_msc(uint8 *msc_buffer)
{
  INC_UINT16 uiLoop, wOperState;
  //INC_UINT8 acStreamBuff[1024*8];
#ifdef INC_MULTI_CHANNEL_ENABLE
  INC_UINT16 fic_Size, msc_Size;
  ST_FIFO* pFicFifo;
#endif
  unsigned long flags;
  uint16 interrupt_size = INC_INTERRUPT_SIZE;
#ifdef INC_MANUAL_INTR_CLEAR
  uint16 fifo_length=0;
#endif

  if (dmb_mode == TDMB_MODE_NETBER)
  {
    interrupt_size = INC_NETBER_INTERRUPT_SIZE;
  }

#ifdef FEATURE_DUMP
  static int idump_cnt = 0;
#endif

#ifdef FEATURE_TDMB_KERNEL_MSG_ON
#ifdef FEATURE_TS_PKT_MSG
  //TDMB_MSG_INC_BB("read start\n");
#else
  g_packet_read_cnt++;
#endif /* FEATURE_TS_PKT_MSG */
#endif /* FEATURE_TDMB_KERNEL_MSG_ON */

  if( m_ucCommandMode == INC_SPI_CTRL)
  {
#ifdef INC_T3700_USE_SPI
    if(ch_stop_check) //Ignore interrupt between t3700_stop() and t3700_start_ctrl(), OW SPI I/F error occurred.
    {
      //TDMB_MSG_INC_BB("Now ch stop status ---> return INTR [%d]\n",ch_stop_check);
      return 0;
    }
#endif

#ifdef INC_MANUAL_INTR_CLEAR    
    INC_CMD_WRITE(T3700_CID, APB_INT_BASE+ 0x02, 0xffff);

    uiLoop = INC_CMD_READ(T3700_CID, APB_MPI_BASE + 0x08) & 0x3FFF;

    fifo_length = uiLoop;

    // get full Data in buffer, Not INC_INTERRUPT_SIZE
    INC_CMD_READ_BURST(T3700_CID, APB_STREAM_BASE, msc_buffer, uiLoop);

    INC_CMD_WRITE(T3700_CID, APB_INT_BASE+ 0x02, 0xfffE);
    //INC_CLEAR_INTERRUPT(TMDB_I2C_CTRL_ID, INC_MPI_INTERRUPT_ENABLE);
#else
    uiLoop = INC_CMD_READ(T3700_CID, APB_MPI_BASE + 0x06);

    if(uiLoop < INC_INTERRUPT_SIZE) 
    {
      TDMB_MSG_INC_BB("interrupt less than INC_INTERRUPT_SIZE %d \n",uiLoop);
      INC_CMD_WRITE(T3700_CID, APB_INT_BASE+ 0x02, 0xfffE);
      return 0;
    }

    INC_CMD_READ_BURST(T3700_CID, APB_STREAM_BASE, msc_buffer, INC_INTERRUPT_SIZE);
#endif

#if defined(FEATURE_TS_PKT_MSG) && !defined(INC_MULTI_CHANNEL_ENABLE)
         for(uiLoop=0; uiLoop<(INC_INTERRUPT_SIZE); uiLoop+=188)
         {
           TDMB_MSG_INC_BB("msc_data(0x%.2X)(0x%.2X)(0x%.2X)(0x%.2X)\n",msc_buffer[uiLoop+0], msc_buffer[uiLoop+1], msc_buffer[uiLoop+2], msc_buffer[uiLoop+3]);
         }
#endif
  }
  else if( m_ucCommandMode == INC_EBI_CTRL)
  {
    uiLoop = INC_CMD_READ(T3700_CID, APB_MPI_BASE + 0x06);
    wOperState 	= INC_CMD_READ(T3700_CID, APB_PHY_BASE+ 0x10);
    wOperState 	= ((wOperState & 0x7000) >> 12);

    if(uiLoop < INC_INTERRUPT_SIZE || wOperState != 5) 
    {
      TDMB_MSG_INC_BB("occur interrupt less than INC_INTERRUPT_SIZE %d Status %d\n", uiLoop, wOperState);
      return 0;
    }
    
    INC_EBI_READ_BURST(T3700_CID, APB_STREAM_BASE, msc_buffer, INC_INTERRUPT_SIZE);
    
#ifndef INC_MULTI_CHANNEL_ENABLE
    return INC_INTERRUPT_SIZE;
#endif
  }
  else if(m_ucCommandMode == INC_I2C_CTRL)
  {
    //INTLOCK();
    //spin_lock_irqsave(dev_id->host->host_lock, flags);
    local_irq_save(flags);

    for(uiLoop = 0 ;uiLoop < interrupt_size; uiLoop++)
    {
      msc_buffer[uiLoop] = *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS;

#ifdef FEATURE_DUMP
      acStreamBuff_dump[INC_INTERRUPT_SIZE * idump_cnt + uiLoop] = msc_buffer[uiLoop];
#endif

#if (defined(FEATURE_TS_PKT_MSG) && !defined(INC_MULTI_CHANNEL_ENABLE))
      if(uiLoop % 188 == 3)
      {
        //if(uiLoop == 3)
        //{
        //  TDMB_MSG_INC_PKT("[%s] msc_buf\n", __func__);
        //}
        TDMB_MSG_INC_PKT("[%s] [%cch.%ccmd] msc_buf[%d](0x%.2X 0x%.2X 0x%.2X 0x%.2X)\n", __func__,
          cCHtype, cCMDtype, uiLoop/188, msc_buffer[uiLoop-3],msc_buffer[uiLoop-2],msc_buffer[uiLoop-1],msc_buffer[uiLoop]);
      }
#endif
    }

    //INTFREE();
    //spin_unlock_irqrestore(dev_id->host->host_lock, flags);
    local_irq_restore(flags);

#if 0 // 20090907 cys netber test
    TDMB_MSG_INC_BB("[%s] msc_buf(0x%.2X)(0x%.2X)(0x%.2X)(0x%.2X)(0x%.2X)(0x%.2X)(0x%.2X)(0x%.2X)\n", __func__ ,
                  msc_buffer[0], msc_buffer[1], msc_buffer[2], msc_buffer[3], 
                  msc_buffer[4], msc_buffer[5], msc_buffer[6], msc_buffer[7]);
#endif

#ifdef FEATURE_DUMP
    idump_cnt++;

    if(idump_cnt == DUMP_CNT)
      TDMB_MSG_INC_BB("[%s] acStreamBuff_dump is Full\n", __func__);
#endif
  }
  else;

  ts_data.type = MSC_DATA;

  //TDMB_MSG_INC_PKT("[%s] msc_buf read end\n", __func__);

#ifdef INC_MULTI_CHANNEL_ENABLE
#ifdef INC_MANUAL_INTR_CLEAR // get full data in buffer, Not INC_INTERRUPT_SIZE
  if(INC_MULTI_FIFO_PROCESS(msc_buffer, fifo_length))
#else
  if(INC_MULTI_FIFO_PROCESS(msc_buffer, INC_INTERRUPT_SIZE))
#endif
  {
    fic_Size = INC_GET_CHANNEL_COUNT(FIC_STREAM_DATA);
    if(fic_Size){
      ts_data.fic_size = fic_Size;
      ts_data.type = FIC_DATA;

      INC_GET_CHANNEL_DATA(FIC_STREAM_DATA, msc_buffer, fic_Size);

      if(fic_Size >= BB_MAX_FIC_SIZE){
#ifdef USE_FEATURE_PHONE_FIC_PROC_IN_MULTI
        INC_QFIFO_ADD(g_pStFifo, msc_buffer, fic_Size);
        fic_data_ready = TRUE;
#else
        memcpy(ts_data.fic_buf, msc_buffer, BB_MAX_FIC_SIZE);
#endif /* USE_FEATURE_PHONE_FIC_PROC_IN_MULTI */
      }
      else {
        pFicFifo = INC_GET_CHANNEL_FIFO(FIC_STREAM_DATA);
        INC_QFIFO_INIT(pFicFifo, 0);
      }
    }

    msc_Size = INC_GET_CHANNEL_COUNT(CHANNEL1_STREAM_DATA);

#ifdef FEATURE_TS_PKT_MSG
    TDMB_MSG_INC_BB("GET FIC SIZE[%d]  MSC SIZE[%d]\n", fic_Size, msc_Size);
#endif

    if(msc_Size){
      ts_data.msc_size = msc_Size;
      if (ts_data.type == FIC_DATA)
        ts_data.type = FIC_MSC_DATA;
      else
        ts_data.type = MSC_DATA;

      //TDMB_MSG_INC_BB("[%s] end. ts_data.type[%d]\n", __func__, ts_data.type);
      
      INC_GET_CHANNEL_DATA(CHANNEL1_STREAM_DATA, msc_buffer, msc_Size);

#ifdef FEATURE_TS_PKT_MSG
      //TDMB_MSG_INC_PKT("[Read_MSC] [%cch.%ccmd] end. msc_buf[0](0x%.2X 0x%.2X 0x%.2X 0x%.2X)\n",
      //  cCHtype, cCMDtype, msc_buffer[0],msc_buffer[1],msc_buffer[2],msc_buffer[3]);
#else
      //TDMB_MSG_INC_PKT("[%s] end\n", __func__);
#endif /* FEATURE_TS_PKT_MSG */

      return msc_Size;
    }
  }
  else
  {
    TDMB_MSG_INC_BB("fifo error\n");
  }
#else /* INC_MULTI_CHANNEL_ENABLE */

  //TDMB_MSG_INC_PKT("[%s] end\n", __func__);

  return INC_INTERRUPT_SIZE;

#endif /* INC_MULTI_CHANNEL_ENABLE */

  return 0;
}


/*====================================================================
FUNCTION       t3700_reconfig_n_ber
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void t3700_get_ber(tdmb_bb_sig_type *psigs)
{
  uint32 rs_err = 0;

  //TDMB_MSG_INC_BB("[%s] Start\n", __func__);

  ensemble_status.uiVtbErr    = INC_CMD_READ(TMDB_I2C_CTRL_ID, APB_VTB_BASE+ 0x06);
  ensemble_status.uiVtbData   = INC_CMD_READ(TMDB_I2C_CTRL_ID, APB_VTB_BASE+ 0x08);
  ensemble_status.uiRSErrBit  = INC_CMD_READ(TMDB_I2C_CTRL_ID, APB_RS_BASE+ 0x02);
  ensemble_status.uiRSErrTS   = INC_CMD_READ(TMDB_I2C_CTRL_ID, APB_RS_BASE+ 0x03);

  ensemble_status.dPreBER = (!ensemble_status.uiVtbData) ? 1 : (uint32)(ensemble_status.uiVtbErr / (ensemble_status.uiVtbData * 64));
  ensemble_status.uiCER = (!ensemble_status.uiVtbData) ? 0 : (uint16)((ensemble_status.uiVtbErr*10000) / (ensemble_status.uiVtbData * 64));

#if 0 // for HW test binary
  TDMB_MSG_INC_BB("[%s] VtbErr(%d)VtbData(%d)RsErrBit(%d)RsErrTs(%d)PreBer(%d)Cer(%d)\n", __func__, ensemble_status.uiVtbErr, ensemble_status.uiVtbData, 
    ensemble_status.uiRSErrBit, ensemble_status.uiRSErrTS, (int)ensemble_status.dPreBER, ensemble_status.uiCER);
#endif

  if(ensemble_status.uiCER == 0) ensemble_status.uiCER = 2000;

  rs_err = ensemble_status.uiRSErrBit * 16;
  rs_err += (ensemble_status.uiRSErrTS * 188 * 8);
  //ensemble_status.dPstBER = (double)ensemble_status.uiRSErrTS/0x14C;
  ensemble_status.dPstBER = (uint32)((rs_err*10000)/(0x14C*188*8));

  // 1126 by jhjung
  if(!ensemble_status.uiCER) 
    ensemble_status.uiCER = 20000;

  psigs->SNR = (uint32)INC_GET_SNR(TMDB_I2C_CTRL_ID);

  if((!ensemble_status.IsFreqLock)&&(psigs->SNR <=3))
    ensemble_status.uiCER = 1;

  psigs->PCBER = ensemble_status.uiCER;
  psigs->RSBER = INC_GET_POSTBER(TMDB_I2C_CTRL_ID); //ensemble_status.dPstBER;

  psigs->RSSI = t3700_Ant_Level(psigs->PCBER);

// 110127 (EBI2 : manual Resync, TSIF : HW AutoResync)
#ifdef INC_T3700_USE_EBI2
  INC_STATUS_CHECK(TMDB_I2C_CTRL_ID);
#endif

  //TDMB_MSG_INC_BB("[%s] snr(%d)pc ber(%d) rs ber(%d) ant(%d)\n", __func__, (int)psigs->SNR, (int)psigs->PCBER, (int)psigs->RSBER, (int)psigs->RSSI);

#if (defined(FEATURE_TDMB_KERNEL_MSG_ON) && !defined(FEATURE_TS_PKT_MSG))
  //TDMB_MSG_INC_BB("[%s] [%cch.%ccmd], Packet read Cnt[%d]\n", __func__, cCHtype, cCMDtype, g_packet_read_cnt);

  g_packet_read_cnt = 0;
#endif /* FEATURE_TS_PKT_MSG */
}


/*====================================================================
FUNCTION       t3700_Ant_Level
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int t3700_Ant_Level(uint32 pcber)
{
  uint8 level = 0;
  static uint8 save_level = 0;
  uint16 ant_tbl[] = {1150, // 0 <-> 1
                       900,  // 1 <-> 2
                       600,  // 2 <-> 3
                       180,  // 3 <-> 4
                       70};  // 4 <-> 5
  uint16 hystery_value[]= {20, 40, 40, 20, 20};

  if(pcber >= ant_tbl[0] || pcber == 1) level = 0;
  else if(pcber >= ant_tbl[1] && pcber < ant_tbl[0]) level = 1;
  else if(pcber >= ant_tbl[2] && pcber < ant_tbl[1]) level = 2;
  else if(pcber >= ant_tbl[3] && pcber < ant_tbl[2]) level = 3;
  else if(pcber >= ant_tbl[4] && pcber < ant_tbl[3]) level = 4;
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


#if defined(INC_MULTI_CHANNEL_ENABLE) && defined(INC_T3700_USE_TSIF)
/*====================================================================
FUNCTION       t3700_header_parsing_tsif_data
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint16 t3700_header_parsing_tsif_data(uint8* p_data_buf, uint32 size)
{
  INC_UINT16 fic_Size=0, ts_size=0;
  ST_FIFO* pFifo;

  if(INC_MULTI_FIFO_PROCESS(p_data_buf, size))
  {
    pFifo =  INC_GET_CHANNEL_FIFO(FIC_STREAM_DATA);
    fic_Size = INC_QFIFO_GET_SIZE(pFifo);
    if(fic_Size)
    {
      INC_QFIFO_BRING(pFifo, p_data_buf, fic_Size);

#ifdef USE_FEATURE_PHONE_FIC_PROC_IN_MULTI      
      INC_QFIFO_ADD(g_pStFifo, p_data_buf, fic_Size);
      fic_data_ready = TRUE;
      tdmb_set_fic_intr();
#endif

      fic_Size = BB_MAX_FIC_SIZE; //android 384
      ts_data.fic_size = fic_Size;
      ts_data.type = FIC_DATA;

      memcpy(ts_data.fic_buf, p_data_buf, fic_Size);
    } 
    
    pFifo =  INC_GET_CHANNEL_FIFO(CHANNEL1_STREAM_DATA);
    ts_size = INC_QFIFO_GET_SIZE(pFifo);
    
    if(ts_size)
    {
      ts_data.msc_size = ts_size;
      if (ts_data.type == FIC_DATA)
        ts_data.type = FIC_MSC_DATA;
      else
        ts_data.type = MSC_DATA;

      INC_QFIFO_BRING(pFifo, p_data_buf, ts_size);
    }
    //TDMB_MSG_INC_BB("[%s] FIC SIZE [%d]   TS SIZE [%d]\n", __func__, fic_Size,ts_size);
  }
  else
  {
    TDMB_MSG_INC_BB("[%s] TSIF Header CHECK FAIL!!! ts %d  fic %d\n", __func__, ts_size,fic_Size);
  }

  return ts_size;
}
#endif /* defined(INC_MULTI_CHANNEL_ENABLE) && defined(INC_T3700_USE_TSIF) */



/*================================================================== */
/*=================    T3900 BB Test Function     ================== */
/*================================================================== */

/*====================================================================
FUNCTION       t3700_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void t3700_test(int servicetype)
{
  static boolean powered = FALSE;
  st_subch_info *stInfo;
  ST_SUBCH_INFO  *stMultiCh;
  INC_CHANNEL_INFO *stChInfo;
  uint8 result_return;
  INC_ERROR_INFO ErrInfo;

  if(!powered)
  {
    if(!tdmb_power_on)
      t3700_power_on();
    else
      TDMB_MSG_INC_BB("[%s] skip t3700_power_on [%d]\n", __func__, tdmb_power_on);

    t3700_init();

    powered = TRUE;
  }

  //t3700_spi_test();

  while(1)
  {
  stInfo = kmalloc(sizeof(st_subch_info), GFP_KERNEL);
  memset(stInfo, 0, sizeof(st_subch_info));

  stMultiCh = kmalloc(sizeof(ST_SUBCH_INFO), GFP_KERNEL);
  memset(stMultiCh, 0, sizeof(ST_SUBCH_INFO));

  stChInfo = kmalloc(sizeof(ST_SUBCH_INFO), GFP_KERNEL);
  memset(stChInfo, 0, sizeof(ST_SUBCH_INFO));

  tdmb_get_fixed_chan_info((service_t)servicetype, &stInfo->astSubChInfo[stInfo->nSetCnt]);

  stInfo->nSetCnt++;

  stMultiCh = kmalloc(sizeof(ST_SUBCH_INFO), GFP_KERNEL);

  stMultiCh->nSetCnt = stInfo->nSetCnt;
  stMultiCh->astSubChInfo[0].ulRFFreq      = stInfo->astSubChInfo[0].ulRFNum;
  stMultiCh->astSubChInfo[0].ucServiceType = stInfo->astSubChInfo[0].uiServiceType;
  stMultiCh->astSubChInfo[0].ucSubChID     = stInfo->astSubChInfo[0].uiSubChID;
  stMultiCh->astSubChInfo[0].uiSchSize = stInfo->astSubChInfo[0].uiSchSize;

  TDMB_MSG_INC_BB("[%s] TEST start freq = %d~~~  [%d]\n", __func__, stMultiCh->astSubChInfo[0].ulRFFreq, dmb_mode);
  TDMB_MSG_INC_BB("[%s] TEST start service type = 0x%x~~~ [0x%x]\n", __func__, stMultiCh->astSubChInfo[0].ucServiceType,stMultiCh->astSubChInfo[0].uiSchSize);
  TDMB_MSG_INC_BB("[%s] TEST start subch id = 0x%x~~~\n", __func__, stMultiCh->astSubChInfo[0].ucSubChID);

  stChInfo->ulRFFreq       = stInfo->astSubChInfo[0].ulRFNum;
  stChInfo->ucServiceType  = stInfo->astSubChInfo[0].uiServiceType;
  stChInfo->ucSubChID      = stInfo->astSubChInfo[0].uiSubChID;

#ifdef INC_MULTI_CHANNEL_ENABLE
  INC_MULTI_SORT_INIT();
  fic_data_ready = FALSE;
#ifdef USE_FEATURE_PHONE_FIC_PROC_IN_MULTI
  g_pStFifo = &g_stFifo;
  INC_QFIFO_INIT(g_pStFifo, BB_MAX_FIC_SIZE*8);
#endif /* USE_FEATURE_PHONE_FIC_PROC_IN_MULTI */
  result_return = INC_MULTI_START_CTRL(TMDB_I2C_CTRL_ID, stMultiCh);
#else
  result_return = INC_CHANNEL_START(TMDB_I2C_CTRL_ID, stMultiCh);
#endif

  //TDMB_MSG_INC_BB("[%s] start result = %d~~~\n", __func__, result_return);


  kfree(stInfo);
  kfree(stMultiCh);
  kfree(stChInfo);

  if(!result_return)
  {
    ErrInfo = INTERFACE_ERROR_STATUS(TMDB_I2C_CTRL_ID);
    TDMB_MSG_INC_BB("[%s] ERROR -----> %X~~~\n", __func__, ErrInfo);
  }
  else
  {
    TDMB_MSG_INC_BB(" -----------------> TDMB TEST OK !!!!!\n");
    break;
  }

  msleep(500);
  msleep(500);
  }
}


//extern tdmb_bb_function_table_type tdmb_bb_function_table;


/*====================================================================
FUNCTION       t3700_i2c_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 t3700_i2c_test(void) 
{
  uint16  i, wData;

  if(!tdmb_power_on)
    t3700_power_on();
  else
    TDMB_MSG_INC_BB("[%s] skip t3700_power_on [%d]\n", __func__, tdmb_power_on);

  for(i=0; i<0xFF; i++)
  {
    INC_CMD_WRITE(TMDB_I2C_CTRL_ID, APB_MPI_BASE+ 0x01, i);
    wData = INC_CMD_READ(TMDB_I2C_CTRL_ID, APB_MPI_BASE+ 0x01);
    if(wData != i)
    {
      TDMB_MSG_INC_BB("[%s 1] I2C RW error Reg[%x]   Val[%x]  [%x]\n", __func__, APB_MPI_BASE+ 0x01, wData, i);
    }
  }

  for(i=0xF000; i<0xF100; i++)
  {
    INC_CMD_WRITE(TMDB_I2C_CTRL_ID, APB_MPI_BASE+ 0x01, i);
    wData = INC_CMD_READ(TMDB_I2C_CTRL_ID, APB_MPI_BASE+ 0x01);
    if(wData != i)
    {
      TDMB_MSG_INC_BB("[%s 2] I2C RW error Reg[%x]   Val[%x]  [%x]\n", __func__, APB_MPI_BASE+ 0x01, wData, i);
    }
  }

  TDMB_MSG_INC_BB("[%s] T3900 I2C Test end\n", __func__);

  return INC_SUCCESS;
}


/*====================================================================
FUNCTION       t3700_ebi2_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 t3700_ebi2_test(void) 
{
  uint16  wData, i;

  if(!tdmb_power_on)
    t3700_power_on();
  else
    TDMB_MSG_INC_BB("[%s] skip t3700_power_on [%d]  cmd mode[%d]\n", __func__, tdmb_power_on ,m_ucCommandMode);

  wData = INC_CMD_READ(TMDB_I2C_CTRL_ID, APB_MPI_BASE);  //a00 번지 가 6이 나와야 함
  TDMB_MSG_INC_BB("[%s] EF11 EBI RW  Reg[%x]   Val[%x] \n", __func__, APB_MPI_BASE, wData);  

  for(i=0; i<10; i++)
  {
    INC_CMD_WRITE(TMDB_I2C_CTRL_ID, APB_MPI_BASE+ 0x01, i);
    wData = INC_CMD_READ(TMDB_I2C_CTRL_ID, APB_MPI_BASE+ 0x01);
    if(wData != i)
    {
      TDMB_MSG_INC_BB("[%s] EBI RW  Reg[%x]   Val[%x]  [%x]\n", __func__, APB_MPI_BASE+ 0x01, wData, i);
    }
  }

  return INC_SUCCESS;
}

/*====================================================================
FUNCTION       t3700_spi_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 t3700_spi_test(void) 
{
  uint16  wData, i;

  if(!tdmb_power_on)
    t3700_power_on();
  else
    TDMB_MSG_INC_BB("[%s] skip t3700_power_on [%d]  cmd mode[%d]\n", __func__, tdmb_power_on ,m_ucCommandMode);

#ifdef FEATURE_DMB_SPI_CMD
  INC_SPI_INIT();
#endif

  wData = INC_CMD_READ(TMDB_I2C_CTRL_ID, APB_MPI_BASE);  //a00 번지 가 6이 나와야 함
  TDMB_MSG_INC_BB("[%s] SPI Read  Reg[%x]   Val[%x] \n", __func__, APB_MPI_BASE, wData);  

  INC_CMD_WRITE(TMDB_I2C_CTRL_ID, APB_MPI_BASE+ 0x01, 0x1234);
  wData = INC_CMD_READ(TMDB_I2C_CTRL_ID, APB_MPI_BASE+ 0x01);
  if(wData != 0x1234)
  {
    TDMB_MSG_INC_BB("[%s] 1234  Reg[%x]   Val[%x]\n", __func__, APB_MPI_BASE+ 0x01, wData);
  }

  for(i=0x1; i<0xffff; i++)
  {
    INC_CMD_WRITE(TMDB_I2C_CTRL_ID, APB_MPI_BASE+ 0x01, i);
    wData = INC_CMD_READ(TMDB_I2C_CTRL_ID, APB_MPI_BASE+ 0x01);
    if(wData != i)
    {
      TDMB_MSG_INC_BB("[%s] SPI RW3  Reg[%x]   Val[%x]  [%x]\n", __func__, APB_MPI_BASE+ 0x01, wData, i);
    }
  }

  TDMB_MSG_INC_BB("[%s] SPI RW  test end !!\n", __func__);

  return INC_SUCCESS;
}

#ifdef FEATURE_COMMUNICATION_CHECK
/*====================================================================
FUNCTION       t3700_reset
DESCRIPTION   only for T3900
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void t3900_reset(void)
{
  TDMB_MSG_INC_BB(" ============= INC_RESET =============\n");
  dmb_set_gpio(DMB_RESET, 0);
  msleep(2);

  dmb_set_gpio(DMB_RESET, 1);
  msleep(100);
}
#endif
