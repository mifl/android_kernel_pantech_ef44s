/******************************************************************************
**
**	Copyright (c) Newport Media Inc.  All rights reserved.
**
** 	Module Name:  nmicmn.h
**		This is the header file to be shared by the user's module and NMI's ASIC driver.
**
**
*******************************************************************************/

#ifndef _NMIISDBTCMN_H_
#define _NMIISDBTCMN_H_

#include "nmiver.h"

#include "nmicmndefs.h"
#include "nmiisdbtdefs.h"
#include "nmitypes.h"
#include "nmiisdbt.h"



extern void nmi_isdbt_common_init(tNmiIsdbtIn *, tNmiIsdbtVtbl *);

#endif
