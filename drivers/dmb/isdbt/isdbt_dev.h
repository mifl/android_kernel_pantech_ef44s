//=============================================================================
// File       : isdbt_dev.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2011/09/29       yschoi         Create
//=============================================================================

#ifndef _ISDBT_DEV_H_
#define _ISDBT_DEV_H_

#include "../dmb_type.h"

#include "isdbt_comdef.h"


/* ========== Message ID for ISDB-T ========== */

#define ISDBT_MSG_DEV(fmt, arg...) \
  DMB_KERN_MSG_ALERT(fmt, ##arg)


/*================================================================== */
/*================    ISDB-T Module Definition     ================= */
/*================================================================== */

#define ISDBT_DEV_NAME "isdbt"
#define ISDBT_PLATFORM_DEV_NAME "isdbt_dev"

#define IOCTL_ISDBT_MAGIC 't'


typedef enum
{
  FUNC_ISDBT_BB_START = 0,     //
  FUNC_ISDBT_BB_END,           // 
  FUNC_ISDBT_BB_SET_FREQ,     // 
  FUNC_ISDBT_BB_GET_FREQ,     //
  FUNC_ISDBT_BB_FAST_SEARCH,  //
  FUNC_ISDBT_BB_GET_STATE,    //
  FUNC_ISDBT_BB_GET_SIG_INFO, //
  FUNC_ISDBT_BB_GET_TUNER_INFO, //
  FUNC_ISDBT_BB_START_TS,      //
  FUNC_ISDBT_BB_STOP_TS,      //
  FUNC_ISDBT_BB_READ_TS,      //
  FUNC_ISDBT_BB_TEST,      //
  FUNC_ISDBT_MAXNR
} sky_isdbt_func_name_type;


#define IOCTL_ISDBT_BB_START            _IO(IOCTL_ISDBT_MAGIC, FUNC_ISDBT_BB_START)
#define IOCTL_ISDBT_BB_STOP             _IO(IOCTL_ISDBT_MAGIC, FUNC_ISDBT_BB_END)
#define IOCTL_ISDBT_BB_SET_FREQ        _IOW(IOCTL_ISDBT_MAGIC, FUNC_ISDBT_BB_SET_FREQ, unsigned char)
#define IOCTL_ISDBT_BB_GET_FREQ        _IOR(IOCTL_ISDBT_MAGIC, FUNC_ISDBT_BB_GET_FREQ, unsigned char)
#define IOCTL_ISDBT_BB_FAST_SEARCH     _IOWR(IOCTL_ISDBT_MAGIC, FUNC_ISDBT_BB_FAST_SEARCH, unsigned char)
#define IOCTL_ISDBT_BB_GET_STATE       _IOR(IOCTL_ISDBT_MAGIC, FUNC_ISDBT_BB_GET_STATE, unsigned char)
#define IOCTL_ISDBT_BB_GET_SIG_INFO    _IOR(IOCTL_ISDBT_MAGIC, FUNC_ISDBT_BB_GET_SIG_INFO, unsigned char)
#define IOCTL_ISDBT_BB_GET_TUNER_INFO _IOR(IOCTL_ISDBT_MAGIC, FUNC_ISDBT_BB_GET_TUNER_INFO, unsigned char)
#define IOCTL_ISDBT_BB_START_TS        _IOR(IOCTL_ISDBT_MAGIC, FUNC_ISDBT_BB_START_TS, unsigned char)
#define IOCTL_ISDBT_BB_STOP_TS         _IOR(IOCTL_ISDBT_MAGIC, FUNC_ISDBT_BB_STOP_TS, unsigned char)
#define IOCTL_ISDBT_BB_READ_TS         _IOR(IOCTL_ISDBT_MAGIC, FUNC_ISDBT_BB_READ_TS, unsigned char)
#define IOCTL_ISDBT_BB_TEST            _IOR(IOCTL_ISDBT_MAGIC, FUNC_ISDBT_BB_TEST, unsigned char)
#define IOCTL_ISDBT_MAXNR  FUNC_ISDBT_MAXNR

#endif

