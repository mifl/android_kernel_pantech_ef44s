//=============================================================================
// File       : T3700_bb.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2007/08/16       wgyoo          Draft
//  1.1.0       2009/04/28       yschoi         Android Porting
//  1.2.0       2010/07/30       wgyoo          T3900 porting
//=============================================================================

#ifndef _T3900_H_
#define _T3900_H_

#include "../tdmb_bb.h"

#include "t3900_includes.h"


#define T3700_CID 0x80

#define MAX_SUB_CH_SIZE 2  // 64 => 2 temporary
#define REF_CS_SIZE     (188*8)


typedef struct _tagCHAN_INFO
{
    uint32 ulRFNum;
    uint16 uiEnsumbleID;
    uint16 uiSubChID;
    uint16 uiServiceType;
    uint16 uiStarAddr;
    uint16 uiBitRate;
    uint16 uiTmID;

    uint16 uiSlFlag;
    uint16 ucTableIndex;
    uint16 ucOption;
    uint16 uiProtectionLevel;
    uint16 uiDifferentRate;
    uint16 uiSchSize;
}chan_info;

typedef struct tagST_SUBCH_INFO
{
    int16 nSetCnt;
    chan_info astSubChInfo[MAX_SUB_CH_SIZE];
}st_subch_info;


extern boolean tdmb_power_on;

/*====================================================================
FUNCTION       tdmb_bb_t3700_init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean tdmb_bb_t3900_init(tdmb_bb_function_table_type *);
void t3700_i2c_read_len(uint8 chipid, unsigned int uiAddr, uint8 ucRecvBuf[], uint16 ucCount);
uint8 t3700_i2c_read_word(uint8 chipid, uint16 reg, uint16 *data);
uint8 t3700_i2c_write_word(uint8 chipid, uint16 reg, uint16 data);
void t3700_set_powersave_mode(void);
void t3700_power_on(void);
void t3700_power_off(void);
uint8 t3700_init(void);
void t3700_test(int servicetype);
uint8 t3700_stop(void);
uint8 t3700_i2c_test(void);
uint8 t3700_ebi2_test(void);
uint8 t3700_spi_test(void);

/*====================================================================
FUNCTION       t3700_ch_scan_start
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void t3700_ch_scan_start(int freq, int band, unsigned char for_air);

/*====================================================================
FUNCTION       t3700_bb_resync
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void t3700_bb_resync(unsigned char imr);

/*====================================================================
FUNCTION       t3700_subch_start
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int t3700_subch_start(uint8 *regs, uint32 data_rate);

/*====================================================================
FUNCTION       t3700_read_int
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tdmb_bb_int_type t3700_read_int(void);

/*====================================================================
FUNCTION       t3700_get_sync_status
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tBBStatus t3700_get_sync_status(void);

/*====================================================================
FUNCTION       t3700_read_fib
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int t3700_read_fib(uint8 *fibs);

/*====================================================================
FUNCTION       t3700_set_subchannel_info
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void t3700_set_subchannel_info(void *sub_ch_info);

/*====================================================================
FUNCTION       t3700_read_msc
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int t3700_read_msc(uint8 *msc_buffer);

/*====================================================================
FUNCTION       t3700_get_ber
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void t3700_get_ber(tdmb_bb_sig_type *);

/*====================================================================
FUNCTION       t3700_Ant_Level
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int t3700_Ant_Level(uint32 pcber);

#if defined(INC_MULTI_CHANNEL_ENABLE) && defined(INC_T3700_USE_TSIF)
/*====================================================================
FUNCTION       t3700_header_parsing_tsif_data
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint16 t3700_header_parsing_tsif_data(uint8* p_data_buf, uint32 size);
#endif

#ifdef FEATURE_COMMUNICATION_CHECK
/*====================================================================
FUNCTION       t3700_reset
DESCRIPTION   only for T3900
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void t3900_reset(void);
#endif

#endif 
