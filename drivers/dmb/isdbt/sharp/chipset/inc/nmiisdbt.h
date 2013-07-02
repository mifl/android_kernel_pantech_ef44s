/******************************************************************************
**
**	Copyright (c) Newport Media Inc.  All rights reserved.
**
** 	Module Name:  nmiisdbt.h
**		These are the parameters used by the ASIC driver. 
**
** 
** 	Version				Date					Author						Descriptions
** -----------		----------	-----------		------------------------------------	
**
**	1.4.0.12				8.28.2008			K.Yu						Alpha release 
**							9.23.2008										Modify "tIsdbtAgcThold" data structure to
**																				support 2 or 3 layer tracking.
*******************************************************************************/
#ifndef _NMIISDBT_H_
#define _NMIISDBT_H_

#define FIX_POINT

/******************************************************************************
**
**	NMI Isdbt Interface Functions Parameters
**
*******************************************************************************/

typedef struct {
	uint32_t frequency;	/* Hz */
	int highside;
} tIsdbtTune;

typedef struct {
#ifdef FIX_POINT
	int snr;
#else
	double snr;
#endif
} tIsdbtSnr;

typedef struct {
	uint32_t bec;				/* bit error count */
	uint32_t becw;			/* bit error count window */
} tIsdbtBer;

typedef struct {
	int		enable;
	uint32_t period;
} tIsdbtBerTimer;

typedef struct {
	uint32_t pec;				/* packet error count */
	uint32_t pecw;			/* packet error count window */
} tIsdbtPer;

typedef struct {
	tNmiIsdbtOpMode 	mode;
	tNmiTsType 			tstype;
	tNmiTsClk				clkrate;
	int clkedge;
	int gatedclk;
	int lsbfirst;
	int dmaen;
	int blocksize;
	int nullen;
	int dropbadts;
	int datapol;
	int validpol;
	int syncpol;
	int errorpol;
	int valid204;
	int clk204;
	int parityen;
	int nullsuppress;
}tIsdbtTso;

#ifndef FIX_POINT
typedef struct {
	/**
		4 layer gain threshold
	**/
	double hi2mi_4;
	double mi2lo_4;
	double lo2by_4;

	double by2lo_4;
	double lo2mi_4;
	double mi2hi_4;

	double lo2by_b_4;
	double mi2lo_b_4;
	double hi2lo_b_4;
	double hi2lo_b2_4;

} tIsdbtAgcThold;
#endif

typedef struct {
	uint32_t					dagc;
	tNmiIsdbtLnaGain	rflna;
	uint32_t					bblna;
	uint32_t					bblnacode;
	int							rflnadb;
	int							bblnadb;
#ifdef FIX_POINT
	int								rssi;
#else
	double						rssi;
#endif
} tIsdbtGainInfo;

typedef struct {
	int	ensleep;
	int	toff;
	int	ton;
} tIsdbtSleep;

typedef struct {
	tNmiIsdbtMode	mode;
	tNmiGuard			guard;
	tNmiCodeRate 	coderate;
	tNmiMod 				modulation;
	int 						interleaver;
	int 			emerg_flag;	// added by tony
} tIsdbtTmccInfo;

typedef union {
	int pulse;
	int activehigh;
	int pulsewidth;		/* in nanosecond unit, minimum 250 ns */
} tCtlIrq;

typedef union {
	struct demodirq {
		uint32_t tmcc:1;
		uint32_t tmccchg:1;
		uint32_t tmccbad:1;
		uint32_t agclock:1;
		uint32_t fftcomplete:1;
		uint32_t syrlock:1;
		uint32_t statechg:1;
		uint32_t global:1;
		uint32_t reserved:24;
	} u;
	uint32_t core;
} tDemodIrq;

typedef union {
	struct viterbiirq {
		uint32_t acqsync:1;
		uint32_t lostsync:1;
		uint32_t acqsyncfail:1;
		uint32_t berdone:1;
		uint32_t beroverflow:1;
		uint32_t fifooverflow:1;
		uint32_t reserved: 26;
	} u;
	uint32_t vit;
} tViterbiIrq;

