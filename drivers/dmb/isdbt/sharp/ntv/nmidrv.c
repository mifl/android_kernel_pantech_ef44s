/******************************************************************************
**
**	Copyright (c) Newport Media Inc.  All rights reserved.
**
** 	Module Name:  nmidrv.c
**	
**		This module implements the necessary functions for driving the NMI ATV 600 Chipset.
**		It can be used as an example for the driver development.      
**
** 
*******************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>

#include "../../isdbt_bb.h"
#include "../sharp_bb.h"

#include "../chipset/inc/nmiisdbtcmn.h"
#include "inc/nmintvplatform.h"
#include "inc/nmiioctl.h"
#include "inc/nmidrv.h"



/******************************************************************************
**
**	Driver Global Variables
**
*******************************************************************************/

#ifdef CONFIG_SKY_ISDBT
#define FEATURE_SKY_ISDBT
#endif


static int already_init = 0;

typedef struct {
	void 							*ctl;
} tBusIo;

typedef struct {
	int								aber;
	int 							aber2;
	int								aper;
	int	 							aper2;
	int								abbv;
	int								npol;
} tNmiSq;


typedef struct {
	uint32_t					chipid;
	tNmiIsdbtVtbl 		vf;
	tNmiSq						sq;
} tNmiIsdbt;


typedef struct {
	tBusIo						bus;
	tNmiBus 					bustype;		
	tNmiBus 					dtvbustype;					/* dtv bus type */

	tNmiIsdbt					dtv;
	int	 							aSnr;						/* accumulated snr from tony cho, 2010-06-09 */

	/**
		current frequency
	**/
	unsigned int 			mfrequency;					/* master locked frequency in Hz */
	unsigned int 			sfrequency;					/* slave locked frequency in Hz */

	int 							dlock;						/* isdbt lock */
	tIsdbtTmccInfo 		tmcc;						/* tmcc info */
	tI2CAdr 					ai2c;						/* i2c address */
	tNmiIsdbtOpMode		opmode;						/* diversity, single ot pip */
	int								dcopt;						/* isdbt decoder option */
	uint32_t					isdbtchipid;				/* master chip id */
	int								firstscan;					/* isdbt first scan flag */
	int								decoder_initialized;		/* isdbt decorder initialzed flag */
	tNmiSq						sq[2];
} tNmiDrv;

static tNmiDrv  drv;

static tOem oem;
tOem *poem = &oem;
static tNtv _ntv;
tNtv *ntv = &_ntv;



/******************************************************************************
**
**	ASIC Helper Functions
**
*******************************************************************************/

static void nmi_log(char *str)
{
#ifdef FEATURE_SKY_ISDBT
    ISDBT_MSG_SHARP_BB("%s", str);
#else
	if(ISVALID(poem->prnt))
		poem->prnt(str);
#endif
}

static void nmi_delay(uint32_t msec)
{
	if(ISVALID(poem->os.delay))
		poem->os.delay(msec);
}

static uint32_t nmi_get_tick(void)
{
	uint32_t __time = 0;
	if(ISVALID(poem->os.gettick))
		__time = poem->os.gettick();
    return __time;
}

/******************************************************************************
**
**	Driver Debug Defines
**
*******************************************************************************/

uint32_t dflag = N_ERR | N_INFO;

static void dPrint(unsigned long flag, char *fmt, ...)
{
	char buf[256];
	va_list args;

	if (flag & dflag) { 
		va_start(args, fmt);
		vsprintf(buf, fmt, args);
		va_end(args);
		nmi_log(buf);	
	}

	return;
}

/******************************************************************************
**
**	Bus Functions
**
*******************************************************************************/
static int nmi_bus_write(tNmiIsdbtChip index, uint8_t *b, int len)
{
	tNmiDrv *pd = &drv;

	if(pd->bustype == nI2C) {
		uint8_t devaddr = (index == nMaster) ? (pd->ai2c.adrM) : (pd->ai2c.adrS);
		poem->bus.i2cw(devaddr, b, (unsigned long)len);
	}else if(pd->bustype == nSPI) {
		poem->bus.spiw(b, (unsigned long)len);
	}
	
	return 0;
}

