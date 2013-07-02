//=============================================================================
// File       : isdbt_bb.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2011/09/29       yschoi         Create
//=============================================================================

#ifndef _ISDBT_BB_H_
#define _ISDBT_BB_H_

#include "../dmb_type.h"
#include "isdbt_comdef.h"


/* ========== Message ID for ISDB-T ========== */

#define ISDBT_MSG_BB(fmt, arg...) \
  DMB_KERN_MSG_ALERT(fmt, ##arg)



#define ISDBT_1SEG_CH_OFFSET 13
#define ISDBT_1SEG_MAX_CH 62
#define ISDBT_1SEG_NUM_OF_CH (ISDBT_1SEG_MAX_CH - ISDBT_1SEG_CH_OFFSET+1) //50

#define TDMB_TS_PKT_SIZE  188
#define TSIF_CHUNK_SIZE    16
#define TSIF_DATA_SIZE    192
#define BB_MAX_DATA_SIZE (TSIF_DATA_SIZE*TSIF_CHUNK_SIZE)



typedef struct
{
  int  ts_size;
  char ts_buf[BB_MAX_DATA_SIZE];
} isdbt_ts_data_type;


typedef enum {
  ISDBT_RETVAL_SUCCESS = 0,
  ISDBT_RETVAL_PARAMETER_ERROR,
  ISDBT_RETVAL_DRIVER_ERROR,
  ISDBT_RETVAL_OTHERS,
  ISDBT_RETVAL_ENUM_MAX,
} tIsdbtRet_type;


typedef enum {
  ISDBT_STATE_PLAY,
  ISDBT_STATE_STOP
} tIsdbtState_type;


typedef enum {
  ISDBT_SYNC_LOCKED,
  ISDBT_SYNC_UNLOCKED
} tIsdbtSyncState_type;


typedef struct
{
  int                  freq_num;
  tIsdbtSyncState_type  sync;
} tIsdbtChInfo;


typedef struct
{
  int freq_num;
  int num;
  tIsdbtChInfo data[50];
} tIsdbtFastSearch;


typedef struct
{
  uint32 ber;
  int per;
  int cninfo;
} tIsdbtSigInfo;


typedef struct
{
  int carrier_mod;
  int coderate;
  int interleave_len;
} tIsdbtTmcc;

typedef struct
{
  int rssi;
  int ant_level;
  uint32 ber;
  int per;
  int snr;
  uint32 doppler_freq;
  tIsdbtTmcc tmcc_info;
} tIsdbtTunerInfo;


typedef struct
{
  void  (*isdbt_bb_power_on) (void);
  void  (*isdbt_bb_power_off) (void);
  int   (*isdbt_bb_init) (void);
  int   (*isdbt_bb_set_freq) (int);
  int   (*isdbt_bb_fast_search) (int);
  int   (*isdbt_bb_start_ts) (int);
  void  (*isdbt_bb_bus_deinit) (void);
  int   (*isdbt_bb_bus_init) (void);
  int   (*isdbt_bb_bus_set_mode) (int);
  void  (*isdbt_bb_deinit) (void);
  int   (*isdbt_bb_init_core) (void);
  void  (*isdbt_bb_get_status) (tIsdbtSigInfo *);
  void  (*isdbt_bb_get_tuner_info) (tIsdbtTunerInfo *);
  void  (*isdbt_bb_get_status_slave) (void);
  void  (*isdbt_bb_test) (int);
} isdbt_bb_function_table_type;


/*====================================================================
FUNCTION       tdmb_bb_func_tbl_init  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean isdbt_bb_func_tbl_init(void);

/*====================================================================
FUNCTION       isdbt_bb_index2freq
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
u32 isdbt_bb_index2freq(int index);

/*====================================================================
FUNCTION       isdbt_bb_power_on
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void isdbt_bb_power_on(void);


/*====================================================================
FUNCTION       isdbt_bb_power_off
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void isdbt_bb_power_off(void);

/*====================================================================
FUNCTION       isdbt_bb_init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int isdbt_bb_init(void);

/*====================================================================
FUNCTION       isdbt_bb_set_freq
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int isdbt_bb_set_freq(int freq);

/*====================================================================
FUNCTION       isdbt_bb_fast_search
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int isdbt_bb_fast_search(int freq);

/*====================================================================
FUNCTION       isdbt_baseband_start_ts
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int isdbt_bb_start_ts(int enable);

/*====================================================================
FUNCTION       isdbt_bb_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void isdbt_bb_test(int index);


/*====================================================================
FUNCTION       isdbt_bb_bus_deinit
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void isdbt_bb_bus_deinit(void);


/*====================================================================
FUNCTION       isdbt_bb_bus_init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int isdbt_bb_bus_init(void);


/*====================================================================
FUNCTION       isdbt_bb_bus_set_mode
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int isdbt_bb_bus_set_mode(int en);


/*====================================================================
FUNCTION       isdbt_bb_deinit
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void isdbt_bb_deinit(void);

/*====================================================================
FUNCTION       isdbt_bb_init_core
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int isdbt_bb_init_core(void);


/*====================================================================
FUNCTION       isdbt_bb_get_status
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void isdbt_bb_get_status(tIsdbtSigInfo *sig_info);


/*====================================================================
FUNCTION       isdbt_bb_get_status
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void isdbt_bb_get_tuner_info(tIsdbtTunerInfo *tuner_info);


/*====================================================================
FUNCTION       sdbt_bb_get_status_slave
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
#ifdef ISDBT_DIVERSITY
void sdbt_bb_get_status_slave(void);
#endif

#endif /* _ISDBT_BB_H_ */