typedef union {
	struct decoderirq {
		uint32_t reserve1: 1;
		uint32_t viterbi:1;
		uint32_t reserve2:1;
		uint32_t rsfail:1;
		uint32_t dmaready:1;
		uint32_t dmacomplete:1;
		uint32_t reserve3:1;
		uint32_t overflow:1;
		uint32_t underflow:1;
		uint32_t reserve4:2;
		uint32_t tsoverflow:1;
		uint32_t dmaoverflow:1;
		uint32_t reserve5:1;
		uint32_t synclost:1;
		uint32_t reserve6:11;
		uint32_t nosig:1;
		uint32_t reserve7:1;
		uint32_t detectsig:1;
		uint32_t reserve8:3;
	} u;
	uint32_t dec;
} tDecoderIrq;

typedef union {
	struct emirq {
		uint32_t enable:1;
		uint32_t rising:1;
		uint32_t watchdog:1;
	} u;
	uint32_t emg;

} tAlarmIrq;

typedef struct {
	tCtlIrq				ctl;
	int					gio0;
	int					gio1;
	int					gio2;
	int					gio3;
	tDemodIrq	 	cirq;
	tViterbiIrq 		virq;
	tDecoderIrq 	dirq;
	tAlarmIrq		alarm;
	int					pllresync;
} tIsdbtIrq;

typedef struct {
	int					gio0;
	int					gio1;
	int					gio2;
	int					gio3;
	tDemodIrq	 	cirq;
	tViterbiIrq 		virq;
	tDecoderIrq 	dirq;
	int					alarm;
	int					pllresync;
} tIsdbtIrqStatus;

typedef struct
{
	int dir;				/* gpio direction */
	int bit;				/* gpio pin */
	int level;				/* gpio level, output only */
} tIsdbtGio;

typedef struct {
	int enable;
	int inverse;
	int format;
	int period;
	int data;
} tIsdbtPwmCtl;

typedef struct {
	uint8_t code;
#ifdef FIX_POINT
	int level;
#else
	double level;
#endif
} tIsdbtBbGain;

typedef struct {
	uint32_t pid;			/* pid to be filter in */
	uint32_t rpid;			/* remap pid number */
	int remap;				/* enable or disable pid remap */
} tIsdbtPid;

typedef struct {
	int enfilt;					
	int enremapfilt;
} tIsdbtPidFilterCtl;

typedef struct {
	int enable;
	int hardwire;
	uint8_t key[16];
} tIsdbtAes;

#ifndef FIX_POINT
typedef struct {
	double	agcacq;
	double	syracq;
	double	ppmacq;
	double	tmccacq;
	double	tsacq;
	double	totalacq;
}tIsdbtAcqTime;
#endif


/******************************************************************************
**
**	NMI Isdbt Function Table
**
*******************************************************************************/