static int nmi_bus_read(tNmiIsdbtChip index, uint8_t *wb, int wsz, uint8_t *rb, int rsz)
{
	tNmiDrv *pd = &drv;

	if(pd->bustype == nI2C) {
		uint8_t devaddr = (index == nMaster) ? (pd->ai2c.adrM) : (pd->ai2c.adrS);
#ifdef FEATURE_DMB_SHARP_I2C_READ
		poem->bus.i2cr(devaddr, wb, (unsigned long)wsz, rb, (unsigned long)rsz);
#else
		poem->bus.i2cw(devaddr, wb, (unsigned long)wsz);
		poem->bus.i2cr(devaddr, rb, (unsigned long)rsz);
#endif
	} else if(pd->bustype == nSPI) {
		poem->bus.spir(rb, (unsigned long)rsz);
	}
	
	return 0;
}

static int nmi_bus_t_write(tNmiIsdbtChip index, uint8_t *b, int len)
{
	poem->bus.spiw(b, (unsigned long)len);
	return 0;
}

static int nmi_bus_t_read(tNmiIsdbtChip index, uint8_t *wb, int wsz, uint8_t *rb, int rsz)
{
	poem->bus.spir(rb, (unsigned long)rsz);
	return 0;
}

static int nmi_bus_t_burstread(int count, int isOverFlow)
{
	unsigned int pos = ntv->tsdmasize;

	dPrint(N_INTR, "nmi_bus_t_burstread, pos (%d)\n", pos);
	
	if(ntv->ptsbuf != NULL){
		dPrint(DF_INTR, "nmi_bus_t_burstread, count (%d)\n", count);
		poem->bus.burstr(ntv->ptsbuf + pos, (unsigned long)count);
		if(isOverFlow == 0)	/* if overflow not happened, move the position. */
		ntv->tsdmasize += count;	/* accumulate the received size. */
		dPrint(DF_INTR, "nmi_bus_t_burstread, ntv->tsdmasize (%d)\n", ntv->tsdmasize);
	}

	return 0;
}


/******************************************************************************
**
**	Driver Atv Functions
**
*******************************************************************************/

static void nmi_drv_deinit(void)
{
	already_init = 0;
	return;
}

int nmi_drv_init(void *pv)
{
	tNmiDrv *pd = &drv;
	int result	= NMI_S_OK;
	tNmiIsdbtIn inp1;
	tNmDrvInit *p = (tNmDrvInit *)pv;

	if (!already_init) {
		memset((void *)pd, 0, sizeof(tNmiDrv));

		pd->dtvbustype = p->isdbt.bustype;						/* dtv bus type */
		memcpy(&pd->ai2c, &p->isdbt.ai2c, sizeof(tI2CAdr));		/* isdbt address for master and slave */

		dPrint(N_INFO, "nmi_drv_init, ISDBT INITIALIZING.......................\n");
		/**
			Isdbt Asic Driver init
		**/
		memcpy(&inp1, &p->isdbt.core, sizeof(tNmiIsdbtIn));

		pd->bustype = pd->dtvbustype;
		
		pd->opmode = inp1.op;		/* save the opmode: nDiversity, nSingle or nPip */
		pd->dcopt = inp1.dco;

		inp1.zone					= N_ERR | N_INFO;
		inp1.hlp.write		= nmi_bus_write;
		inp1.hlp.read			= nmi_bus_read;
		inp1.hlp.t.write	= nmi_bus_t_write;
		inp1.hlp.t.read		= nmi_bus_t_read;
		inp1.hlp.t.burstread	= nmi_bus_t_burstread;
		inp1.hlp.delay		= nmi_delay;
		inp1.hlp.gettick	= nmi_get_tick;
		inp1.hlp.log			= nmi_log;

#ifndef FIX_POINT
		dPrint(N_INFO, "nmi_drv_init, ISDBT op mode=[%d], dco=[%d], xo=[%d], ldoby=[%d]\n", pd->opmode, inp1.dco, (int)inp1.xo, inp1.ldoby);
#else
		dPrint(N_INFO, "nmi_drv_init, ISDBT op mode=[%d], dco=[%d], ldoby=[%d]\n", pd->opmode, inp1.dco, inp1.ldoby);
#endif
		nmi_isdbt_common_init(&inp1, &pd->dtv.vf); 
		already_init = 1;
	}

	return result;
}

