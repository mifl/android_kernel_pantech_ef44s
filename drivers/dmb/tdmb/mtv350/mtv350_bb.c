//=============================================================================
// File       : MTV350_bb.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2010/10/28       yschoi         Create
//=============================================================================

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
//#include <mach/gpio.h>

#include "../tdmb_comdef.h"

#include "../../dmb_hw.h"
#include "../../dmb_interface.h"
#include "../tdmb_dev.h"
#include "../tdmb_test.h"

#include "mtv350_bb.h"
#include "raontv.h"

#ifdef FEATURE_DMB_RTV_USE_FM_PATH
#include "raontv_internal.h" //for FM path
#endif

/*================================================================== */
/*=================        MTV BB Function        ================== */
/*================================================================== */

#define TMDB_I2C_CTRL_ID RAONTV_CHIP_ADDR

extern tdmb_mode_type dmb_mode;

st_subch_info  g_stEnsembleInfo;

boolean tdmb_power_on = FALSE;

int g_packet_read_cnt = 0;

int g_sync_status = 0;
static uint16 prev_subch_id;

static boolean mtv350_function_register(tdmb_bb_function_table_type *);


/*====================================================================
FUNCTION       tdmb_bb_mtv350_init  
DESCRIPTION    matching mtv350 Function at TDMB BB Function
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean tdmb_bb_mtv350_init(tdmb_bb_function_table_type *function_table_ptr)
{
  boolean bb_initialized;

  bb_initialized = mtv350_function_register(function_table_ptr);

  return bb_initialized;
}


static boolean mtv350_function_register(tdmb_bb_function_table_type *ftable_ptr)
{
  ftable_ptr->tdmb_bb_drv_init          = mtv350_init;
  ftable_ptr->tdmb_bb_power_on          = mtv350_power_on;
  ftable_ptr->tdmb_bb_power_off         = mtv350_power_off;
  ftable_ptr->tdmb_bb_ch_scan_start     = mtv350_ch_scan_start;
  ftable_ptr->tdmb_bb_resync            = mtv350_bb_resync;
  ftable_ptr->tdmb_bb_subch_start       = mtv350_subch_start;
  ftable_ptr->tdmb_bb_read_int          = mtv350_read_int;
  ftable_ptr->tdmb_bb_get_sync_status   = mtv350_get_sync_status;
  ftable_ptr->tdmb_bb_read_fib          = mtv350_read_fib;
  ftable_ptr->tdmb_bb_set_subchannel_info = mtv350_set_subchannel_info;
  ftable_ptr->tdmb_bb_read_msc          = mtv350_read_msc;
  ftable_ptr->tdmb_bb_get_ber           = mtv350_get_ber;
  ftable_ptr->tdmb_bb_ch_stop           = mtv350_stop;
  ftable_ptr->tdmb_bb_powersave_mode    = mtv350_set_powersave_mode;
  ftable_ptr->tdmb_bb_ch_test           = mtv350_test;

  return TRUE;
}


/*====================================================================
FUNCTION       mtv350_i2c_write  
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 mtv350_i2c_write(uint8 reg, uint8 data)
{
  uint8 ret = 0;

#ifdef FEATURE_DMB_I2C_CMD
  ret = dmb_i2c_write(TMDB_I2C_CTRL_ID, reg, sizeof(uint8), data, sizeof(uint8));
#endif /* FEATURE_DMB_I2C_CMD */

  return ret;
}


/*====================================================================
FUNCTION       mtv350_i2c_read  
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 mtv350_i2c_read(uint8 reg)
{
  uint8 ret = 0;
  uint8 data = 0;

#ifdef FEATURE_DMB_I2C_CMD
  ret = dmb_i2c_read(TMDB_I2C_CTRL_ID, reg, sizeof(uint8), (uint16*)&data, sizeof(uint8));
#endif /* FEATURE_DMB_I2C_CMD */
  if(ret) 
  {
    return data;
  }
  return ret;
}


