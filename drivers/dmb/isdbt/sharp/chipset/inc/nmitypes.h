/******************************************************************************
**
**	Copyright (c) Newport Media Inc.  All rights reserved.
**
** 	Module Name:  nmitypes.h
**		These are the typed used by the ASIC driver.
**
*******************************************************************************/

#ifndef _NMI_TYPES_
#define _NMI_TYPES_

#if 1//defined (ANDROID)
//#include "stdint.h"
#else
typedef unsigned char			uint8_t;
typedef unsigned long     		uint32_t; //johnny
typedef unsigned short			uint16_t;
typedef unsigned char 			bool_t;
#endif

#if defined (MQX_OS)
typedef unsigned int 			hSem;
#else
typedef void					*hSem;
typedef void					*hTimer;
#endif

#ifndef NULL
#define NULL     0
#endif

#ifndef FALSE
#define FALSE  0
#endif

#ifndef TRUE
#define TRUE  1
#endif


#endif
	
