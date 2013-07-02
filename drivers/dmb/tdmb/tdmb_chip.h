//=============================================================================
// File       : Tdmb_chip.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2011/09/29       yschoi         Create
//=============================================================================

#ifndef _TDMB_CHIP_H_
#define _TDMB_CHIP_H_

#include "tdmb_comdef.h"

#ifdef FEATURE_TDMB_USE_INC_T3900
#include "t3900/t3900_includes.h"
#include "t3900/t3900_bb.h"
#endif

#ifdef FEATURE_TDMB_USE_INC_T3700
#include "t3700/t3700_includes.h"
#include "t3700/t3700_bb.h"
#endif

#ifdef FEATURE_TDMB_USE_FCI_FC8050
#include "fc8050/fc8050_wrapper.h"
#ifdef FEATURE_DMB_TSIF_IF
#include "fc8050/fc8050_demux.h"
#endif
#endif

#ifdef FEATURE_TDMB_USE_RTV_MTV350
#include "mtv350/mtv350_bb.h"
#include "mtv350/raontv.h"
#ifdef FEATURE_TDMB_MULTI_CHANNEL_ENABLE
#include "mtv350/raontv_cif_dec.h"
#endif
#endif

#ifdef FEATURE_TDMB_USE_TCC_TCC3170
#include "tcc3170/tcc3170_bb.h"
#endif


#endif /* _TDMB_CHIP_H_ */