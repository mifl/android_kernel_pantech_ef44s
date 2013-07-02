//=============================================================================
// File       : sharp_bb.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2011/09/29       yschoi         Create
//=============================================================================

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
//#include <mach/gpio.h>

#include "../../dmb_hw.h"
#include "../../dmb_interface.h"

#include "../isdbt_comdef.h"
#include "../isdbt_dev.h"
#include "../isdbt_bb.h"
#include "../isdbt_test.h"

#include "sharp_bb.h"

#include "ntv/inc/nmiioctl.h"
#include "ntv/inc/nmidrv.h"



/*================================================================== */
/*=================       SHARP BB Definition      ================= */
/*================================================================== */

//extern void nmi_test(void);

boolean isdbt_chip_power_on = FALSE;

//static uint32_t set_frequency = 0;

extern tOem *poem;
extern tNtv *ntv;


#define I2C_CHIP_ADDR 0x61

//#define FEATURE_TEST_READ_CHIPID

#ifdef FEATURE_TEST_READ_CHIPID
#define NMI_DRV_DEINIT								0x10001001
#define NMI_DRV_INIT_CORE							0x10001002
#define NMI_DRV_GET_CHIPID						0x10001003
#define NMI_DRV_RUN										0x10001004
#define DRV_MASTER_CTL(code, pv)		nmi_drv_ctl(code, nISDBTMode, nMaster, pv)

extern uint32_t isdbt_get_chipid(tNmiIsdbtChip cix);
#endif

#if 0
static void nmi_bus_deinit(void);
static int nmi_bus_init(void);
static void nmi_isdbt_deinit(void);
static int nmi_isdbt_init(void);
static int nmi_bus_set_mode(int en);
static int npm_init_core(void);
static void npm_isdbt_get_status(void);
static void npm_isdbt_get_status_slave(void);
#endif



/*================================================================== */
/*=================       SHARP BB Function       ================== */
/*================================================================== */

static boolean sharp_function_register(isdbt_bb_function_table_type *ftable_ptr)
{
  ISDBT_MSG_SHARP_BB("[%s] !!!\n", __func__);

  ftable_ptr->isdbt_bb_power_on         = sharp_power_on;
  ftable_ptr->isdbt_bb_power_off        = sharp_power_off;
  ftable_ptr->isdbt_bb_init             = sharp_bb_init;
  ftable_ptr->isdbt_bb_set_freq         = sharp_bb_set_frequency;
  ftable_ptr->isdbt_bb_fast_search      = sharp_bb_fast_search;
  ftable_ptr->isdbt_bb_start_ts         = sharp_bb_start_ts;
  ftable_ptr->isdbt_bb_get_status       = sharp_bb_get_status;
  ftable_ptr->isdbt_bb_get_tuner_info   = sharp_bb_get_tuner_info;
#if 0
  ftable_ptr->isdbt_bb_bus_deinit       = nmi_bus_deinit;
  ftable_ptr->isdbt_bb_bus_init         = nmi_bus_init;
  ftable_ptr->isdbt_bb_bus_set_mode     = nmi_bus_set_mode;
  ftable_ptr->isdbt_bb_deinit           = nmi_isdbt_deinit;
  ftable_ptr->isdbt_bb_init_core        = npm_init_core;
  ftable_ptr->isdbt_bb_get_status       = npm_isdbt_get_status;
  ftable_ptr->isdbt_bb_get_status_slave = npm_isdbt_get_status_slave;
#endif
  ftable_ptr->isdbt_bb_test             = sharp_test;

  return TRUE;
}

/*====================================================================
FUNCTION       tdmb_bb_fc8050_init  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean isdbt_bb_sharp_init(isdbt_bb_function_table_type *function_table_ptr)
{
  boolean bb_initialized;

  bb_initialized = sharp_function_register(function_table_ptr);

  return bb_initialized;
}

int sharp_i2c_write_data(unsigned char c, unsigned char *data, unsigned long data_width)
{
  int ret = 0;

#ifdef FEATURE_DMB_I2C_CMD
  ret = dmb_i2c_write_data(data[0], data, data_width);
#endif /* FEATURE_DMB_I2C_CMD */

  return ret;
}

