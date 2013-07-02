//=============================================================================
// File       : sharp_bb.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2011/09/29       yschoi         Create
//=============================================================================

#ifndef _SHARP_BB_H_
#define _SHARP_BB_H_

#include "../../dmb_type.h"


/* ========== Message ID for SHARP ========== */

#define ISDBT_MSG_SHARP_BB(fmt, arg...) \
  DMB_KERN_MSG_ALERT(fmt, ##arg)



/*====================================================================
FUNCTION       isdbt_bb_sharp_init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean isdbt_bb_sharp_init(isdbt_bb_function_table_type *);

int sharp_i2c_write_data(unsigned char c, unsigned char *data, unsigned long data_width);
#ifdef FEATURE_DMB_SHARP_I2C_READ
int sharp_i2c_read_data(unsigned char c, unsigned char *wdata, unsigned long wdata_width,unsigned char *rdata, unsigned long rdata_width);
#else
int sharp_i2c_read_data(unsigned char c, unsigned char *data, unsigned long data_width);
#endif

/*====================================================================
FUNCTION       sharp_power_on
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void sharp_power_on(void);

/*====================================================================
FUNCTION       sharp_power_off
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void sharp_power_off(void);

/*====================================================================
FUNCTION       sharp_bb_init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int sharp_bb_init(void);

/*====================================================================
FUNCTION       sharp_bb_set_frequency
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int sharp_bb_set_frequency(int freq_idx);

/*====================================================================
FUNCTION       sharp_bb_fast_search
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int sharp_bb_fast_search(int freq);

/*====================================================================
FUNCTION       sharp_bb_get_status
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void sharp_bb_get_status(tIsdbtSigInfo *sig_info);

/*====================================================================
FUNCTION       sharp_bb_get_tuner_info
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void sharp_bb_get_tuner_info(tIsdbtTunerInfo *tuner_info);

/*====================================================================
FUNCTION       sharp_bb_start_ts
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int sharp_bb_start_ts(int enable);

/*====================================================================
FUNCTION       sharp_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void sharp_test(int index);

/*====================================================================
FUNCTION       sharp_i2c_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void sharp_i2c_test(void);

#endif /* _SHARP_BB_H_ */