/*====================================================================
FUNCTION       mtv350_i2c_read_len  
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 mtv350_i2c_read_len(uint8 reg, uint8 *data, uint16 data_len)
{
  uint8 ret = 0;

#ifdef FEATURE_DMB_I2C_CMD
  dmb_i2c_read_len(TMDB_I2C_CTRL_ID, reg, data, data_len, sizeof(uint8));
#endif /* FEATURE_DMB_I2C_CMD */

  return ret;
}


/*====================================================================
FUNCTION       mtv350_set_powersave_mode
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void mtv350_set_powersave_mode(void)
{
#if 0 // 20101102 cys
  dmb_set_gpio(DMB_RESET, FALSE);
  msleep(1);
#endif // 0
}


/*====================================================================
FUNCTION       mtv350_power_on
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void mtv350_power_on(void)
{
// 1. DMB_EN : LOW
// 2. DMB_EN : HIGH

  TDMB_MSG_RTV_BB("[%s] start!!!\n", __func__);

  dmb_power_on();

  //dmb_set_gpio(DMB_RESET, 0);
  dmb_set_gpio(DMB_PWR_EN, 0);
  msleep(1);

  dmb_power_on_chip();
  mdelay(20);

  //dmb_set_gpio(DMB_RESET, 1);
  //msleep(10);

  RTV_GUARD_INIT;

  tdmb_power_on = TRUE;

  TDMB_MSG_RTV_BB("[%s] end!!!\n", __func__);
}


/*====================================================================
FUNCTION       mtv350_power_off
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void mtv350_power_off(void)
{
// 1. DMB_EN : LOW

  TDMB_MSG_RTV_BB("[%s] start!!!\n", __func__);

  dmb_power_off();

  tdmb_power_on = FALSE;

  TDMB_MSG_RTV_BB("[%s] end!!!\n", __func__);
}


/*====================================================================
FUNCTION       mtv350_ch_scan_start
DESCRIPTION 
    if for_air is greater than 0, this is for air play.
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void mtv350_ch_scan_start(int freq, int band, unsigned char for_air)
{
  int res;
  TDMB_MSG_RTV_BB("[%s] Channel Frequency[%d] Band[%d] Mode[%d]\n", __func__, freq, band, for_air);

  rtvTDMB_CloseFIC();

  res = rtvTDMB_ScanFrequency(freq);

  // 04/20 blcok weak -> strong field, cannot fic data out problem
  //if (res == RTV_SUCCESS)
  {
    //TDMB_MSG_RTV_BB("[%s] SUCCESS\n", __func__);
    rtvTDMB_OpenFIC();
  }

  // 0501 for autoscan
  if(dmb_mode == TDMB_MODE_AUTOSCAN)
  {
    g_sync_status = (res == RTV_SUCCESS ? RTV_TDMB_CHANNEL_LOCK_OK : 0);
  }
  else
  {
    g_sync_status = RTV_TDMB_CHANNEL_LOCK_OK;
  }

  TDMB_MSG_RTV_BB("[%s]  %s  res[%d] sync_status[%d]\n",__func__,res==RTV_SUCCESS?"OK":"FAIL" ,res, g_sync_status);
}


/*====================================================================
FUNCTION       mtv350_init
DESCRIPTION            
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 mtv350_init(void)
{
  uint8 res;

  res = rtvTDMB_Initialize(RTV_COUNTRY_BAND_KOREA);
  prev_subch_id = 0xFFFF;

  TDMB_MSG_RTV_BB("[%s] res[%d] clock[%d]\n", __func__, res, RTV_SRC_CLK_FREQ_KHz);
  return res;
}


/*====================================================================
FUNCTION       mtv350_bb_resync
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void mtv350_bb_resync(unsigned char imr)
{
  //TDMB_MSG_RTV_BB("[%s] Do nothing\n", __func__);
}


/*====================================================================
FUNCTION       mtv350_stop
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 mtv350_stop(void)
{
  TDMB_MSG_RTV_BB("[%s] MTV350 STOP\n", __func__);

  rtvTDMB_CloseSubChannel(prev_subch_id); // for single 
  prev_subch_id = 0xFFFF;

  rtvTDMB_CloseFIC();

  // 20101102 cys
  //return RTV_STOP(TMDB_I2C_CTRL_ID);
  return 1;
}


/*====================================================================
FUNCTION       mtv350_subch_start
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int mtv350_subch_start(uint8 *regs, uint32 data_rate)
{ 
  int res;
  uint16 uiSubChID, uiServiceType;
  uint32 ulRFNum;

  uiSubChID = g_stEnsembleInfo.astSubChInfo[0].uiSubChID;
  uiServiceType = (g_stEnsembleInfo.astSubChInfo[0].uiServiceType) == TDMB_BB_SVC_DMB ? RTV_SERVICE_VIDEO : RTV_SERVICE_AUDIO;
  ulRFNum = g_stEnsembleInfo.astSubChInfo[0].ulRFNum;


#ifdef FEATURE_TDMB_MULTI_CHANNEL_ENABLE// PJSIN 20110223 add-- [ 1 
  rtvTDMB_CloseFIC();
#endif// ]-- end 
 
  res = rtvTDMB_OpenSubChannel(ulRFNum, uiSubChID, uiServiceType, 188*8);
  prev_subch_id = uiSubChID;

#ifdef FEATURE_TDMB_MULTI_CHANNEL_ENABLE// PJSIN 20110223 add-- [ 1 
	rtvTDMB_OpenFIC();
#endif// ]-- end 

  TDMB_MSG_RTV_BB("[%s] uiSubChID[%d]  uiServiceType[%d]  ulRFNum[%d]\n", __func__, uiSubChID, uiServiceType, (int)ulRFNum);

  if(res != RTV_SUCCESS)
  {
  	if (res == RTV_ALREADY_OPENED_SUB_CHANNEL)
		TDMB_MSG_RTV_BB("[%s] Already opened %d\n", __func__, res);
	else {
      TDMB_MSG_RTV_BB("[%s] RTV_IOCTL_TDMB_SET_SUBCHANNEL error %d\n", __func__, res);
      return 0;
	}
  }
  
  return 1;
}


/*====================================================================
FUNCTION       mtv350_read_int
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tdmb_bb_int_type mtv350_read_int(void)
{
  // 20101102 cys
  TDMB_MSG_RTV_BB("[%s] Do nothing \n", __func__);

  //mtv350_isr_handler();

  return TDMB_BB_INT_MSC;
}


/*====================================================================
FUNCTION       mtv350_get_sync_status
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tBBStatus mtv350_get_sync_status(void)
{
  // 0501 block for autoscan
  //g_sync_status = rtvTDMB_GetLockStatus();

  TDMB_MSG_RTV_BB("[%s] ret[%d]\n", __func__, g_sync_status);

  //if(g_sync_status  == RTV_TDMB_CHANNEL_LOCK_OK)
  if(g_sync_status >= RTV_TDMB_OFDM_LOCK_MASK)
  { 
    return BB_SUCCESS;
  }
  else
  {
    return BB_SYNCLOST;
  }
}


/*====================================================================
FUNCTION       mtv350_read_fib
DESCRIPTION
DEPENDENCIES
RETURN VALUE number of FIB
SIDE EFFECTS
======================================================================*/
int mtv350_read_fib(uint8 *fibs)
{
  int fib_num;

  fib_num = rtvTDMB_ReadFIC(fibs);

  // 20101102 cys
  TDMB_MSG_RTV_BB("[%s] fib num %d  [%x]\n", __func__,fib_num,fibs[0]);

  return fib_num;//rtvTDMB_ReadFIC(fibs);
}