#ifdef FEATURE_DMB_SHARP_I2C_READ
int sharp_i2c_read_data(unsigned char c, unsigned char *wdata, unsigned long wdata_width, unsigned char *rdata, unsigned long rdata_width)
{
  int ret = 0;
#ifdef FEATURE_DMB_I2C_CMD
  dmb_i2c_read_len_multi(wdata[0], wdata, wdata_width, rdata, rdata_width);
#endif
  return ret;
}
#else
int sharp_i2c_read_data(unsigned char c, unsigned char *data, unsigned long data_width)
{
  int ret =0;
#ifdef FEATURE_DMB_I2C_CMD
  ret = dmb_i2c_read_data_only(data[0], data, data_width);
#endif /* FEATURE_DMB_I2C_CMD */

  return ret;
}
#endif

#if 1 // cys not used
uint8 sharp_i2c_write_word(uint8 chipid, uint16 reg, uint16 data)
{
  uint8 ret = 0;

#ifdef FEATURE_DMB_I2C_CMD
  ret = dmb_i2c_write(chipid, reg, sizeof(uint16), data, sizeof(uint16));
#endif /* FEATURE_DMB_I2C_CMD */

  return ret;
}

uint8 sharp_i2c_read_word(uint8 chipid, uint16 reg, uint16 *data)
{
#ifdef FEATURE_DMB_I2C_CMD
  dmb_i2c_read(chipid, reg, sizeof(uint16), data, sizeof(uint16));
#endif /* FEATURE_DMB_I2C_CMD */

  return TRUE;
}

void sharp_i2c_read_len(uint8 chipid, unsigned int uiAddr, uint8 ucRecvBuf[], uint16 ucCount)
{
#ifdef FEATURE_DMB_I2C_CMD
  dmb_i2c_read_len(chipid, uiAddr, ucRecvBuf, ucCount, sizeof(uint16));
#endif /* FEATURE_DMB_I2C_CMD */

  return;
}
#endif


/*====================================================================
FUNCTION       sharp_power_on
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void sharp_power_on(void)
{
// 1. DMB_RESET : LOW
// 2. DMB_PWR_EN : HIGH
// 3. DMB_RESET : HIGH

  ISDBT_MSG_SHARP_BB("[%s] start!!!\n", __func__);

  dmb_power_on();

  dmb_set_gpio(DMB_RESET, 0);
  msleep(1);

  dmb_power_on_chip();
  msleep(100);

  dmb_set_gpio(DMB_RESET, 1);
  msleep(100);

  isdbt_chip_power_on = TRUE;

  ISDBT_MSG_SHARP_BB("[%s] end!!!\n", __func__);
}


/*====================================================================
FUNCTION       sharp_power_off
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void sharp_power_off(void)
{
// 1. DMB_RESET : LOW
// 2. DMB_PWR_EN : LOW

  ISDBT_MSG_SHARP_BB("[%s] start!!![%d]\n", __func__, isdbt_chip_power_on);

  if(!isdbt_chip_power_on)
  {
    return;
  }

  dmb_set_gpio(DMB_RESET, 0);
  msleep(1);

  dmb_power_off();

  isdbt_chip_power_on = FALSE;

  ISDBT_MSG_SHARP_BB("[%s] end!!!\n", __func__);
}


/*====================================================================
FUNCTION       GetTick
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static long GetTick(void)
{
  struct timeval tick;
  //gettimeofday (&tick, 0);
  do_gettimeofday(&tick);
  return (tick.tv_sec*1000 + tick.tv_usec/1000);
}


/*====================================================================
FUNCTION       sharp_bb_pre_init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void sharp_bb_pre_init(void)
{
  ISDBT_MSG_SHARP_BB("[%s] start\n", __func__);

  poem->prnt = NULL;//ISDBT_MSG_SHARP_BB;
  poem->os.delay = msleep;
  poem->os.gettick = GetTick;

  poem->bus.i2cw = sharp_i2c_write_data;
  poem->bus.i2cr = sharp_i2c_read_data;
  poem->bus.spiw = NULL;
  poem->bus.spir = NULL;
  poem->bus.burstr = NULL;

  ntv->tsdmasize = NMI_TS_BUF_SIZE; // cys temp

  ISDBT_MSG_SHARP_BB("[%s] end\n", __func__);
}


/*====================================================================
FUNCTION       sharp_bb_init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int sharp_bb_init(void)
{
  tNmDrvInit nmiDrvInit;
  int ret = 0;

  //ISDBT_MSG_SHARP_BB("[%s] \n", __func__);

  sharp_bb_pre_init();

  memset(&nmiDrvInit, 0, sizeof(tNmDrvInit));
  nmiDrvInit.isdbt.bustype = nI2C;
  nmiDrvInit.isdbt.transporttype = nTS;
  nmiDrvInit.isdbt.blocksize = 1;
  nmiDrvInit.isdbt.ai2c.adrM = 0x61;
  nmiDrvInit.isdbt.core.bustype = nmiDrvInit.isdbt.bustype;
  nmiDrvInit.isdbt.core.transporttype = nmiDrvInit.isdbt.transporttype;
  nmiDrvInit.isdbt.core.dco = 1;
  nmiDrvInit.isdbt.core.ldoby = 1; /* Set to 1 if the IO voltage is less than 2.2v */
  nmiDrvInit.isdbt.core.op = nSingle;
  //nmiDrvInit.isdbt.core.xo = 32.;

  nmi_drv_init(&nmiDrvInit);
  ret = nmi_drv_init_core(nISDBTMode, nMaster);

  ISDBT_MSG_SHARP_BB("[%s]  result [%d]\n", __func__, ret);

  return ret;
}


