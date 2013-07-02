//=============================================================================
// File       : dmb_comdef.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2011/09/29       yschoi         Create
//=============================================================================

#ifndef _DMB_COMDEF_H_
#define _DMB_COMDEF_H_

#ifdef CONFIG_SKY_TDMB
#include "tdmb/tdmb_modeldef.h"
#endif

#ifdef CONFIG_SKY_ISDBT
#include "isdbt/isdbt_modeldef.h"
#endif


#ifdef CONFIG_SKY_DMB_MODULE
 #define FEATURE_DMB_MODULE
#endif /* CONFIG_SKY_DMB_MODULE */

#ifdef CONFIG_SKY_DMB_EBI_IF
 #define FEATURE_DMB_EBI_IF

 #ifdef CONFIG_SKY_DMB_EBI_CMD // EF10, SP33K, EF12S, EF13
  #define FEATURE_DMB_EBI_CMD
 #endif
#endif /* CONFIG_SKY_DMB_EBI_IF */

#ifdef CONFIG_SKY_DMB_TSIF_IF
 #define FEATURE_DMB_TSIF_IF
 #define FEATURE_DMB_TSIF_READ_ONCE
#endif

#ifdef CONFIG_SKY_DMB_I2C_CMD // EF10 √ ±‚
 #define FEATURE_DMB_I2C_CMD
#endif

#ifdef CONFIG_SKY_DMB_I2C_HW
 #define FEATURE_DMB_I2C_HW
#endif

#ifdef CONFIG_SKY_DMB_SPI_IF
 #define FEATURE_DMB_SPI_IF

 #ifdef CONFIG_SKY_DMB_SPI_CMD
  #define FEATURE_DMB_SPI_CMD
 #endif
#endif

#ifdef CONFIG_SKY_DMB_PMIC_19200
  #define FEATURE_DMB_PMIC_19200
#endif

// DMB kernel MSG FEATURE
#define FEATURE_DMB_KERNEL_MSG_ON



/*================================================================== */
/*================      DMB Module Definition      ================= */
/*================================================================== */

#define DMB_DEV_NAME "dmb"
#define DMB_PLATFORM_DEV_NAME "dmb_dev"


/*================================================================== */
/*================      DMB Kconfig Tree           ================= */
/*================================================================== */
//#
//# DMB - Digital Multimedia Broadcasting
//#
/*

CONFIG_SKY_DMB

  CONFIG_SKY_DMB_MODULE

  CONFIG_SKY_TDMB
  CONFIG_SKY_ISDBT
  CONFIG_SKY_ISDBTMM

  CONFIG_SKY_DMB_EBI_IF
    CONFIG_SKY_DMB_EBI_CMD

  CONFIG_SKY_DMB_TSIF_IF

  CONFIG_SKY_DMB_I2C_CMD
    CONFIG_SKY_DMB_I2C_HW
    CONFIG_SKY_DMB_I2C_GPIO

  CONFIG_SKY_DMB_SPI_IF
    CONFIG_SKY_DMB_SPI_HW
    CONFIG_SKY_DMB_SPI_TEGRA
    CONFIG_SKY_DMB_SPI_GPIO

  CONFIG_SKY_TDMB_MICRO_USB_DETECT

*/



/*================================================================== */
/*================     Kernel Message for DMB      ================= */
/*================================================================== */

#ifdef FEATURE_DMB_KERNEL_MSG_ON
#define DMB_KERN_MSG_EMERG(fmt, arg...) \
  pr_emerg(fmt, ##arg)

#define DMB_KERN_MSG_ALERT(fmt, arg...) \
  pr_alert(fmt, ##arg)

#define DMB_KERN_MSG_CRIT(fmt, arg...) \
  pr_crit(fmt, ##arg)

#define DMB_KERN_MSG_ERR(fmt, arg...) \
  pr_err(fmt, ##arg)

#define DMB_KERN_MSG_WARNING(fmt, arg...) \
  pr_warning(fmt, ##arg)

#define DMB_KERN_MSG_NOTICE(fmt, arg...) \
  pr_notice(fmt, ##arg)

#define DMB_KERN_MSG_INFO(fmt, arg...) \
  pr_info(fmt, ##arg)
#else
#define DMB_KERN_MSG_EMERG(fmt, arg...) \
  {}

#define DMB_KERN_MSG_ALERT(fmt, arg...) \
  {}

#define DMB_KERN_MSG_CRIT(fmt, arg...) \
  {}

#define DMB_KERN_MSG_ERR(fmt, arg...) \
  {}

#define DMB_KERN_MSG_WARNING(fmt, arg...) \
  {}

#define DMB_KERN_MSG_NOTICE(fmt, arg...) \
  {}

#define DMB_KERN_MSG_INFO(fmt, arg...) \
  {}
#endif /* FEATURE_DMB_KERNEL_MSG_ON */

#endif /* _DMB_COMDEF_H_ */