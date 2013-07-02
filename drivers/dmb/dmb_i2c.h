//=============================================================================
// File       : dmb_i2c.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2009/05/06       yschoi         Create
//  1.1.0       2011/09/29       yschoi         tdmb_i2c.c => dmb_i2c.h
//=============================================================================

#ifndef _DMB_I2C_H_
#define _DMB_I2C_H_

#include "dmb_type.h"


/* ========== Message ID for TDMB ========== */

#define DMB_MSG_I2C(fmt, arg...) \
  DMB_KERN_MSG_ALERT(fmt, ##arg)


/*================================================================== */
/*================      TDMB Module Definition     ================= */
/*================================================================== */

#define DMB_I2C_DEV_NAME "dmb_i2c"



void dmb_i2c_api_Init(void);
void dmb_i2c_api_DeInit(void);

uint8 dmb_i2c_write_data(uint16 reg, uint8 *data, uint16 data_len);
uint8 dmb_i2c_write(uint8 chip_id, uint16 reg, uint8 reg_len, uint16 data, uint8 data_len);
uint8 dmb_i2c_write_len(uint8 chip_id, uint16 reg, uint8 reg_len, uint8 *data, uint16 data_len);

uint16 dmb_i2c_read_data(uint16 reg, uint8 reg_len, uint8 *data, uint8 *read_buf, uint16 data_len);
uint16 dmb_i2c_read_data_only(uint16 reg, uint8 *read_buf, uint16 data_len);
uint16 dmb_i2c_read(uint8 chip_id, uint16 reg, uint8 reg_len, uint16 *data, uint8 data_len);
void dmb_i2c_read_len(uint8 chip_id, uint16 reg, uint8 *read_buf, uint16 read_buf_len, uint8 reg_len);
uint16 dmb_i2c_read_len_multi(uint16 reg, uint8 *data, uint16 reg_len, uint8 *read_buf, uint16 data_len);

#endif /* _DMB_I2C_H_ */