int nmi_drv_init_core(tNmDrvMode mode, tNmiIsdbtChip cix)
{
	tNmiDrv *pd = &drv;
	int result = NMI_S_OK;

	if(already_init) {

		/**
			initialize chip
		**/
		dPrint(N_INFO, "nmi_drv_init_core: isdbt, chip init, index=[%d]\n", cix); 	
		if(pd->dtv.vf.init(cix) <= 0) {
			dPrint(N_ERR, "nmi_drv_init_core: isdbt, fail to chip init, index=[%d]\n", cix); 	
			result = NMI_E_FAIL;
			goto _fail_;
		}

		/* probe the chip */
		if(pd->dtv.vf.probe(nMaster) <= 0) {
			dPrint(N_ERR, "nmi_drv_init_core: isdbt, fail to probe the master chip\n"); 	
			result = NMI_E_NO_MASTER;
			goto _fail_;
		}
		
		dPrint(N_INFO, "nmi_drv_init_core: isdbt, found the master chip\n"); 		
		if(pd->opmode != nSingle) {
			if(pd->dtv.vf.probe(nSlave) <= 0) {
				dPrint(N_ERR, "nmi_drv_init_core: isdbt, fail to probe the slave chip\n"); 	
				result = NMI_E_NO_SLAVE;
				goto _fail_;	
			}

			dPrint(N_INFO, "nmi_drv_init_core: isdbt, found the slave chip\n"); 		
		}
	}

_fail_:
	return result;
}

static uint32_t nmi_drv_get_chipid(tNmDrvMode mode, tNmiIsdbtChip index)
{
	tNmiDrv *pd = &drv;
	uint32_t chipid;

	chipid = pd->dtv.vf.getchipid(index);
	if(index == nMaster) 
		pd->isdbtchipid = chipid;

	return chipid;
}

static void nmi_drv_write_register(tNmDrvMode mode, tNmiIsdbtChip cix, unsigned char *b, unsigned long len, unsigned long adr)
{
	tNmiDrv *pd = &drv;

	if (already_init && b) {
		if (len == 1) {
			pd->dtv.vf.w8(cix, adr, *b);
		} else if (len == 4) {
			pd->dtv.vf.w32(cix, adr, *((unsigned long *)b));
		}
	}
}

static void nmi_drv_read_register(tNmDrvMode mode, tNmiIsdbtChip cix, unsigned char *b, unsigned long len, unsigned long adr)
{
	tNmiDrv *pd = &drv;

	if (already_init && b) {
		if (len == 1) {
			*b = pd->dtv.vf.r8(cix, adr);
		} else if (len == 4) {
			*((unsigned long *)b) = pd->dtv.vf.r32(cix, adr);
		}
	}
}

static void nmi_drv_read_rf_register(tNmDrvMode mode, tNmiIsdbtChip cix, unsigned char *b, unsigned long len, unsigned long adr)
{
	tNmiDrv *pd = &drv;

	if (already_init && b) {
		*b = pd->dtv.vf.rfr(cix, adr);
	}
}

static void nmi_drv_write_rf_register(tNmDrvMode mode, tNmiIsdbtChip cix, unsigned char *b, unsigned long len, unsigned long adr)
{
	tNmiDrv *pd = &drv;

	if (already_init && b) {
		pd->dtv.vf.rfw(cix, adr, *b);
	}
}

int nmi_drv_run(tNmDrvMode mode, tNmiIsdbtChip cix, void *pv)
{
	tNmiDrv *pd = &drv;
	int lock = 0;

	tIsdbtTune tune;
	tIsdbtRun *p = (tIsdbtRun *)pv;

	tune.frequency = p->frequency;
	tune.highside = p->highside;

	/**
		Tune to frequency
	**/
	pd->dtv.vf.tune(cix, (void *)&tune);

	/**
		Configure demod:
		opmode = DIVERSITY, PIP, or SINGLE
	**/
	pd->dtv.vf.demod(cix);

	/**
		Check locking
	**/
	lock = pd->dtv.vf.checklock(cix, 900);
	dPrint(N_INFO, "[nmi_drv_run]: isdbt, tune, lock (%d)\n", lock);
	
	if(lock) {
		tIsdbtTmccInfo tmccinfo;
		
		pd->dtv.vf.decoder(cix, 0, NULL);
		pd->dtv.vf.gettmccinfo(cix, &tmccinfo);
		memcpy((void *)&pd->tmcc, (void *)&tmccinfo, sizeof(tIsdbtTmccInfo));

		pd->decoder_initialized = 1;
	}else {
		pd->decoder_initialized = 0;
	}

	/**
		save the current frequency
	**/
	if(cix == nMaster)
		pd->mfrequency = p->frequency;
	else 
		pd->sfrequency = p->frequency;

	return lock;
}

static void nmi_drv_soft_reset(tNmDrvMode mode, tNmiIsdbtChip cix)
{
	tNmiDrv *pd = &drv;
	pd->dtv.vf.reset(cix);
}