/*====================================================================
FUNCTION       mtv350_set_subchannel_info
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void mtv350_set_subchannel_info(void * sub_ch_info)
{
  static chan_info *ch_info;

  TDMB_MSG_RTV_BB("[%s]\n", __func__);

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
}


/*====================================================================
FUNCTION       mtv350_read_msc
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int mtv350_read_msc(uint8 *msc_buffer)
{
  // 20101102 cys
  TDMB_MSG_RTV_BB("[%s] Do nothing\n", __func__);
  
  return 0;
}


/*====================================================================
FUNCTION       mtv350_Ant_Level
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static uint16 mtv350_Ant_Level(uint16 pcber)
{
  uint8 level = 0;
  static uint8 save_level = 0;
  uint16 ant_tbl[] = {850, // 0 <-> 1
                               600, // 1 <-> 2
                               400, // 2 <-> 3
                               250, // 3 <-> 4
                               150};  // 4 <-> 5

  uint16 hystery_value[]= {0,50,50,20,20};

  //TDMB_MSG_RTV_BB("[%s]\n", __func__);

  if(pcber < ant_tbl[4]) level = 5;//(ber >= 1 && ber <= 100) printf("ANT: 5\n");
  else if(pcber >= ant_tbl[4] && pcber < ant_tbl[3]) level = 4;//(ber >= 101 && ber <= 150) printf("ANT: 4\n");
  else if(pcber >= ant_tbl[3] && pcber < ant_tbl[2]) level = 3;//(ber >= 151 && ber <= 300) printf("ANT: 3\n");
  else if(pcber >= ant_tbl[2] && pcber < ant_tbl[1]) level = 2;//(ber >= 301 && ber <= 500) printf("ANT: 2\n");
  else if(pcber >= ant_tbl[1] && pcber < ant_tbl[0]) level = 1;//(ber >= 501 && ber <= 800) printf("ANT: 1\n");
  else if(pcber >= ant_tbl[0]) level = 0;//(ber >= 801) printf("ANT: 0\n");

  if(level == save_level + 1) // Level ÀÌ 1Ä­ ¿Ã¶ó°£ °æ¿ì¿¡¸¸.
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
FUNCTION       mtv350_reconfig_n_ber
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void mtv350_get_ber(tdmb_bb_sig_type *psigs)
{
  int rssi;

  //TDMB_MSG_RTV_BB("[%s]\n", __func__);

  rssi = (rtvTDMB_GetRSSI() / (int)RTV_TDMB_RSSI_DIVIDER);

  // 04/20 block
  //if(g_sync_status == RTV_TDMB_CHANNEL_LOCK_OK)// ì±„ë„ lock ?¼ë•Œë§?ê°’ì„ ?½ìŒ
      psigs->PCBER = rtvTDMB_GetCER();//(int)RTV_TDMB_CER_DIVIDER;
  //else
  //    psigs->PCBER = 20000;

  psigs->RSBER = rssi;//rtvTDMB_GetPER();
  psigs->SNR = (rtvTDMB_GetCNR()/100);
  psigs->RSSI = mtv350_Ant_Level(psigs->PCBER);//rtvTDMB_GetCNR() / (int)RTV_TDMB_CNR_DIVIDER;

  //TDMB_MSG_BB("[%s] pcber[%d], rssi[%d] snr[%d] rsber[%d]\n", __func__, psigs->PCBER, psigs->RSSI, psigs->SNR, psigs->RSBER);

#if (defined(FEATURE_TDMB_KERNEL_MSG_ON) && !defined(FEATURE_TS_PKT_MSG))
  TDMB_MSG_RTV_BB("[%s] Packet read Cnt[%d]\n", __func__, g_packet_read_cnt);

  g_packet_read_cnt = 0;
#endif /* FEATURE_TS_PKT_MSG */
}

