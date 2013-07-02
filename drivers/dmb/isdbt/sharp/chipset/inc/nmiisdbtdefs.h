/******************************************************************************
**
**	Copyright (c) Newport Media Inc.  All rights reserved.
**
** 	Module Name:  nmiisdbtdefs.h
**		These are the defines that are specific for the ISDBT ASIC driver.
**
** 
** 	Version				Date					Author						Descriptions
** -----------		----------	-----------		------------------------------------	
**
**	1.4.0.12				8.28.2008		K.Yu						 Alpha release 
**	1.4.0.17				10.31.2008		K.Yu						There are naming conflicts. Make the name 
**																					unique.
*******************************************************************************/

#ifndef _NMI_ISDBT_DEFS_H_
#define _NMI_ISDBT_DEFS_H_

/******************************************************************************
**
**	Isdbt Chip Type
**
*******************************************************************************/

typedef enum {
	nSlave = 0,
	nMaster,
} tNmiIsdbtChip;

/******************************************************************************
**
**	Isdbt Operation Mode
**
*******************************************************************************/

typedef enum {
	nDiversity = 0,
	nPip,
	nSingle,
} tNmiIsdbtOpMode;

/******************************************************************************
**
**	Isdbt Lna Gain Stages
**
*******************************************************************************/
typedef enum
{
	nBypass = 0,
	nLoGain,
	nMiGain,
	nHiGain,
} tNmiIsdbtLnaGain;

/*******************************************************************************
**
**	Isdbt OFDM
**
*******************************************************************************/

typedef enum {
	nMode2 = 0,
	nMode3,
} tNmiIsdbtMode;

/*******************************************************************************
**
**	Isdbt Diversity State Machine (DSM)
**
*******************************************************************************/

typedef enum {
	nIdle = 0,
	nDiv,
	nAOnly,
	nBOnly,
} tNmiDsmState;

#endif
