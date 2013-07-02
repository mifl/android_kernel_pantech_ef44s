//=============================================================================
// File       : isdbt_bb.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2011/09/29       yschoi         Create
//=============================================================================

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <mach/gpio.h>
#include <asm/irq.h>

#include "isdbt_comdef.h"
#include "isdbt_dev.h"
#include "isdbt_bb.h"
#include "isdbt_test.h"
#include "isdbt_chip.h"


/*================================================================== */
/*================      ISDB-T BB Definition       ================= */
/*================================================================== */

#if defined(FEATURE_ISDBT_USE_SHARP)
#define ISDBT_BB_DRIVE_INIT(x) \
      isdbt_bb_sharp_init(x); \
      ISDBT_MSG_BB("ISDBT BB ---> [SHARP]\n");
//#elif defined(FEATURE_ISDBT_USE_FCI_FC8050)
#else
#error code "no tdmb baseband"
#endif

isdbt_bb_function_table_type isdbt_bb_function_table;
static boolean isdbt_bb_initialized = FALSE;


/*================================================================== */
/*======================   ISDB-T BB Function    =================== */
/*================================================================== */

static void isdbt_baseband_power_on(void);
static void isdbt_baseband_power_off(void);
static int isdbt_baseband_init(void);
static int isdbt_baseband_set_freq(int freq);
static int isdbt_baseband_fast_search(int freq);
static int isdbt_baseband_start_ts(int enable);
static void isdbt_baseband_bus_deinit(void);
static int isdbt_baseband_bus_init(void);
static void isdbt_baseband_deinit(void);
static int isdbt_baseband_init_core(void);
static void isdbt_baseband_get_status(tIsdbtSigInfo *ber_info);
static void isdbt_baseband_get_tuner_info(tIsdbtTunerInfo *tuner_info);
static void isdbt_baseband_get_status_slave(void);
static void isdbt_baseband_test(int index);