/*====================================================================
FUNCTION       sharp_bb_set_frequency
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int sharp_bb_set_frequency(int freq_idx)
{
  tIsdbtScan scan;

  memset(&scan, 0, sizeof(tIsdbtScan));

  if(freq_idx == 100)
  {
    scan.frequency = 473143000;
  }
  else
  {
    scan.frequency = isdbt_bb_index2freq(freq_idx);
  }

  //set_frequency = scan.frequency;

  nmi_drv_scan(nISDBTMode, nMaster, &scan);

  if(scan.found)
  {
    ISDBT_MSG_SHARP_BB("[%s] found the channel, freq=[%d]\n", __func__, scan.frequency);
  }
  else
  {
    ISDBT_MSG_SHARP_BB("[%s] No. channel, freq=[%d]\n", __func__, scan.frequency);
  }

  //ISDBT_MSG_SHARP_BB("[%s]  freq[%d] result [%d]\n", __func__, scan.frequency, scan.found);

  return scan.found;
}


/*====================================================================
FUNCTION       sharp_bb_fast_search
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int sharp_bb_fast_search(int freq)
{
  int result=0;

  ISDBT_MSG_SHARP_BB("[%s]  freq[%d] \n", __func__, freq);

  return result;
}


#ifdef FEATURE_DMB_LNA_CTRL
/*====================================================================
FUNCTION       sharp_bb_lna_ctrl
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void sharp_bb_lna_ctrl(int sqindicator)
{
  static int lna_ctrl = 0;

  if(lna_ctrl)
  {
    if(sqindicator >= 90)
    {
      //ISDBT_MSG_SHARP_BB("[%s] DMB_LNA OFF, sqindicator[%d]\n", __func__, sqindicator);
      dmb_set_gpio(DMB_LNA, 0);
      lna_ctrl = 0;
    }
  }
  else
  {
    if(sqindicator < 35)
    {
      //ISDBT_MSG_SHARP_BB("[%s] DMB_LNA ON, sqindicator[%d]\n", __func__, sqindicator);
      dmb_set_gpio(DMB_LNA, 1);
      lna_ctrl = 1;
    }
  }
}
#endif /* FEATURE_DMB_LNA_CTRL */