typedef struct {
	int (*init)(tNmiIsdbtChip);
	int (*probe)(tNmiIsdbtChip);
	uint32_t (*getchipid)(tNmiIsdbtChip);
	uint8_t (*r8)(tNmiIsdbtChip, uint32_t);
	void (*w8)(tNmiIsdbtChip, uint32_t, uint8_t);
	uint32_t (*r32)(tNmiIsdbtChip, uint32_t);
	void (*w32)(tNmiIsdbtChip, uint32_t, uint32_t);
	uint8_t (*rfr)(tNmiIsdbtChip, uint32_t);
	void (*rfw)(tNmiIsdbtChip, uint32_t, uint8_t);
	void (*tune)(tNmiIsdbtChip, void *);
	void (*demod)(tNmiIsdbtChip);
	void (*decoder)(tNmiIsdbtChip, int, tIsdbtTmccInfo *);
 	int (*checklock)(tNmiIsdbtChip, uint32_t);
	int (*agclock)(tNmiIsdbtChip);
	int (*syrlock)(tNmiIsdbtChip);
	int (*tmcclock)(tNmiIsdbtChip);
	int (*feclock)(tNmiIsdbtChip);
	int (*scan)(tNmiIsdbtChip, void *);
	int (*scanpoll)(void *);
#ifndef FIX_POINT
	double (*scantime)(void);
#endif
	void (*reset)(tNmiIsdbtChip);
#ifdef FIX_POINT
	uint32_t (*getsnr)(tNmiIsdbtChip, int);
#else
	double (*getsnr)(tNmiIsdbtChip, int);
#endif
	void (*getber)(tNmiIsdbtChip, void *);
	void (*getper)(tNmiIsdbtChip, void *);
	void (*pwrdown)(void);
	void (*pwrup)(int);
	void (*sleeptimer)(void *);
	void (*setgainthold)(tNmiIsdbtChip, void *);
 	void (*getrssi)(tNmiIsdbtChip, void *);
#ifndef FIX_POINT
	double (*getfreqoffset)(tNmiIsdbtChip);
	double (*gettimeoffset)(tNmiIsdbtChip);
#endif
	void (*dacctl)(int, uint8_t);
	void (*pwmctl)(tNmiIsdbtChip, void *);
	void (*combine)(int);
	void (*setgain)(tNmiIsdbtChip, tNmiIsdbtLnaGain);
	void (*tscfg)(void *);
	void (*tsctl)(int);
	void (*gettmccinfo)(tNmiIsdbtChip,void *);
	void (*updatepll)(tNmiIsdbtChip);
	void (*handleintr)(tIsdbtIrqStatus *);
	uint32_t(*getdoppler)(tNmiIsdbtChip);
#ifndef FIX_POINT
	//double (*getdoppler)(tNmiIsdbtChip);
	double (*getchanlen)(tNmiIsdbtChip);
#endif
	void (*enableirq)(void *);
 	void (*bertimer)(tNmiIsdbtChip, void *);
	int (*getberb)(tNmiIsdbtChip, void *);
 	void (*chkpll)(tNmiIsdbtChip);
	void (*dsm)(int);
	int (*dsmstate)(void);
	int (*dsmlock)(void);
	int (*addpid)(tNmiIsdbtChip, tIsdbtPid *);
	int (*removepid)(tNmiIsdbtChip, uint32_t);
	void (*resetpid)(tNmiIsdbtChip);
	void (*pidfilter)(tNmiIsdbtChip, tIsdbtPidFilterCtl *);
	void (*encryption)(tNmiIsdbtChip, tIsdbtAes *);
#ifndef FIX_POINT
	void (*getacqtime)(tNmiIsdbtChip, void *);
#endif
	void (*highxtaldrive)(int);
	void (*spidma)(int);
	void (*configspipad)(void);
	void (*outputxo)(void);
	void (*dtvtspattern) (int);
} tNmiIsdbtVtbl;

/******************************************************************************
**
**	NMI Isdbt OS dependent Functions
**
*******************************************************************************/

typedef struct {
	int (*read)(tNmiIsdbtChip, uint8_t *, int, uint8_t *, int);
	int (*write)(tNmiIsdbtChip, uint8_t *, int);
	void (*delay)(uint32_t);
	uint32_t (*gettick)(void);
	void (*log)(char *);
	struct transport {
		int (*read)(tNmiIsdbtChip, uint8_t *, int, uint8_t *, int);
		int (*write)(tNmiIsdbtChip, uint8_t *, int);
		int (*burstread)(int, int);	/* added the 2nd parameter, int, by tony */
	} t;
} tNmiIsdbtHlpVtbl;

/******************************************************************************
**
**	NMI Isdbt Common Share Data Structure
**
*******************************************************************************/

typedef struct {
	tNmiBus 					bustype;
	tNmiBus					transporttype;
	tNmiPackage			pkg;	
	tNmiIsdbtOpMode	op;
	int							ldoby;	/* Set to 1 if the IO voltage is less than 2.2v */
	uint32_t					zone;
	tNmiIsdbtHlpVtbl				hlp;
	int							spiworkaround;
#ifndef FIX_POINT
	double					xo;
#endif
	int							dco;
    int							no_ts_err_chk;
} tNmiIsdbtIn;

#endif
