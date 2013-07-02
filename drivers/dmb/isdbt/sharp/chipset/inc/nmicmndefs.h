/******************************************************************************
**
**	Copyright (c) Newport Media Inc.  All rights reserved.
**
** 	Module Name:  nmicmndefs.h
**		These are the common defines that are being used by the ASIC driver.
**
** 
** 	Version				Date					Author						Descriptions
** -----------		----------	-----------		------------------------------------	
**
**	1.4.0.12				8.28.2008		K.Yu						 Alpha release 
**	1.4.0.17				10.31.2008		K.Yu						There are naming conflicts. Make the name 
**																					unique.
*******************************************************************************/

#ifndef _NMICMNDEFS_H_
#define _NMICMNDEFS_H_

/******************************************************************************
**
**	NMI Bus Type
**
*******************************************************************************/

typedef enum {
	nTS = 0,
	nI2C,
	nSPI,
	nUSB,
	nSPI1,
} tNmiBus;

/******************************************************************************
**
**	NMI Crystall Defines
**
*******************************************************************************/

typedef enum {
	nXO13 = 1,
	nXO26,
	nXO32,
} tNmiXo;

/******************************************************************************
**
**	NMI Package Defines
**
*******************************************************************************/
typedef enum {
	nBGA = 0,
	nCSP,

} tNmiPackage;

/******************************************************************************
**
**	OFDM Defines
**
*******************************************************************************/

typedef enum {
	nQPSK = 1,
	nQAM16,
	nQAM64,
} tNmiMod;

typedef enum {
	nRate1_2 = 0,
	nRate2_3,
	nRate3_4,
	nRate5_6,
	nRate7_8,
} tNmiCodeRate;

typedef enum {
	nGuard1_32 = 0,
	nGuard1_16,
	nGuard1_8,
	nGuard1_4,
} tNmiGuard;


/******************************************************************************
**
**	Transport Stream Defines
**
*******************************************************************************/
typedef enum
{
	nSerial = 0,
	nParallel,
}tNmiTsType;

typedef enum {
	nClk_s_8 = 0,
	nClk_s_4,
	nClk_s_2,
	nClk_s_1,
	nClk_s_05,

	nClk_p_1 = 0,
	nClk_p_05,
	nClk_p_025,
	nClk_p_0125,
	nClk_p_00625,

} tNmiTsClk;

/******************************************************************************
**
**	Debug Flags Defines
**
*******************************************************************************/

#define N_INIT			0x1
#define N_ERR			0x2
#define N_WARN		0x4
#define N_FUNC			0x8
#define N_INFO			0x10
#define N_TUNE			0x20
#define N_VERB			0x40
#define N_UV				0x80
#define N_SCAN			0x100
#define N_SEEK			0x200
#define N_INTR			0x400

#endif