/*====================================================================
FUNCTION       tdmb_bb_func_tbl_init  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean isdbt_bb_func_tbl_init(void)
{
  ISDBT_MSG_BB("[%s]~!!!\n", __func__);

  if(isdbt_bb_initialized)
    return TRUE;    

  isdbt_bb_function_table.isdbt_bb_power_on         = isdbt_baseband_power_on;
  isdbt_bb_function_table.isdbt_bb_power_off        = isdbt_baseband_power_off;
  isdbt_bb_function_table.isdbt_bb_init             = isdbt_baseband_init;
  isdbt_bb_function_table.isdbt_bb_set_freq         = isdbt_baseband_set_freq;
  isdbt_bb_function_table.isdbt_bb_fast_search      = isdbt_baseband_fast_search;
  isdbt_bb_function_table.isdbt_bb_get_status       = isdbt_baseband_get_status;
  isdbt_bb_function_table.isdbt_bb_get_tuner_info   = isdbt_baseband_get_tuner_info;
  isdbt_bb_function_table.isdbt_bb_start_ts         = isdbt_baseband_start_ts;
  isdbt_bb_function_table.isdbt_bb_bus_deinit       = isdbt_baseband_bus_deinit;
  isdbt_bb_function_table.isdbt_bb_bus_init         = isdbt_baseband_bus_init;
  isdbt_bb_function_table.isdbt_bb_deinit           = isdbt_baseband_deinit;
  isdbt_bb_function_table.isdbt_bb_init_core        = isdbt_baseband_init_core;
  isdbt_bb_function_table.isdbt_bb_get_status_slave = isdbt_baseband_get_status_slave;
  isdbt_bb_function_table.isdbt_bb_test             = isdbt_baseband_test;

  isdbt_bb_initialized = ISDBT_BB_DRIVE_INIT(&isdbt_bb_function_table);

  return isdbt_bb_initialized;
}


/*====================================================================
FUNCTION       isdbt_bb_index2freq
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
u32 isdbt_bb_index2freq(int index)
{
  u32 freq = 0;

  //ISDBT_MSG_BB("[%s]~!!!\n", __func__);

  freq = (473143 + 6000 * (index - 13))* 1000;

  return freq;
}


/*====================================================================
FUNCTION       isdbt_bb_power_on
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void isdbt_bb_power_on(void)
{
  ISDBT_MSG_BB("[%s]~!!!\n", __func__);

  if(!isdbt_bb_initialized)
  {
    if(isdbt_bb_func_tbl_init() == FALSE)
      return;
  }

  isdbt_bb_function_table.isdbt_bb_power_on();
}


/*====================================================================
FUNCTION       isdbt_bb_power_off
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void isdbt_bb_power_off(void)
{
  ISDBT_MSG_BB("[%s]~!!!\n", __func__);

  isdbt_bb_function_table.isdbt_bb_power_off();
}


/*====================================================================
FUNCTION       isdbt_bb_init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int isdbt_bb_init(void)
{
  int ret = -1;

  //ISDBT_MSG_BB("[%s]~!!!\n", __func__);

  ret = isdbt_bb_function_table.isdbt_bb_init();

  return ret;
}


/*====================================================================
FUNCTION       isdbt_bb_set_freq
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int isdbt_bb_set_freq(int freq)
{
  int ret = -1;

  ISDBT_MSG_BB("[%s]~!!!freq[%d]\n", __func__, freq);

  ret = isdbt_bb_function_table.isdbt_bb_set_freq(freq);

  return ret;
}


/*====================================================================
FUNCTION       isdbt_bb_fast_search
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int isdbt_bb_fast_search(int freq)
{
  int ret = -1;

  //ISDBT_MSG_BB("[%s]~!!!\n", __func__);

  ret = isdbt_bb_function_table.isdbt_bb_fast_search(freq);

  return ret;
}


/*====================================================================
FUNCTION       isdbt_bb_get_status
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void isdbt_bb_get_status(tIsdbtSigInfo *sig_info)
{
  //ISDBT_MSG_BB("[%s]~!!!\n", __func__);

  isdbt_bb_function_table.isdbt_bb_get_status(sig_info);

  return;
}


/*====================================================================
FUNCTION       isdbt_bb_get_tuner_info
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void isdbt_bb_get_tuner_info(tIsdbtTunerInfo *tuner_info)
{
  //ISDBT_MSG_BB("[%s]~!!!\n", __func__);

  isdbt_bb_function_table.isdbt_bb_get_tuner_info(tuner_info);

  return;
}


/*====================================================================
FUNCTION       isdbt_bb_get_status
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int isdbt_bb_start_ts(int enable)
{
  int ret=0;

  //ISDBT_MSG_BB("[%s]~!!! enable[%d]\n", __func__,enable);

  isdbt_bb_function_table.isdbt_bb_start_ts(enable);

  return ret;
}


/*====================================================================
FUNCTION       isdbt_bb_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void isdbt_bb_test(int index)
{
  ISDBT_MSG_BB("[%s]~!!!\n", __func__);

  isdbt_bb_function_table.isdbt_bb_test(index);
}


/*====================================================================
FUNCTION       isdbt_bb_bus_deinit
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void isdbt_bb_bus_deinit(void)
{
  ISDBT_MSG_BB("[%s]~!!!\n", __func__);

  isdbt_bb_function_table.isdbt_bb_bus_deinit();

  return;
}


/*====================================================================
FUNCTION       isdbt_bb_bus_init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int isdbt_bb_bus_init(void)
{
  int ret = -1;

  ISDBT_MSG_BB("[%s]~!!!\n", __func__);

  isdbt_bb_function_table.isdbt_bb_bus_init();

  return ret;
}


/*====================================================================
FUNCTION       isdbt_bb_bus_set_mode
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int isdbt_bb_bus_set_mode(int en)
{
  int ret = -1;

  ISDBT_MSG_BB("[%s]~!!!\n", __func__);

  ret = isdbt_bb_function_table.isdbt_bb_bus_set_mode(en);

  return ret;
}


/*====================================================================
FUNCTION       isdbt_bb_deinit
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void isdbt_bb_deinit(void)
{
  ISDBT_MSG_BB("[%s]~!!!\n", __func__);

  isdbt_bb_function_table.isdbt_bb_deinit();

  return;
}


/*====================================================================
FUNCTION       isdbt_bb_init_core
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int isdbt_bb_init_core(void)
{
  int ret = -1;

  ISDBT_MSG_BB("[%s]~!!!\n", __func__);

  ret = isdbt_bb_function_table.isdbt_bb_init_core();

  return ret;
}

/*====================================================================
FUNCTION       sdbt_bb_get_status_slave
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
#ifdef ISDBT_DIVERSITY
void sdbt_bb_get_status_slave(void)
{
  ISDBT_MSG_BB("[%s]~!!!\n", __func__);

  return;
}
#endif


/*====================================================================
FUNCTION       isdbt_baseband_power_on
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void isdbt_baseband_power_on(void)
{
  return;
}


/*====================================================================
FUNCTION       isdbt_baseband_power_off
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void isdbt_baseband_power_off(void)
{
  return;
}


/*====================================================================
FUNCTION       isdbt_baseband_init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int isdbt_baseband_init(void)
{
  int ret = 0;

  return ret;
}


/*====================================================================
FUNCTION       isdbt_baseband_set_freq
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int isdbt_baseband_set_freq(int freq)
{
  int ret = 0;

  return ret;
}


/*====================================================================
FUNCTION       isdbt_baseband_fast_search
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int isdbt_baseband_fast_search(int freq)
{
  int ret = 0;

  return ret;
}


/*====================================================================
FUNCTION       isdbt_baseband_bus_deinit
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void isdbt_baseband_bus_deinit(void)
{
  return;
}


/*====================================================================
FUNCTION       isdbt_baseband_bus_init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int isdbt_baseband_bus_init(void)
{
  int ret = 0;

  return ret;
}


/*====================================================================
FUNCTION       isdbt_baseband_deinit
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void isdbt_baseband_deinit(void)
{
  return;
}


/*====================================================================
FUNCTION       isdbt_baseband_init_core
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int isdbt_baseband_init_core(void)
{
  int ret = 0;

  return ret;
}


/*====================================================================
FUNCTION       isdbt_baseband_get_status
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void isdbt_baseband_get_status(tIsdbtSigInfo *ber_info)
{
  return;
}


/*====================================================================
FUNCTION       isdbt_baseband_get_tuner_info
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void isdbt_baseband_get_tuner_info(tIsdbtTunerInfo *tuner_info)
{
  return;
}


/*====================================================================
FUNCTION       isdbt_baseband_start_ts
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int isdbt_baseband_start_ts(int enable)
{
  return 0;
}


/*====================================================================
FUNCTION       isdbt_baseband_get_status_slave
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void isdbt_baseband_get_status_slave(void)
{
  return;
}


/*====================================================================
FUNCTION       isdbt_baseband_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void isdbt_baseband_test(int index)
{
  return;
}