void nmi_isdbt_get_status(tNmiIsdbtChip cix, tIsdbtSignalStatus *p)
{
	int tmcclock;
	tNmiDrv *pd = &drv;
	tIsdbtBer ber;
	//tIsdbtPer per;
	uint32_t nber; // cys
	//int nber;//, nper;
	tIsdbtGainInfo ginfo;
#ifndef FIX_POINT
	tIsdbtAcqTime acqtime;
#endif
	tIsdbtTmccInfo tmccinfo;
	uint32_t sq, coeff;
	int acq_mode = p->get_simple_mode;

	//dPrint(N_INFO, "nmi_isdbt_get_status: cix=[%d]\n", cix); 
	
	tmcclock = 0;
	pd->dtv.vf.chkpll(cix);		// it shoud be checked with system guys...
	tmcclock = pd->dtv.vf.tmcclock(cix);

	if (tmcclock) {
		p->lock 			= 1;
		if(!pd->decoder_initialized) {
			dPrint(N_INFO, "nmi_isdbt_get_status: pd->decoder_initialized != 1\n"); 
			pd->dtv.vf.decoder(cix, 0, NULL);
			pd->dtv.vf.gettmccinfo(cix,&tmccinfo);
			memcpy((void *)&pd->tmcc, (void *)&tmccinfo, sizeof(tIsdbtTmccInfo));
			pd->decoder_initialized = 1;
		} 
#if 1
		pd->dtv.vf.gettmccinfo(cix,&tmccinfo);
		if ((tmccinfo.mode != pd->tmcc.mode) ||
				(tmccinfo.guard != pd->tmcc.guard) ||
				(tmccinfo.coderate != pd->tmcc.coderate) ||
				(tmccinfo.modulation != pd->tmcc.modulation) ||
				(tmccinfo.interleaver != pd->tmcc.interleaver)) {
			dPrint(N_INFO, "nmi_isdbt_get_status: calling the decoder...\n"); 
			pd->dtv.vf.decoder(cix, 0, NULL);
		}
		p->mode				= tmccinfo.mode;
		p->guard			= tmccinfo.guard;
		p->modulation		= tmccinfo.modulation;
		p->interleaver		= tmccinfo.interleaver;
		p->fec				= tmccinfo.coderate;
		// [[ added by tony
		p->emerg_flag		= tmccinfo.emerg_flag;
		// ]]
#else	
		// nmi_drv_run() should have the backup of the tmcc.
		p->mode				= pd->tmcc.mode;
		p->guard			= pd->tmcc.guard;
		p->modulation		= pd->tmcc.modulation;
		p->interleaver		= pd->tmcc.interleaver;
		p->fec				= pd->tmcc.coderate;
		p->emerg_flag		= pd->tmcc.emerg_flag;
#endif
		if(acq_mode != __DEBUG_SIMPLE__) {
      p->doppler      = pd->dtv.vf.getdoppler(cix);
#ifndef FIX_POINT
			p->chanlen			= pd->dtv.vf.getchanlen(cix);		
#endif
		}
	} else {
		//dPrint(N_INFO, "nmi_isdbt_get_status, tmcclock = 0\n"); 
		p->lock = 0;
	}

	/* get snr */
	if ((cix == nMaster) && (pd->opmode == nDiversity)) {
#ifndef FIX_POINT
		p->dsnr = pd->dtv.vf.getsnr(cix, 1);	/* combiner */
#else
		p->nsnr = pd->dtv.vf.getsnr(cix, 1);
#endif

#if 0 // cys blocked
		p->dsmstate = pd->dtv.vf.dsmstate();
		p->dsmlock = pd->dtv.vf.dsmlock();
#endif
	} else {
#ifndef FIX_POINT
		p->dsnr = pd->dtv.vf.getsnr(cix, 0);
#else
		p->nsnr = pd->dtv.vf.getsnr(cix, 0);
#endif
	}
	
#ifndef FIX_POINT
	dPrint(N_VERB, "lock (%d), snr (%f)\n", p->lock, p->dsnr);
#else
	dPrint(N_VERB, "lock (%d), snr (%d)\n", p->lock, p->nsnr);
#endif

#if 0 // cys blocked
	if(acq_mode != __DEBUG_SIMPLE__) {
		/**
			get ber before viterbi 
		**/
		if (pd->dtv.vf.getberb(cix, (void *)&ber) > 0) {
			nber = ber.bec / ber.becw;
			p->dbbvinst = nber;
			pd->sq[cix].abbv = ((pd->sq[cix].abbv * pd->sq[cix].npol) + nber)/(pd->sq[cix].npol + 1);//pd->sq[cix].abbv = ((pd->sq[cix].abbv * pd->sq[cix].npol) + dber)/(pd->sq[cix].npol + 1); 
			p->dbbv = pd->sq[cix].abbv;
		} else {
			/**
				review: BER counter not turn on 
			**/
			p->dbbvinst = 1;
			p->dbbv = 1;
		}
	}
#endif

	/**
		get ber 
		ber should be less than 0.0002.
	**/
	if (tmcclock) {
		pd->dtv.vf.getber(cix, (void *)&ber);
    nber = ((ber.bec * 1000000) / ber.becw); // cys
		//nber = ber.bec/ber.becw;
		p->dberinst = nber;
	
		pd->sq[cix].aber = ((pd->sq[cix].aber * pd->sq[cix].npol) + nber)/(pd->sq[cix].npol + 1);
		p->dber = pd->sq[cix].aber;
	} else {
		p->dberinst = 9999;
	}

#ifdef FIX_POINT
	if(tmcclock) 
		p->dperinst = 0;
	else			
		p->dperinst = 1;
#else
	if(acq_mode != __DEBUG_SIMPLE__) {
		/**
			get per 
			per should be less than 0.0001.
		**/
		pd->dtv.vf.getper(cix, (void *)&per);
		nper = per.pec /per.pecw;
		p->dperinst = nper;
		pd->sq[cix].aper = ((pd->sq[cix].aper * pd->sq[cix].npol) + nper)/(pd->sq[cix].npol + 1);
		p->dper = pd->sq[cix].aper;	
		
		pd->sq[cix].npol++;
	}
#endif

	/**
		get rssi 
	**/
	pd->dtv.vf.getrssi(cix, (void *)&ginfo);
	p->dagc = ginfo.dagc;
	p->lnagain = (int)ginfo.bblnadb;
	p->lnagaincode = ginfo.bblnacode;
	p->lnamode = ginfo.rflna;
	p->rssi = ginfo.rssi;

	if(acq_mode != __DEBUG_SIMPLE__) {
		/**
			get offset 
		**/
#ifndef FIX_POINT
		p->freqoffset = pd->dtv.vf.getfreqoffset(cix);
		p->timeoffset = pd->dtv.vf.gettimeoffset(cix);
#endif
		
		/* get acquisation time */
		if(cix == nMaster) {
#ifndef FIX_POINT
			pd->dtv.vf.getacqtime(cix,&acqtime);

			p->agcacq		= acqtime.agcacq;
			p->syracq		= acqtime.syracq;
			p->ppmacq		= acqtime.ppmacq;
			p->tmccacq	= acqtime.tmccacq;
			p->tsacq			= acqtime.tsacq;
			p->totalacq		= acqtime.totalacq;
#endif
		}

#if 0 // cys blocked
		if (pd->opmode == nDiversity) {
			p->regad48 = pd->dtv.vf.r32(cix, 0xad48);
			p->regad4c = pd->dtv.vf.r32(cix, 0xad4c);
			p->regacc8 = pd->dtv.vf.r32(cix, 0xacc8);
		}
#endif
	}

	/**
		save the current frequency
	**/
	if(cix == nMaster)
		p->frequency = pd->mfrequency;
	else
		p->frequency = pd->sfrequency;

	if(p->rssi < -100)
		p->rssi = -100;

#ifndef FIX_POINT
	coeff = p->dsnr * 0.0625;
#else
	coeff = p->nsnr * 625;	//coeff = p->nsnr * 1;  [rachel]
#endif
	sq = 2 * (100 + p->rssi);
	sq *= coeff;
	
	if(p->lock == 0)
		p->sqindicator = 0;
#ifndef FIX_POINT
	else if(sq > 100)
		p->sqindicator = 100;
	else 
		p->sqindicator = (unsigned int)sq;
#else	// [[ rachel	
	else if(sq > (100*10000))
		p->sqindicator = 100;
	else
		p->sqindicator = (unsigned int)(sq/10000);
#endif	// ]] rachel

#if 0 //cys test
  ISDBT_MSG_SHARP_BB("snr[%d], rssi[%d], bec[%d], becw[%d], sqindi[%d], lnamode[%d], dagc[%d], lnagain[%d], lnagaincode[%d]\n",
                            p->nsnr, p->rssi, ber.bec, ber.becw, p->sqindicator, p->lnamode, p->dagc, p->lnagain, p->lnagaincode);
#endif

	return;
}

