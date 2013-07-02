//=============================================================================
// File       : dmb_tsif.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2010/12/06       yschoi         Create
//  1.1.0       2011/09/29       yschoi         tdmb_tsif.h => dmb_tsif.h
//=============================================================================

#ifndef _DMB_TSIF_H_
#define _DMB_TSIF_H_

#include "dmb_type.h"


/* ========== Message ID for DMB ========== */

#define DMB_MSG_TSIF(fmt, arg...) \
  DMB_KERN_MSG_ALERT(fmt, ##arg)



void dmb_tsif_data_parser(char* user_buf, void * data_buffer, int size);
void dmb_tsif_force_stop(void);

#ifdef FEATURE_DMB_TSIF_CLK_CTL
void dmb_tsif_clk_enable(void);
void dmb_tsif_clk_disable(void);
#endif /* FEATURE_DMB_TSIF_CLK_CTL */

void dmb_tsif_test(void);

#endif /* _DMB_TSIF_H_ */
