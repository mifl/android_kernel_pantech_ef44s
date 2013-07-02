//=============================================================================
// File       : Tdmb_dev.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2009/05/06       yschoi         Create
//=============================================================================

#ifndef _TDMB_DEV_INCLUDES_H_
#define _TDMB_DEV_INCLUDES_H_


#include "../dmb_type.h"

#include "tdmb_comdef.h"



/* ========== Message ID for TDMB ========== */

#define TDMB_MSG_DEV(fmt, arg...) \
  DMB_KERN_MSG_ALERT(fmt, ##arg)


/*================================================================== */
/*================      TDMB Module Definition     ================= */
/*================================================================== */

#define TDMB_DEV_NAME "tdmb"
#define TDMB_PLATFORM_DEV_NAME "tdmb_dev"
//#define TDMB_I2C_DEV_NAME "tdmb_i2c"

#define IOCTL_TDMB_MAGIC 't'


typedef enum
{
  FUNC_TDMB_BB_DRV_INIT = 0,
  FUNC_TDMB_BB_INIT,
  FUNC_TDMB_BB_POWER_ON,
  FUNC_TDMB_BB_POWER_OFF, // 3
  FUNC_TDMB_BB_SET_ANTENNA_PATH, // 4
  FUNC_TDMB_BB_CH_SCAN_START, // 5
  FUNC_TDMB_BB_GET_FREQUENCY, // 6
  FUNC_TDMB_BB_FIC_PROCESS,
  FUNC_TDMB_BB_SET_FIC_ISR, // 8
  FUNC_TDMB_BB_CHW_INTHANDLER2,
  FUNC_TDMB_BB_EBI2_CHW_INTHANDLER,
  FUNC_TDMB_BB_RESYNC, // 11
  FUNC_TDMB_BB_SUBCH_START, // 12
  FUNC_TDMB_BB_DRV_START,
  FUNC_TDMB_BB_DRV_STOP,
  FUNC_TDMB_BB_REPORT_DEBUG_INFO,
  FUNC_TDMB_BB_GET_TUNING_STATUS, // 16
  FUNC_TDMB_BB_SET_FIC_CH_RESULT,
  FUNC_TDMB_BB_GET_FIC_CH_RESULT,
  FUNC_TDMB_BB_READ_INT,
  FUNC_TDMB_BB_GET_SYNC_STATUS, // 20
  FUNC_TDMB_BB_READ_FIB, // 21
  FUNC_TDMB_BB_SET_SUBCHANNEL_INFO, // 22
  FUNC_TDMB_BB_READ_MSC,
  FUNC_TDMB_BB_I2C_TEST,
  FUNC_TDMB_BB_GET_BER, // 25
  FUNC_TDMB_BB_CH_STOP, // 26
  FUNC_TDMB_BB_CH_TEST,
  FUNC_TDMB_GVAR_RW, //28
  FUNC_TDMB_MAXNR
} sky_tdmb_func_name_type;


#define IOCTL_TDMB_BB_DRV_INIT             _IO(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_DRV_INIT)
#define IOCTL_TDMB_BB_INIT                  _IO(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_INIT)
#define IOCTL_TDMB_BB_POWER_ON             _IO(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_POWER_ON)
#define IOCTL_TDMB_BB_POWER_OFF            _IO(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_POWER_OFF)
#define IOCTL_TDMB_BB_SET_ANTENNA_PATH    _IOW(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_SET_ANTENNA_PATH, unsigned char)
#define IOCTL_TDMB_BB_CH_SCAN_START        _IOW(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_CH_SCAN_START, unsigned char)
#define IOCTL_TDMB_BB_GET_FREQUENCY        _IOWR(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_GET_FREQUENCY, unsigned char)
#define IOCTL_TDMB_BB_FIC_PROCESS          _IO(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_FIC_PROCESS)
#define IOCTL_TDMB_BB_SET_FIC_ISR          _IOWR(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_SET_FIC_ISR, unsigned char)
#define IOCTL_TDMB_BB_CHW_INTHANDLER2      _IO(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_CHW_INTHANDLER2)
#define IOCTL_TDMB_BB_EBI2_CHW_INTHANDLER  _IOWR(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_EBI2_CHW_INTHANDLER, unsigned char)
#define IOCTL_TDMB_BB_RESYNC                  _IOW(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_RESYNC, unsigned char)
#define IOCTL_TDMB_BB_SUBCH_START            _IOWR(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_SUBCH_START, unsigned char)
#define IOCTL_TDMB_BB_DRV_START              _IO(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_DRV_START)
#define IOCTL_TDMB_BB_DRV_STOP               _IO(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_DRV_STOP)
#define IOCTL_TDMB_BB_REPORT_DEBUG_INFO     _IO(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_REPORT_DEBUG_INFO)
#define IOCTL_TDMB_BB_GET_TUNING_STATUS       _IOR(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_GET_TUNING_STATUS, unsigned char)
#define IOCTL_TDMB_BB_SET_FIC_CH_RESULT     _IOW(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_SET_FIC_CH_RESULT, unsigned char)
#define IOCTL_TDMB_BB_GET_FIC_CH_RESULT     _IOR(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_GET_FIC_CH_RESULT, unsigned char)
#define IOCTL_TDMB_BB_READ_INT               _IOR(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_READ_INT, unsigned char)
#define IOCTL_TDMB_BB_GET_SYNC_STATUS       _IOR(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_GET_SYNC_STATUS, unsigned char)
#define IOCTL_TDMB_BB_READ_FIB               _IOR(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_READ_FIB, unsigned char)
#define IOCTL_TDMB_BB_SET_SUBCHANNEL_INFO  _IOW(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_SET_SUBCHANNEL_INFO, unsigned char)
#define IOCTL_TDMB_BB_READ_MSC               _IOR(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_READ_MSC, unsigned char)
#define IOCTL_TDMB_BB_I2C_TEST               _IO(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_I2C_TEST)
#define IOCTL_TDMB_BB_GET_BER                _IOR(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_GET_BER, unsigned char)
#define IOCTL_TDMB_BB_CH_STOP                _IO(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_CH_STOP)
#define IOCTL_TDMB_BB_CH_TEST                _IOWR(IOCTL_TDMB_MAGIC, FUNC_TDMB_BB_CH_TEST, unsigned char)
#define IOCTL_TDMB_GVAR_RW                   _IO(IOCTL_TDMB_MAGIC, FUNC_TDMB_GVAR_RW)
#define IOCTL_TDMB_MAXNR  FUNC_TDMB_MAXNR

boolean tdmb_set_isr(int on_off);

#endif