void nmi_drv_scan(tNmDrvMode mode, tNmiIsdbtChip cix, void *pv)
{
	tNmiDrv *pd = &drv;

	tIsdbtTune tune;
	tIsdbtScan *p = (tIsdbtScan *)pv;
	pd->mfrequency = tune.frequency = p->frequency;	// tony, 20111122, to save the current frequency
	tune.highside = 0;
	p->found = pd->dtv.vf.scanpoll((void *)&tune);
#ifndef FIX_POINT
	p->tim.tscan = pd->dtv.vf.scantime();
	dPrint(N_VERB, "scan time (%f)\n", p->tim.tscan);
#endif
	//dPrint(N_INFO, "Scan: freq(%d), found (%d)\n", p->frequency, p->found);

	if(p->found) {
		int lock;
		if(!pd->firstscan) {
			/**
				enable TS
			**/
			pd->dtv.vf.tsctl(1);	/* why ???, tony */

			/**
				need to run decoder for the first time
			**/
			lock = pd->dtv.vf.checklock(cix, 900);
			if (lock) {
				pd->dtv.vf.decoder(cix, 0, NULL);
				pd->firstscan = 1;
			} else {
				dPrint(N_ERR, "Error: find in scan but can't locked...\n");
				p->found = 0;
			}
		}

#if 0 // not essential
		if (p->found) {
			unsigned int val = 0, val1 = 0;
			uint32_t stim = nmi_get_tick();
			do {
				val = pd->dtv.vf.r32(nMaster, 0xa8f0);
				dPrint(N_VERB, "0xa8f0 (%08x)\n", val);
				if (val != 0) {
					if (val == val1) {
#ifndef FIX_POINT		// rachel - have to check SCAN TIME
						tIsdbtAcqTime acq;
		
						pd->dtv.vf.getacqtime(cix, (void *)&acq);
						p->tim.found = 1;
						p->tim.tsync = acq.totalacq - p->tim.tscan; 
#endif
						break;
					} else {
						val1 = val;
					}
				} else {
					val1 = val;
				}
			} while ((nmi_get_tick() - stim) <= 1010);
		}
#endif
	}
	//dPrint(N_INFO, "scan time end\n");
#if 0 // only for debugging
	/**
		get demod status
		tony, 20111122
	**/
	{
		tIsdbtSignalStatus signal;
		memset(&signal, 0, sizeof(tIsdbtSignalStatus));
		signal.get_simple_mode = 0;
		nmi_isdbt_get_status(nMaster, &signal);
		dPrint(N_VERB, "nmi_drv_scan, freq=[%d], lock=[%d], snr=[%d], rssi=[%d], lnamode=[%d]\n", 
				signal.frequency, signal.lock, signal.nsnr, signal.rssi, signal.lnamode);
	}
#endif
}

