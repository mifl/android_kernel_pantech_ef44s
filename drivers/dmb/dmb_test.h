//=============================================================================
// File       : dmb_test.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2012/03/29       yschoi         Create
//=============================================================================

#ifndef _DMB_TEST_H_
#define _DMB_TEST_H_

#include "dmb_type.h"


/* ========== Message ID for DMB ========== */

#define DMB_MSG_TEST(fmt, arg...) \
  DMB_KERN_MSG_ALERT(fmt, ##arg)


#define FILENAME_BEFORE_PARSING "/data/misc/dmb/pre_parsing.txt"
#define FILENAME_AFTER_PARSING "/data/misc/dmb/msc_after_parsing.txt"
#define FILENAME_RAW_MSC "/data/misc/dmb/raw_msc.txt"
#define FILENAME_RAW_FIC "/data/misc/dmb/raw_fic.txt"

/*===========================================================================
FUNCTION       dmb_data_dump
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
void dmb_data_dump(u8 *p, u32 size, char *filename);

#ifdef FEATURE_TEST_ON_BOOT
/*===========================================================================
FUNCTION       dmb_test_on_boot
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
void dmb_test_on_boot(void);
#endif

#endif /* _DMB_TEST_H_ */
