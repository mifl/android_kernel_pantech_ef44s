//=============================================================================
// File       : dmb_spi.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2011/04/20       yschoi         Create
//  1.1.0       2011/09/29       yschoi         tdmb_spi.h => dmb_spi.h
//=============================================================================

#ifndef _DMB_SPI_H_
#define _DMB_SPI_H_

#include "dmb_type.h"
//#include <linux/spi/spi.h>


/* ========== Message ID for DMB ========== */

#define DMB_MSG_SPI(fmt, arg...) \
  DMB_KERN_MSG_ALERT(fmt, ##arg)



#define DMB_SPI_DEV_NAME "dmb_spi"


void dmb_spi_clear_int(void);
void dmb_spi_init (void);

void* dmb_spi_setup(void);

extern int dmb_spi_write_then_read(u8 *txbuf, unsigned n_tx,u8 *rxbuf, unsigned n_rx);


#endif /* _DMB_SPI_H_ */