static void nmi_isdbt_switch_cmb(tNmiIsdbtChip cix, int enable)
{
	tNmiDrv *pd = &drv;

	pd->dtv.vf.combine(enable);
}


#ifndef FIX_POINT
static void nmi_isdbt_set_gain_thold(tNmiIsdbtChip cix, tIsdbtGainThold *inp)
{
	tNmiDrv *pd = &drv;
	tIsdbtAgcThold agcthold;

	agcthold.hi2mi_4	= inp->hi2mi_4;
	agcthold.mi2lo_4	= inp->mi2lo_4;
	agcthold.lo2by_4	= inp->lo2by_4;
	agcthold.by2lo_4	= inp->by2lo_4;
	agcthold.lo2mi_4	= inp->lo2mi_4;
	agcthold.mi2hi_4	= inp->mi2hi_4;

	agcthold.lo2by_b_4 = inp->lo2by_b_4;
	agcthold.mi2lo_b_4 = inp->mi2lo_b_4;
	agcthold.hi2lo_b_4 = inp->hi2lo_b_4;
	agcthold.hi2lo_b2_4 = inp->hi2lo_b2_4;

	pd->dtv.vf.setgainthold(cix, &agcthold);
}
#endif

void nmi_drv_video(tNmDrvMode mode, tNmDtvStream *p, int enable)
{
	tNmiDrv *pd = &drv;

	/* isdbt mode */
	if (enable) {
		tIsdbtTso tsconfig;

		memset((void*)&tsconfig, 0, sizeof(tIsdbtTso));
		if(p->transporttype == nTS) {
			memcpy(&tsconfig, &p->tso, sizeof(tIsdbtTso));
			tsconfig.mode = pd->opmode/*p->tso.mode*/; 
			//tsconfig.tstype = nSerial;
			tsconfig.blocksize = 1;
			//tsconfig.clkrate = nClk_s_05;
			
			dPrint(N_VERB, "nmi_drv_video: TS mode, mode=[%d], tstype=[%d], blocksize[%d], clkrate[%d], datapol[%d], validpol[%d], syncpol[%d], clkedge[%d], gatedclk[%d]\n", 
					tsconfig.mode, tsconfig.tstype, 
					tsconfig.blocksize, tsconfig.clkrate,
					tsconfig.datapol, tsconfig.validpol,
					tsconfig.syncpol, tsconfig.clkedge,
					tsconfig.gatedclk); 
			
			pd->dtv.vf.tscfg((void *)&tsconfig);
			
			dPrint(N_VERB, "nmi_drv_video: tsctl(1)\n"); 
			pd->dtv.vf.tsctl(1);
		}else if(p->transporttype == nSPI) {
			tIsdbtIrq irqconfig;

			memset((void*)&irqconfig, 0, sizeof(tIsdbtIrq));
			memcpy(&tsconfig, &p->tso, sizeof(tIsdbtTso));
			tsconfig.mode = pd->opmode/*p->tso.mode*/; 
			//tsconfig.blocksize = p->tso.blocksize;
			//tsconfig.dropbadts = p->tso.dropbadts;
			dPrint(N_VERB, "nmi_drv_video: spi mode, tscfg(opmode=[%d], blocksize=[%d]) --> enableirq --> spidma(1)\n", tsconfig.mode, tsconfig.blocksize); 
			pd->dtv.vf.tscfg((void *)&tsconfig);

			irqconfig.pllresync = p->irq.pllresync;
			irqconfig.dirq.u.dmaready = p->irq.dirq.u.dmaready;
			pd->dtv.vf.enableirq((void *)&irqconfig);

			pd->dtv.vf.spidma(1);
		}
	} else {
		if(p->transporttype == nTS) {
			dPrint(N_VERB, "nmi_drv_video: tsctl(0)\n"); 
			pd->dtv.vf.tsctl(0);
		}
		else if(p->transporttype == nSPI) {
			dPrint(N_VERB, "nmi_drv_video: spidma(0)\n"); 
			pd->dtv.vf.spidma(0);
		}
	}
}

