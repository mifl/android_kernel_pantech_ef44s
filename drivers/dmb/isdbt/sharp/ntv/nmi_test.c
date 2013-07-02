//=============================================================================
// File       : sharp_bb.c
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

#include "../../../dmb_interface.h"

#include "../../isdbt_comdef.h"
#include "../../isdbt_bb.h"
#include "../../isdbt_test.h"

#include "../sharp_bb.h"

//#include "nmitypes.h"
//#include "nmiisdbtcmn.h"
//#include "nmibus.h"
//#include "nmiisdbt.h"
#include "inc/nmitv.h"
#include "inc/nmintv.h"
#include "inc/nmiioctl.h"
#include "inc/nmintvoem.h"


#define __single__       1
#define __diversity__   2
#define __pip__           3
#define NMI326_OP_MODE  __single__

#define DUMP_TCC_ANDROID_TSIF     1
#define DUMP_SEC_P1_ANDROID_TSIF  2
#define DUMP_FIH_ANDROID_TSIF     3
#define TARGET_PLATFORM_TYPE    DUMP_TCC_ANDROID_TSIF

#if 0
#define DUMP_SQ_INFO

const int megaByte = 1024 * 1024;

#define TS_PACKET_COUNT  16
#define TS_BUFFER_SIZE   188 * TS_PACKET_COUNT	

static int dump_size;
char* tsfilepath, *sqfilepath;
FILE *tsfp;
FILE *sqfp;
#endif

static tNmDrvInit nmDrvInit;
static tNmDrvInit *pNmDrvInit = NULL;

tOem __oem;

enum _state {NMI_READY, NMI_RUNNING, NMI_DESTROY};
typedef enum _state tNmDrvState;

tNmDrvState __state;


static void __oem_init(void);
static int __nmi326_init(void);
static int __nmi326_tune(int nfreq);
static int __nmi326_stream_start(int nfreq);
  

/*================================================================== */
/*=================       SHARP BB Test Func.      ================= */
/*================================================================== */

/*====================================================================
FUNCTION       nmi_test
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void nmi_test(void)
{
  int nfreq = 0;

  ISDBT_MSG_TEST("[%s] start!!1\n", __func__);

  nfreq = ISDBT_TEST_CH;

  __oem_init();

  if(__nmi326_init() < 0)
  {
    ISDBT_MSG_TEST("__nmi326_init() fail\n");
    return;
  }

    /* tune the frequency */
  if(__nmi326_tune(nfreq) == 0)
  {
    ISDBT_MSG_TEST("tune success\n");

    /* start the ts streamming */
    __nmi326_stream_start(nfreq);
  }

  ISDBT_MSG_TEST("[%s] end!!1\n", __func__);
}


static void __oem_init(void)
{
  __oem.pwrup = sharp_power_on;
  __oem.pwrdn = sharp_power_off;

#if 0
  __oem.rstdnatv = ;
  __oem.rstdndtv = ;
  __oem.rstupatv = ;
  __oem.rstupdtv = ;
  __oem.prnt = ;

  __oem.bus.init = ;
  __oem.bus.deinit = ;
#endif
  __oem.bus.i2cw = sharp_i2c_write_data;
  __oem.bus.i2cr = sharp_i2c_read_data;
#if 0
  __oem.bus.spiw = ;
  __oem.bus.spir = ;
  __oem.bus.burstr = ;
  __oem.bus.spiw = ;

  __oem.intr.init = ;
  __oem.intr.deinit = ;
  __oem.intr.enable = ;
  __oem.intr.disable = ;
  __oem.intr.done = ;
#endif
  //__oem.os._malloc = ;
  //__oem.os._mfree = ;
  //__oem.os._mset = ;
  //__oem.os._mcpy = ;
  //__oem.os.screate = ;
  //__oem.os.sget = ;
  //__oem.os.srelease = ;
  //__oem.os.sdelete = ;
  //__oem.os.gettick = ;

  __oem.os.delay = msleep;
}


