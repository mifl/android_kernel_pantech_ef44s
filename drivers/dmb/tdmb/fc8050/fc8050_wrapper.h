//=============================================================================
// Telechips Confidential Proprietary
// Copyright (C) 2005-2006 Telechips Corporation
// All rights are reserved by Telechips Corporation

// File       : fc8050_bb.h
// Description: 
//
// Revision History:
// Version   Date         Author       Description of Changes
//-----------------------------------------------------------------------------
// 1.0.0     2007/08/16   hkyu       Draft
//=============================================================================

#ifndef _FC8050_H_
#define _FC8050_H_

//#include "tdmb_bb.h"
#include "../tdmb_bb.h"
#include "fci_types.h"
#include "fc8050_bbm.h"
#include "fc8050_regs.h"
#include "fci_tun.h"

#ifdef FEATURE_DMB_SPI_CMD
#include "fc8050_spi.h"
#endif

/*================================================================== */
#define ATMEL_TUNER      1
#define HITACHI_TUNER    2
#define TBK_TUNER         3
#define INC_TUNER         4

#define INTEGRANT_TUNER       10
#define INTEGRANT_FM_TUNER    11
#define INTEGRANT_24M_TUNER   12
#define INTEGRANT_LBAND_TUNER 13


#define FC2501_24M_TUNER    20
#define FC2501_LBAND_TUNER  21
#define FC2501_24M_TUNER_B  22
#define FC2501_LBAND_TUNER_B  23
#define FC2501_19M_TUNER    28
#define FC2501_19M_LBAND_TUNER  29

#define TUA6045_TUNER     30
#define TUA6045_24M_TUNER 31

#define FNX14701_TUNER    40

#define APOLLO_TUNER      50
#define APOLLO_24M_TUNER  51
#define APOLLO_32M_TUNER  52

#define CTX305R_TUNER     60
#define CTX305R_LBAND_TUNER 61

#define RFT400S_TUNER   70
#define RFT400S_LBAND_TUNER 71

#define FC2550_TUNER    90
#define FC2550_LBAND_TUNER  91

#define FC2580_DVBH_TUNER   100
#define FC2580_LBAND_TUNER  101
#define FC2580_VHFBAND_TUNER  102

#define FC8050_VHFBAND_TUNER  110
#define FC8050_LBAND_TUNER    111

#define ARM_I2C_CTRL      1
#define FPGA_I2C_CTRL     2
#define ITD_I2C_CTRL      3

#define  TUNER_NOK    -1
#define  TUNER_OK     0
#define  TUNER_RETRY  10

#define BBM_BASE_ADDRESS  EBI2_GP1_BASE /* EBI2_CS1 */
#define BBM_BASE_OFFSET      0x2

#define VIDEO_MASK 0x01
#define AUDIO_MASK 0x08


//#define FC8050_FREQ_XTAL  24576 //24.576MHz

//#define BBM_OK    1
//#define BBM_NOK   0

#define FREQ_FREE   0
#define FREQ_LOCK   1

typedef enum {
  FC8050_L_BAND,
  FC8050_VHF_BAND
} fc8050_band_type;

typedef struct _tagCHAN_INFO
{
  uint32  ulRFNum;
  uint16  uiEnsumbleID;
  uint16  uiSubChID;
  uint16  uiServiceType;
  uint16  uiStarAddr;
  uint16  uiBitRate;
  uint16  uiTmID;

  uint16  uiSlFlag;
  uint16  ucTableIndex;
  uint16  ucOption;
  uint16  uiProtectionLevel;
  uint16  uiDifferentRate;
  uint16  uiSchSize;
}chan_info;

void msWait(unsigned long ms);


/*====================================================================
FUNCTION       FC8050_BUFFER_RESET  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/

void FC8050_BUFFER_RESET(u8 mask);

/*====================================================================
FUNCTION       fc8050_i2c_write_byte  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_i2c_write_byte(uint8 chipid, uint8 reg, uint8 *data, uint16 length);

/*====================================================================
FUNCTION       fc8050_i2c_write_word  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_i2c_write_word(uint8 chipid, uint16 reg, uint16 *data, uint16 length);

/*====================================================================
FUNCTION       fc8050_i2c_write_len  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_i2c_write_len(uint8 chipid, uint8 reg, uint8 *data, uint16 length);

/*====================================================================
FUNCTION       fc8050_i2c_read_byte  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_i2c_read_byte(uint8 chipid, uint8 reg, uint8 *data, uint16 length);

/*====================================================================
FUNCTION       fc8050_i2c_read_word  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_i2c_read_word(uint8 chipid, uint16 reg, uint16 *data, uint16 length);

/*====================================================================
FUNCTION       fc8050_i2c_read_len  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_i2c_read_len(uint8 chipid, uint8 reg, uint8 *buf_ptr, uint16 length);

/*====================================================================
FUNCTION       tdmb_bb_fc8050_init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean tdmb_bb_fc8050_init(tdmb_bb_function_table_type *);

/*====================================================================
FUNCTION       fc8050_power_on
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_power_on(void);

/*====================================================================
FUNCTION       fc8050_power_off
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_power_off(void);

/*====================================================================
FUNCTION       fc8050_set_int
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_set_int(uint8 enable);

/*====================================================================
FUNCTION       fc8050_drv_init
DESCRIPTION            HPI, SPI mode change Reg. 0x40 - 0x4B setting
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_drv_init(void);

/*====================================================================
FUNCTION       fc8050_ch_scan_start
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_ch_scan_start(int freq, int band, unsigned char for_air);

/*====================================================================
FUNCTION       fc8050_bb_resync
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_bb_resync(unsigned char imr);

/*====================================================================
FUNCTION       fc8050_subch_start
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int fc8050_subch_start(uint8 *regs, uint32 data_rate);

/*====================================================================
FUNCTION       fc8050_read_int
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
#if 0//def FC8050_USE_TSIF
tdmb_bb_int_type fc8050_read_int(u8* data, u32 length);
#else
tdmb_bb_int_type fc8050_read_int(void);
#endif

/*====================================================================
FUNCTION       fc8050_get_sync_status
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tBBStatus fc8050_get_sync_status(void);

/*====================================================================
FUNCTION       fc8050_read_fib
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int fc8050_read_fib(uint8 *fibs);

/*====================================================================
FUNCTION       fc8050_set_subchannel_info
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_set_subchannel_info(void *sub_ch_info);

/*====================================================================
FUNCTION       fc8050_read_msc
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int fc8050_read_msc(uint8 *msc_buffer);

/*====================================================================
FUNCTION       fc8050_get_ber
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_get_ber(tdmb_bb_sig_type *);

/*====================================================================
FUNCTION       fc8050_stop
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 fc8050_stop(void);

/*====================================================================
FUNCTION       fc8050_RFInputPower
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int fc8050_RFInputPower(void);

/*====================================================================
FUNCTION       fc8050_Ant_Level
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int fc8050_Ant_Level(int RSSI, uint32 pcber);

/*====================================================================
FUNCTION       fc8050_get_demux_buffer
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_get_demux_buffer(u8 **res_ptr, u32 *res_size);

/*====================================================================
FUNCTION       fc8050_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_test(int ch);

/*====================================================================
FUNCTION       fc8050_if_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void fc8050_if_test(uint8 test_id);

#endif /* _FC8050_H_ */