static void nmi_drv_isdbt_rst_cnt(tNmiIsdbtChip cix)
{
	tNmiDrv *pd = &drv;
	
	pd->sq[cix].aber = 0;
	pd->sq[cix].aper = 0;
	pd->sq[cix].abbv = 0;
	pd->sq[cix].npol = 1;

	pd->sq[cix].aber2 = 0;
	pd->sq[cix].aper2 = 0;

}

static void nmi_drv_sleep(tNmDrvMode mode)
{
	tNmiDrv *pd = &drv;
	pd->dtv.vf.pwrdown();
	return;
}

static void nmi_drv_wakeup(tNmDrvMode mode)
{
	tNmiDrv *pd = &drv;
	pd->dtv.vf.pwrup(0);
	return;
}

static void nmi_drv_dtv_ts_test_pattern(int pattern)
{
	tNmiDrv *pd = &drv;
	dPrint(N_INFO, "nmi_drv_dtv_ts_test_pattern pattern (%d)\n", pattern);
	pd->dtv.vf.dtvtspattern(pattern);
}

static void nmi_drv_dtv_excute_isr(tNmDrvIsdbtIrq *p)
{
	tNmiDrv *pd = &drv;
	tIsdbtIrqStatus irq = {0};

	pd->dtv.vf.handleintr(&irq);

	memcpy(&p->irq, &irq, sizeof(tIsdbtIrqStatus));
}

void nmi_isdbt_dsm_on(int on)
{
	tNmiDrv *pd = &drv;
	pd->dtv.vf.dsm(on);
}

