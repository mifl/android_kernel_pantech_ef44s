//=============================================================================
// File       : dmb_interface.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2011/09/29       yschoi         Create
//=============================================================================

#ifndef _DMB_INTERFACE_H_
#define _DMB_INTERFACE_H_

#include "dmb_type.h"

#ifdef CONFIG_SKY_DMB_TSIF_IF
#include "dmb_tsif.h"
#endif

#ifdef CONFIG_SKY_DMB_SPI_IF
#include "dmb_spi.h"
#endif

#ifdef CONFIG_SKY_DMB_I2C_CMD
#include "dmb_i2c.h"
#endif

#endif /* _DMB_INTERFACE_H_ */

