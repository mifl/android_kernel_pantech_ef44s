//=============================================================================
// File       : Tdmb_comdef.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2009/05/06       yschoi         Create
//=============================================================================

#ifndef _TDMB_COMDEF_INCLUDES_H_
#define _TDMB_COMDEF_INCLUDES_H_


#include "../dmb_comdef.h"


#ifdef CONFIG_SKY_TDMB_MODULE
#define FEATURE_TDMB_MODULE
#endif /* CONFIG_SKY_TDMB_MODULE */

#ifdef CONFIG_SKY_TDMB_INC_BB // EF10, EF12, SP33K
  #define FEATURE_TDMB_USE_INC

  #ifdef CONFIG_SKY_TDMB_INC_BB_T3700
  #define FEATURE_TDMB_USE_INC_T3700
  #endif

  #ifdef CONFIG_SKY_TDMB_INC_BB_T3900
  #define FEATURE_TDMB_USE_INC_T3900
  #endif

  #define FEATURE_TDMB_MULTI_CHANNEL_ENABLE
#endif /* CONFIG_TDMB_INC_BB */

#ifdef CONFIG_SKY_TDMB_FCI_BB  // EF13
  #define FEATURE_TDMB_USE_FCI

  #ifdef CONFIG_SKY_TDMB_FCI_BB_FC8050
  #define FEATURE_TDMB_USE_FCI_FC8050
  #endif

  #define FEATURE_TDMB_MULTI_CHANNEL_ENABLE
#endif /* CONFIG_TDMB_FCI_BB */

#ifdef CONFIG_SKY_TDMB_RTV_BB  // EF33
  #define FEATURE_TDMB_USE_RTV

  #ifdef CONFIG_SKY_TDMB_RTV_BB_MTV350
  #define FEATURE_TDMB_USE_RTV_MTV350
  #endif

  #define FEATURE_TDMB_MULTI_CHANNEL_ENABLE
#endif /* CONFIG_TDMB_FCI_BB */

#ifdef CONFIG_SKY_TDMB_TCC_BB
  #define FEATURE_TDMB_USE_TCC

  #ifdef CONFIG_SKY_TDMB_TCC_BB_TCC3170
  #define FEATURE_TDMB_USE_TCC_TCC3170
  #endif

  #define FEATURE_TDMB_MULTI_CHANNEL_ENABLE
#endif /* CONFIG_SKY_TDMB_TCC_BB */


// driver 에서는 define 하고 framework에서 구분하기로 함.
#if 0//(defined(CONFIG_EF10_BOARD) || defined(CONFIG_EF12_BOARD) || defined(CONFIG_SP33_BOARD))
  // Eclair
  // DMB include visual radio
#else
  // Froyo
  #define FEATURE_TDMB_VISUAL_RADIO_SERVICE
#endif



/*================================================================== */
/*================     TDMB Kconfig Tree           ================= */
/*================================================================== */
//#
//# TDMB - Terrestrial Digital Multimedia Broadcasting
//#
/*

CONFIG_SKY_TDMB

  CONFIG_SKY_TDMB_INC_BB
    CONFIG_SKY_TDMB_INC_BB_T3700
    CONFIG_SKY_TDMB_INC_BB_T3900
    
  CONFIG_SKY_TDMB_FCI_BB
    CONFIG_SKY_TDMB_FCI_BB_FC8050

  CONFIG_SKY_TDMB_TCC_BB
    CONFIG_SKY_TDMB_TCC_BB_TCC3170

  CONFIG_SKY_TDMB_RTV_BB
    CONFIG_SKY_TDMB_RTV_BB_MTV350

*/


#endif /* _TDMB_COMDEF_INCLUDES_H_ */