int nmi_drv_ctl(uint32_t code, tNmDrvMode mode, tNmiIsdbtChip cix, void *inp)
{
	tNmiDrv *pd = &drv;
	int result = 1;

	switch (code) {
	case NMI_DRV_ISR_PROCESS:
		nmi_drv_dtv_excute_isr((tNmDrvIsdbtIrq*)inp);
		break;
	case NMI_DRV_DTV_TS_TEST_PATTERN:
		{
			int pattern = *((int *)inp);
			nmi_drv_dtv_ts_test_pattern(pattern);
		}
		break;
		
	case NMI_DRV_SWITCH_COMBINER:
		nmi_isdbt_switch_cmb(cix,*((int *)inp));
		break;
	case NMI_DRV_DEEP_SLEEP:
		nmi_drv_sleep(mode);
		break;
	case NMI_DRV_WAKEUP:
		nmi_drv_wakeup(mode);
		break;
	case NMI_DRV_INIT:
		result = nmi_drv_init(inp);
		break;
	case NMI_DRV_DEINIT:
		nmi_drv_deinit();
		break;
	case NMI_DRV_INIT_CORE:
		result = nmi_drv_init_core(mode, cix);
		break;
	case NMI_DRV_GET_CHIPID:
		*((uint32_t *)inp) = nmi_drv_get_chipid(mode, cix);	
		break;
	case NMI_DRV_RUN:
		result = nmi_drv_run(mode, cix, inp);	/* result means lock or not */
		break;
	case NMI_DRV_RW_DIR:
		{
			tNmDrvRWReg *p = (tNmDrvRWReg *)inp;
			if (p->dir) {		/* Read */
				nmi_drv_read_register(mode, cix, (uint8_t *)&p->dat, p->sz, p->adr);				
			} else {				/* Write */
				nmi_drv_write_register(mode, cix, (uint8_t *)&p->dat, p->sz, p->adr);				
			}
		}
		break;
	case NMI_DRV_RF_RW_DIR:
		{
			tNmDrvRWReg *p = (tNmDrvRWReg *)inp;
			if (p->dir) {		/* Read */
				nmi_drv_read_rf_register(mode, cix, (uint8_t *)&p->dat, p->sz, p->adr);				
			} else {				/* Write */
				nmi_drv_write_rf_register(mode, cix, (uint8_t *)&p->dat, p->sz, p->adr);				
			}
		}
		break;
	case NMI_DRV_SOFT_RESET:
		nmi_drv_soft_reset(mode, cix);
		break;
	case NMI_DRV_GET_ISDBT_STATUS:
		nmi_isdbt_get_status(cix, (tIsdbtSignalStatus *)inp);
		break;
	case NMI_DRV_SCAN:
		nmi_drv_scan(mode, cix, inp);
		break;	
	case NMI_DRV_START_STREAM:
		nmi_drv_video(mode, (tNmDtvStream*)inp, 1);
		break;
	case NMI_DRV_STOP_STREAM:
		nmi_drv_video(mode, (tNmDtvStream*)inp, 0);
		break;
	case NMI_DRV_ISDBT_RST_COUNT:
		nmi_drv_isdbt_rst_cnt(cix);
		break;
	case NMI_DRV_ISDBT_SET_GAIN:
		{
			tNmiIsdbtLnaGain *p = (tNmiIsdbtLnaGain*)inp;
			pd->dtv.vf.setgain(cix, *p);	
		}break;
	case NMI_DRV_ISDBT_UPDATE_PLL:
		pd->dtv.vf.updatepll(cix);
		break;

	case NMI_DRV_VERSION:
		{
			tNmiDriverVer *vp = (tNmiDriverVer *)inp; 
		
			vp->dVer.major 	= ASIC_DTV_MAJOR_VER;
			vp->dVer.minor 	= ASIC_DTV_MINOR_VER;
			vp->dVer.rev1 	= ASIC_DTV_REV1;
			vp->dVer.rev2 	= ASIC_DTV_REV2;
		}
		break;
	case NMI_DRV_GAIN_THOLD:
#ifndef FIX_POINT
		nmi_isdbt_set_gain_thold(cix, (tIsdbtGainThold *)inp);
		#endif
break;
	case NMI_DRV_SET_GAIN:
		pd->dtv.vf.setgain(cix, (tNmiIsdbtLnaGain)inp);	
		break;
	case NMI_DRV_UPDATE_PLL:
		pd->dtv.vf.updatepll(cix);
		break;
	case NMI_DRV_SWITCH_DSM:
		{
			int enable = *(int*)inp;
			nmi_isdbt_dsm_on(enable);
		}
		break;
	case NMI_DRV_ENABLE_PID_FILTER:
		{
			tIsdbtPidFilterCtl *ppid = (tIsdbtPidFilterCtl*)inp;
			pd->dtv.vf.pidfilter(cix, ppid);
		}
		break;
	case NMI_DRV_DISABLE_PID_FILTER:
		{
			tIsdbtPidFilterCtl ppid;
			ppid.enfilt = 0;
			pd->dtv.vf.pidfilter(cix, &ppid);	
		}break;
	case NMI_DRV_ADD_PID:
		pd->dtv.vf.addpid(cix, (tIsdbtPid*)inp);
		break;
	case NMI_DRV_REMOVE_PID: 
		{
			unsigned int pid = *(unsigned int*)inp;
			pd->dtv.vf.removepid(cix, pid);
		}break;
	default:
		break;
	}
	
	return result;
}