#ifdef FEATURE_DMB_RTV_USE_FM_PATH
/*====================================================================
FUNCTION       mtv350_use_FM_path(Reg. 0x27)
DESCRIPTION earjack : FM path set 0x92, retractable : set 0x96, don't need antenna switch.
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void mtv350_use_FM_path(int set_FM_path)
{
  RTV_REG_MAP_SEL(RF_PAGE);
  RTV_REG_SET(0x27, (0x92 | (set_FM_path<<2)));

  TDMB_MSG_RTV_BB("Ant type[%d] : RTV use FM path set[%x]\n ",set_FM_path, (0x92 | (set_FM_path<<2)));  
}
#endif

/*================================================================== */
/*=================     MTV BB TEst Function      ================== */
/*================================================================== */
void mtv350_test(int servicetype)
{
  static boolean powered = FALSE;
  st_subch_info *stInfo;
  int res;

  if(!powered)
  {
    if(!tdmb_power_on)
      mtv350_power_on();
    else
      TDMB_MSG_RTV_BB("[%s] skip mtv350_power_on [%d]\n", __func__, tdmb_power_on);

    powered = TRUE;
  }

  TDMB_MSG_RTV_BB("[%s] mtv350_init\n", __func__);

  mtv350_init();

  mtv350_i2c_write(0x03, 0x07); //24.576Mhz ê²½ìš°
  res = mtv350_i2c_read(0x00); //0x8A ?˜ì????
  TDMB_MSG_RTV_BB("[%s] RW test  Reg. 0x00 val[0x%x]==0x8a\n", __func__, res);

  stInfo = kmalloc(sizeof(st_subch_info), GFP_KERNEL);
  memset(stInfo, 0, sizeof(st_subch_info));

  tdmb_get_fixed_chan_info((service_t)servicetype, &stInfo->astSubChInfo[stInfo->nSetCnt]);

  stInfo->nSetCnt++;

  // 2012/04/26: RAONTECH
  stInfo->astSubChInfo[0].uiServiceType = (stInfo->astSubChInfo[0].uiServiceType==0x18) ? 0x01 : 0x02; //VIDEO 1, AUDIO 2
  TDMB_MSG_RTV_BB("[%s] TEST start freq [%d]  dmb_mode [%d]\n", __func__, (int)stInfo->astSubChInfo[0].ulRFNum, (int)dmb_mode);
  TDMB_MSG_RTV_BB("[%s] TEST start service type [0x%x]  schsize[0x%x]\n", __func__, stInfo->astSubChInfo[0].uiServiceType, stInfo->astSubChInfo[0].uiSchSize);
  TDMB_MSG_RTV_BB("[%s] TEST start subch id [0x%x]\n", __func__, stInfo->astSubChInfo[0].uiSubChID);

  res = rtvTDMB_ScanFrequency(stInfo->astSubChInfo[0].ulRFNum);
  TDMB_MSG_RTV_BB("rtvTDMB_ScanFrequency %d\n", res);  
  if(res == RTV_SUCCESS)
  {
    TDMB_MSG_RTV_BB("rtvTDMB_ScanFrequency OK %d\n", res);
  }

  g_sync_status = rtvTDMB_GetLockStatus();

  TDMB_MSG_RTV_BB("RTV GetLockStatus  g_sync_status[%d]  [%d]\n", g_sync_status,RTV_TDMB_CHANNEL_LOCK_OK);

  rtvTDMB_CloseSubChannel(prev_subch_id); // for single 
  res = rtvTDMB_OpenSubChannel(stInfo->astSubChInfo[0].ulRFNum, stInfo->astSubChInfo[0].uiSubChID, stInfo->astSubChInfo[0].uiServiceType, 188*8);
  prev_subch_id = stInfo->astSubChInfo[0].uiSubChID;

  if(res != RTV_SUCCESS)
  {
	if (res == RTV_ALREADY_OPENED_SUB_CHANNEL)
		TDMB_MSG_RTV_BB("[%s] Already opened %d\n", __func__, res);
	else
      TDMB_MSG_RTV_BB("[%s] RTV_IOCTL_TDMB_SET_SUBCHANNEL error %d\n", __func__, res);
  }

  kfree(stInfo);
}


extern tdmb_bb_function_table_type tdmb_bb_function_table;


uint8 mtv350_i2c_test(void) 
{
  uint16  wData;

  if(!tdmb_power_on)
    mtv350_power_on();
  else
    TDMB_MSG_RTV_BB("[%s] skip mtv350_power_on [%d]\n", __func__, tdmb_power_on);

  msleep(100); //for test
  
  mtv350_i2c_write(0x03, 0x07); //24.576Mhz
  //mtv350_i2c_write(0x03, 0x87); // 32Mhzë³´ë‹¤ ??ê²½ìš° 
  wData = mtv350_i2c_read(0x00); //0x8A ?˜ì????
  TDMB_MSG_RTV_BB("[%s] %d\n", __func__, wData);

  return RTV_SUCCESS;
}


uint8 mtv350_ebi2_test(void) 
{

  return RTV_SUCCESS;
}

