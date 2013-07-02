//=============================================================================
// File       : dmb_hw.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2010/11/02       yschoi         Create
//  1.1.0       2011/09/29       yschoi         tdmb_gpio.h => dmb_hw.h
//=============================================================================

#ifndef _DMB_HW_H_
#define _DMB_HW_H_

#include "dmb_type.h"
#include "dmb_comdef.h"


/* ========== Message ID for TDMB ========== */

#define DMB_MSG_HW(fmt, arg...) \
  DMB_KERN_MSG_ALERT(fmt, ##arg)



typedef enum {
  DMB_ANT_EARJACK,
  DMB_ANT_INTERNAL,
  DMB_ANT_EXTERNAL
} dmb_bb_antenna_type;



#ifdef FEATURE_DMB_GPIO_INIT
/*====================================================================
FUNCTION       dmb_gpio_init
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void dmb_gpio_init(void);
#endif

#ifdef CONFIG_SKY_TDMB_MICRO_USB_DETECT
/*====================================================================
FUNCTION       dmb_micro_usb_ant_detect
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int dmb_micro_usb_ant_detect(void);
#endif


/*====================================================================
FUNCTION       dmb_set_gpio
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void dmb_set_gpio(uint gpio, bool value);

/*====================================================================
FUNCTION       dmb_power_on
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void dmb_power_on(void);

/*====================================================================
FUNCTION       dmb_power_on_chip
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void dmb_power_on_chip(void);


/*====================================================================
FUNCTION       dmb_power_off
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void dmb_power_off(void);

/*====================================================================
FUNCTION       dmb_set_ant_path
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void dmb_set_ant_path(int ant_type);



#endif /* _DMB_HW_H_ */

