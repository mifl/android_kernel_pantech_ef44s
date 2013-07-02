//=============================================================================
// File       : TCC3170_bb.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       
//  1.1.0       2012/03/07       yschoi         porting New SDK
//=============================================================================

#ifndef __TCC3170_BB_H__
#define __TCC3170_BB_H__

#include "../tdmb_bb.h"

#define TDMB_MSG_TCC_BB(fmt, arg...) \
  DMB_KERN_MSG_ALERT(fmt, ##arg)

#define TCC3170_CID 0xA0

#define MAX_SUB_CH_SIZE 2

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
FUNCTION       tdmb_bb_tcc3170_init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean tdmb_bb_tcc3170_init(tdmb_bb_function_table_type *);

uint8 tcc3170_i2c_write_word(uint8 chipid, uint8 reg, uint8 data);
uint8 tcc3170_i2c_write_len(uint8 chipid, uint8 reg, uint8 *data, uint32 data_size);
uint8 tcc3170_i2c_read_word(uint8 chipid, uint8 reg, uint8 *data);
void tcc3170_i2c_read_len(uint8 chipid, uint8 uiAddr, uint8 ucRecvBuf[], uint32 ucCount);

void tcc3170_set_powersave_mode(void);
void tcc3170_power_on(void);
void tcc3170_power_off(void);
uint8 tcc3170_init(void);
void tcc3170_test(int servicetype);
uint8 tcc3170_stop(void);
uint8 tcc3170_rw_test(void);


/*====================================================================
FUNCTION       tcc3170_ch_scan_start
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_ch_scan_start(int freq, int band, unsigned char for_air);

/*====================================================================
FUNCTION       tcc3170_bb_resync
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_bb_resync(unsigned char imr);

/*====================================================================
FUNCTION       tcc3170_subch_start
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_subch_start(uint8 *regs, uint32 data_rate);

/*====================================================================
FUNCTION       tcc3170_read_int
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tdmb_bb_int_type tcc3170_read_int(void);

/*====================================================================
FUNCTION       tcc3170_get_sync_status
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tBBStatus tcc3170_get_sync_status(void);

/*====================================================================
FUNCTION       tcc3170_read_fib
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_read_fib(uint8 *fibs);

/*====================================================================
FUNCTION       tcc3170_set_subchannel_info
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_set_subchannel_info(void *sub_ch_info);

/*====================================================================
FUNCTION       tcc3170_read_msc
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_read_msc(uint8 *msc_buffer);

/*====================================================================
FUNCTION       tcc3170_get_ber
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_get_ber(tdmb_bb_sig_type *);

/*====================================================================
FUNCTION       tcc3170_Ant_Level
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_Ant_Level(uint32 pcber,int rssi);

#ifdef FEATURE_DMB_TSIF_IF
/*====================================================================
FUNCTION       tcc3170_tsif_parser_callback
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_tsif_parser_callback(int dev_idx, unsigned char *_stream, int _size, int _subch_id, int _type);

/*====================================================================
FUNCTION       tcc3170_tsif_data_parser
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void tcc3170_tsif_data_parser(uint8* data_buf, uint32 size);
#endif /* FEATURE_DMB_TSIF_IF */

#ifdef FEATURE_DMB_SPI_IF
/*====================================================================
FUNCTION       tcc3170_spi_put_data
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_spi_put_data(uint8* data_buf, uint32 size, uint8 type);

/*====================================================================
FUNCTION       tcc3170_spi_put_fic_data
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_spi_put_fic_data(uint8* data_buf, uint32 size);

/*====================================================================
FUNCTION       tcc3170_spi_put_msc_data
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int tcc3170_spi_put_msc_data(uint8* data_buf, uint32 size);
#endif /* FEATURE_DMB_SPI_IF */


#endif 