static int __nmi326_init(void)
{
  int ret = 0;
  unsigned nChipId = 0;

  /**
  initialzie the nmi chipset driver 
  **/
  pNmDrvInit = &nmDrvInit;
  memset(pNmDrvInit, 0, sizeof(tNmDrvInit));

  /**
  1) control path: I2C, data path: TSIF
  2) control path: SPI, data path: SPI
  **/
  pNmDrvInit->isdbt.bustype = nI2C; /* control path is I2C. */
  pNmDrvInit->isdbt.transporttype = nTS; /* data path is TSIF. */

  if(pNmDrvInit->isdbt.transporttype == nSPI) {
  /* isdbt.blocksize */
  pNmDrvInit->isdbt.blocksize = 30; /* if the data path is the SPI, blocksize is limitted to 40. 
  Otherwise, for example, in case of the TSIF the blocksize should be one. */
  }else {
  /* isdbt.blocksize */
  pNmDrvInit->isdbt.blocksize = 1;
  }

  pNmDrvInit->isdbt.hbus = 0;
  pNmDrvInit->isdbt.pfdtvsqcb = NULL; /* signal information callback. if the data path is TSIF, sq callback not required. the host can acquire
  the sq information through calling NTV()->get_sq(&isd, nNORMAL). */
  pNmDrvInit->isdbt.pfdtvtscb = NULL; /* ts data callback. if the data path is TSIF, ts data callback is not required. The host can get the data 
  through the TSIF DMA. */
  pNmDrvInit->isdbt.sqcallinterval = 2000; /* reading the signal information is done periodically with every sqcallinterval value in msec.
  it defaults 2000 msec. */

  /* isdbt.core */
  pNmDrvInit->isdbt.core.bustype = nI2C;
  pNmDrvInit->isdbt.core.transporttype = nTS;
  pNmDrvInit->isdbt.core.dco = 1; 
  pNmDrvInit->isdbt.core.ldoby = 0; /* Set to 1 if the IO voltage is less than 2.2v */
  pNmDrvInit->isdbt.core.no_ts_err_chk = 0; /* Enable TS error indicator bit modification. 
  If set, TS error indicator bit (bit 7 of TSP byte 2) is modified 
  based on the ts_erind_mode value. */
  pNmDrvInit->isdbt.core.spiworkaround = 0;
#if (NMI326_OP_MODE == __diversity__)
  ISDBT_MSG_TEST("NMI326_OP_MODE == __diversity__ !!!\n" );
  pNmDrvInit->isdbt.core.op = nDiversity;
#elif (NMI326_OP_MODE == __pip__)
  ISDBT_MSG_TEST("NMI326_OP_MODE == __pip__ !!!\n" );
  pNmDrvInit->isdbt.core.op = nPip;
#else
  ISDBT_MSG_TEST("NMI326_OP_MODE == __single__ !!!\n" );
  pNmDrvInit->isdbt.core.op = nSingle;
#endif
#ifndef FIX_POINT
  pNmDrvInit->isdbt.core.xo = 32.;
#endif

  /* isdbt.ai2c */
  pNmDrvInit->isdbt.ai2c.adrM = 0x61; /* master i2c address, it's dependent on the hardware configuration. */
#if (NMI326_OP_MODE != __single__)
  pNmDrvInit->isdbt.ai2c.adrS = 0x60; /* slave i2c address, it's dependent on the hardware configuration. */
#endif

  /* isdbt.tso */
  pNmDrvInit->isdbt.tso.tstype = nSerial; /* TS output mode (0=serial, 1=parallel). */
  pNmDrvInit->isdbt.tso.clkrate = nClk_s_2; /* TS output clock rate in serial mode (0=8MHz, 1=4MHz, 2=2MHz, 3=1MHz, 4=0.5MHz); 
  in parallel mode clock rate is 8 times slower. 
  Clock rate should be set to 2M when null_en=1 
  to enable constant data rate output.  */
  pNmDrvInit->isdbt.tso.blocksize = pNmDrvInit->isdbt.blocksize;
  pNmDrvInit->isdbt.tso.clk204 = 0;	/* TS clock output toggles during parity bytes. 
  When set, TS clock is active for all 204 TSP bytes (data + parity), 
  otherwise it is active for the first 188 TSP bytes (data only). 
  This bit has no effect when parity_en=0 or when gated_clk=0. */
  pNmDrvInit->isdbt.tso.clkedge = 0;	/* TS clock edge (0=rising, 1=falling). 
  Selects active clock edge in the middle of the data. 
  Selects active clock edge in the middle of the data. */

  pNmDrvInit->isdbt.tso.datapol = 0; /* TS data output polarity (0=non-inverted, 1=inverted). */
  pNmDrvInit->isdbt.tso.dmaen = 0; /* Enable DMA output, it should be zero. */
  pNmDrvInit->isdbt.tso.dropbadts = 0; /* Suppress TSP output when the error indicator bit is set (i.e. the packets that failed RS decoding) */
  pNmDrvInit->isdbt.tso.errorpol = 0; /* TS error output polarity (0=active high, 1=active low). */
  pNmDrvInit->isdbt.tso.gatedclk = 0; /* Gate off TS clock when TSP output is idle (no TSP data is sent). 
  When not set TS clock is always running provided that tso_en = 1. 
  Also see the clock_204 control bit description. if non-zero, no clock generated when TSP output is idle. */
  pNmDrvInit->isdbt.tso.lsbfirst = 0; /* Reverse bit order within a byte for serial TS output mode. 
  Default order is MSB first, setting this bit sends LSB first.  */

  pNmDrvInit->isdbt.tso.mode = pNmDrvInit->isdbt.core.op; /* nDiversity, nPiP, or nSingle */
  pNmDrvInit->isdbt.tso.nullen = 0; /* Enable null packet insertion in TS remux to sustain constant data rate */
  pNmDrvInit->isdbt.tso.nullsuppress = 0; /* Suppress outputting of null TS packets. */
  pNmDrvInit->isdbt.tso.parityen = 0; /* Enable sending 16 parity bytes at the end of each TSP. 
  If this bit is set, TSP bytes 189-204 corresponding to the RS parity bytes are 
  streamed out, otherwise the TSP size is 188 data bytes, and the next 16 
  or more bytes are don't care. */
  pNmDrvInit->isdbt.tso.syncpol = 0; /* TS sync output polarity (0=active high, 1=active low). */
  pNmDrvInit->isdbt.tso.valid204 = 0; /* TS valid output high during parity bytes. 
  When set, TS valid is high for all 204 TSP bytes (data + parity), 
  otherwise it is high for the first 188 TSP bytes (data only). 
  This bit has no effect when parity_en=0. */

  pNmDrvInit->isdbt.tso.validpol = 0; /* TS valid output polatiry (0=active high, 1=active low).  */

  ret = ntv_common_init(pNmDrvInit, &__oem);
  if(ret != NTV_SUCCESS) {
  ISDBT_MSG_TEST("fail to init, ntv_common_init != NTV_SUCCESS !!!\n" );
  return -1;
  }

  ret = NTV()->set_tv_mode(_ISDBT_);
  if(ret != NTV_SUCCESS) {
  ISDBT_MSG_TEST("fail to init, NTV()->set_tv_mode != NTV_SUCCESS !!!\n" );
  return -1;
  }

  /* optional, the interface can be verified. */
  ret = NTV()->probe(&nChipId);
  if(ret != NTV_SUCCESS) {
  ISDBT_MSG_TEST("fail to init, NTV()->probe != NTV_SUCCESS !!!\n" );
  return -1;
  }

  ret = NTV()->init();
  if(ret != NTV_SUCCESS) {
  ISDBT_MSG_TEST("fail to init, NTV()->init != NTV_SUCCESS !!!\n" );
  return -1;
  }

#if (NMI326_OP_MODE == __diversity__)
  ret = NTV()->dsm(1); /* DSM (Diversity State Machine) on */
  if(ret != NTV_SUCCESS) {
  ISDBT_MSG_TEST("NTV()->dsm / FAIL\n" );
  return -1;
  }
#endif

  ISDBT_MSG_TEST("nmi initialization success\n" );

  return 0;
}

static int __nmi326_tune(int nfreq)
{
	int ret = 0;
	tIsdbtRun tune;
	NTV_ASSERT(nfreq != 0);
	tune.frequency = nfreq;
	tune.highside = 0;
	
	ISDBT_MSG_TEST("__nmi326_tune(), freq=[%d]\n", tune.frequency);
	ret = NTV()->tune((void*)&tune);	
#if (NMI326_OP_MODE == __pip__)
	tune.frequency = 479143000;
	tune.highside = 0;
	ISDBT_MSG_TEST("____________PiP mode, slave freq=[%d]\n", tune.frequency);
	ret = NTV()->tune_slave(&tune);	
#endif

	return ret;	/* if zero, tune success, otherwise if not, tune fails. */
}

static int __nmi326_stream_start(int nfreq)
{
  int ret = 0;

  ret = NTV()->start();
  __state = NMI_RUNNING;
  return ret; 
}

