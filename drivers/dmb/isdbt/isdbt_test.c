//=============================================================================
// File       : isdbt_test.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2011/09/29       yschoi         Create
//=============================================================================

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>

#include "../dmb_interface.h"
#include "../dmb_hw.h"
#include "isdbt_comdef.h"
#include "isdbt_bb.h"
#include "isdbt_dev.h"
#include "isdbt_test.h"
#include "isdbt_chip.h"



/*===================================================================
                     Pre Declalation  function
====================================================================*/



/*===========================================================================
FUNCTION       isdbt_ch_test
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
void isdbt_ch_test(void)
{
  ISDBT_MSG_TEST("[%s] !!\n", __func__);

  isdbt_bb_power_on();

  dmb_set_ant_path(DMB_ANT_EARJACK);

#if defined(FEATURE_TEST_ON_BOOT) && defined(FEATURE_DMB_TSIF_IF)
  dmb_tsif_test();
#endif /* FEATURE_DMB_TSIF_IF */

#ifdef FEATURE_ISDBT_USE_SHARP  
  sharp_test(100);
#else
  #error
#endif
}


/*===========================================================================
FUNCTION       isdbt_test_on_boot
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
#if defined(FEATURE_TEST_ON_BOOT)// 부팅중에 테스트
void isdbt_test_on_boot(void)
{
  ISDBT_MSG_TEST("[%s] Test start!!!\n", __func__);

#ifdef FEATURE_COMMAND_TEST_ON_BOOT
  isdbt_bb_power_on();

  #if defined(FEATURE_DMB_SPI_CMD)
    //t3700_spi_test();
  #elif defined(FEATURE_DMB_EBI_CMD)
    //t3700_ebi2_test();
  #else
    sharp_i2c_test();
  #endif
#else
  isdbt_ch_test();
#endif

  ISDBT_MSG_TEST("[%s] Test end!!!\n", __func__);
}
#endif