/*====================================================================
FUNCTION       sharp_bb_get_ant_value
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int sharp_bb_get_ant_value(int sqindicator)
{
  static int saved_level = 0;
  int level = 0;
  int ant_table[] = {7, 15, 25};
  int hystery_val[] = {3, 5, 5};
  
  if(sqindicator < ant_table[0]) level = 0;
  else if((sqindicator >= ant_table[0] && sqindicator < ant_table[1])) level = 1;
  else if((sqindicator >= ant_table[1] && sqindicator < ant_table[2])) level = 2;
  else if(sqindicator >= ant_table[2]) level = 3;

  if(/*(level > 0) &&*/ (abs(level - saved_level) == 1))
  {
    if((level > saved_level) && (sqindicator >= (ant_table[level-1]+hystery_val[level-1]))) // ANT bigger case
    {
      saved_level = level;
    }
    else if((level < saved_level) && (sqindicator <= ant_table[level])) // ANT smaller case
    {
      saved_level = level;
    }
  }
  else
  {
    saved_level = level;
  }

  return saved_level;
}


/*====================================================================
FUNCTION       sharp_bb_get_status
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void sharp_bb_get_status(tIsdbtSigInfo *sig_info)
{
  //ISDBT_MSG_SHARP_BB("[%s] \n", __func__);

  tIsdbtSignalStatus status;
    
  memset(&status, 0, sizeof(tIsdbtSignalStatus));

  status.get_simple_mode = 1; /* if zero, you will get all signal information. */
  
  nmi_isdbt_get_status(nMaster, &status);

  sig_info->ber = status.dberinst;
  sig_info->per = status.dperinst;
  // ant level -> sqindicator (0~100) by NMI.
  sig_info->cninfo = sharp_bb_get_ant_value(status.sqindicator);

#ifdef FEATURE_DMB_LNA_CTRL
  sharp_bb_lna_ctrl(status.sqindicator);
#endif

  if(status.lock)
  {
    //ISDBT_MSG_SHARP_BB("[%s] Locked, freq=[%d], ant=[%d], rssi=[%d]\n", __func__, status.frequency, sig_info->cninfo, status.rssi);
  }
  else
  {
    ISDBT_MSG_SHARP_BB("[%s] Unlocked, freq=[%d], ant=[%d], rssi=[%d]\n", __func__, status.frequency, sig_info->cninfo,status.rssi);
  }
}


/*====================================================================
FUNCTION       sharp_bb_get_tuner_info
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void sharp_bb_get_tuner_info(tIsdbtTunerInfo *tuner_info)
{
  tIsdbtSignalStatus status;

  //ISDBT_MSG_SHARP_BB("[%s] \n", __func__);
  
  memset(&status, 0, sizeof(tIsdbtSignalStatus));

  status.get_simple_mode = 0; /* if zero, you will get all signal information. */

  nmi_isdbt_get_status(nMaster, &status);

  // ant level -> sqindicator (0~100) by NMI.
  tuner_info->rssi = status.rssi;
  tuner_info->ant_level = sharp_bb_get_ant_value(status.sqindicator);
  tuner_info->ber = status.dberinst;
  tuner_info->per = status.dperinst;
  tuner_info->snr = status.nsnr;
  tuner_info->doppler_freq = status.doppler; // Hz ´ÜÀ§
  tuner_info->tmcc_info.carrier_mod = status.modulation;
  tuner_info->tmcc_info.coderate= status.sqindicator;//status.fec;
  tuner_info->tmcc_info.interleave_len= status.interleaver;

#ifdef FEATURE_DMB_LNA_CTRL
  sharp_bb_lna_ctrl(status.sqindicator);
#endif

  if(status.lock)
  {
    //ISDBT_MSG_SHARP_BB("[%s] Locked, freq[%d], sq[%d], rssi[%d], ber=[%ld]\n", __func__, status.frequency, status.sqindicator, status.rssi, tuner_info->ber);
    //ISDBT_MSG_SHARP_BB("[%s] Locked, freq[%d], sq[%d], rssi[%d], ber=[%ld], Dfreq=[%ld]\n", __func__, status.frequency, status.sqindicator, status.rssi, tuner_info->ber, tuner_info->doppler_freq);
  }
  else
  {
    ISDBT_MSG_SHARP_BB("[%s] Unlocked, freq=[%d], sq=[%d], rssi=[%d], ber=[%ld]\n", __func__, status.frequency, status.sqindicator,status.rssi, tuner_info->ber);
  }
}


