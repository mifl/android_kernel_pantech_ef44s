//=============================================================================
// File       : isdbt_test.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2011/09/29       yschoi         Create
//=============================================================================

#ifndef _ISDBT_TEST_H_
#define _ISDBT_TEST_H_



/*===================================================================
                         define
====================================================================*/

/* ========== Message ID for ISDB-T ========== */

#define ISDBT_MSG_TEST(fmt, arg...) \
  DMB_KERN_MSG_ALERT(fmt, ##arg)



#define ISDBT_TEST_CH 777

/*===========================================================================
FUNCTION       isdbt_test_on_boot
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
#if defined(FEATURE_TEST_ON_BOOT)// 부팅중에 테스트
void isdbt_test_on_boot(void);
#endif

#endif /* _ISDBT_TEST_H_ */