//=============================================================================
// File       : MTV350_bb.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2010/10/28       yschoi         Create
//=============================================================================

#ifndef __MTV350_BB_H__
#define __MTV350_BB_H__

#include "../tdmb_bb.h"


/* ========== Message ID for TDMB ========== */

#define TDMB_MSG_RTV_BB(fmt, arg...) \
  DMB_KERN_MSG_ALERT(fmt, ##arg)

#define TDMB_MSG_RTV_PKT(fmt, arg...) \
  DMB_KERN_MSG_ALERT(fmt, ##arg)

typedef enum _tagPLL_MODE
{
  INPUT_CLOCK_24576KHZ = 0,
  INPUT_CLOCK_12000KHZ,
  INPUT_CLOCK_19200KHZ,
  INPUT_CLOCK_27000KHZ,
}PLL_MODE, *PPLL_MODE;

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



/*====================================================================
FUNCTION       tdmb_bb_mtv350_init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean tdmb_bb_mtv350_init(tdmb_bb_function_table_type *);
uint8 mtv350_i2c_read_len(uint8 reg, uint8* data, uint16 data_len);
uint8 mtv350_i2c_read(uint8 reg);
uint8 mtv350_i2c_write(uint8 reg, uint8 data);
void mtv350_set_powersave_mode(void);
void mtv350_power_on(void);
void mtv350_power_off(void);
uint8 mtv350_init(void);
void mtv350_test(int servicetype);
uint8 mtv350_stop(void);
uint8 mtv350_i2c_test(void);
uint8 mtv350_ebi2_test(void);


/*====================================================================
FUNCTION       mtv350_ch_scan_start
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void mtv350_ch_scan_start(int freq, int band, unsigned char for_air);

/*====================================================================
FUNCTION       mtv350_bb_resync
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void mtv350_bb_resync(unsigned char imr);

/*====================================================================
FUNCTION       mtv350_subch_start
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int mtv350_subch_start(uint8 *regs, uint32 data_rate);

/*====================================================================
FUNCTION       mtv350_read_int
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tdmb_bb_int_type mtv350_read_int(void);

/*====================================================================
FUNCTION       mtv350_get_sync_status
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
tBBStatus mtv350_get_sync_status(void);

/*====================================================================
FUNCTION       mtv350_read_fib
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int mtv350_read_fib(uint8 *fibs);

/*====================================================================
FUNCTION       mtv350_set_subchannel_info
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void mtv350_set_subchannel_info(void *sub_ch_info);

/*====================================================================
FUNCTION       mtv350_read_msc
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int mtv350_read_msc(uint8 *msc_buffer);

/*====================================================================
FUNCTION       mtv350_get_ber
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void mtv350_get_ber(tdmb_bb_sig_type *);

#ifdef FEATURE_DMB_RTV_USE_FM_PATH
/*====================================================================
FUNCTION       mtv350_use_FM_path
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void mtv350_use_FM_path(int set_FM_path);
#endif
#endif /* __MTV318_H__*/