/*====================================================================
FUNCTION       sharp_bb_start_ts
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int sharp_bb_start_ts(int enable)
{
  int result = 0;
  //tIsdbtRun tune;
  tNmDtvStream stream;
  
  ISDBT_MSG_SHARP_BB("[%s] enable [%d]\n", __func__, enable);

  //tune.highside = 0;
  //tune.frequency = set_frequency;
  //nmi_drv_run(nISDBTMode, nMaster, &tune);

  memset(&stream, 0, sizeof(tNmDtvStream));
  stream.transporttype = nTS;
  stream.tso.mode = nSingle;
  stream.tso.tstype = nSerial;
  stream.tso.blocksize = 1;
  stream.tso.clkrate = nClk_s_2; // 2Mhz
  stream.tso.datapol = 0; /* refer to the comment in nmi325test.c */
  stream.tso.validpol = 0; /* refer to the comment in nmi325test.c */
  stream.tso.syncpol = 0; /* refer to the comment in nmi325test.c */
  stream.tso.clkedge = 0; /* refer to the comment in nmi325test.c */

  nmi_drv_video(nISDBTMode, &stream, enable);

  return result;
}

#ifdef FEATURE_TEST_READ_CHIPID
/*====================================================================
FUNCTION       i2c_read_chipID
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static unsigned int i2c_read_chipID(void)
{
  uint8_t b[16];
  uint32_t val;
  int len;
  uint32_t adr = 0x6400;
  int ret = 0;

  ISDBT_MSG_SHARP_BB("[%s] \n", __func__);

  b[0] = 0x80; /* word access */
  b[1] = (uint8_t)(adr >> 16);
  b[2] = (uint8_t)(adr >> 8);
  b[3] = (uint8_t)(adr);
  b[4] = 0x00;
  b[5] = 0x04;
  len = 6;

  //chip.inp.hlp.read(cix, b, len, (uint8_t *)&val, 4)
  ret = sharp_i2c_write_data(NMI_I2C_ID >> 1, b, len);
  if (ret)
  {
    ISDBT_MSG_SHARP_BB("[%s] 1111111\n", __func__);
  }
  else
  {
    ISDBT_MSG_SHARP_BB("[%s] 2222222\n", __func__);
  }

  ret = sharp_i2c_read_data(NMI_I2C_ID >> 1, (uint8_t *)&val, 4);
  if(ret)
  {
    ISDBT_MSG_SHARP_BB("[%s] 3333333\n", __func__);
  }
  else
  {
    ISDBT_MSG_SHARP_BB("[%s] 4444444\n", __func__);
  }

  return val;
}


/*====================================================================
FUNCTION       read_chipID
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void read_chipID(void)
{
  unsigned int nChipId = 0;

  ISDBT_MSG_SHARP_BB("[%s] start!!!\n", __func__);

  //DRV_MASTER_CTL(NMI_DRV_GET_CHIPID, &nChipId);
  //nChipId = isdbt_get_chipid(nMaster);
  //nChipId = 0;
  nChipId = i2c_read_chipID();

  ISDBT_MSG_SHARP_BB("[%s] chipID[0x%x]\n", __func__, nChipId);
}
#endif


/*====================================================================
FUNCTION       sharp_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void sharp_test(int index)
{
#ifdef FEATURE_TEST_READ_CHIPID
  int i;
#else
  tIsdbtSigInfo sig_info;
#endif

  ISDBT_MSG_SHARP_BB("[%s] start!!!\n", __func__);

  sharp_bb_pre_init();

#ifdef FEATURE_TEST_READ_CHIPID
  sky_isdbt_pre_init();

  for(i = 0; i < 100; i++)
  {
    //nmi_test();
    read_chipID();
  }
#else
  sharp_bb_init();

  ISDBT_MSG_SHARP_BB("[%s] freq index[%d]\n", __func__, index);
  sharp_bb_set_frequency(index);
  sharp_bb_start_ts(1);
  sharp_bb_get_status(&sig_info);
#endif
  ISDBT_MSG_SHARP_BB("[%s] end!!!\n", __func__);
}


/*====================================================================
FUNCTION       sharp_i2c_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void sharp_i2c_test(void)
{

}

