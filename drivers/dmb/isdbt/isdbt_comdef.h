//=============================================================================
// File       : isdbt_comdef.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2011/09/29       yschoi         Create
//=============================================================================

#ifndef _ISDBT_COMDEF_H_
#define _ISDBT_COMDEF_H_


#include "../dmb_comdef.h"


#ifdef CONFIG_SKY_ISDBT_SHARP_BB
  #define FEATURE_ISDBT_USE_SHARP
#endif



/*================================================================== */
/*================     ISDB-T Kconfig Tree           ================= */
/*================================================================== */
//#
//# ISDBT
//#
/*

CONFIG_SKY_ISDBT

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


#endif /* _ISDBT_COMDEF_H_ */