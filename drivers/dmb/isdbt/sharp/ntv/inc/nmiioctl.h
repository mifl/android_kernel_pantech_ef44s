/******************************************************************************
**
**	Copyright (c) Newport Media Inc.  All rights reserved.
**
** 	Module Name:  nmiiioctl.h
**	
** 
*******************************************************************************/

#ifndef _NMI_IOCTL_H_
#define _NMI_IOCTL_H_

#include "../../chipset/inc/nmitypes.h"
#include "../../chipset/inc/nmicmndefs.h"
#include "../../chipset/inc/nmiisdbtdefs.h"
#include "../../chipset/inc/nmiisdbt.h"

/******************************************************************************
**
**	NMI IO Return Code 
**
*******************************************************************************/

typedef enum {
	NMI_S_OK = 0,
	NMI_E_FAIL = -1,

	NMI_E_BUS_LOAD = -100,
	NMI_E_BUS_INIT,
	NMI_E_BUS_IO,

	NMI_E_NO_MASTER = -200,
	NMI_E_NO_SLAVE,
} tNmiRetCode;

/******************************************************************************
**
**	Nmi driver Interfaces 
**
*******************************************************************************/
typedef enum {
	nATVMode = 1,
	nISDBTMode,
	nBothMode,
} tNmDrvMode;

#define NMI_DRV_INIT									0x10001000

struct _i2c_addr {
	uint8_t adrM;			// master i2c address
	uint8_t adrS;			// slave i2c address
};

typedef struct _i2c_addr tI2CAdr;
struct _isdbt_init {
	tNmiBus						bustype;				/* control pass */
	tNmiBus						transporttype;			/* data pass */
	tNmiIsdbtIn					core;					/* core driver parameter */
	int 						blocksize;				/* tsp block size in case if isdbt SPI */		
	unsigned 					sqcallinterval;			/* isdbt sq in the spi callback period, in msec unit */
	int (*pfdtvtscb) (unsigned char *, unsigned int);	/* isdbt ts callback */
	int (*pfdtvsqcb) (void *);							/* signal quality callback */
	tIsdbtTso 					tso;					/* tsif configuration */
	int 						hbus;					/* set the bus module driver handle if needed*/
	tI2CAdr 					ai2c;					/* I2C address, if master chip then 0x61 or 0x63, otherwise if slave chip, then 0x60 or 0x62  */
};
typedef struct {
	struct _isdbt_init isdbt;
} tNmDrvInit;


#define NMI_DRV_DEINIT									0x10001001
#define NMI_DRV_INIT_CORE								0x10001002
#define NMI_DRV_GET_CHIPID								0x10001003
#define NMI_DRV_RUN										0x10001004

typedef struct {
	uint32_t			frequency;
	int					highside;
} tIsdbtRun;

#define NMI_DRV_RW_DIR									0x10001005
#define NMI_DRV_RF_RW_DIR								0x10001006
typedef struct {
	int 				dir;
	uint32_t 			adr;
	int					sz;
	uint32_t 			dat;	
} tNmDrvRWReg;

#define NMI_DRV_SOFT_RESET								0x10001007

#define NMI_DRV_GET_ISDBT_STATUS								0x10001008
typedef struct 
{
	int							lock;
	int							dagc;
	tNmiIsdbtLnaGain			lnamode;
	int							lnagain;
	int							lnagaincode;
#ifndef FIX_POINT
	double						agcacq;
	double						syracq;
	double						ppmacq;
	double						tmccacq;
	double						tsacq;
	double						totalacq;
#endif
	int							mode;
	int							guard;
	int							modulation;
	int							fec;
	int							interleaver;
	int							dagc_lock;
#ifndef FIX_POINT
	double						dsnr;
#else
	int							nsnr;
#endif
	int							dbbv;/* ber before viterbi */
	int							dbbvinst;
	int							dber;
	uint32_t				dberinst;
	int							dber2;
	int							dber2inst;
	int							dper;
	int							dperinst;
	int						  dper2;
#ifndef FIX_POINT
	double						freqoffset;
	double						timeoffset;
	double						rssi;
	double						doppler;
	double						chanlen;
#else
	int							rssi;
	uint32_t					doppler;
#endif
	int							dsmstate;
	int							dsmlock;
	uint32_t					regad48;
	uint32_t					regad4c;
	uint32_t					regacc8;
	/**
		current frequency
	**/
	unsigned int 				frequency;

	/**
		signal quality indicator
	**/
	unsigned int 				sqindicator;

	/**
		emrgency flag
	**/
	int 						emerg_flag;
	
	/**
		Get Mininum SQ
	**/
	int               get_simple_mode;    //add johnny 201123
} tIsdbtSignalStatus;

typedef struct 
{
	tIsdbtSignalStatus master;
	tIsdbtSignalStatus slave;
}tIsdbtSignalInfo;

#define NMI_DRV_SCAN									0x10001009

typedef struct
{
	int found;
	int tscan;
	int tsync;
} tIsdbtScanTime;

typedef struct
{
	uint32_t 		frequency;
	int				found;	
	tIsdbtScanTime tim;
} tIsdbtScan;

#define NMI_DRV_START_STREAM							0x10001015
typedef struct {
	tNmiBus transporttype;
	tIsdbtTso tso;
	tIsdbtIrq irq;
}tNmDtvStream;

typedef struct {
	tIsdbtIrq irq;
}tNmDtvIrq;

#define NMI_DRV_STOP_STREAM								0x10001016
#define NMI_DRV_ISDBT_RST_COUNT							0x10001018
#define NMI_DRV_ISDBT_SET_GAIN							0x10001019
#define NMI_DRV_VERSION									0x20000000
typedef struct {
	int major;
	int minor;
	int rev1;
	int rev2;
}tNmiDtvVer;

typedef struct {
	tNmiDtvVer dVer;
} tNmiDriverVer;

#define NMI_DRV_ISDBT_UPDATE_PLL						0x40001000
#define NMI_DRV_DEEP_SLEEP								0x80000000				// tony, 2010-07-21
#define NMI_DRV_WAKEUP									0x80000001				// tony, 2010-07-21
#define NMI_DRV_DTV_TS_TEST_PATTERN						0x90000004

#define NMI_DRV_ISR_PROCESS								0x90000006
typedef struct {
	tIsdbtIrqStatus irq;
}tNmDrvIsdbtIrq;

#define NMI_DRV_SWITCH_COMBINER							0x9000000b
#define NMI_DRV_GAIN_THOLD								0x90001000

#ifndef FIX_POINT
typedef struct
{
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

} tIsdbtGainThold;
#endif

#define NMI_DRV_SET_GAIN								0x90001001
#define	NMI_DRV_UPDATE_PLL								0x90001002
#define NMI_DRV_SWITCH_DSM								0x90001003
#define NMI_DRV_PROBE_CHIP								0x90001004
#define NMI_DRV_ENABLE_PID_FILTER						0x90001005
#define NMI_DRV_DISABLE_PID_FILTER						0x90001006
#define NMI_DRV_ADD_PID									0x90001007
#define NMI_DRV_REMOVE_PID									0x90001008


/******************************************************************************
**
**	Application Interface 
**
*******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
int nmi_drv_ctl(uint32_t, tNmDrvMode, tNmiIsdbtChip, void *);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif
