//=============================================================================
// File       : nmidrv.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2012/02/15       yschoi         Create
//=============================================================================

#ifndef _NMIDRV_H_
#define _NMIDRV_H_

#include "../../../isdbt_modeldef.h"

#define NMI_I2C_ID          0xc2

#define TS_PACKET_SIZE			188
#define TS_BUFFER_COEF			10
#define TS_TRANSFER_COEF		TS_PACKET_SIZE
#define NMI_MAX_TSP_PER_DMA		40
#define NMI_TS_BUF_SIZE			(TS_PACKET_SIZE * TS_BUFFER_COEF * NMI_MAX_TSP_PER_DMA)

#define ISVALID(func)	(func != NULL) ? (1): (0)

#define	DF_SILENT	0x00000000	// no messages printed.
#define DF_CT				0x00000001	// call trace messages enabled.
#define DF_ERR			0x00000002	// error messages enabled.
#define DF_WARN		0x00000004	// warning messages enabled.
#define DF_INFO		0x00000008	// info messages enabled.
#define DF_SQ				0x00000010	// signal status messages enabled.
#define DF_INTR		0x00000020	// interrupt status
#define DF_VERB		0x00000040	// verbose.


typedef struct {
	int (*i2cw) 	(unsigned char, unsigned char *, unsigned long);	/* i2c write */
#ifdef FEATURE_DMB_SHARP_I2C_READ
	int (*i2cr) 	(unsigned char, unsigned char *, unsigned long, unsigned char *, unsigned long); /* i2c read */
#else
	int (*i2cr) 	(unsigned char, unsigned char *, unsigned long);	/* i2c read */
#endif
	int (*spiw) 	(unsigned char *, unsigned long);					/* spi write */
	int (*spir) 	(unsigned char *, unsigned long);					/* spi read */
	int (*burstr)	(unsigned char *, unsigned long);					/* dma burst read */
}tBus;

typedef struct {
	long(*gettick) (void);									/* get time */
	void (*delay)		(unsigned int);							/* delay (msec)*/
}tOs;

typedef struct {
	void (*prnt)		(char *);								/* print debug string */
	tBus bus;
	tOs os;
}tOem;

typedef struct _tNtv
{
	unsigned int tsdmasize;								// received size
	unsigned char ptsbuf[NMI_TS_BUF_SIZE];								// ts buffer
}tNtv;



/*====================================================================
FUNCTION       nmi_drv_init  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int nmi_drv_init(void *pv);

/*====================================================================
FUNCTION       nmi_drv_init_core  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int nmi_drv_init_core(tNmDrvMode mode, tNmiIsdbtChip cix);

/*====================================================================
FUNCTION       nmi_drv_run  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int nmi_drv_run(tNmDrvMode mode, tNmiIsdbtChip cix, void *pv);

/*====================================================================
FUNCTION       nmi_isdbt_get_status  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void nmi_isdbt_get_status(tNmiIsdbtChip cix, tIsdbtSignalStatus *p);

/*====================================================================
FUNCTION       nmi_drv_scan  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void nmi_drv_scan(tNmDrvMode mode, tNmiIsdbtChip cix, void *pv);

/*====================================================================
FUNCTION       nmi_drv_video  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void nmi_drv_video(tNmDrvMode mode, tNmDtvStream *p, int enable);


#endif /* _NMIDRV_H_ */
