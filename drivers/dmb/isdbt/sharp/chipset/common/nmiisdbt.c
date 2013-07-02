/******************************************************************************
**
**	Copyright (c) Newport Media Inc.  All rights reserved.
**
** 	Module Name:  isdbt.c
**	
**		This module implements the hardware configurations for the NMI ISDBT chipset.  The
**		module hides the hardware details thru the exported functions table.  The user can
**		then utilize the function table to properly enable the chipset's ISDBT functions.  The 
**		module is OS 	independent and implemented in ANSI C which should help to integrate
**		into different enviroment.  Due to the OS independent nature, this module doesn't
**		include any particular OS header files.  The user should include this file in his or her
**		own OS context and provide the appropriate header files.       
**
** 
**
*******************************************************************************/

static void isdbt_update_pll(tNmiIsdbtChip);

/******************************************************************************
**
**	Internal Macro
**
*******************************************************************************/

#define ISNM325(id) (((id & 0xfff00) == 0x32500) ? 1 : 0) 
#define ISNM325E0(id) (((id & 0xfffff) == 0x325e0) ? 1 : 0) 
#define ISNM325F0(id) (((id & 0xfffff) == 0x325f0) ? 1 : 0) 
#define ISMASTER(id) ((((id >> 24) & 0x1) == 1) ? 1 : 0)

/******************************************************************************
**
**	RF Functions
**
*******************************************************************************/

static uint8_t isdbtrfread(tNmiIsdbtChip cix, uint32_t adr)
{
	int retry;
	uint32_t reg;

	wReg32(cix, 0x6310, adr);

	retry = 1000;
	do {
		reg = rReg32(cix, 0x632c);
		if (reg & 0x200)
			break;
	} while (retry--);

	if (retry <= 0) {
		nmi_debug(N_ERR, "RF Read reg: Fail, wait STOP DET intr, (%d)(%08x)\r\n", cix, reg);
		chip.buserror = 1;
		return 0;
	}

	rReg32(cix, 0x6340);

	wReg32(cix, 0x6310, 0x100);

	retry = 1000;
	do {
		reg = rReg32(cix, 0x632c);		
		if (reg & 0x200)
			break;
	} while (retry--);

	if (retry <= 0) {
		nmi_debug(N_ERR, "RF Read reg: Fail, wait STOP DET intr, (%d)(%08x)\r\n", cix, reg);
		chip.buserror = 1;
		return 0;
	}

	reg = rReg32(cix, 0x6310);
	rReg32(cix, 0x6340);		

	return (uint8_t)reg;
}

static void isdbtrfi2cwrite(tNmiIsdbtChip cix, uint32_t sadr, uint8_t *b, int cnt)
{
	int retry, i, wc, ix;
	uint32_t reg;

	ix = 0;
	do {
		if (cnt > 7)
			wc = 7;
		else
			wc = cnt;

		wReg32(cix, 0x6500, (wc + 1));	/* length */
		wReg32(cix, 0x6504, sadr + ix);			/* start address */
		for(i=0; i<wc; i++) {
			wReg32(cix, 0x6504, b[i+ix]);			/* data */
		}
		wReg32(cix, 0x6508, 0x1);				/* set the GO bit */

		retry = 1000;
		do {
			reg = rReg32(cix, 0x6508);		/* wait till GO is clear */
			if ((reg & 0x1) == 0)
				break;
		} while (retry--);

			reg = rReg32(cix, 0x650c);			/* check the DONE bit */
			if (!(reg & 0x1)) {
			nmi_debug(N_ERR, "RF write reg: Fail, cix(%d), adr(%02x), len (%d)\n", cix, sadr, cnt);
				chip.buserror = 1;
			}

		cnt -= wc;
		ix += wc;
	} while (cnt > 0);

	return;
}

static void isdbtrfwrite(tNmiIsdbtChip cix, uint32_t adr, uint8_t val)
{
	isdbtrfi2cwrite(cix, adr, &val, 1);
	return;
}

static void isdbtrfburstwrite(tNmiIsdbtChip cix, uint32_t sadr, uint8_t *b, int cnt)
{
	isdbtrfi2cwrite(cix, sadr, b, cnt);
	return;
}

static void isdbt_high_xtal_drive(int enable)
{
	uint8_t val8;

	val8 = isdbtrfread(nMaster, 0x00);
	if (enable) {
		val8 &= ~(1 << 5);
	} else {
		val8 |= (1 << 5);
	}
	isdbtrfwrite(nMaster, 0x00, val8);

	return;
}

typedef struct 
{
	uint32_t pll1;
	uint32_t norm1;
	uint32_t norm2;
	uint32_t sx5;	
} tNmiPllTbl;

#if 0 // cys not used
static tNmiPllTbl plltbl_12[] = {
/* 13 */ 	{0x619045c4,	0x7d,	0x7e,	0x58},
				{0x61900bc4,	0x3c,	0x7f,	0xd8},
				{0x619048c2,	0xbe,	0x7f,	0x58},
				{0x619048c2,	0xbe,	0x7f,	0xd8},
				{0x619034c2,	0x01,	0x80,	0x58},
				{0x61900bc4,	0x3c,	0x7f,	0x58},
				{0x619034c2,	0x01,	0x80,	0xd8},
				{0x619034c2,	0x01,	0x80,	0xd8},
				{0x61903cc4,	0x9a,	0x7e,	0x58},
				{0x61905bc2,	0x7f,	0x7f,	0x58},
/* 23 */	{0x61905bc2,	0x7f,	0x7f,	0x58},
				{0x619063c4,	0x1b,	0x7e,	0x58},
				{0x61900bc4,	0x3c,	0x7f,	0x58},
				{0x619063c4,	0x1b,	0x7e,	0x58},
				{0x619052c2,	0x9d,	0x7f,	0x58},
				{0x61904fc4,	0x5c,	0x7e,	0x58},
				{0x619028c4,	0xdc,	0x7e,	0x58},
				{0x61900bc4,	0x3c,	0x7f,	0x58},
				{0x61904fc4,	0x5c,	0x7e,	0x58},
				{0x61904fc4,	0x5c,	0x7e,	0x58},
/* 33 */	{0x619048c2,	0xbe,	0x7f,	0x58},
				{0x619001c4,	0x5d,	0x7f,	0x58},
				{0x619034c2,	0x01,	0x80,	0x58},
				{0x61901ec4,	0xfd,	0x7e,	0x58},
				{0x61905bc2,	0x7f,	0x7f,	0x58},
				{0x61905bc2,	0x7f,	0x7f,	0x58},
				{0x61900bc4,	0x3c,	0x7f,	0x58},
				{0x61904fc4,	0x5c,	0x7e,	0x58},
				{0x619052c2,	0x9d,	0x7f,	0x58},
				{0x61900bc4,	0x3c,	0x7f,	0x58},
/* 43 */	{0x619028c4,	0xdc,	0x7e,	0x58},
				{0x619052c2,	0x9d,	0x7f,	0x58},
				{0x619052c2,	0x9d,	0x7f,	0x58},
				{0x619034c2,	0x01,	0x80,	0xd8},
				{0x619048c2,	0xbe,	0x7f,	0x58},
				{0x61901cc6,	0xbc,	0x7d,	0x58},
				{0x619034c2,	0x01,	0x80,	0xd8},
				{0x61905bc2,	0x7f,	0x7f,	0x58},
				{0x61904fc4,	0x5c,	0x7e,	0x58},
				{0x61905bc2,	0x7f,	0x7f,	0x58},
/* 53 */	{0x61904fc4,	0x5c,	0x7e,	0x58},
				{0x619059c4,	0x3b,	0x7e,	0x58},
				{0x61900bc4,	0x3c,	0x7f,	0x58},
				{0x619032c4,	0xbb,	0x7e,	0x58},
				{0x619034c2,	0x01,	0x80,	0x58},
				{0x61901ec4,	0xfd,	0x7e,	0x58},
				{0x619012c6,	0xdd,	0x7d,	0x58},
				{0x619052c2,	0x9d,	0x7f,	0x58},
				{0x619052c2,	0x9d,	0x7f,	0x58},
				{0x619034c2,	0x01,	0x80,	0x58},
/* 63 */	{0x619034c2,	0x01,	0x80,	0x58},
				{0x61901ec4,	0xfd,	0x7e,	0x58},
				{0x619034c2,	0x01,	0x80,	0x58},
				{0x619048c2,	0xbe,	0x7f,	0x58},
				{0x619032c4,	0xbb,	0x7e,	0x58},
				{0x61901cc6,	0xbc,	0x7d,	0x58},
};

static tNmiPllTbl plltbl_13[] = {
/* 13 */{0x619002b4,	0x01,	0x80,	0x58},
				{0x61901db4,	0x9f,	0x7f,	0xd8},
				{0x619014b4,	0xbf,	0x7f,	0x58},
				{0x619002b4,	0x01,	0x80,	0xd8},
				{0x619002b4,	0x01,	0x80,	0x58},
				{0x619038b4,	0x3d,	0x7f,	0x58},
				{0x619002b4,	0x01,	0x80,	0xd8},
				{0x619002b4,	0x01,	0x80,	0xd8},
				{0x619001b6,	0x9c,	0x7e,	0x58},
				{0x619026b4,	0x7e,	0x7f,	0x58},
/* 23 */{0x619026b4,	0x7e,	0x7f,	0x58},
				{0x619025b6,	0x1d,	0x7e,	0x58},
				{0x619041b4,	0x1d,	0x7f,	0x58},
				{0x619040b6,	0xbd,	0x7d,	0x58},
				{0x619002b4,	0x01,	0x80,	0x58},
				{0x61905cb4,	0xbc,	0x7e,	0x58},
				{0x619053b4,	0xdc,	0x7e,	0x58},
				{0x619038b4,	0x3d,	0x7f,	0x58},
				{0x61905cb4,	0xbc,	0x7e,	0x58},
				{0x619013b6,	0x5c,	0x7e,	0x58},
/* 33 */	{0x619014b4,	0xbf,	0x7f,	0x58},
				{0x61902fb4,	0x5e,	0x7f,	0x58},
				{0x619002b4,	0x01,	0x80,	0x58},
				{0x619002b4,	0x01,	0x80,	0x58},
				{0x61901cb6,	0x3c,	0x7e,	0x58},
				{0x619002b4,	0x01,	0x80,	0x58},
				{0x61901db4,	0x9f,	0x7f,	0x58},
				{0x619013b6,	0x5c,	0x7e,	0x58},
				{0x61901db4,	0x9f,	0x7f,	0x58},
				{0x619038b4,	0x3d,	0x7f,	0x58},
/* 43 */{0x619013b6,	0x5c,	0x7e,	0x58},
				{0x61901db4,	0x9f,	0x7f,	0x58},
				{0x61901db4,	0x9f,	0x7f,	0x58},
				{0x619002b4,	0x01,	0x80,	0xd8},
				{0x619014b4,	0xbf,	0x7f,	0x58},
				{0x619040b6,	0xbd,	0x7d,	0x58},
				{0x619002b4,	0x01,	0x80,	0xd8},
				{0x619014b4,	0xbf,	0x7f,	0x58},
				{0x619002b4,	0x01,	0x80,	0x58},
				{0x619026b4,	0x7e,	0x7f,	0x58},
/* 53 */{0x619013b6,	0x5c,	0x7e,	0x58},
				{0x61901cb6,	0x3c,	0x7e,	0x58},
				{0x619041b4,	0x1d,	0x7f,	0x58},
				{0x61905cb4,	0xbc,	0x7e,	0x58},
				{0x619002b4,	0x01,	0x80,	0x58},
				{0x61901db4,	0x9f,	0x7f,	0x58},
				{0x619037b6,	0xdd,	0x7d,	0x58},
				{0x61901db4,	0x9f,	0x7f,	0x58},
				{0x619026b4,	0x7e,	0x7f,	0x58},
				{0x619002b4,	0x01,	0x80,	0x58},
/* 63 */{0x619002b4,	0x01,	0x80,	0x58},
				{0x61904ab4,	0xfd,	0x7e,	0x58},
				{0x619002b4,	0x01,	0x80,	0x58},
				{0x619014b4,	0xbf,	0x7f,	0x58},
				{0x61902fb4,	0x5e,	0x7f,	0x58},
				{0x619040b6,	0xbd,	0x7d,	0x58},
};

static tNmiPllTbl plltbl_13_008896[] = {
{0x619060b2,		0x0	,	0x80,	0x58},
{0x619017b4,		0x9e,	0x7f,	0xd8},
{0x61900eb4,		0xbf,	0x7f,	0x58},
{0x619060b2,		0x0	,	0x80,	0xd8},
{0x619060b2,		0x0	,	0x80,	0x58},
{0x619032b4,		0x3d,	0x7f,	0x58},
{0x619060b2,		0x0	,	0x80,	0xd8},
{0x619060b2,		0x0	,	0x80,	0xd8},
{0x61905fb4,		0x9b,	0x7e,	0x58},
{0x619020b4,		0x7e,	0x7f,	0x58},
{0x619020b4,		0x7e,	0x7f,	0x58},
{0x61901fb6,		0x1c,	0x7e,	0x58},
{0x61903bb4,		0x1c,	0x7f,	0x58},
{0x61903ab6,		0xbd,	0x7d,	0x58},
{0x619060b2,		0x0	,	0x80,	0x58},
{0x619056b4,		0xbc,	0x7e,	0x58},
{0x61904db4,		0xdc,	0x7e,	0x58},
{0x619032b4,		0x3d,	0x7f,	0x58},
{0x619056b4,		0xbc,	0x7e,	0x58},
{0x61900db6,		0x5b,	0x7e,	0x58},
{0x61900eb4,		0xbf,	0x7f,	0x58},
{0x619029b4,		0x5d,	0x7f,	0x58},
{0x619060b2,		0x0	,	0x80,	0x58},
{0x619060b2,		0x0	,	0x80,	0x58},
{0x619016b6,		0x3c,	0x7e,	0x58},
{0x619060b2,		0x0	,	0x80,	0x58},
{0x61903bb4,		0x1c,	0x7f,	0x58},
{0x61900db6,		0x5b,	0x7e,	0x58},
{0x619017b4,		0x9e,	0x7f,	0x58},
{0x619032b4,		0x3d,	0x7f,	0x58},
{0x61900db6,		0x5b,	0x7e,	0x58},
{0x619017b4,		0x9e,	0x7f,	0x58},
{0x619017b4,		0x9e,	0x7f,	0x58},
{0x619060b2,		0x0	,	0x80,	0xd8},
{0x61900eb4,		0xbf,	0x7f,	0x58},
{0x61903ab6,		0xbd,	0x7d,	0x58},
{0x619060b2,		0x0	,	0x80,	0xd8},
{0x61900eb4,		0xbf,	0x7f,	0x58},
{0x619056b4,		0xbc,	0x7e,	0x58},
{0x619020b4,		0x7e,	0x7f,	0x58},
{0x61900db6,		0x5b,	0x7e,	0x58},
{0x619016b6,		0x3c,	0x7e,	0x58},
{0x61903bb4,		0x1c,	0x7f,	0x58},
{0x619056b4,		0xbc,	0x7e,	0x58},
{0x619060b2,		0x0	,	0x80,	0x58},
{0x61904db4,		0xdc,	0x7e,	0x58},
{0x619031b6,		0xdc,	0x7d,	0x58},
{0x619017b4,		0x9e,	0x7f,	0x58},
{0x619020b4,		0x7e,	0x7f,	0x58},
{0x619060b2,		0x0	,	0x80,	0x58},
{0x619060b2,		0x0	,	0x80,	0x58},
{0x619044b4,		0xfc,	0x7e,	0x58},
{0x619060b2,		0x0	,	0x80,	0x58},
{0x61900eb4,		0xbf,	0x7f,	0x58},
{0x619029b4,		0x5d,	0x7f,	0x58},
{0x61903ab6,		0xbd,	0x7d,	0x58},
};


static tNmiPllTbl plltbl_19_2[] = {
/* 13 */{0x61905f78,	0x01,	0x80,	0x58},
				{0x61901a7a,	0x5c,	0x7f,	0xd8},
				{0x6190077a,	0xc1,	0x7f,	0x58},
				{0x61905f78,	0x01,	0x80,	0xd8},
				{0x61905f78,	0x01,	0x80,	0x58},
				{0x6190207a,	0x3c,	0x7f,	0x58},
				{0x61905f78,	0x01,	0x80,	0xd8},
				{0x61905f78,	0x01,	0x80,	0xd8},
				{0x61903e7a,	0x9d,	0x7e,	0x58},
				{0x6190147a,	0x7c,	0x7f,	0x58},
/* 23 */{0x6190147a,	0x7c,	0x7f,	0x58},
				{0x6190577a,	0x1a,	0x7e,	0x58},
				{0x6190267a,	0x1c,	0x7f,	0x58},
				{0x6190057c,	0xbc,	0x7d,	0x58},
				{0x61905f78,	0x01,	0x80,	0x58},
				{0x6190387a,	0xbd,	0x7e,	0x58},
				{0x6190327a,	0xdc,	0x7e,	0x58},
				{0x6190207a,	0x3c,	0x7f,	0x58},
				{0x6190387a,	0xbd,	0x7e,	0x58},
				{0x61904a7a,	0x5e,	0x7e,	0x58},
/* 33 */{0x6190077a,	0xc1,	0x7f,	0x58},
				{0x61901a7a,	0x5b,	0x7f,	0x58},
				{0x61905f78,	0x01,	0x80,	0x58},
				{0x61905f78,	0x01,	0x80,	0x58},
				{0x6190517a,	0x39,	0x7e,	0x58},
				{0x61905f78,	0x01,	0x80,	0x58},
				{0x6190267a,	0x1c,	0x7f,	0x58},
				{0x61904a7a,	0x5e,	0x7e,	0x58},
				{0x61900e7a,	0x9b,	0x7f,	0x58},
				{0x6190207a,	0x3c,	0x7f,	0x58},
/* 43 */{0x61904a7a,	0x5e,	0x7e,	0x58},
				{0x61900e7a,	0x9b,	0x7f,	0x58},
				{0x61900e7a,	0x9b,	0x7f,	0x58},
				{0x61905f78,	0x01,	0x80,	0xd8},
				{0x6190077a,	0xc1,	0x7f,	0x58},
				{0x6190057c,	0xbc,	0x7d,	0x58},
				{0x61905f78,	0x01,	0x80,	0xd8},
				{0x6190077a,	0xc1,	0x7f,	0x58},
				{0x6190387a,	0xbd,	0x7e,	0x58},
				{0x6190147a,	0x7b,	0x7f,	0x58},
/* 53 */{0x61904a7a,	0x5e,	0x7e,	0x58},
				{0x6190517a,	0x39,	0x7e,	0x58},
				{0x6190267a,	0x1c,	0x7f,	0x58},
				{0x6190387a,	0xbd,	0x7e,	0x58},
				{0x61905f78,	0x01,	0x80,	0x58},
				{0x6190327a,	0xdc,	0x7e,	0x58},
				{0x6190637a,	0xdc,	0x7d,	0x58},
				{0x61900e7a,	0x9c,	0x7f,	0x58},
				{0x6190147a,	0x7c,	0x7f,	0x58},
				{0x61905f78,	0x01,	0x80,	0x58},
/* 63 */{0x61905f78,	0x01,	0x80,	0x58},
				{0x61902c7a,	0xfc,	0x7e,	0x58},
				{0x61905f78,	0x01,	0x80,	0x58},
				{0x6190077a,	0xc1,	0x7f,	0x58},
				{0x61901a7a,	0x5b,	0x7f,	0x58},
/* 68 */{0x6190057c,	0xbc,	0x7d,	0x58},
};

/**
	This is for the 24.576 MHz
**/
#if 1
static tNmiPllTbl plltbl_24_576[] = {
/*13*/{0x61900A60,	0xb8,	0x7e,	0x58},
			{0x61905A5E,	0x40,	0x7f,	0x58},
			{0x61901D60,	0x39,	0x7e,	0x58},
			{0x61902b60,	0xDB,	0x7d,	0x58},
			{0x61903E5E,	0xFF,	0x7F,	0x58},
			{0x61900060,	0xfc,	0x7e,	0x58},
			{0x61903E5E,	0xFF,	0x7F,	0x58},
			{0x6190565E,	0x5B,	0x7F,	0x58},
			{0x61900E60,	0x9D,	0x7e,	0x58},
			{0x61903960,	0x7e,	0x7d,	0x58},
/*23*/{0x61900060,	0xfc,	0x7e,	0x58},
			{0x61903460,	0xa0,	0x7d,	0x58},
			{0x61905A5E,	0x40,	0x7f,	0x58},
			{0x61901D60,	0x39,	0x7e,	0x58},
			{0x61905A5E,	0x40,	0x7f,	0x58},
			{0x61900A60,	0xb8,	0x7e,	0x58},
			{0x61901D60,	0x39,	0x7e,	0x58},
			{0x61900560,	0xDA,	0x7E,	0x58},
			{0x61901860,	0x5a,	0x7e,	0x58},
			{0x61902660,	0xfd,	0x7d,	0x58},
/*33*/{0x6190475E,	0xC2,	0x7F,	0x58},
			{0x6190565E,	0x5B,	0x7F,	0x58},
			{0x6190475E,	0xC2,	0x7F,	0x58},
			{0x61903E5E,	0xFF,	0x7F,	0x58},
			{0x61901D60,	0x39,	0x7e,	0x58},
			{0x61903060,	0xBA,	0x7d,	0x58},
			{0x61905F5E,	0x1E,	0x7F,	0x58},
			{0x61901860,	0x5a,	0x7e,	0x58},
			{0x61904760,	0x22,	0x7d,	0x58},
			{0x61900A60,	0xb8,	0x7e,	0x58},
/*43*/{0x61902160,	0x1E,	0x7e,	0x58},
			{0x61900560,	0xDA,	0x7E,	0x58},
			{0x61904C5E,	0x9F,	0x7F,	0x58},
			{0x61903E5E,	0xFF,	0x7F,	0xd8},
			{0x6190475E,	0xC2,	0x7F,	0x58},
			{0x61903960,	0x7e,	0x7d,	0x58},
			{0x6190475E,	0xC2,	0x7F,	0x58},
			{0x6190475E,	0xC2,	0x7F,	0x58},
			{0x61900A60,	0xb8,	0x7e,	0x58},
			{0x61903460,	0xa0,	0x7d,	0x58},
/*53*/{0x61905F5E,	0x1E,	0x7F,	0x58},
			{0x61903460,	0xa0,	0x7d,	0x58},
			{0x61905F5E,	0x1E,	0x7F,	0x58},
			{0x61903060,	0xBA,	0x7d,	0x58},
			{0x6190435e,	0xdd,	0x7F,	0x58},
			{0x61900560,	0xDA,	0x7E,	0x58},
			{0x61902b60,	0xDB,	0x7d,	0x58},
			{0x61903E5E,	0xFF,	0x7F,	0x58},
			{0x61905A5E,	0x40,	0x7f,	0x58},
			{0x61903E5E,	0xFF,	0x7F,	0x58},
};
#else
static tNmiPllTbl plltbl_24_576[] = {
/* 13 */{0x61900a60,	0xb8,	0x7e,	0x58},
				{0x61905A5E,	0x40,	0x7f,	0xd8},
				{0x61901D60,	0x39,	0x7e,	0x58},
				{0x61903E5E,	0xFF,	0x7F,	0x58},
				{0x61903E5E,	0xFF,	0x7F,	0x58},
				{0x61905A5E,	0x40,	0x7f,	0x58},
				{0x61903E5E,	0xFF,	0x7F,	0x58},
				{0x61903E5E,	0xFF,	0x7F,	0x58},
				{0x61900E60,	0x9D,	0x7e,	0x58},
				{0x6190515E,	0x7D,	0x7F,	0x58},
/* 23 */{0x6190515E,	0x7D,	0x7F,	0x58},
				{0x61902160,	0x1E,	0x7e,	0x58},
				{0x61905F5E,	0x1E,	0x7F,	0x58},
				{0x61903060,	0xBA,	0x7d,	0x58},
				{0x61903E5E,	0xFF,	0x7F,	0x58},
				{0x61900A60,	0xb8,	0x7e,	0x58},
				{0x61900560,	0xDA,	0x7E,	0x58},
				{0x61905A5E,	0x40,	0x7f,	0x58},
				{0x61900A60,	0xb8,	0x7e,	0x58},
				{0x61901860,	0x5a,	0x7e,	0x58},
/* 33 */{0x6190475E,	0xC2,	0x7F,	0x58},
				{0x6190565E,	0x5B,	0x7F,	0x58},
				{0x61903E5E,	0xFF,	0x7F,	0x58},
				{0x61903E5E,	0xFF,	0x7F,	0x58},
				{0x61901D60,	0x39,	0x7e,	0x58},
				{0x61903E5E,	0xFF,	0x7F,	0x58},
				{0x61905F5E,	0x1E,	0x7F,	0x58},
				{0x61901860,	0x5a,	0x7e,	0x58},
				{0x61904C5E,	0x9F,	0x7F,	0x58},
				{0x61903E5E,	0xFF,	0x7F,	0x58},
/* 43 */{0x61901860,	0x5a,	0x7e,	0x58},
				{0x61904C5E,	0x9F,	0x7F,	0x58},
				{0x61904C5E,	0x9F,	0x7F,	0x58},
				{0x61903E5E,	0xFF,	0x7F,	0xd8},
				{0x6190475E,	0xC2,	0x7F,	0x58},
				{0x61903060,	0xBA,	0x7d,	0x58},
				{0x61903E5E,	0xFF,	0x7F,	0xd8},
				{0x6190475E,	0xC2,	0x7F,	0x58},
				{0x61900A60,	0xb8,	0x7e,	0x58},
				{0x6190515E,	0x7D,	0x7F,	0x58},
/* 53 */{0x61901860,	0x5a,	0x7e,	0x58},
				{0x61901860,	0x5a,	0x7e,	0x58},
				{0x61905F5E,	0x1E,	0x7F,	0x58},
				{0x61900A60,	0xb8,	0x7e,	0x58},
				{0x61903E5E,	0xFF,	0x7F,	0x58},
				{0x61900560,	0xDA,	0x7E,	0x58},
				{0x61902b60,	0xDB,	0x7d,	0x58},
				{0x61904C5E,	0x9F,	0x7F,	0x58},
				{0x6190515E,	0x7D,	0x7F,	0x58},
				{0x61903E5E,	0xFF,	0x7F,	0x58},
/* 63 */{0x61903E5E,	0xFF,	0x7F,	0x58},
				{0x61904556,	0xfc,	0x7e,	0x58},
				{0x61903E5E,	0xFF,	0x7F,	0x58},
				{0x6190475E,	0xC2,	0x7F,	0x58},	
				{0x6190565E,	0x5B,	0x7F,	0x58},
/* 68 */{0x61903060,	0xBA,	0x7d,	0x58}
};
#endif

/**
	This is for the 26 MHz
**/
static tNmiPllTbl plltbl_26[] = {
/* 13 */{0x6190015a,	0x01,	0x80,	0x58},
				{0x61900f5a,	0x9c,	0x7f,	0xd8},
				{0x61900a5a,	0xbf,	0x7f,	0x58},
				{0x6190015a,	0x01,	0x80,	0xd8},
				{0x6190015a,	0x01,	0x80,	0x58},
				{0x61901c5a,	0x3d,	0x7f,	0x58},
				{0x6190015a,	0x01,	0x80,	0xd8},
				{0x6190015a,	0x01,	0x80,	0xd8},
				{0x6190335a,	0x99,	0x7e,	0x58},
				{0x6190185a,	0x5a,	0x7f,	0x58},
/* 23 */{0x61900f5a,	0x9b,	0x7f,	0x58},
				{0x6190455a,	0x19,	0x7e,	0x58},
				{0x6190215a,	0x19,	0x7f,	0x58},
				{0x6190525a,	0xbd,	0x7d,	0x58},
				{0x6190015a,	0x01,	0x80,	0x58},
				{0x61902e5a,	0xbc,	0x7e,	0x58},
				{0x61902a5a,	0xd9,	0x7e,	0x58},
				{0x61901c5a,	0x3d,	0x7f,	0x58},
				{0x61902e5a,	0xbc,	0x7e,	0x58},
				{0x61903c5a,	0x59,	0x7e,	0x58},
/* 33 */{0x61900a5a,	0xbf,	0x7f,	0x58},
				{0x6190185a,	0x5a,	0x7f,	0x58},
				{0x6190015a,	0x01,	0x80,	0x58},
				{0x6190015a,	0x01,	0x80,	0x58},
				{0x6190405a,	0x3c,	0x7e,	0x58},
				{0x6190015a,	0x01,	0x80,	0x58},
				{0x6190215a,	0x19,	0x7f,	0x58},
				{0x61903c5a,	0x59,	0x7e,	0x58},
				{0x61900f5a,	0x9b,	0x7f,	0x58},
				{0x61901c5a,	0x3d,	0x7f,	0x58},
/* 43 */{0x61903c5a,	0x59,	0x7e,	0x58},
				{0x61900f5a,	0x9b,	0x7f,	0x58},
				{0x61900f5a,	0x9b,	0x7f,	0x58},
				{0x6190015a,	0x01,	0x80,	0xd8},
				{0x61900a5a,	0xbf,	0x7f,	0x58},
				{0x6190525a,	0xbd,	0x7d,	0x58},
				{0x6190015a,	0x01,	0x80,	0xd8},
				{0x61900a5a,	0xbf,	0x7f,	0x58},
				{0x61902e5a,	0xbc,	0x7e,	0x58},
				{0x6190135a,	0x7e,	0x7f,	0x58},
/* 53 */{0x61903c5a,	0x59,	0x7e,	0x58},
				{0x6190405a,	0x3c,	0x7e,	0x58},
				{0x6190215a,	0x19,	0x7f,	0x58},
				{0x61902e5a,	0xbc,	0x7e,	0x58},
				{0x6190015a,	0x01,	0x80,	0x58},
				{0x61902a5a,	0xd9,	0x7e,	0x58},
				{0x61904e5a,	0xda,	0x7d,	0x58},
				{0x61900f5a,	0x9c,	0x7f,	0x58},
				{0x61900f5a,	0x9b,	0x7f,	0x58},
				{0x6190015a,	0x01,	0x80,	0x58},
/* 63 */{0x6190015a,	0x01,	0x80,	0x58},
				{0x6190255a,	0xfd,	0x7e,	0x58},
				{0x6190015a,	0x01,	0x80,	0x58},
				{0x61900a5a,	0xbf,	0x7f,	0x58},
				{0x6190185a,	0x5a,	0x7f,	0x58},
/* 68 */{0x6190525a,	0xbd,	0x7d,	0x58}
};

/**
	This is for the 27 MHz
**/
static tNmiPllTbl plltbl_27[] = {
/* 13 */{0x61902256,	0x03,	0x80,	0x58},
				{0x61902f56,	0xa1,	0x7f,	0xd8},
				{0x61902b56,	0xbf,	0x7f,	0x58},
				{0x61902256,	0x03,	0x80,	0xd8},
				{0x61902256,	0x03,	0x80,	0x58},
				{0x61903c56,	0x40,	0x7f,	0x58},
				{0x61902256,	0x03,	0x80,	0xd8},
				{0x61902256,	0x03,	0x80,	0xd8},
				{0x61905256,	0x9c,	0x7e,	0x58},
				{0x61903856,	0x5d,	0x7f,	0x58},
/* 23 */{0x61902f56,	0xa1,	0x7f,	0x58},
				{0x61906356,	0x1f,	0x7e,	0x58},
				{0x61904156,	0x1a,	0x7f,	0x58},
				{0x61900c58,	0xc0,	0x7d,	0x58},
				{0x61902256,	0x03,	0x80,	0x58},
				{0x61904e56,	0xba,	0x7e,	0x58},
				{0x61904956,	0xdf,	0x7e,	0x58},
				{0x61903c56,	0x40,	0x7f,	0x58},
				{0x61904e56,	0xba,	0x7e,	0x58},
				{0x61905b56,	0x5a,	0x7e,	0x58},
/* 33 */{0x61902b56,	0xbf,	0x7f,	0x58},
				{0x61903856,	0x5d,	0x7f,	0x58},
				{0x61902256,	0x03,	0x80,	0x58},
				{0x61902256,	0x03,	0x80,	0x58},
				{0x61905f56,	0x3c,	0x7e,	0x58},
				{0x61902256,	0x03,	0x80,	0x58},
				{0x61904156,	0x1a,	0x7f,	0x58},
				{0x61905b56,	0x5a,	0x7e,	0x58},
				{0x61902f56,	0xa1,	0x7f,	0x58},
				{0x61903c56,	0x40,	0x7f,	0x58},
/* 43 */{0x61905b56,	0x5a,	0x7e,	0x58},
				{0x61902f56,	0xa1,	0x7f,	0x58},
				{0x61902f56,	0xa1,	0x7f,	0x58},
				{0x61902256,	0x03,	0x80,	0xd8},
				{0x61902b56,	0xbf,	0x7f,	0x58},
				{0x61900c58,	0xc0,	0x7d,	0x58},
				{0x61902256,	0x03,	0x80,	0xd8},
				{0x61902b56,	0xbf,	0x7f,	0x58},
				{0x61904e56,	0xba,	0x7e,	0x58},
				{0x61903456,	0x7b,	0x7f,	0x58},
/* 53 */{0x61905b56,	0x5a,	0x7e,	0x58},
				{0x61905f56,	0x3c,	0x7e,	0x58},
				{0x61904156,	0x1a,	0x7f,	0x58},
				{0x61904e56,	0xba,	0x7e,	0x58},
				{0x61902256,	0x03,	0x80,	0x58},
				{0x61904956,	0xdf,	0x7e,	0x58},
				{0x61900858,	0xdd,	0x7d,	0x58},
				{0x61902256,	0x03,	0x80,	0x58},
				{0x61904556,	0xfc,	0x7e,	0x58},
				{0x61902256,	0x03,	0x80,	0x58},
/* 63 */{0x61902256,	0x03,	0x80,	0x58},
				{0x61904556,	0xfc,	0x7e,	0x58},
				{0x61902256,	0x03,	0x80,	0x58},
				{0x61902b56,	0xbf,	0x7f,	0x58},
				{0x61903856,	0x5d,	0x7f,	0x58},
/* 68 */{0x61900c56,	0xc0,	0x7d,	0x58},
};
#endif

/**
	This is for the 32 MHz
**/
static tNmiPllTbl plltbl_32[] = {
/*13*/{0x61903948,	0x01,	0x80,	0x58},
			{0x61903d48,	0xdd,	0x7f,	0xd8},
			{0x61904048,	0xc3,	0x7f,	0x58},
			{0x61903948,	0x01,	0x80,	0xd8},
			{0x61903948,	0x01,	0x80,	0x58},
			{0x61904f48,	0x3d,	0x7f,	0x58},
			{0x61903948,	0x01,	0x80,	0xd8},
			{0x61903948,	0x01,	0x80,	0xd8},
			{0x61906148,	0x9f,	0x7e,	0x58},
			{0x61904b48,	0x61,	0x7f, 	0x58},
/*23*/{0x61904448,	0x9f,	0x7f, 	0x58},
			{0x61900c4a,	0x1c,	0x7e, 	0x58},
			{0x61905348,	0x1a,	0x7f, 	0x58},
			{0x6190174a,	0xbc,	0x7d, 	0x58},
			{0x61903948,	0x01,	0x80, 	0x58},
			{0x61905E48,	0xb9,	0x7e, 	0x58},
			{0x61905a48,	0xdc,	0x7e,	0x58},
			{0x61904f48,	0x3d,	0x7f, 	0x58},
			{0x61905E48,	0xb9,	0x7e,	0x58},
			{0x6190054a,	0x59,	0x7e,	0x58},
/*33*/{0x61904048,	0xc3,	0x7f, 	0x58},
			{0x61904b48,	0x61,	0x7f, 	0x58},
			{0x61903948,	0x01,	0x80, 	0x58},
			{0x61903948,	0x01,	0x80, 	0x58},
			{0x61900c4a,	0x1b,	0x7e, 	0x58},
			{0x61903948,	0x01,	0x80, 	0x58},
			{0x61905348,	0x1a,	0x7f, 	0x58},
			{0x6190054a,	0x59,	0x7e, 	0x58},
			{0x61904448,	0x9f,	0x7f, 	0x58},
			{0x61904f48,	0x3d,	0x7f, 	0x58},
/*43*/{0x6190054a,	0x59,	0x7e, 	0x58},
			{0x61904448,	0x9f,	0x7f, 	0x58},
			{0x61904448,	0x9f,	0x7f, 	0x58},
			{0x61903948,	0x01,	0x80, 	0xd8},
			{0x61904048,	0xc2,	0x7f, 	0x58},
			{0x6190174a,	0xbc,	0x7d, 	0x58},
			{0x61903948,	0x01,	0x80, 	0xd8},
			{0x61904048,	0xc3,	0x7f, 	0x58},
			{0x61905e48,	0xb9,	0x7e, 	0x58},
			{0x61904848,	0x7b,	0x7f, 	0x58},
/*53*/{0x6190054a,	0x59,	0x7e, 	0x58},
			{0x61900c4a,	0x1c,	0x7e, 	0x58},
			{0x61905348,	0x1a,	0x7f, 	0x58},
			{0x61905e48,	0xb9,	0x7e, 	0x58},
			{0x61903948,	0x01,	0x80, 	0x58},
			{0x61905a48,	0xdc,	0x7e, 	0x58},
			{0x6190134a,	0xdf,	0x7d, 	0x58},
			{0x61903948,	0x01,	0x80, 	0x58},
			{0x61904f48,	0x3d,	0x7f, 	0x58},
			{0x61903948,	0x01,	0x80, 	0x58},
/*63*/{0x61903948,	0x01,	0x80, 	0x58},
			{0x61905648,	0xff,  	0x7e, 	0x58},
			{0x61903948,	0x01,	0x80, 	0x58},
			{0x61904048,	0xc2,	0x7f, 	0x58},
			{0x61904b48,	0x61,	0x7f, 	0x58},
/*68*/{0x6190174a,	0xbc,	0x7d, 	0x58}
};

static void isdbt_adj_pll(tNmiIsdbtChip cix)
{
	tNmiPriv *pp = (tNmiPriv *)&chip.priv[cix];
	int index;
	tNmiPllTbl *plltbl;
	//uint32_t val32;

	index = (pp->frequency - 473143)/6000;

	/* just to protect the input frequency range */
	if((index < 0) || (index > 55)) {
		nmi_debug(N_ERR, "Out of Frequency Range, frequency(%d)\n", pp->frequency);
		return;
	}

#ifndef FIX_POINT
	if (chip.inp.xo == 12.) 
		plltbl = plltbl_12;
	else if (chip.inp.xo == 13.) 
		plltbl = plltbl_13;
	else if (chip.inp.xo == 13.008896) 
		plltbl = plltbl_13_008896;
	else if (chip.inp.xo == 19.2) 
		plltbl = plltbl_19_2;
	else if (chip.inp.xo == 24.576) 
		plltbl = plltbl_24_576;
	else if (chip.inp.xo == 26)
		plltbl = plltbl_26;
	else if (chip.inp.xo == 27)
		plltbl = plltbl_27;
	else
		plltbl = plltbl_32;
#else
	plltbl = plltbl_32;
#endif

	if (pp->sx5 != plltbl[index].sx5) {
		isdbtrfwrite(cix, 0x2b, (uint8_t)plltbl[index].sx5);
		pp->sx5 = plltbl[index].sx5;
	}

	if (pp->norm1 != plltbl[index].norm1) {
		wReg32(cix, 0xa06c, plltbl[index].norm1);
		pp->norm1 = plltbl[index].norm1;
	}

	if (pp->norm2 != plltbl[index].norm2) {
		wReg32(cix, 0xa070, plltbl[index].norm2);
		pp->norm2 = plltbl[index].norm2;
	}

	if ((pp->pll1 != plltbl[index].pll1)) {
		wReg32(cix, 0x6468, plltbl[index].pll1);	
		isdbt_update_pll(cix);
		pp->pll1 = plltbl[index].pll1;
	}
}

static void isdbt_rssi_ctl(tNmiIsdbtChip cix)
{
	tNmiPriv 	*pp = (tNmiPriv *)&chip.priv[cix];
	int ix;
	uint32_t val1, val2;

	ix = 13 + (pp->frequency - 473143)/6000;
	if ((ix >= 13) && (ix <= 24)) {
		val1 = 0x10425ef;
		val2 = 0x72;
	} else if ((ix >= 25) && (ix <= 31)) {
		val1 = 0x10c29ea;
		val2 = 0x76;
	} else if ((ix >= 32) && (ix <= 37)) {
		val1 = 0x11029e4;
		val2 = 0x7a;
	} else if ((ix >= 38) && (ix <= 43)) {
		val1 = 0x12433df;
		val2 = 0x80;
	} else if ((ix >= 44) && (ix <= 49)) {
		val1 = 0x12433de;
		val2 = 0x80;
	} else if ((ix >= 50) && (ix <= 52)) {
		val1 = 0x12c37da;
		val2 = 0x83;
	} else if ((ix >= 53) && (ix <= 55)) {
		val1 = 0x12835d8;
		val2 = 0x7e;
	} else if ((ix >= 56) && (ix <= 58)) {
		val1 = 0x11c31d9;
		val2 = 0x79;
	} else if ((ix >= 59) && (ix <= 60)) {
		val1 = 0x1142dda;
		val2 = 0x74;
	} else if ((ix >= 61) && (ix <= 68)) {
		val1 = 0x10c29da;
		val2 = 0x6f;
	}
	wReg32(cix, 0xa92c, val1);
	wReg32(cix, 0xa930, val2);

	return;
}

#ifdef FIX_POINT

typedef struct 
{
	uint8_t r1;
	uint8_t r2;
	uint8_t r3;
	uint8_t r4;	
} tNmiTuneTbl;

static tNmiTuneTbl tunetbl_32[] = {
{0x76,	0x4f,	0x0d,	0x03},
{0x77,	0x4f,	0x0d,	0x0b},
{0x79,	0x4f,	0x0d,	0x03},
{0x7a,	0x4f,	0x0d,	0x0b},
{0x7c,	0x4f,	0x0d,	0x03},
{0x7d,	0x4f,	0x0d,	0x0b},
{0x7f,	0x4f,	0x0d,	0x03},
{0x80,	0x4f,	0x0d,	0x0b},
{0x82,	0x4f,	0x0d,	0x03},
{0x83,	0x4f,	0x0d,	0x0b},
{0x85,	0x4f,	0x0d,	0x03},
{0x86,	0x4f,	0x0d,	0x0b},
{0x88,	0x4f,	0x0d,	0x03},
{0x89,	0x4f,	0x0d,	0x0b},
{0x8b,	0x4f,	0x0d,	0x03},
{0x8c,	0x4f,	0x0d,	0x0b},
{0x8e,	0x4f,	0x0d,	0x03},
{0x8f,	0x4f,	0x0d,	0x0b},
{0x91,	0x4f,	0x0d,	0x03},
{0x92,	0x4f,	0x0d,	0x0b},
{0x94,	0x4f,	0x0d,	0x03},
{0x95,	0x4f,	0x0d,	0x0b},
{0x97,	0x4f,	0x0d,	0x03},
{0x98,	0x4f,	0x0d,	0x0b},
{0x9a,	0x4f,	0x0d,	0x03},
{0x9b,	0x4f,	0x0d,	0x0b},
{0x9d,	0x4f,	0x0d,	0x03},
{0x9e,	0x4f,	0x0d,	0x0b},
{0xa0,	0x4f,	0x0d,	0x03},
{0xa1,	0x4f,	0x0d,	0x0b},
{0xa3,	0x4f,	0x0d,	0x03},
{0xa4,	0x4f,	0x0d,	0x0b},
{0xa6,	0x4f,	0x0d,	0x03},
{0xa7,	0x4f,	0x0d,	0x0b},
{0xa9,	0x4f,	0x0d,	0x03},
{0xaa,	0x4f,	0x0d,	0x0b},
{0xac,	0x4f,	0x0d,	0x03},
{0xad,	0x4f,	0x0d,	0x0b},
{0xaf,	0x4f,	0x0d,	0x03},
{0xb0,	0x4f,	0x0d,	0x0b},
{0xb2,	0x4f,	0x0d,	0x03},
{0xb3,	0x4f,	0x0d,	0x0b},
{0xb5,	0x4f,	0x0d,	0x03},
{0xb6,	0x4f,	0x0d,	0x0b},
{0xb8,	0x4f,	0x0d,	0x03},
{0xb9,	0x4f,	0x0d,	0x0b},
{0xbb,	0x4f,	0x0d,	0x03},
{0xbc,	0x4f,	0x0d,	0x0b},
{0xbe,	0x4f,	0x0d,	0x03},
{0xbf,	0x4f,	0x0d,	0x0b},
{0xc1,	0x4f,	0x0d,	0x03},
{0xc2,	0x4f,	0x0d,	0x0b},
{0xc4,	0x4f,	0x0d,	0x03},
{0xc5,	0x4f,	0x0d,	0x0b},
{0xc7,	0x4f,	0x0d,	0x03},
{0xc8,	0x4f,	0x0d,	0x0b}
};
#endif
static void isdbt_config_tuner(tNmiIsdbtChip cix, void *pv)
{
	tIsdbtTune *tune = (tIsdbtTune *)pv;
	tNmiPriv 		*pp = (tNmiPriv *)&chip.priv[cix];
	uint32_t		dfrequency;
#ifndef FIX_POINT
   	double 		target, refclk;
	uint32_t		n0, alpha;
#endif
	uint8_t			val[4];

	dfrequency = tune->frequency;
	if ((dfrequency < 473143000) || (dfrequency > 803143000)) {
		nmi_debug(N_ERR, "Out Of Frequency Range...(%d)\n", dfrequency);
		return;
	}

	dfrequency /= 1000;			/* KHz */
	pp->frequency = dfrequency;

	if (tune->highside) {		/* high side injection */
		if (pp->ddfs1 != 0x16) {
			isdbtrfwrite(cix, 0x84,  0x16);
			isdbtrfwrite(cix, 0x85,  0xf8);
			pp->ddfs1 = 0x16;
		}
		dfrequency += 380;
	} else {							/* low side injection */
		if (pp->ddfs1 != 0x68) {
			isdbtrfwrite(cix, 0x84,  0x68);
			isdbtrfwrite(cix, 0x85,  0x08);
			pp->ddfs1 = 0x68;
		}
		dfrequency -= 380;
	}

#ifdef FIX_POINT
	{
		int index = (pp->frequency - 473143)/6000;
		val[0] = tunetbl_32[index].r1;
		val[1] = tunetbl_32[index].r2;
		val[2] = tunetbl_32[index].r3;
		val[3] = tunetbl_32[index].r4;
	}
#else
	if (chip.inp.xo == 12.)
		refclk = 12000*2;
	else if (chip.inp.xo == 13.)
		refclk = 13000*2;
	else if (chip.inp.xo == 13.008896)
		refclk = 13008.896*2;
	else if (chip.inp.xo == 19.2) 
		refclk = 19200;
	else if (chip.inp.xo == 24.576) 
		refclk = 24576;
	else if (chip.inp.xo == 26)
		refclk = 26000;
	else if (chip.inp.xo == 27)
		refclk = 27000;
	else
		refclk = 32000/2;

	target = dfrequency * 4 / refclk;

	n0 = (long)target;
	val[0] = (uint8_t)n0;
	
	alpha = (long)((target - n0) * (0x1000000));
	alpha >>= 4;
	val[1] = (uint8_t)(alpha & 0xff);
	val[2] = (uint8_t)((alpha >> 8) & 0xff);
	val[3] = (uint8_t)((alpha >> 16) & 0xff);
#endif

	isdbtrfburstwrite(cix, 0x01, val, 4);
	
	/**
		adjust pll if necessary
	**/
	isdbt_adj_pll(cix);

	/**
		set the rssi calculation based on frequency
	**/
	isdbt_rssi_ctl(cix);

	return;
}

/******************************************************************************
**
**	Demod Functions
**
*******************************************************************************/

static int isdbt_agc_lock(tNmiIsdbtChip cix)
{
	uint32_t val32;

	val32 = rReg32(cix, 0xa004);
	if ((val32 & 0xf) >= 3)
		return 1;
	return 0;
}

static int isdbt_syr_lock(tNmiIsdbtChip cix)
{
	uint32_t val32;

	val32 = rReg32(cix, 0xa004);

	if((val32 >> 5) & 0x1)
		return 1;

	return 0;
}

static int isdbt_tmcc_lock(tNmiIsdbtChip cix)
{
	uint32_t val32;

	val32 = rReg32(cix, 0xa004);				
	if ((val32 >> 7) & 0x1) 
		return 1;

	return 0;
}

static int isdbt_fec_lock(tNmiIsdbtChip cix)
{
	uint32_t val32;

	val32 = rReg32(cix, 0xa824);
	if (((val32 >> 20) & 0x3) == 0x2)
		return 1;
	return 0;
}

#if 0 // cys not used
static int isdbt_check_agc(tNmiIsdbtChip cix)
{
	uint32_t stim;
	int lock = 0;

	stim = nmi_get_tick();
	do {
		nmi_delay(5);
		lock = isdbt_agc_lock(cix);
		if (lock)
			break;		
	} while ((nmi_get_tick() - stim) <= 60);
	return lock;
}

static int isdbt_check_syr(tNmiIsdbtChip cix)
{
	uint32_t stim;
	int lock = 0;

	stim = nmi_get_tick();
	do {
		nmi_delay(1);
		lock = isdbt_syr_lock(cix);
		if (lock)
			break;		
	} while ((nmi_get_tick() - stim) <= 101);

	return lock;
}

static int isdbt_detect_tmcc(tNmiIsdbtChip cix)
{
	uint32_t stim, val32;
	int lock = -1;

	stim = nmi_get_tick();
	do {
		nmi_delay(1);
		val32 = rReg32(cix, 0xa004);
		if ((val32 & 0xf) >= 5) {
			uint32_t x, y;

			x = rReg32(cix, 0xa174);
			y = rReg32(cix, 0xa048);

			if ((y >> 2) & 0x1) {
				if (x >= 0xa)
					lock = 1;
			} else {
				if (x >= 0xd) 
					lock = 1;
			}

			if (x <= 0x6)
				lock = 0;
 			break;
		}
	} while ((nmi_get_tick() - stim) <= 80);

	return lock;
}

static int isdbt_check_tmcc(tNmiIsdbtChip cix, uint32_t wait)
{
	uint32_t stim, val32;
	int lock = 0;

	stim = nmi_get_tick();
	do {
		nmi_delay(50);
		val32 = rReg32(cix, 0xa004);
		if ((val32 & 0xf) >= 6) {
			lock = 1;
 			break;
		}
	} while ((nmi_get_tick() - stim) <= (wait+50));

	return lock;
}
#endif

static int isdbt_check_lock(tNmiIsdbtChip cix, uint32_t wait)
{
	uint32_t stim;
	int lock = 0;

	stim = nmi_get_tick();
	do {
		nmi_delay(100);
		
		lock = isdbt_tmcc_lock(cix);
		if (lock)
			break;
	} while ((nmi_get_tick() - stim) <= (wait + 100));		/* review */

	return lock;
}

static void isdbt_get_tmcc_info(tNmiIsdbtChip cix, void *pv)
{
	//tNmiPriv *pp = (tNmiPriv *)&chip.priv[cix];
	tIsdbtTmccInfo *ti = (tIsdbtTmccInfo *)pv;
	uint32_t val32;
	
	val32 = rReg32(cix, 0xa048);
	ti->guard = val32 & 0x3;
	ti->mode = (val32 >> 2) & 0x1;

	// [[ to get the emerg_flag
	val32 = rReg32(cix, 0xa180);
	ti->emerg_flag = (val32 >> 2) & 0x1;
	// ]] added by tony

	val32 = rReg32(cix, 0xa188);
	ti->modulation = val32 & 0x3;
	ti->coderate = (val32 >> 3) & 0x7;

	val32 = rReg32(cix, 0xa18c);
	switch(((val32 >> 4) & 0x7)) {
	case 0:
		ti->interleaver = 0;
		break;
	case 1:
		if(ti->mode)
			ti->interleaver = 1;
		else
			ti->interleaver = 2;
		break;
	case 2:
		if(ti->mode)
			ti->interleaver = 2;
		else
			ti->interleaver = 4;
		break;
	case 3:
		if(ti->mode)
			ti->interleaver = 4;
		else
			ti->interleaver = 8;
		break;
	default:
		ti->interleaver = 0;
		break;
	}

	return;
}

#ifdef FIX_POINT

static uint32_t log_10[256] = {
0, 301, 477, 602, 698, 778, 845, 903, 
954, 1000, 1041, 1079, 1113, 1146, 1176, 1204, 
1230, 1255, 1278, 1301, 1322, 1342, 1361, 1380, 
1397, 1414, 1431, 1447, 1462, 1477, 1491, 1505, 
1518, 1531, 1544, 1556, 1568, 1579, 1591, 1602, 
1612, 1623, 1633, 1643, 1653, 1662, 1672, 1681, 
1690, 1698, 1707, 1716, 1724, 1732, 1740, 1748, 
1755, 1763, 1770, 1778, 1785, 1792, 1799, 1806, 
1812, 1819, 1826, 1832, 1838, 1845, 1851, 1857, 
1863, 1869, 1875, 1880, 1886, 1892, 1897, 1903, 
1908, 1913, 1919, 1924, 1929, 1934, 1939, 1944, 
1949, 1954, 1959, 1963, 1968, 1973, 1977, 1982, 
1986, 1991, 1995, 2000, 2004, 2008, 2012, 2017, 
2021, 2025, 2029, 2033, 2037, 2041, 2045, 2049, 
2053, 2056, 2060, 2064, 2068, 2071, 2075, 2079, 
2082, 2086, 2089, 2093, 2096, 2100, 2103, 2107, 
2110, 2113, 2117, 2120, 2123, 2127, 2130, 2133, 
2136, 2139, 2143, 2146, 2149, 2152, 2155, 2158, 
2161, 2164, 2167, 2170, 2173, 2176, 2178, 2181, 
2184, 2187, 2190, 2193, 2195, 2198, 2201, 2204, 
2206, 2209, 2212, 2214, 2217, 2220, 2222, 2225, 
2227, 2230, 2232, 2235, 2238, 2240, 2243, 2245, 
2247, 2250, 2252, 2255, 2257, 2260, 2262, 2264, 
2267, 2269, 2271, 2274, 2276, 2278, 2281, 2283, 
2285, 2287, 2290, 2292, 2294, 2296, 2298, 2301, 
2303, 2305, 2307, 2309, 2311, 2313, 2315, 2318, 
2320, 2322, 2324, 2326, 2328, 2330, 2332, 2334, 
2336, 2338, 2340, 2342, 2344, 2346, 2348, 2350, 
2352, 2354, 2356, 2357, 2359, 2361, 2363, 2365, 
2367, 2369, 2371, 2372, 2374, 2376, 2378, 2380, 
2382, 2383, 2385, 2387, 2389, 2390, 2392, 2394, 
2396, 2397, 2399, 2401, 2403, 2404, 2406, 2408
}; 

static uint32_t isdbt_get_snr(tNmiIsdbtChip cix, int combine)
{
	uint32_t usnr;
  uint32_t snr;
	//tNmiPriv	*pp = (tNmiPriv *)&chip.priv[cix];

	if (combine) {
		usnr = (uint8_t)(rReg32(cix, 0xac50) >> 24);
	} else {
		usnr = rReg32(cix, 0xa250) & 0x3ff;
		usnr >>= 2;
	}

	if (usnr == 0) {
		snr = 62;
	} else {
		snr = (50000 - 20 * log_10[usnr-1])/1000;
	}

	return snr;
}


#else
static double isdbt_get_snr(tNmiIsdbtChip cix, int combine)
{
	uint32_t usnr;
  	double snr;
	tNmiPriv	*pp = (tNmiPriv *)&chip.priv[cix];

	if (combine) {
		usnr = (uint8_t)(rReg32(cix, 0xac50) >> 24);
		usnr *= 4;
	} else {
		usnr = rReg32(cix, 0xa250) & 0x3ff;
		//usnr >>= 2;
	}
	if (usnr == 0) {
		snr = 62.;
	} else {
		snr = 50. - 20. * log10(usnr/4);
	}

	return snr;
}
#endif

#ifdef FIX_POINT
static uint32_t isdbt_get_doppler_frequency(tNmiIsdbtChip cix)
{
	uint32_t val32;
	uint32_t f = 0, w, doppler;
	tNmiGuard guard;
	tNmiIsdbtMode mode;

	val32 = rReg32(cix, 0xa048);
	guard = (val32 & 0x3);
	mode = (val32 >> 2) & 0x1;

	if (mode == nMode2) {
		switch (guard) {
		case nGuard1_4:
			f = 25;
			break;
		case nGuard1_8:
			f = 28;
			break;
		default:
			break;
		}
	} else if (mode == nMode3) {
		switch (guard) {
		case nGuard1_4:
			f = 12;
			break;
		case nGuard1_8:
			f = 14;
			break;
		case nGuard1_16:
			f = 15;
			break;
		default:
			break;
		}
	}

	val32 = rReg32(cix, 0xa414);
	w = (val32 & 0xff);

	doppler = f * (w - 5);

	return (doppler>>4); 
}

#else
static double isdbt_get_channel_length(tNmiIsdbtChip cix)
{
	uint32_t w, y;
	double cl;
	
	y = rReg32(cix, 0xa048);
	y = (y >> 2) & 0x1;

	w = rReg32(cix, 0xa268);
	cl = 0.6563 * w * (1 + y);

	return cl;
}

static double isdbt_get_doppler_frequency(tNmiIsdbtChip cix)
{
	uint32_t val32;
	double f = 0, w, doppler;
	tNmiGuard guard;
	tNmiIsdbtMode mode;

	val32 = rReg32(cix, 0xa048);
	guard = (val32 & 0x3);
	mode = (val32 >> 2) & 0x1;

	if (mode == nMode2) {
		switch (guard) {
		case nGuard1_4:
			f = 1.5501;
			break;
		case nGuard1_8:
			f = 1.7223;
			break;
		default:
			break;
		}
	} else if (mode == nMode3) {
		switch (guard) {
		case nGuard1_4:
			f = 0.775;
			break;
		case nGuard1_8:
			f = 0.8612;
			break;
		case nGuard1_16:
			f = 0.9118;
			break;
		default:
			break;
		}
	}

	val32 = rReg32(cix, 0xa414);
	w = (double)(val32 & 0xff);

	doppler = f * (w - 5);

	return doppler; 
}
#endif

static void isdbt_set_ber_timer(tNmiIsdbtChip cix, void *pv)
{
	uint32_t val32;
	tIsdbtBerTimer *tm = (tIsdbtBerTimer *)pv;

	if (tm->enable) {
		val32 = rReg32(cix, 0xab40);
		if (val32 & 0x2)
			val32 &= ~0x2;
		wReg32(cix, 0xab40, val32);
		val32 = tm->period & 0x3fffff;
		wReg32(cix, 0xab60, val32);
		val32 = rReg32(cix, 0xab40);
		val32 |= 0x2;
		wReg32(cix, 0xab40, val32);
 	} else {
		val32 = rReg32(cix, 0xab40);
		if (val32 & 0x2)
			val32 &= ~0x2;
		wReg32(cix, 0xab40, val32);
	}
	return;
}

static int isdbt_get_ber_before(tNmiIsdbtChip cix, void *pv)
{
	uint32_t 		bec;
  	uint32_t 		win;
	tIsdbtBer		*tber = (tIsdbtBer *)pv;
	uint32_t 		val32;
	int ret;

	val32 = rReg32(cix, 0xab40);
	if (val32 & 0x2) {
		val32 = rReg32(cix, 0xab50);
		if ((val32 >> 4) & 0x1) { 				/* overflow */
			tber->bec = 1;
			tber->becw = 1;
			ret = 1;
		} else {
			bec = rReg32(cix, 0xab6c);
			win = rReg32(cix, 0xab60);
			win <<= 8;
			nmi_debug(N_VERB, "BER(Before Viterbi), count (%d), windows (%d)\n", bec, win);

			tber->bec = bec;
			tber->becw = win;
			ret = 1;
		}		
	} else {
		ret = -1;
	}

	return ret;
}

static void isdbt_get_ber(tNmiIsdbtChip cix, void *pv)
{
	uint32_t 		bec;
  	uint32_t 		win;
	tIsdbtBer		*tber = (tIsdbtBer *)pv;
	int lock = isdbt_tmcc_lock(cix);

	if (lock) {
		bec 	= rReg32(cix, 0xabe8);
		win	= rReg32(cix, 0xabe4);
		win &= 0x7;
		win = 1 << (win + 3 + 10);
		nmi_debug(N_VERB, "BER, count (%d), windows (%d)\n", bec, win);

		tber->bec = bec;
		tber->becw = win;
	} else {
		tber->bec = 1;
		tber->becw = 1;
	}

	return;
}

static void isdbt_get_per(tNmiIsdbtChip cix, void *pv)
{
	uint32_t 		pec;
  	uint32_t 		win;
  	uint32_t 		val32;
	tIsdbtPer		*tper = (tIsdbtPer *)pv;
	int lock = isdbt_tmcc_lock(cix);
	
	if (lock) {
		val32 = rReg32(cix, 0xa834);
		win = val32 & 0xf;
		if (win == 0)
			win = 16;
		win = (1 << win) - 1; 
		pec = (val32 >> 16) & 0xffff;
		nmi_debug(N_VERB, "PER, count (%d), windows (%d)\n", pec, win);

		tper->pec = pec;
		tper->pecw = win;
    } else {
      tper->pec = 1;
      tper->pecw = 1;
    }
    
    return;
}

#ifndef FIX_POINT
static double isdbt_get_time_offset(tNmiIsdbtChip cix)
{
	uint32_t 	uoffset = 0;
	int			ioffset;
	double		doffset;

	wReg32(cix,0xa0d0, 1);
	uoffset = rReg32(cix, 0xa078);
	uoffset <<= 8;
	uoffset |= rReg32(cix, 0xa074);
	wReg32(cix,0xa0d0, 0);	

	ioffset = ((uoffset & (1 << 15)) == (1 << 15))? -1:0;
	ioffset &= ~((1 << 16)-1); 
	ioffset |= (uoffset & ((1 << 16)-1));

	doffset = (ioffset * 8.3458) / 1000;
	return doffset;
}

static double isdbt_get_frequency_offset(tNmiIsdbtChip cix)
{
	uint32_t	mode;
   	uint32_t 	uoffset = 0;
   	int 			ioffset;
   	double 	doffset;
    
	wReg32(cix,0xa0d0, 1);						
	uoffset = (rReg32(cix, 0xa088) << 16);		
	uoffset |= (rReg32(cix, 0xa084) << 8);		
	uoffset |= rReg32(cix, 0xa080);			
	wReg32(cix,0xa0d0, 0);						

	ioffset = ((uoffset & (1 << 23)) == (1 << 23)) ? -1 : 0;
	ioffset &= ~((1 << 24) - 1);
	ioffset |= (uoffset & ((1 << 24) - 1));

	doffset = (125000/63) * (double)ioffset/(1 << 14);

	mode = rReg32(cix, 0xa048);
	if ((mode >> 2) & 0x1) 
		doffset /= 2.;

	return doffset;
}
#endif

static void isdbt_software_reset(tNmiIsdbtChip cix)
{
	isdbtrfwrite(cix, 0x81, 0x63);
	wReg32(cix, 0xa14c, 0x31); 
	wReg32(cix, 0xa000, 0x00);
	isdbtrfwrite(cix, 0x81, 0x62);
	wReg32(cix, 0xa14c, 0x30); 
	wReg32(cix, 0xa000, 0x20);
}

static void isdbt_update_pll(tNmiIsdbtChip cix)
{
	uint32_t	val1, val2;
	//tNmiPriv	*pp = (tNmiPriv *)&chip.priv[cix];
#ifndef FIX_POINT
	double xo = chip.inp.xo;
	double d;
#endif
	uint32_t wait;


	val1 = rReg32(cix,0x6448);
	val1 |= (1 << 15);
	wReg32(cix,0x6448,val1);
	//nmi_delay(1);

	val2 = rReg32(cix, 0x6418);
	val2 |= (0x01 << 14);
	wReg32(cix, 0x6418, val2);

#ifndef FIX_POINT
	d = (32./xo) * 5.;
	wait = (uint32_t)d + 1;
#else
	wait = 5;
#endif

#ifdef PC_ENV
	/**
		emulate delay
	**/
	{
		int i, ix;
		if (cix == nMaster)
			ix = nSlave;
		else
			ix = nMaster;
		for (i = 0; i < (int)((30/5)*wait); i++)
			rReg32(ix, 0x6400);
	}
#else
	nmi_delay(wait);
#endif
	val2 &= ~(0x01 << 14);
	wReg32(cix, 0x6418, val2);

	val1 &= ~(1 << 15);
	wReg32(cix,0x6448,val1);

	return;
}

static void isdbt_check_pll_reset(tNmiIsdbtChip cix)
{
	//tNmiPriv		*pp = (tNmiPriv *)&chip.priv[cix];
	uint8_t val8;

	val8 = isdbtrfread(cix, 0xfc);
	nmi_debug(N_VERB, "0xfc (%02x)\n", val8);

	if (val8 & 0x1) {
		isdbtrfwrite(cix, 0xfc, 0x1); 
		isdbt_update_pll(cix);
	}
}

static void isdbt_config_demod(tNmiIsdbtChip cix) 
{
	tNmiIsdbtOpMode opmode = chip.inp.op;
	//tNmiPriv		*pp = (tNmiPriv *)&chip.priv[cix];
	uint32_t		/*frequency = pp->frequency,*/ val32;

	/**
		turn off demod
	**/
	isdbtrfwrite(cix, 0x81, 0x63);
	wReg32(cix, 0xa14c, 0x31);
	wReg32(cix, 0xa000, 0x00);
	if ((opmode == nDiversity) || (opmode == nPip)) {
		/**
			turn on combiner
		**/
		val32 = rReg32(cix, 0xac40);
		val32 |= (1 << 0);
		val32 &= ~(1 << 2);
		wReg32(cix, 0xac40, val32);

		val32 = 0x1a0104;
		wReg32(cix, 0xad00, val32);
		val32 = 0x1a0104;
		wReg32(cix, 0xad08, val32);

		val32 = rReg32(cix, 0x641c);
		val32 |= (1 << 13) | (1 << 14);
		wReg32(cix, 0x641c, val32);

	}
	/**
		turn on demod
	**/
	isdbtrfwrite(cix, 0x81, 0x62);
	wReg32(cix, 0xa14c, 0x30);
	wReg32(cix, 0xa000, 0x20);
	return;
}

/******************************************************************************
**
**	Decoder Functions
**
*******************************************************************************/
static void isdbt_spi_dma(int enable)
{
	uint32_t tso, fsm;
	int overflow = 0;
	/**
		shutdown dmod
	**/
	isdbtrfwrite(nMaster, 0x81, 0x63);
	wReg32(nMaster, 0xa14c, 0x31);
	wReg32(nMaster, 0xa000, 0x00);

	/**
		disable the Dma
	**/
	tso = rReg32(nMaster, 0xa804);
	tso &= ~(1 << 8);
	wReg32(nMaster, 0xa804, tso);

	/**
		make sure the decoder is not in the active state
	**/
	do {
		fsm = rReg32(nMaster, 0xa80c);
		if (((fsm >> 12) & 0x3) == 0x2) {
			uint32_t tsz, adr, esz;

			/**
				in active state, clear it
			**/
			tsz = (tso >> 10) & 0x7f;
			tsz *= 188;

			/**
				read and calculate the starting address from the decoder.
			**/
			adr = rReg32(nMaster, 0xa83c);
			adr *= 4;
			adr += 0x60050;

			/**
				claculate the size
			**/
			esz = 0x6204c - adr + 4;

			/**
				read the last 4 bytes
			**/
			if (esz < tsz) {
				tsz -= esz;
				adr = 0x60050 + tsz - 4;
			} else {
				adr += tsz - 4;					
			} 
			wReg32(nMaster, 0x7864, adr);
			wReg32(nMaster, 0x7868, 4);

			overflow = 1;
			/**
				read the last 4 bytes
			**/
			nmi_burst_read((void*)&overflow);

		} else
			break;
	} while (1);

	/**
		enable the Dma if set so
	**/
	if (enable) {
		tso |= (1 << 8);
		wReg32(nMaster, 0xa804, tso);
	} 

	/**
		let go demod
	**/
	isdbtrfwrite(nMaster, 0x81, 0x62);
	wReg32(nMaster, 0xa14c, 0x30);
	wReg32(nMaster, 0xa000, 0x20);

	return;
}

static void isdbt_ts_ctl(int enable)
{
	uint32_t val1, val2, val3;

	if (enable) {
		val1 = rReg32(nMaster, 0xa804);
		val2 = rReg32(nMaster, 0x6448);
		val3 = rReg32(nMaster, 0x641c);
		if ((val1 >> 1) & 0x1) {		/* parallel TS */
			val3 |= ((1 << 7) | (1 << 8) | (1 << 9) | (1 << 10) | (1 << 11) | (1 << 12));
			val2 &= ~(1 << 23); 
		} else {		/* serial TS */
			val3 |= ((1 << 8) | (1 << 9) | (1 << 10) | (1 << 11) | (1 << 12));
			val2 &= ~(1 << 22);
		}
		wReg32(nMaster, 0x641c, val3);
		wReg32(nMaster, 0x6448, val2);
		val1 |= 0x1;
		wReg32(nMaster, 0xa804, val1);
	} else {
		val1 = rReg32(nMaster, 0xa804);
		val2 = rReg32(nMaster, 0x6448);
		val3 = rReg32(nMaster, 0x641c);
		if ((val1 >> 1) & 0x1) {		/* parallel TS */
			val3 &= ~((1 << 7) | (1 << 8) | (1 << 9) | (1 << 10) | (1 << 11) | (1 << 12));
			val2 |= (1 << 23); 
		} else {		/* serial TS */
			val3 &= ~((1 << 8) | (1 << 9) | (1 << 10) | (1 << 11) | (1 << 12));
			val2 |= (1 << 22);
		}
		wReg32(nMaster, 0x641c, val3);
		wReg32(nMaster, 0x6448, val2);
		val1 &= ~0x1;
		wReg32(nMaster, 0xa804, val1);
	}
	nmi_debug(N_VERB, "0xa804 (%08x)\n", rReg32(nMaster, 0xa804));
}

static void isdbt_ts_cfg(void *pv)
{
	uint32_t val32;
	tIsdbtTso *ts = (tIsdbtTso *)pv;

	if (ts == NULL)
		return;

	/**
		TSO Master 
	**/
	val32 = rReg32(nMaster, 0xa838);
	if (ts->datapol) {		/* data polarity */
		val32 |= 0x1;			/* inverted */
	} else {
		val32 &= ~0x1;		/* not inverted */
	}

	if (ts->validpol) {		/* valid polarity */
		val32 |= (0x1 << 1);	/* active low */	
	} else {
		val32 &= ~(0x1 << 1);	/* active high */
	}

	if (ts->syncpol) {		/* sync polarity */
		val32 |= (0x1 << 2);	/* active low */	
	} else {
		val32 &= ~(0x1 << 2);	/* active high */
	}

	if (ts->errorpol) {		/* error polarity */
		val32 |= (0x1 << 3);	/* active low */	
	} else {
		val32 &= ~(0x1 << 3);	/* active high */
	}

	if (ts->valid204) {		/* valid for parity */
		val32 |= (0x1 << 4);	/* enable */	
	} else {
		val32 &= ~(0x1 << 4);	/* disable */
	}

	if (ts->clk204) {		/* clock for parity */
		val32 |= (0x1 << 5);	/* enable */
	} else {
		val32 &= ~(0x1 << 5);	/* disable */
	}

	if (ts->parityen) {		/* parity bytes */
		val32 |= (0x1 << 8);	/* enable */
	} else {
		val32 &= ~(0x1 << 8);	/* disable */
	}

	if (ts->nullsuppress) {
		val32 |= (0x1 << 10);
	} else {
		val32 &= ~(0x1 << 10);
	}

	wReg32(nMaster, 0xa838, val32);
	nmi_debug(N_VERB, "0xa838 (%08x)\n", rReg32(nMaster, 0xa838));
 
	val32 = rReg32(nMaster, 0xa804);
	val32 &= ~0x1;						/* disable TS */
	if (ts->tstype == nSerial) {
		val32 &= ~(0x1 << 1);		/* serial */
	} else {
		val32 |= (0x1 << 1);		/* parallel */
	}
	val32 &= ~(0x7 << 2);
	val32 |= (ts->clkrate & 0x7) << 2;
	if (ts->clkedge)
		val32 |= (0x1 << 5);		/* falling */
	else
		val32 &= ~(0x1 << 5);		/* rising */
	if (ts->gatedclk)
		val32 |= (0x1 << 6);		/* gated clock */
	else
		val32 &= ~(0x1 << 6);		/* free run */
	if (ts->lsbfirst)						
		val32 |= (0x1 << 7);		/* LSB first */
	else
		val32 &= ~(0x1 << 7);		/* MSB first */
	if (ts->dmaen)
		val32 |= (0x1 << 8);
	else
		val32 &= ~(0x1 << 8);			/* dma disable */
	val32 &= ~(0x7f << 10);		/* clear block size */
	val32 |= (ts->blocksize << 10);
	if (chip.inp.dco == 0) {					/* remux enable */
		val32 &= ~(1 << 19);
	} else {
		val32 |= (1 << 19);			/* remux disable */
	}
	if (ts->nullen)
		val32 |= (0x1 << 20);		/* null inserted */
	else
		val32 &= ~(0x1 << 20);	/* no null insertion */

	if (ts->dropbadts)
		val32 |= (0x1 << 21);		/* drop RS failed TSP */
	else
		val32 &= ~(0x1 << 21);	/* don't drop */
	val32 &= ~(0x1 << 24);		/* disable test */
	wReg32(nMaster, 0xa804, val32);
	nmi_debug(N_VERB, "0xa804 (%08x)\n", rReg32(nMaster, 0xa804));


	if (ts->mode == nPip) {
		/**
			These are fixed...
		**/
		val32 = rReg32(nSlave, 0xa804);
		val32 |= 0x1;
		val32 &= ~(1 << 1);			/* serial mode */
		val32 &= ~(0x7 << 2);
		val32 |= (0x1 << 2);		/* 4 MHz */ 
		val32 &= ~(0x1 << 5);
		val32 |= (0x1 << 6);
		val32 |= (0x1 << 7);
		wReg32(nSlave, 0xa804, val32);
		/**
			Turn on TS merging
		**/
		wReg32(nSlave, 0xac40, 0);
		wReg32(nMaster, 0xac40, 0);
		wReg32(nSlave, 0xac40, 0x3);
		wReg32(nMaster, 0xac40, 0x3);

		val32 = rReg32(nMaster, 0xa800);
		val32 |= (1 << 30);
		wReg32(nMaster, 0xa800, val32);
		val32 = rReg32(nSlave, 0xa800);
		val32 &= ~(1 << 30);
		wReg32(nSlave, 0xa800, val32);
	}

	return;
}

static uint32_t tsremux[20][10] = {
	{0x20820820, 0x8208208, 0x82082082, 0x20820820, 0x8208, 0, 0, 0, 0, 0},
	{0x84422110, 0x44221108, 0x42211088, 0x22110884, 0x8844, 0, 0, 0, 0, 0},
	{0x24924924, 0x49249249, 0x92492492, 0x24924924, 0x9249, 0, 0, 0, 0, 0},
	{0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaa, 0, 0, 0, 0, 0},
	{0x4081040, 0x8104082, 0x10408204, 0x40820408, 0x82040810, 0, 0, 0, 0, 0},
	{0x21084210, 0x8421084, 0x42108421, 0x10842108, 0x84210842, 0, 0, 0, 0, 0},
	{0x24491248, 0x49124892, 0x12489224, 0x48922449, 0x92244912, 0, 0, 0, 0, 0},
	{0x554a9962, 0x4a9952aa, 0x9952aa55, 0x52aa554a, 0xaa554a99, 0, 0, 0, 0, 0},
	{0x10210420, 0x21041082, 0x2104208, 0x10410821, 0x21042082, 0x4108210, 0x10420821, 0x41082102, 0x8210, 0},
	{0x22111110, 0x84444222, 0x21111108, 0x44442222, 0x11111088, 0x44422222, 0x11110884, 0x44222221, 0x8844, 0},
	{0x52292524, 0x25251292, 0x22925249, 0x52512925, 0x29252492, 0x25129252, 0x92524925, 0x51292522, 0x9252, 0},
	{0x3335566a, 0x335556ab, 0x335566ab, 0x35556ab3, 0x35566ab3, 0x5556ab33, 0x5566ab33, 0x556ab333, 0xb335, 0},
	{0x20820820, 0x8208208, 0x82082082, 0x20820820, 0x8208208, 0x82082082, 0x20820820, 0x8208208, 0x82082082, 0},
	{0x84221110, 0x44221088, 0x22111084, 0x22108884, 0x11108444, 0x10888422, 0x10844422, 0x88842211, 0x84442210, 0},
	{0x24924924, 0x49249249, 0x92492492, 0x24924924, 0x49249249, 0x92492492, 0x24924924, 0x49249249, 0x92492492, 0},
	{0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0},
	{0x2081040, 0x8102082, 0x10404104, 0x20820208, 0x41040810, 0x2081040, 0x8102082, 0x10404104, 0x20820208, 0x41040810},
	{0x20884210, 0x8421084, 0x42104421, 0x10842088, 0x44210842, 0x20884210, 0x8421084, 0x42104421, 0x10842088, 0x44210842},
	{0x22489248, 0x49122492, 0x92484924, 0x24922248, 0x49244912, 0x22489248, 0x49122492, 0x92484924, 0x24922248, 0x49244912},
	{0x52ca594a, 0x299532a6, 0x594a694d, 0x32a652ca, 0x694d2995, 0x52ca594a, 0x299532a6, 0x594a694d, 0x32a652ca, 0x694d2995}
};

static void isdbt_config_decoder(tNmiIsdbtChip cix, int hastmcc, tIsdbtTmccInfo *tmcc)
{
	tNmiIsdbtMode		mode;
	tNmiGuard				guard;
	tNmiMod					modulation;
	tNmiCodeRate			coderate;
	int 							index = -1;
	tNmiPriv *pp = (tNmiPriv *)&chip.priv[cix];
	tIsdbtTmccInfo		tmcci;
	uint32_t comb;
	uint32_t pkterr;
	uint32_t val32;
	tIsdbtBerTimer bbv; 			

	if ((chip.inp.dco == 2) && (chip.inp.op == nSingle))
		return;

	if (hastmcc) {
		pp->mode = mode = tmcc->mode;
		pp->guard = guard = tmcc->guard;
		pp->modulation = modulation = tmcc->modulation;
		pp->coderate = coderate = tmcc->coderate;	
	} else {
		memset((void *)&tmcci, 0, sizeof(tIsdbtTmccInfo));
		isdbt_get_tmcc_info(cix, (void *)&tmcci);
		pp->mode = mode = tmcci.mode;
		pp->guard = guard = tmcci.guard;
		pp->modulation = modulation = tmcci.modulation;
		pp->coderate = coderate = tmcci.coderate;	
	}

	/**
		Set the auto reset tracking thresholds.
		1. single mode enable PER based auto reset,
		2. diversity mode disable PER based auto reset.
	**/
	comb = rReg32(cix, 0xac40);
	if (!(comb & 0x1)) {		/* Combiner Off */
		if (modulation == nQPSK) {
			if (coderate == nRate1_2) {
				pkterr = 0x68;
			} else if (coderate == nRate2_3) {
				pkterr = 0x8a;
			} else {
				pkterr = 0xff;
			}
		} else if (modulation == nQAM16) {
			if (coderate == nRate1_2)
				pkterr = 0xd0;
			else 
				pkterr = 0xff;
		} else if (modulation == nQAM64) {
			if (coderate == nRate1_2)
				pkterr = 0x138;
			else
				pkterr = 0xff;
		} else {
			pkterr = 0x8a;
			nmi_debug(N_ERR, "Error, bad modulation (%d), check Tmcc lock...\n", modulation);
		}

		wReg32(cix, 0xa0d8, (uint8_t)pkterr);
		val32 = rReg32(cix, 0xa0dc);
		val32 &= ~(0x7);
		val32 |= (uint8_t)(pkterr >> 8);
		wReg32(cix, 0xa0dc, val32);

	} else {	/* Combiner On */
		if (chip.inp.op == nDiversity) {	/* ykk */
			val32 = rReg32(cix, 0xa0dc);
			val32 &= ~(1 << 5);
			wReg32(cix, 0xa0dc, val32);
		}	
	}

	/**
		Set and Enable PER & BER
	**/
	if (modulation == nQPSK) {
		wReg32(cix, 0xa230,  0xff);
		wReg32(cix, 0xac44,  0xff);
		wReg32(cix, 0xa834,  0x18);		/* PER */
		wReg32(cix, 0xabe4,  0x21d);		/* BER */
	} else if (modulation == nQAM16) {
		wReg32(cix, 0xa230,  0x80);
		wReg32(cix, 0xac44,  0x80);
		wReg32(cix, 0xa834,  0x19);
		wReg32(cix, 0xabe4,  0x21e);

	} else if (modulation == nQAM64) {
		wReg32(cix, 0xa230,  0x40);
		wReg32(cix, 0xac44,  0x40);
		wReg32(cix, 0xa834,  0x1a);
		wReg32(cix, 0xabe4,  0x21f);
	} else {
		nmi_debug(N_ERR, "Error, bad modulation (%d), Use Qpsk...\n", modulation);
		wReg32(cix, 0xa230,  0xff);
		wReg32(cix, 0xac44,  0xff);
		wReg32(cix, 0xa834,  0x18);		/* PER */
		wReg32(cix, 0xabe4,  0x21d);		/* BER */
	}	

	/**
		Set the BER timer (before Viterbi)
	**/
	if (coderate == nRate1_2) {
		val32 = rReg32(cix, 0xabe4);
		val32 &= 0x7;
		bbv.enable = 1;
		bbv.period = ((1 << (10 + val32)) * 2) >> 8;
		isdbt_set_ber_timer(cix, (void *)&bbv);
	} else if (coderate == nRate2_3) {
		val32 = rReg32(cix, 0xabe4);
		val32 &= 0x7;
		bbv.enable = 1;
		bbv.period = ((1 << (10 + val32)) * 3) >> 9;
		isdbt_set_ber_timer(cix, (void *)&bbv);
	} else {
		bbv.enable = 0;
		isdbt_set_ber_timer(cix, (void *)&bbv);
	} 

	/**
		TS Remux
	**/
	if (chip.inp.dco == 0) {
		if ((mode == nMode2) && (guard == nGuard1_8) && (modulation == nQPSK) && (coderate == nRate1_2)) {
			index = 0;
		} else if ((mode == nMode2) && (guard == nGuard1_8) && (modulation == nQPSK) && (coderate == nRate2_3)) {
			index = 1;
		} else if ((mode == nMode2) && (guard == nGuard1_8) && (modulation == nQAM16) && (coderate == nRate1_2)) {
			index = 2;
		} else if ((mode == nMode2) && (guard == nGuard1_8) && (modulation == nQAM64) && (coderate == nRate1_2)) {
			index = 3;
		} else if ((mode == nMode2) && (guard == nGuard1_4) && (modulation == nQPSK) && (coderate == nRate1_2)) {
			index = 4;
		} else if ((mode == nMode2) && (guard == nGuard1_4) && (modulation == nQPSK) && (coderate == nRate2_3)) {
			index = 5;
		} else if ((mode == nMode2) && (guard == nGuard1_4) && (modulation == nQAM16) && (coderate == nRate1_2)) {
			index = 6;
		} else if ((mode == nMode2) && (guard == nGuard1_4) && (modulation == nQAM64) && (coderate == nRate1_2)) {
			index = 7;
		} else if ((mode == nMode3) && (guard == nGuard1_16) && (modulation == nQPSK) && (coderate == nRate1_2)) {
			index = 8;
		} else if ((mode == nMode3) && (guard == nGuard1_16) && (modulation == nQPSK) && (coderate == nRate2_3)) {
			index = 9;
		} else if ((mode == nMode3) && (guard == nGuard1_16) && (modulation == nQAM16) && (coderate == nRate1_2)) {
			index = 10;
		} else if ((mode == nMode3) && (guard == nGuard1_16) && (modulation == nQAM64) && (coderate == nRate1_2)) {
			index = 11;
		} else if ((mode == nMode3) && (guard == nGuard1_8) && (modulation == nQPSK) && (coderate == nRate1_2)) {
			index = 12;
		} else if ((mode == nMode3) && (guard == nGuard1_8) && (modulation == nQPSK) && (coderate == nRate2_3)) {
			index = 13;
		} else if ((mode == nMode3) && (guard == nGuard1_8) && (modulation == nQAM16) && (coderate == nRate1_2)) {
			index = 14;
		} else if ((mode == nMode3) && (guard == nGuard1_8) && (modulation == nQAM64) && (coderate == nRate1_2)) {
			index = 15;
		} else if ((mode == nMode3) && (guard == nGuard1_4) && (modulation == nQPSK) && (coderate == nRate1_2)) {
			index = 16;
		} else if ((mode == nMode3) && (guard == nGuard1_4) && (modulation == nQPSK) && (coderate == nRate2_3)) {
			index = 17;
		} else if ((mode == nMode3) && (guard == nGuard1_4) && (modulation == nQAM16) && (coderate == nRate1_2)) {
			index = 18;
		} else if ((mode == nMode3) && (guard == nGuard1_4) && (modulation == nQAM64) && (coderate == nRate1_2)) {
			index = 19;
		} else {
			nmi_debug(N_ERR, "Error, bad Tmcc info, check TMCC lock...\n"); 
		} 

		if (index >= 0) {
			wReg32(cix, 0xa8b8, tsremux[index][0]);
			wReg32(cix, 0xa8bc, tsremux[index][1]);
			wReg32(cix, 0xa8c0, tsremux[index][2]);
			wReg32(cix, 0xa8c4, tsremux[index][3]);
			wReg32(cix, 0xa8c8, tsremux[index][4]);
			wReg32(cix, 0xa8cc, tsremux[index][5]);
			wReg32(cix, 0xa8d0, tsremux[index][6]);
			wReg32(cix, 0xa8d4, tsremux[index][7]);
			wReg32(cix, 0xa8d8, tsremux[index][8]);
			wReg32(cix, 0xa8dc, tsremux[index][9]);
		} 
	}

	val32 = rReg32(cix, 0xa800);
	val32 |= 0x1006;
	wReg32(cix, 0xa800, val32);

	return;
}

/******************************************************************************
**
**	Tracking Functions
**
*******************************************************************************/

static uint32_t isdbt_get_dagc(tNmiIsdbtChip cix)
{
	uint32_t	agc = 0;

	/**
		write to freeze the AGC status readings 
	**/
	agc = rReg32(cix, 0xa920);
	agc &= 0x1ffffff;

	return agc;
}

#ifndef FIX_POINT
static void isdbt_set_agc_thold(tNmiIsdbtChip cix, void *pv)
{
	//tNmiPriv *pp = (tNmiPriv *)&chip.priv[cix];
	tIsdbtAgcThold *p = (tIsdbtAgcThold *)pv;
	uint32_t val32;
	
	val32 = (uint32_t)(p->by2lo_4 * 4);
	isdbtrfwrite(cix, 0xdd, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xde, (uint8_t)(val32 & 0xff));

	val32 = (uint32_t)(p->lo2mi_4 * 4);
	isdbtrfwrite(cix, 0xdf, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xe0, (uint8_t)(val32 & 0xff));

	val32 = (uint32_t)(p->mi2hi_4 * 4);
	isdbtrfwrite(cix, 0xe1, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xe2, (uint8_t)(val32 & 0xff));

	val32 = (uint32_t)(p->lo2by_4 * 4);
	isdbtrfwrite(cix, 0xe3, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xe4, (uint8_t)(val32 & 0xff));

	val32 = (uint32_t)(p->mi2lo_4 * 4);
	isdbtrfwrite(cix, 0xe5, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xe6, (uint8_t)(val32 & 0xff));

	val32 = (uint32_t)(p->hi2mi_4 * 4);
	isdbtrfwrite(cix, 0xe7, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xe8, (uint8_t)(val32 & 0xff));

	val32 = (uint32_t)(p->lo2by_b_4 * 4);
	isdbtrfwrite(cix, 0xe9, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xea, (uint8_t)(val32 & 0xff));

	val32 = (uint32_t)(p->mi2lo_b_4 * 4);
	isdbtrfwrite(cix, 0xeb, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xec, (uint8_t)(val32 & 0xff));

	val32 = (uint32_t)(p->hi2lo_b_4 * 4);
	isdbtrfwrite(cix, 0xed, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xee, (uint8_t)(val32 & 0xff));

	val32 = (uint32_t)(p->hi2lo_b2_4 * 4);
	isdbtrfwrite(cix, 0xef, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xf0, (uint8_t)(val32 & 0xff));

	return;
	
}
#endif

static void isdbt_bb_gain_code(tNmiIsdbtChip cix, tIsdbtBbGain *bb)
{
	uint32_t val1;

	val1 = rReg32(cix, 0xa920);
	val1 >>= 25;
	val1 &= 0x1f;

	switch (val1) {
		case 16:
			bb->code = 0x70;
#ifdef FIX_POINT
			bb->level = 40;
#else
			bb->level = 39.9;
#endif
			break;
		case 15:
			bb->code = 0x6f;
#ifdef FIX_POINT
			bb->level = 35;
#else
			bb->level = 34.5;
#endif
			break;
		case 14:
			bb->code = 0x6e;
#ifdef FIX_POINT
			bb->level = 27;
#else
			bb->level = 27.4;
#endif
			break;
		case 13:
			bb->code = 0x6d;
#ifdef FIX_POINT
			bb->level = 25;
#else
			bb->level = 25.4;
#endif
			break;
		case 12:
			bb->code = 0x6c;
#ifdef FIX_POINT
			bb->level = 23;
#else
			bb->level = 22.6;
#endif
			break;
		case 11:
			bb->code = 0x6b;
#ifdef FIX_POINT
			bb->level = 21;
#else
			bb->level = 20.7;
#endif
			break;
		case 10:
			bb->code = 0x6a;
#ifdef FIX_POINT
			bb->level = 19;
#else
			bb->level = 19.2;
#endif
			break;
		case 9:
			bb->code = 0x69;
#ifdef FIX_POINT
			bb->level = 17;
#else
			bb->level = 16.6;
#endif
			break;
		case 8:
			bb->code = 0x68;
#ifdef FIX_POINT
			bb->level = 13;
#else
			bb->level = 13.1;
#endif
			break;
		case 7:
			bb->code = 0x67;
#ifdef FIX_POINT
			bb->level = 10;
#else
			bb->level = 9.9;
#endif
			break;
		case 6:
			bb->code = 0x66;
#ifdef FIX_POINT
			bb->level = 8;
#else
			bb->level = 7.8;
#endif
			break;
		case 5:
			bb->code = 0x65;
#ifdef FIX_POINT
			bb->level = 5;
#else
			bb->level = 4.5;
#endif
			break;
		case 4:
			bb->code = 0x64;
#ifdef FIX_POINT
			bb->level = 2;
#else
			bb->level = 2.3;
#endif
			break;
		case 3:
			bb->code = 0x63;
#ifdef FIX_POINT
			bb->level = -1;
#else
			bb->level = -0.9;
#endif
			break;
		case 2:
			bb->code = 0x62;
#ifdef FIX_POINT
			bb->level = -4;
#else
			bb->level = -3.5;
#endif
			break;
		case 1:
			bb->code = 0x61;
#ifdef FIX_POINT
			bb->level = -7;
#else
			bb->level = -6.9;
#endif
			break;
		case 0:
		default:
			bb->code = 0x60;
#ifdef FIX_POINT
			bb->level = -12;
#else
			bb->level = -11.9;
#endif
			break;
	}

	return;
}

static void isdbt_set_gain(tNmiIsdbtChip cix, tNmiIsdbtLnaGain gain)
{
	uint8_t val;
	tNmiPriv *pp = (tNmiPriv *)&chip.priv[cix];

	val = isdbtrfread(cix, 0xf3);
	if (gain == nBypass) {
		if (!(val & 0x1))
			val |= 0x1;
		val &= ~(0x3 << 1);
		pp->notrack = 1;		
	} else if (gain == nLoGain) {
		if (!(val & 0x1))
			val |= 0x1;
		val &= ~(0x3 << 1);
		val |= (0x1 << 1);
		pp->notrack = 1;
	} else if (gain == nMiGain) {
		if (!(val & 0x1))
			val |= 0x1;
		val &= ~(0x3 << 1);
		val |= (0x2 << 1);
		pp->notrack = 1;
	} else if (gain == nHiGain) {
		if (!(val & 0x1))
			val |= 0x1;
		val &= ~(0x3 << 1);
		val |= (0x3 << 1);
		pp->notrack = 1;
	} else {
		if (val & 0x1)
			val &= ~0x1;
		pp->notrack = 0;		
	}
	isdbtrfwrite(cix, 0xf3, val);
}

#ifdef FIX_POINT
static void isdbt_get_rssi(tNmiIsdbtChip cix, void *pv)
{
	//tNmiPriv *pp =	(tNmiPriv *)&chip.priv[cix];
	tIsdbtGainInfo *info = (tIsdbtGainInfo *)pv;
	uint32_t val32, agc, code;
	int rssi = 0, drflna, dbblna;
	tNmiIsdbtLnaGain lnag;
	tIsdbtBbGain bbgain;

	/**
		Offset DB
	**/
	val32 = rReg32(cix, 0xa920);
	val32 >>= 30;
	val32 &= 0x3;
	if (val32 == 0x3) {
		drflna = 12;
		lnag = nHiGain;
	} else if (val32 == 0x2) {
		drflna = 10;
		lnag = nMiGain;
	} else if (val32 == 0x1) {
		drflna =  4;
		lnag = nLoGain;
	} else if (val32 == 0x0) {
		drflna = -3;
		lnag = nBypass;
	} else {
		nmi_debug(N_ERR, "Error, invalid gain code (%d), use Bypass...\n", val32);
		drflna = -3;
		lnag = nBypass;
	}

	/**
		DAGC GAIN DB
	**/
	agc = isdbt_get_dagc(cix);

	/**
		BB LNA DB
	**/
	isdbt_bb_gain_code(cix, &bbgain);
	code = bbgain.code;
	dbblna = bbgain.level;

	/**
		RSSI
	**/
	{
		val32 = rReg32(cix, 0xa924);
		nmi_debug(N_VERB, "0xa924 (%08x)\n", val32);
		
		if ((val32 >> 10) & 0x1) {
			rssi = (int)val32 - (1 << 11);
		} else {                                             // tony, it should be added.
           rssi = (int)val32;               
       }

		/**
			The format is S7.3
		**/
		rssi >>= 3;
		rssi -= 40;			/* subtract chip offset */ 
	}

	/**
		Info
	**/
	info->rssi = rssi;
	info->dagc = agc;
	info->rflna = lnag;
	info->rflnadb = (int)drflna;
	info->bblnadb = (int)dbblna;
	info->bblnacode = code;  
	
	return;
}

#else
static void isdbt_get_rssi(tNmiIsdbtChip cix, void *pv)
{
	tNmiPriv *pp =	(tNmiPriv *)&chip.priv[cix];
	tIsdbtGainInfo *info = (tIsdbtGainInfo *)pv;
	uint32_t val32, agc, code;
	double dbblna, drflna, doffset, dagc, rssi;
	tNmiIsdbtLnaGain lnag;
	tIsdbtBbGain bbgain;

	/**
		Offset DB
	**/
	val32 = rReg32(cix, 0xa920);
	val32 >>= 30;
	val32 &= 0x3;
	if (val32 == 0x3) {
		doffset = 4.0;
		drflna = 12.2;
		lnag = nHiGain;
	} else if (val32 == 0x2) {
		doffset = 4.0;
		drflna = 9.9;
		lnag = nMiGain;
	} else if (val32 == 0x1) {
		doffset = 3.5;
		drflna =  3.9;
		lnag = nLoGain;
	} else if (val32 == 0x0) {
		doffset = 1.0;
		drflna = -2.9;
		lnag = nBypass;
	} else {
		nmi_debug(N_ERR, "Error, invalid gain code (%d), use Bypass...\n", val32);
		doffset = 1.0;
		drflna = -2.9;
		lnag = nBypass;
	}

	/**
		DAGC GAIN DB
	**/
	agc = isdbt_get_dagc(cix);
	if (agc > 0)	
		dagc = 20. * log10((double)agc/1024);
	else 
		dagc = 0;

	/**
		BB LNA DB
	**/
	isdbt_bb_gain_code(cix, &bbgain);
	code = bbgain.code;
	dbblna = bbgain.level;

	/**
		RSSI
	**/
	{
		int neg = 0;
		val32 = rReg32(cix, 0xa924);
		nmi_debug(N_VERB, "0xa924 (%08x)\n", val32);
		/**
			2's complement
		**/
		if ((val32 >> 10) & 0x1) {
			val32 ^= 0x7ff;
			val32 += 1;
			neg = 1;
		}

		/**
			The format is S7.3
		**/
		rssi = (val32 & 0x1) * 0.125 + ((val32 >> 1) & 0x1) * 0.25 + ((val32 >> 2) & 0x1) * 0.5 + ((val32 >> 3) & 0x7f);
		if (neg)
			rssi = -1 * rssi;
		rssi -= 40.;			/* subtract chip offset */ 
	}

	/**
		Info
	**/
	info->rssi = rssi;
	info->dagc = agc;
	info->rflna = lnag;
	info->rflnadb = (int)drflna;
	info->bblnadb = (int)dbblna;
	info->bblnacode = code;  
	
	return;
}
#endif

#ifndef FIX_POINT
static void isdbt_get_acq_time(tNmiIsdbtChip cix, void *pv)
{
	tIsdbtAcqTime	*acq	= (tIsdbtAcqTime *)pv;
	uint32_t val32;
	double factor = 64./63.;

	val32 = rReg32(cix,0xa8e0);
	acq->agcacq = (double)(val32/(2000. * factor));
	val32 = rReg32(cix,0xa8e4);
	acq->syracq = (double)(val32/(2000. * factor));
	val32 = rReg32(cix,0xa8e8);
	acq->ppmacq = (double)(val32/(2000. * factor));
	val32 = rReg32(cix,0xa8ec); 
	acq->tmccacq	 = (double)(val32/(2000. * factor));
	val32 = rReg32(cix,0xa8f0);
	acq->tsacq = (double)(val32/(2000. * factor));
	acq->totalacq		= acq->agcacq + acq->syracq + acq->ppmacq + acq->tmccacq + acq->tsacq;
}
#endif

/******************************************************************************
**
**	Scan Functions
**
*******************************************************************************/
#ifndef FIX_POINT
static double isdbt_scan_time(void)
{
	uint32_t val32;
	double tim;
	val32 = rReg32(nMaster, 0xa8f4);
	nmi_debug(N_VERB, "0xa8f4 (%08x)\n", val32);
	tim = (double)(val32/2000.);
	
	return tim;
}
#endif

static int isdbt_scan_poll(void *pv)
{
	uint32_t val32, stim, val1 = 0, val2 = 0;
	int lock = 0, mlock = -1, slock = -1;

	/**
		disable AGC wait
	**/
	wReg32(nMaster, 0xa640, 0x0);

	/**
		Increase RFLNA Bandwidth during scan
	**/
	isdbtrfwrite(nMaster, 0xf6, 0x20);
	isdbtrfwrite(nMaster, 0xf7, 0x4E);
	isdbtrfwrite(nMaster, 0xf8, 0x00);
	/**
		reset the demod
	**/
	isdbtrfwrite(nMaster, 0x81, 0x63);
	wReg32(nMaster, 0xa14c, 0x31);
	wReg32(nMaster, 0xa000, 0x0);
	if ((chip.inp.op == nDiversity) || (chip.inp.op == nPip)) {
		isdbtrfwrite(nSlave, 0x81, 0x63);
		wReg32(nSlave, 0xa14c, 0x31);
		wReg32(nSlave, 0xa000, 0x0);
	}

	/**
		clear the no signal or detected signal interrupt
	**/
	val32 = rReg32(nMaster, 0xa818);
	val32 |= (1 << 26) | (1 << 28);
	wReg32(nMaster, 0xa818, val32);
	//val32 = rReg32(nMaster, 0xa818);
	//nmi_debug(N_VERB, "poll scan: master - 0xa818 (%08x)\n", val32);	

	if ((chip.inp.op == nDiversity) || (chip.inp.op == nPip)) {
		val32 = rReg32(nSlave, 0xa818);
		val32 |= (1 << 26) | (1 << 28);
		wReg32(nSlave, 0xa818, val32);
		//val32 = rReg32(nSlave, 0xa818);
		//nmi_debug(N_VERB, "poll scan: slave - 0xa818 (%08x)\n", val32);	
	}

	/**
		Program Tuner
	**/
	isdbt_config_tuner(nMaster, pv);
	if ((chip.inp.op == nDiversity) || (chip.inp.op == nPip)) {
		isdbt_config_tuner(nSlave, pv);
	}
	/**
		Program Demod
	**/	
	isdbt_config_demod(nMaster);
	if ((chip.inp.op == nDiversity) || (chip.inp.op == nPip)) {
		isdbt_config_demod(nSlave);
	}

	nmi_delay(10);
	
	stim = nmi_get_tick();
	do {
		val1 = rReg32(nMaster, 0xa818);
		if ((chip.inp.op == nDiversity) || (chip.inp.op == nPip)) {
			val2 = rReg32(nSlave, 0xa818);
		}
		if ((chip.inp.op == nDiversity) || (chip.inp.op == nPip)) {
			if ((val1 >> 28) & 0x1) {
				mlock = 1;
				lock |= 0x1;
			} else if ((val1 >> 26) & 0x1) {
				mlock = 0;
			}
			if ((val2 >> 28) & 0x1) {
				slock = 1;
				lock |= 0x2;
			} else if ((val2 >> 26) & 0x1) {
				slock = 0;
			}

			if ((mlock >= 0) && (slock >= 0))
				break;

		} else {
			if ((val1 >> 28) & 0x1) {
				lock = 1;
				break;
			} else if ((val1 >> 26) & 0x1) {
				lock = 0;
				break;
			}
		}
	} while ((nmi_get_tick() - stim) <= 600/*480*/);
	/**
		Return RFLNA bandwidth to normal
	**/
	isdbtrfwrite(nMaster, 0xf6, 0xa0);
	isdbtrfwrite(nMaster, 0xf7, 0x86);
	isdbtrfwrite(nMaster, 0xf8, 0x01);

	/**
		enable AGC wait
	**/
	wReg32(nMaster, 0xa640, 0x1);

	nmi_debug(N_VERB, "0xa818 (%08x)\n", rReg32(nMaster, 0xa818));


	return lock;
}

static int isdbt_scan(tNmiIsdbtChip cix, void *pv)
{
	//tNmiPriv *pp = (tNmiPriv *)&chip.priv[nMaster];
	int lock;

	lock = isdbt_scan_poll(pv);		

	return lock;
}

#if 0 // cys not used
static void isdbt_enable_long_scan(tNmiIsdbtChip cix)
{
	/**
	
		Normally band scan time is < 120ms.  
		If the host is willing to wait for 165ms the accuracy can be improved
	**/
	wReg32(cix, 0xa6a8, 0x35); 
	wReg32(cix, 0xa6ac, 0x303); 
}


/******************************************************************************
**
**	Gpio Functions
**
*******************************************************************************/
static void isdbt_set_gio(tNmiIsdbtChip cix, void *pv)
{
	tIsdbtGio *gio = (tIsdbtGio *)pv;
	uint32_t val32;

	if (gio->bit > 3) {
		nmi_debug(N_ERR, "Error, only 4 gio pins, (%d)\n", gio->bit);
		return;
	}
	
	/* set the direction */
	val32 = rReg32(cix, 0x6004);
	if (gio->dir) {	/* output */
		if (!((val32 >> gio->bit) & 0x1)) {
			val32 |= (1 << gio->bit);
			wReg32(cix, 0x6004, val32);
		}

		/* write the value */
		val32 = rReg32(cix, 0x6000);
		if (gio->level > 0)	
			val32 |= (1 << gio->bit);
		else
			val32 &= ~(1 << gio->bit);
		wReg32(cix, 0x6000, val32);

	} else {	/* input */
		if ((val32 >> gio->bit) & 0x1) {
			val32 &= ~(1 << gio->bit);
			wReg32(cix, 0x6004, val32);
		}

		/* read the value */
		val32 = rReg32(cix, 0x6050);
		gio->level = (val32 >> gio->bit) & 0x1;
	}

	return;
}
#endif

/******************************************************************************
**
**	Init Functions
**
*******************************************************************************/
static void isdbt_demod_init(tNmiIsdbtChip cix)
{
	//tNmiPriv *pp = (tNmiPriv *)&chip.priv[cix];
	uint32_t val32;

	wReg32(cix, 0xa6c0, 0x2d812c);
	wReg32(cix, 0xa640, 0x1);
	wReg32(cix, 0xa6a4, 0xa7);

	wReg32(cix, 0xa928, 0x1138);			/* rssi */
	wReg32(cix, 0xa934, 0x3edfbc);
	wReg32(cix, 0xa938, 0x37f3);
	wReg32(cix, 0xa93c, 0x1881e);
	wReg32(cix, 0xa940, 0x2e04a);
	wReg32(cix, 0xa944, 0x48076);
	wReg32(cix, 0xa948, 0x590a6);
	wReg32(cix, 0xa94c, 0x6d0c5);
	wReg32(cix, 0xa950, 0x8e8ee);
	wReg32(cix, 0xa954, 0x150);

	wReg32(cix, 0xa3a8, 0x52);
	wReg32(cix, 0xa3ac, 0x52);
	wReg32(cix, 0xa39c, 0x52);
	wReg32(cix, 0xa3a0, 0x52);
	wReg32(cix, 0xa604, 0x20);

	wReg32(cix, 0xa0b8, 0x40);
	wReg32(cix, 0xa200, 0x30);

	val32 = rReg32(cix, 0xa800);
	val32 &= ~(0x7 << 16);
	val32 |= (0x3 << 16);
	if (chip.inp.no_ts_err_chk)
		val32 &= ~(1 << 10);
	wReg32(cix, 0xa800, val32);

	nmi_debug(N_VERB, "0xa804 (%08x)\n", rReg32(cix, 0xa804));

	if ((chip.inp.dco == 1) || ((chip.inp.dco == 2) && (chip.inp.op == nSingle))) { 
		val32 = rReg32(cix, 0xa804);
		val32 |= (0x1 << 19);
		wReg32(cix, 0xa804, val32);

	}

	if ((chip.inp.dco == 2) && (chip.inp.op == nSingle)) {
		val32 = rReg32(cix, 0xa224);
		val32 &= ~(1 << 2);
		wReg32(cix, 0xa224, val32);

		wReg32(cix, 0xa834, 0x19);
		wReg32(cix, 0xabe4, 0x21e);

		val32 = rReg32(cix, 0xa800);
		val32 |= (1 << 1) | (1 << 2) | (1 << 12);
		wReg32(cix, 0xa800, val32);

		wReg32(cix, 0xa0d8, 0x38);
		
		val32 = rReg32(cix, 0xa0dc);
		val32 &= ~0x7;
		val32 |= 0x1;
		wReg32(cix, 0xa0dc, val32); 
	}


	return;
}

static uint8_t rfiv_e0[] = {
/*5 ~ 12   */		0xd0, 0x07, 0xac, 0x00, 0x33, 0x00, 0x80, 0xe3,				
/*13 ~ 20 */	  	0x01, 0x2f, 0x01, 0x80, 0x08, 0x01, 0x86, 0x8c,
/*21 ~ 28 */	  	0x01, 0x00, 0x01, 0x01, 0x76, 0x60, 0x30, 0xe0,
/*29 ~ 36 */	  	0x30, 0xb0, 0xf8, 0xe0, 0x5e, 0xd0, 0x33, 0x02,
/*37 ~ 44 */	  	0x80, 0x00, 0x80, 0xf6, 0xd4, 0xe0, 0xd8, 0x05,				
/*45 ~ 53 */		0x85, 0x7b, 0x83, 0xeb, 0xff, 0x01, 0x01, 0x00, 0x00};

static uint8_t rfiv_f0[] = {
/*5 ~ 12   */		0xd0, 0x07, 0xac, 0x00, 0x33, 0x00, 0x80, 0xe3,				
/*13 ~ 20 */	  	0x01, 0x2f, 0x01, 0x80, 0x08, 0x01, 0x86, 0x8c,
/*21 ~ 28 */	  	0x01, 0x00, 0x01, 0x01, 0x76, 0x60, 0x30, 0xe0,
/*29 ~ 36 */	  	0x30, 0xb0, 0x78/*0xf8*/, 0xe0, 0x5e, 0xd0, 0x33, 0x02,
/*37 ~ 44 */	  	0x80, 0x00, 0x80, 0xf6, 0xd4, 0xe0, 0xd8, 0x05,				
/*45 ~ 53 */		0x85, 0x07, 0x83, 0xeb, 0xff, 0x01, 0x01, 0x00, 0x00};


static void isdbt_tuner_init(tNmiIsdbtChip cix)
{
	uint8_t			val8;
	uint32_t 		val32;
	tNmiPriv		*pp = (tNmiPriv *)&chip.priv[cix];
	uint8_t			*rfiv;

	isdbtrfwrite(cix, 0x00, 0xbf);

	/**
		Use burst write to speed up
	**/
	if (ISNM325F0(pp->chipid))
		rfiv = rfiv_f0;
	else
		rfiv = rfiv_e0;

	isdbtrfburstwrite(cix, 0x5, rfiv, 49);
	pp->sx5 = rfiv[43];		/* register 0x2b */

#ifndef FIX_POINT
	if ((chip.inp.xo == 13.) || (chip.inp.xo == 13.008896) || (chip.inp.xo == 12.)) {
		isdbtrfwrite(cix, 0x1f, 0xb8);
	} else if (chip.inp.xo == 32.) {
		isdbtrfwrite(cix, 0x1f, 0xf8);
	}
#else
	isdbtrfwrite(cix, 0x1f, 0xf8);
#endif

	if (chip.inp.ldoby) {
		isdbtrfwrite(cix, 0x30, 0xfb);
	} 

	/**
		set the AGC tracking thresholds
	**/
#ifdef FIX_POINT
	val32 = 153;
#else
	val32 = (uint32_t)(38.4/*41.3*/ * 4);
#endif
	isdbtrfwrite(cix, 0xdd, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xde, (uint8_t)(val32 & 0xff));

	val32 = (uint32_t)(40/*53*/ * 4);
	isdbtrfwrite(cix, 0xdf, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xe0, (uint8_t)(val32 & 0xff));

#ifdef FIX_POINT
	val32 = 165;
#else
	val32 = (uint32_t)(41.4/*43.75*/ * 4);
#endif
	isdbtrfwrite(cix, 0xe1, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xe2, (uint8_t)(val32 & 0xff));

#ifdef FIX_POINT
	val32 = 45;
#else
	val32 = (uint32_t)(11.3/*15.3*/ * 4);
#endif
	isdbtrfwrite(cix, 0xe3, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xe4, (uint8_t)(val32 & 0xff));

	val32 = (uint32_t)(10/*13.3*/ * 4);
	isdbtrfwrite(cix, 0xe5, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xe6, (uint8_t)(val32 & 0xff));

#ifdef FIX_POINT
	val32 = 34;
#else
	val32 = (uint32_t)(8.6/*12*/ * 4);
#endif
	isdbtrfwrite(cix, 0xe7, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xe8, (uint8_t)(val32 & 0xff));

	val32 = (uint32_t)(255 * 4);
	isdbtrfwrite(cix, 0xe9, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xea, (uint8_t)(val32 & 0xff));

	val32 = (uint32_t)(255 * 4);
	isdbtrfwrite(cix, 0xeb, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xec, (uint8_t)(val32 & 0xff));

#ifdef FIX_POINT
	val32 = 137;
#else
	val32 = (uint32_t)(34.3/*36*//*45*/ * 4);
#endif
	isdbtrfwrite(cix, 0xed, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xee, (uint8_t)(val32 & 0xff));

	val32 = (uint32_t)(75/*80*//*101*/ * 4);
	isdbtrfwrite(cix, 0xef, (uint8_t)((val32 >> 8) & 0xf));
	isdbtrfwrite(cix, 0xf0, (uint8_t)(val32 & 0xff));

	pp->ddfs1 = isdbtrfread(cix, 0x84);

	/**
		AGC bandwidth
	**/
	isdbtrfwrite(cix, 0xf6, 0xa0);
	isdbtrfwrite(cix, 0xf7, 0x86);
	isdbtrfwrite(cix, 0xf8, 0x01);

	/**
		Undo Reset
	**/
	val8 = isdbtrfread(cix,0x81);
	val8 |= (1 << 5) | (1 << 6);
	val8 &= ~(1 << 0);
	isdbtrfwrite(cix,0x81,val8);

	return;
}

uint32_t isdbt_get_chipid(tNmiIsdbtChip cix) 
{
	uint32_t chipid;

	chipid	= rReg32(cix, 0x6400);
	return chipid;
}

static int isdbt_probe_chip(tNmiIsdbtChip cix)
{
	uint32_t chipid;
	int master = 0;
	int found = 0;

	chipid = isdbt_get_chipid(cix);
	if (ISNM325(chipid)) { 
		if (ISMASTER(chipid))
			master = 1;

		if ((master && (cix == nMaster)) ||
			(!master && (cix == nSlave)))
			found = 1;
	} 

	return found;
}

static void isdbt_config_spi_pad(void)
{
	uint32_t val32;

	if (chip.inp.bustype == nI2C) {
		val32 = rReg32(nMaster, 0x644c);
		val32 &= ~(1 << 14);
		wReg32(nMaster, 0x644c, val32);

		val32 = rReg32(nMaster, 0x6408);
		val32 &= ~(0x7 << 24);
		val32 |= (0x6 << 24);
		wReg32(nMaster, 0x6408, val32);
	}
}


static int isdbt_chip_init(tNmiIsdbtChip cix)
{
	uint32_t 		val32;
	tNmiPriv		*pp = (tNmiPriv *)&chip.priv[cix];

	val32 = isdbt_get_chipid(cix);
	if (!(ISNM325E0(val32) || ISNM325F0(val32))) {
		nmi_debug(N_ERR,"Wrong Chip ID (%08x), Check the Bus\n", val32);
		return 0;
	} else {

	/*** 
		We can work in diversity mode, check if the chip index is matching the
		chip id.
	***/

		if ((cix == nMaster) && !ISMASTER(val32)) {
			nmi_debug(N_ERR,"Mismatch, chip index (%d), chip id (%08x)\n", cix, val32);
			return 0;
		} 

		if ((cix == nSlave) && ISMASTER(val32)) {
			nmi_debug(N_ERR,"Mismatch, chip index (%d), chip id (%08x)\n", cix, val32);
			return 0;
		} 
	}
	pp->chipid = val32;	

	/**
		Enable sigma delta block
	**/
	val32 = rReg32(cix,0x6414);
	val32 &= ~(1 << 15);
	wReg32(cix,0x6414,val32);
	val32 |= (1 << 15);
	wReg32(cix,0x6414,val32);

	/*** 
		Configure PLL 
	***/
	wReg32(cix, 0x6460, 0x471cd540);
#ifndef FIX_POINT
	if (chip.inp.xo == 12.) {
		wReg32(cix, 0x6468, 0x619034c2);	
		pp->pll1 = 0x619034c2;
		wReg32(nMaster, 0xa06c, 0x01);
		wReg32(nMaster, 0xa070, 0x80);

		pp->norm1 = 0x01;
		pp->norm2 = 0x80;

	} else if (chip.inp.xo == 13.) {
		wReg32(cix, 0x6468, 0x619002b4);	
		pp->pll1 = 0x619002b4;
		wReg32(nMaster, 0xa06c, 0x01);
		wReg32(nMaster, 0xa070, 0x80);

		pp->norm1 = 0x01;
		pp->norm2 = 0x80;

	} else if (chip.inp.xo == 13.008896) {
		wReg32(cix, 0x6468, 0x619060b2);	
		pp->pll1 = 0x619060b2;
		wReg32(nMaster, 0xa06c, 0x00);
		wReg32(nMaster, 0xa070, 0x80);

		pp->norm1 = 0x00;
		pp->norm2 = 0x80;

	} else if (chip.inp.xo == 19.2) {
		wReg32(cix, 0x6468, 0x61905f78);	
		pp->pll1 = 0x61905f78;
		wReg32(nMaster, 0xa06c, 0x01);
		wReg32(nMaster, 0xa070, 0x80);

		pp->norm1 = 0x01;
		pp->norm2 = 0x80;

	} else if (chip.inp.xo == 24.576) {
		wReg32(cix, 0x6468, 0x61903e5e);	
		pp->pll1 = 0x61903e5e;
		wReg32(nMaster, 0xa06c, 0xff);
		wReg32(nMaster, 0xa070, 0x7f);

		pp->norm1 = 0xff;
		pp->norm2 = 0x7f;

	} else if (chip.inp.xo == 26) {
		wReg32(cix, 0x6468, 0x6190015a);	
		pp->pll1 = 0x6190015a;
		wReg32(nMaster, 0xa06c, 0x01);
		wReg32(nMaster, 0xa070, 0x80);

		pp->norm1 = 0x01;
		pp->norm2 = 0x80;

	} else if (chip.inp.xo == 27) {
		wReg32(cix, 0x6468, 0x61902256);
		pp->pll1 = 0x61902256;
		wReg32(nMaster, 0xa06c, 0x03);
		wReg32(nMaster, 0xa070, 0x80);

		pp->norm1 = 0x03;
		pp->norm2 = 0x80;
	} else {
		wReg32(cix, 0x6468, 0x61903948);
		pp->pll1 = 0x61903948;
		wReg32(nMaster, 0xa06c, 0x00);
		wReg32(nMaster, 0xa070, 0x80);

		pp->norm1 = 0x00;
		pp->norm2 = 0x80;
	}
#else
	wReg32(cix, 0x6468, 0x61903948);
	pp->pll1 = 0x61903948;
	wReg32(nMaster, 0xa06c, 0x00);
	wReg32(nMaster, 0xa070, 0x80);

	pp->norm1 = 0x00;
	pp->norm2 = 0x80;
#endif
		
	wReg32(cix, 0x6464, 0x10d);

	/*** 
		Update PLL  
	***/
	isdbt_update_pll(cix);

	/*** 
		Pins mux  
	***/
	if ((chip.inp.transporttype == nTS)) {
		wReg32(cix, 0x6408, 0x9249241);
		wReg32(cix, 0x640c, 0x1249);
	}

	/***
		Disable TS output
	***/
	val32 = rReg32(cix,0x6448);
	val32 |= (1 << 4);
	val32 &= ~(1 << 17);
	val32 |= (1 << 22);
	val32 |= (1 << 23);
	wReg32(cix, 0x6448, val32);

	val32 = rReg32(cix, 0x641c);
	val32 |= (1 << 4) | (1 << 15);
	wReg32(cix, 0x641c, val32);

	/*** 
		Enable Crystal Buffer  
	***/
	val32 = rReg32(cix, 0x6458);
	if ((chip.inp.op == nDiversity) || (chip.inp.op == nPip)) {
		val32 |= (1 << 7);
	} else {
		val32 &= ~(1 << 7);
	}
	val32 |= (1 << 2);
	wReg32(cix, 0x6458, val32);

	/**
		Power sequence control
	**/
	wReg32(cix, 0x643c, 0x800200);
	wReg32(cix, 0x6440, 0xa8000c0);
	wReg32(cix, 0x6444, 0x5000005);

	/*** 
		RF I2C Initialization  
	***/
	wReg32(cix, 0x636c, 0x00000000);
	wReg32(cix, 0x6300, 0x00000065);
	wReg32(cix, 0x631c, 0x00000006);
	wReg32(cix, 0x6320, 0x0000000d);
	wReg32(cix, 0x6304, 0x00000060);
	wReg32(cix, 0x6330, 0x00000aff);
	wReg32(cix, 0x6338, 0x00000001);
	wReg32(cix, 0x633c, 0x00000000);
	wReg32(cix, 0x636c, 0x00000001);

	/***
		Pad
	***/
	val32 = rReg32(cix, 0x644c);
	if (!chip.inp.spiworkaround) {
		val32 &= ~(1 << 12);
	}
	val32 &= ~(1 << 13);
	val32 &= ~(1 << 14);
	val32 &= ~(0x7 << 18);
	val32 |= (0x2 << 18);
	val32 &= ~(0x7 << 21);
	val32 |= (0x2 << 21);
	val32 &= ~(0x7 << 24);
	val32 |= (0x2 << 24);
	if ((chip.inp.op == nDiversity) || (chip.inp.op == nPip)) {
		if (cix == nMaster) {
			val32 &= ~0x3;
		} else {
			val32 &= ~(1 << 0);
			val32 &= ~(1 << 1);
			val32 &= ~(1 << 2);
			val32 &= ~(1 << 4);
		}
	} else {
		val32 &= ~(1 << 8);
		val32 &= ~(1 << 11);
	}
	wReg32(cix, 0x644c, val32);

	/*** 
		Tuner Initialize  
	***/
	isdbt_tuner_init(cix);

	/*** 
		Demod Initialize  
	***/
	isdbt_demod_init(cix);

	/***
		Enable PLL reset interrupt
	***/
	isdbtrfwrite(cix, 0xfc, 0x1);
	isdbtrfwrite(cix, 0xfb, 0x1);
		
	return 1;
}

/******************************************************************************
**
**	Power Control Functions
**
*******************************************************************************/

static void isdbt_sleep_timer(void *pv)
{
	tNmiPriv		*pp = (tNmiPriv *)&chip.priv[nMaster];
	tIsdbtSleep *zz = (tIsdbtSleep *)pv;
	uint32_t val32;
	int retry;
	tIsdbtTune tune;
	int lock;
	
	if (!zz)
		return;

	if (zz->ensleep) {
		if ((chip.inp.op == nDiversity) || (chip.inp.op == nPip)) {
			/**
				power down slave
			**/
			wReg8(nSlave, 0x7001, 0);
		}

		/**
			check the norminal rate
		**/
#ifndef FIX_POINT
		if (chip.inp.xo == 12.) {
			if ((pp->norm1 != 0x01) || (pp->norm2 != 0x80)) {
				wReg32(nMaster, 0xa06c, 0x01);
				wReg32(nMaster, 0xa070, 0x80);
				pp->norm1 = 0x01;
				pp->norm2 = 0x80;
				wReg32(nMaster, 0x6460, 0x471cd540);
				wReg32(nMaster, 0x6468, 0x619034c2);			
				isdbt_update_pll(nMaster);
				pp->pll1 = 0x619034c2;
			}

		} else if (chip.inp.xo == 13.) {
			if ((pp->norm1 != 0x01) || (pp->norm2 != 0x80)) {
				wReg32(nMaster, 0xa06c, 0x01);
				wReg32(nMaster, 0xa070, 0x80);
				pp->norm1 = 0x01;
				pp->norm2 = 0x80;
				wReg32(nMaster, 0x6460, 0x471cd540);
				wReg32(nMaster, 0x6468, 0x619002b4);			
				isdbt_update_pll(nMaster);
				pp->pll1 = 0x619002b4;
			}

		} else if (chip.inp.xo == 13.008896) {
			if ((pp->norm1 != 0x00) || (pp->norm2 != 0x80)) {
				wReg32(nMaster, 0xa06c, 0x00);
				wReg32(nMaster, 0xa070, 0x80);
				pp->norm1 = 0x00;
				pp->norm2 = 0x80;
				wReg32(nMaster, 0x6460, 0x471cd540);
				wReg32(nMaster, 0x6468, 0x619060b2);			
				isdbt_update_pll(nMaster);
				pp->pll1 = 0x619060b2;
			}

		} else if (chip.inp.xo == 19.2) {
			if ((pp->norm1 != 0x01) || (pp->norm2 != 0x80)) {
				wReg32(nMaster, 0xa06c, 0x01);
				wReg32(nMaster, 0xa070, 0x80);
				pp->norm1 = 0x01;
				pp->norm2 = 0x80;
				wReg32(nMaster, 0x6460, 0x471cd540);
				wReg32(nMaster, 0x6468, 0x61905f78);			
				isdbt_update_pll(nMaster);
				pp->pll1 = 0x61905f78;
			}
		}else if (chip.inp.xo == 24.576) {
			if ((pp->norm1 != 0xff) || (pp->norm2 != 0x7f)) {
				wReg32(nMaster, 0xa06c, 0xff);
				wReg32(nMaster, 0xa070, 0x7f);
				pp->norm1 = 0xff;
				pp->norm2 = 0x7f;
				wReg32(nMaster, 0x6460, 0x471cd540);
				wReg32(nMaster, 0x6468, 0x61903e5e);			
				isdbt_update_pll(nMaster);
				pp->pll1 = 0x61903e5e;
			}
		}else if (chip.inp.xo == 26) {
			if ((pp->norm1 != 0x01) || (pp->norm2 != 0x80)) {
				wReg32(nMaster, 0xa06c, 0x01);
				wReg32(nMaster, 0xa070, 0x80);
				pp->norm1 = 0x01;
				pp->norm2 = 0x80;
				wReg32(nMaster, 0x6460, 0x471cd540);
				wReg32(nMaster, 0x6468, 0x6190015a);			
				isdbt_update_pll(nMaster);
				pp->pll1 = 0x6190015a;
			}
		} else if (chip.inp.xo == 32) {
			if ((pp->norm1 != 0x03) || (pp->norm2 != 0x80)) {
				wReg32(nMaster, 0xa06c, 0x03);
				wReg32(nMaster, 0xa070, 0x80);
				pp->norm1 = 0x03;
				pp->norm2 = 0x80;
				wReg32(nMaster, 0x6460, 0x471cd540);
				wReg32(nMaster, 0x6468, 0x61902256);			
				isdbt_update_pll(nMaster);
				pp->pll1 = 0x61902256;
			}
		} else {
			if ((pp->norm1 != 0x00) || (pp->norm2 != 0x80)) {
				wReg32(nMaster, 0xa06c, 0x00);
				wReg32(nMaster, 0xa070, 0x80);
				pp->norm1 = 0x00;
				pp->norm2 = 0x80;
				
				wReg32(nMaster, 0x6460, 0x471cd540);
				wReg32(nMaster, 0x6468, 0x61903948);			
				isdbt_update_pll(nMaster);

				pp->pll1 = 0x61903948;
			}
		}
#else
		if ((pp->norm1 != 0x03) || (pp->norm2 != 0x80)) {
			wReg32(nMaster, 0xa06c, 0x03);
			wReg32(nMaster, 0xa070, 0x80);
			pp->norm1 = 0x03;
			pp->norm2 = 0x80;
			wReg32(nMaster, 0x6460, 0x471cd540);
			wReg32(nMaster, 0x6468, 0x61902256);			
			isdbt_update_pll(nMaster);
			pp->pll1 = 0x61902256;
		}
#endif


		/**
			Tmcc polling
		**/
		val32 = rReg32(nMaster, 0x6408);
		val32 &= ~0x7;
		val32 |= 0x1;
		wReg32(nMaster, 0x6408, val32);

		val32 = rReg32(nMaster, 0x6448);
		val32 |= (1 << 4) | (1 << 14);
		wReg32(nMaster, 0x6448, val32);

		val32 = rReg32(nMaster, 0x6458);
		val32 &= ~0x1;
		val32 |= (1 << 1);
		wReg32(nMaster, 0x6458, val32);

		val32 = rReg32(nMaster, 0xa584);
		val32 |= 0x1;
		wReg32(nMaster, 0xa584, val32);
		val32 &= ~0x1;
		wReg32(nMaster, 0xa584, val32);

		wReg32(nMaster, 0xac08, zz->toff);
		wReg32(nMaster, 0xac0c, zz->ton);

		val32 = rReg32(nMaster, 0xac00);
		val32 |= 0x1;
		wReg32(nMaster, 0xac00, val32);

		retry = 100;
		do {
			val32 = rReg32(nMaster, 0xa584);
			if (val32 & 0x8) 
				break; 
		} while (retry--);

		if (retry <= 0) {
			nmi_debug(N_ERR, "Failed, can't put chip to SLEEP, (%08x)\n", val32);
		} else {
			wReg8(nMaster, 0x7001, 0);
			chip.pwrdown = 1;
		}
	} else {
		/**
			wake up master
		**/
		wReg8(nMaster, 0x7001, 1);
		nmi_delay(175);
		wReg8(nMaster, 0x7002, 0x40);
		nmi_delay(1);
		chip.pwrdown = 0;

		wReg32(nMaster, 0xac00, 0xe);
		wReg32(nMaster, 0xa14c, 0x30);
		wReg32(nMaster, 0xa000, 0x20);

		val32 = rReg32(nMaster, 0x6448);
		val32 &= ~(1 << 14);
		wReg32(nMaster, 0x6448, val32);

		val32 = rReg32(nMaster, 0x6458);
		val32 |= 0x1;
		val32 &= ~(1 << 1);
		wReg32(nMaster, 0x6458, val32);

		isdbt_chip_init(nMaster);
		pp = (tNmiPriv *)&chip.priv[nMaster];
		tune.frequency = pp->frequency * 1000;
		tune.highside = 0;
		isdbt_config_tuner(nMaster, (void *)&tune);
		isdbt_config_demod(nMaster);
		lock = isdbt_check_lock(nMaster, 900);
		if (lock) {
			isdbt_config_decoder(nMaster, 0, NULL);			
		}		

		/**
			wake up slave
		**/
		if ((chip.inp.op == nDiversity) || (chip.inp.op == nPip)) {

			wReg8(nSlave, 0x7001, 1);
			nmi_delay(175);
			wReg8(nSlave, 0x7002, 0x40);
			nmi_delay(1);

			isdbt_chip_init(nSlave);
			pp = (tNmiPriv *)&chip.priv[nSlave];
			tune.frequency = pp->frequency * 1000;
			tune.highside = 0;
			isdbt_config_tuner(nSlave, (void *)&tune);
			isdbt_config_demod(nSlave);
			lock = isdbt_check_lock(nSlave, 900);
			if (lock) {
				isdbt_config_decoder(nSlave, 0, NULL);			
			}		
		}	
	}
	
	return;
}

static void isdbt_power_down(void)
{
	//tNmiPriv	*pp = (tNmiPriv *)&chip.priv[nMaster];

	if ((chip.inp.op == nDiversity) || (chip.inp.op == nPip)) {
		/**
			power down slave
		**/
		wReg8(nSlave, 0x7001, 0);
	}
	/**
		power down master
	**/
	wReg8(nMaster, 0x7001, 0);
	chip.pwrdown = 1;

	return;
}

static void isdbt_power_up(int recovery)
{
	tNmiPriv *pp = (tNmiPriv *)&chip.priv[nMaster];
	tIsdbtTune tune;
	int lock;

	/**
		power up master
	**/
	wReg8(nMaster, 0x7001, 1);
	nmi_delay(175);
	wReg8(nMaster, 0x7002, 0x40);
	nmi_delay(1);
	chip.pwrdown = 0;

	/**
		power up slave
	**/
	if ((chip.inp.op == nDiversity) || (chip.inp.op == nPip)) {
		wReg8(nSlave, 0x7001, 1);
		nmi_delay(175);
		wReg8(nSlave, 0x7002, 0x40);
		nmi_delay(1);
	}

	/**
		do initialization again
	**/
	isdbt_chip_init(nMaster);
	if ((chip.inp.op == nDiversity) || (chip.inp.op == nPip)) {
		isdbt_chip_init(nSlave);
	}	

	if (recovery) {
		tune.frequency = pp->frequency * 1000;
		tune.highside = 0;
		isdbt_config_tuner(nMaster, (void *)&tune);
		isdbt_config_demod(nMaster);
		lock = isdbt_check_lock(nMaster, 900);
		if (lock) {
			isdbt_config_decoder(nMaster, 0, NULL);			
		}		

		if ((chip.inp.op == nDiversity) || (chip.inp.op == nPip)) {
			pp = (tNmiPriv *)&chip.priv[nSlave];

			tune.frequency = pp->frequency * 1000;
			tune.highside = 0;
			isdbt_config_tuner(nSlave, (void *)&tune);
			isdbt_config_demod(nSlave);
			lock = isdbt_check_lock(nSlave, 900);
			if (lock) {
				isdbt_config_decoder(nSlave, 0, NULL);			
			}		
		}	
	}

	return;
}

/******************************************************************************
**
**	Interrupt Functions
**
*******************************************************************************/

static void isdbt_irq_enable(void *pv)
{
	tIsdbtIrq *pirq = (tIsdbtIrq *)pv;
	uint32_t irq = 0, ctl = 0;

	if (pirq != NULL) {
		/**
			Note:
				bit 0: gpio 0 interrupt enable,
				bit 1: gpio 1 interrupt enable,
				bit 2: gpio 2 interrupt enable,
				bit 3: gpio 3 interrupt enable,
				bit 4: demod interrupt enable,
				bit 5: decoder interrupt enable,
				bit 24: emergency interrupt enable,
				bit 26: pll resync interrupt enable,
		**/

		/**
			irq control
		**/
		ctl = rReg32(nMaster, 0x6410);
		if (pirq->ctl.pulse) {
			ctl |= (1 << 0);
			ctl |= (1 << 1);
		} else {
			ctl &= ~(1 << 1);
		}
		if (pirq->ctl.activehigh) {
			ctl |= (1 << 2);
		} else {
			ctl &= ~(1 << 2);
		}
		if (pirq->ctl.pulsewidth) {
			uint32_t tp;
			tp = (pirq->ctl.pulsewidth & 0x1fff);
			tp /= 250;
			if (tp == 0)
				tp = 1;
			ctl &= ~(0x1fff << 3);
			ctl |= tp & 0x1fff;
		}
		wReg32(nMaster, 0x6410, ctl);

		/**
			gpio interrupt enable
		**/
		if (pirq->gio0 || pirq->gio1 || pirq->gio2 || pirq->gio3) {
			uint32_t misc = 0, mux, dir;

			wReg32(nMaster, 0x6450, 0xf);
			misc = rReg32(nMaster, 0x6450);
			mux = rReg32(nMaster, 0x6404);
			dir = rReg32(nMaster, 0x6004);
			if (pirq->gio0) {
				mux &= ~(0x7 << 0);
				dir &= ~(0x1 << 0);
				misc |= (1 << 4);
				irq |= (1 << 0);
			} else if (pirq->gio1) {
				mux &= ~(0x7 << 3);
				dir &= ~(0x1 << 1);
				misc |= (1 << 5);
				irq |= (1 << 1);
			} else if (pirq->gio2) {
				dir &= ~(0x1 << 2);
				misc |= (1 << 6);
				irq |= (1 << 2);
			} else if (pirq->gio3) {
				mux &= ~(0x7 << 6);
				dir &= ~(0x1 << 3);
				misc |= (1 << 7);
				irq |= (1 << 3);
			} 
			wReg32(nMaster, 0x6404, mux);
			wReg32(nMaster, 0x6004, dir);
			wReg32(nMaster, 0x6450, misc);
		}

		/**
			clear and enable demod interrupts
		**/
		wReg32(nMaster, 0xa00c, 0xf);
		wReg32(nMaster, 0xa008, pirq->cirq.core);
		if (pirq->cirq.core != 0)
			irq |= (1 << 4);

		/**
			clear and enable decoder interrupts
		**/
		wReg32(nMaster, 0xa818, 0xffffffff);
		wReg32(nMaster, 0xa810, pirq->dirq.dec);
		wReg32(nMaster, 0xab50, 0xf);
		wReg32(nMaster, 0xab4c, pirq->virq.vit);
		if ((pirq->dirq.dec != 0) || (pirq->virq.vit != 0))
			irq |= (1 << 5);

		/**
			emergency alarm
		**/
		if (pirq->alarm.u.enable) {
			uint32_t emg;
			emg = rReg32(nMaster, 0xac00);
			if (pirq->alarm.u.rising) {
				emg |= (1 << 1);
			} else {
				emg &= ~(1 << 1);
			}
			if (pirq->alarm.u.watchdog) {
				emg |= (1 << 3);
			} else {
				emg &= ~(1 << 3);
			}
			wReg32(nMaster, 0xac00, emg);

			irq |= (1 << 24);
		}

		/**
			pll resync interrupt
		**/
		if (pirq->pllresync) {
			irq |= (1 << 26);
		}
		
		/**
			enable top level interrupt controller
		**/
		wReg32(nMaster, 0x6100, irq);		
	}

	return;
}

static void isdbt_handle_intr(tIsdbtIrqStatus *pirq)
{
	uint32_t status, val32;
	int overflow = 0;

#if 0
	nmi_debug(N_INFO, "0xa008 (%08x), 0xab4c (%08x), 0xa810 (%08x)\n", rReg32(nMaster, 0xa008),
							rReg32(nMaster, 0xab4c), rReg32(nMaster, 0xa810));
#endif

	if (pirq == NULL) {
		nmi_debug(N_ERR, "Error, parameter is NULL\n");
		return;
	}

	/**
		emergency interrupt.
		wake up the chip if it is in sleep.
	**/
	if (chip.pwrdown) {
		tIsdbtSleep zz;
		memset((void *)&zz, 0, sizeof(tIsdbtSleep));		
		isdbt_sleep_timer((void *)&zz);
	}

	status = rReg32(nMaster, 0x6120);
	nmi_debug(N_INTR, "[Intr]: status (%08x)\n", status);

	if ((status >> 24) & 0x1) {
		val32 = rReg32(nMaster, 0xac04);
		wReg32(nMaster, 0xac04, val32);
		pirq->alarm = 1;
	}

	/**
		pll resync interrupt
	**/
	if ((status >> 26) & 0x1) {
		isdbt_check_pll_reset(nMaster);
		if ((chip.inp.op == nDiversity) || (chip.inp.op == nPip)) {
			isdbt_check_pll_reset(nSlave);
		}
		pirq->pllresync = 1;
	}

		
	/**
		gpio interrupts
	**/
	if (status & 0xf) {
		val32 = rReg32(nMaster, 0x6450);
		val32 |= (status & 0xf);
		wReg32(nMaster, 0x6450, val32);
		if (status & 0x1)
			pirq->gio0 = 1;
		else if (status & 0x2)
			pirq->gio1 = 1;
		else if (status & 0x4)
			pirq->gio2 = 1;
		else if (status & 0x8)
			pirq->gio3 = 1;
	}

	/**
		Read and clear the core interrupt status
	**/
	if ((status >> 4) & 0x1) {
		val32 = rReg32(nMaster, 0xa008);
		wReg32(nMaster, 0xa008, val32);
		pirq->cirq.core = val32;
	}

	/**
		Read and clear the decoder interrupt status
	**/
	if ((status >> 5) & 0x1) {
		uint32_t irqsts;
		/**
			Viterbi
		**/
		irqsts = rReg32(nMaster, 0xab4c);
		wReg32(nMaster, 0xab4c, irqsts);
		pirq->virq.vit = irqsts;

		/**
			Spi Dma
		**/
		irqsts = rReg32(nMaster, 0xa818);
		wReg32(nMaster, 0xa818, irqsts);
		pirq->dirq.dec = irqsts;
		pirq->dirq.u.overflow = 0;	// added by tony to let the application know oveflow happened.
		if ((irqsts >> 4) & 0x1) {
			uint32_t adr, tsz, esz, fsm, tso;

			nmi_debug(N_INTR, "Dma Start...(%08x)\n", irqsts);

			/**
				check if DMA enable...
			**/
			tso = rReg32(nMaster, 0xa804);
			if ((tso >> 8) & 0x1) {

				/**
					get the size
				**/
 				tsz = (tso >> 10) & 0x7f;
				tsz *= 188;
				nmi_debug(N_INTR, "tsz (%d)\n", tsz);	// tony
				do {
					/**
						read and calculate the starting address from the decoder.
					**/
					adr = rReg32(nMaster, 0xa83c);
					adr *= 4;
					adr += 0x60050;

					/**
						claculate the size
					**/
					esz = 0x6204c - adr + 4;

					if ((irqsts >> 11) & 0x1) {	/* overflow */
						overflow = 1;
						pirq->dirq.u.overflow = 1;	// added by tony to let the application know oveflow happened.
						nmi_debug(N_INFO, "*** TS OverFlow ***\n");
						/**
							disable DMA
						**/
						tso &= ~(1 << 8);
						wReg32(nMaster, 0xa804, tso);

						/**
							read the last 4 bytes
						**/
						if (esz < tsz) {
							tsz -= esz;
							adr = 0x60050 + tsz - 4;
						} else {
							adr += tsz - 4;					
						} 
						nmi_debug(N_INTR, "start address...(%08x)\n", adr);
						wReg32(nMaster, 0x7864, adr);
						wReg32(nMaster, 0x7868, 4);

						/**
							read the last 4 bytes
						**/
						nmi_burst_read((void*)&overflow);

						/**
							enable DMA
						**/
						tso |= (1 << 8);
						wReg32(nMaster, 0xa804, tso);

					} else {

						wReg32(nMaster, 0x7864, adr);
						if (esz < tsz) {			/* wrap around */
							nmi_debug(N_INTR, "wrap around\n");	// tony
							/**
								first transfer
							**/
							wReg32(nMaster, 0x7868, esz);

							/**
								read the data
							**/
							nmi_burst_read((void*)&overflow);
					        /**
								second transfer
							**/
							wReg32(nMaster, 0x7864, 0x60050);
							wReg32(nMaster, 0x7868, (tsz - esz));

							/**
								read the data
							**/
							nmi_burst_read((void*)&overflow);
						} else {
							wReg32(nMaster, 0x7868, tsz);
							/**
								read the data
							**/
							nmi_burst_read((void*)&overflow);
						} 
					}

					/**
						read and clear status
					**/
					irqsts = rReg32(nMaster, 0xa818);
					wReg32(nMaster, 0xa818, irqsts);

					/**
						read the FSM state
					**/
					fsm = rReg32(nMaster, 0xa80c);
					nmi_debug(N_INTR, "Fsm Status...(%d)\n", ((fsm >> 12) & 0x3));
					if (((fsm >> 12) & 0x3) != 0x2) {
						nmi_debug(N_INTR, "Dma done...\n");
						break;
					}
				} while (1);
			}
		}
	}
		
	return;	

}

/******************************************************************************
**
**	Dsm
**
*******************************************************************************/
static int isdbt_get_dsm_master_lock(void)
{
	uint32_t val32;

	val32 = rReg32(nMaster, 0xad4c);
	return ((val32 >> 8) & 0x1);
}

static int isdbt_get_dsm_state(void)
{
	uint32_t val32;

	val32 = rReg32(nMaster, 0xad48);
	val32 &= 0x7;
	return (int)val32;
}

static void isdbt_switch_dsm(int enable)
{
	uint32_t val32;

	if (enable) {
		nmi_debug(N_INFO, "dsm on\n");
		val32 = rReg32(nMaster, 0x6404);
		val32 &= ~(0x7 << 6);
		val32 |= (0x1 << 6);
		wReg32(nMaster, 0x6404, val32);

		wReg32(nMaster, 0xad50, 0x70ff9b);
		wReg32(nMaster, 0xad5c, 0x70ff9b);
		wReg32(nMaster, 0xad68, 0x17fda);
		wReg32(nMaster, 0xad6c, 0x64000001);
		wReg32(nMaster, 0xad74, 0x17fda);
		wReg32(nMaster, 0xad78, 0x64000001);
		wReg32(nMaster, 0xad44, 0xaf03ff);
		wReg32(nMaster, 0xab20, 0x100);
		wReg32(nMaster, 0xab00, 0x2);
		wReg32(nMaster, 0xa14c, 0x31);
		wReg32(nMaster, 0xa000, 0x0);
		wReg32(nMaster, 0xa14c, 0x30);

		val32 = rReg32(nMaster, 0xabe4);
		wReg32(nMaster, 0xaba4, val32);
		val32 = rReg32(nMaster, 0xa834);
		nmi_debug(N_VERB, "0xa834 (%08x)\n", val32);
		wReg32(nMaster, 0xa830, val32);
		nmi_debug(N_VERB, "0xa830 (%08x)\n", rReg32(nMaster, 0xa830));


		val32 = rReg32(nSlave, 0x6404);
		val32 &= ~(0x7 << 6);
		val32 |= (0x1 << 6);
		wReg32(nSlave, 0x6404, val32);		

		wReg32(nSlave, 0xad50, 0x70ff9b);
		wReg32(nSlave, 0xad5c, 0x70ff9b);
		wReg32(nSlave, 0xad68, 0x17fda);
		wReg32(nSlave, 0xad6c, 0x64000001);
		wReg32(nSlave, 0xad74, 0x17fda);
		wReg32(nSlave, 0xad78, 0x64000001);
		wReg32(nSlave, 0xad44, 0xaf03ff);
		wReg32(nSlave, 0xab20, 0x100);
		wReg32(nSlave, 0xab00, 0x2);
		wReg32(nSlave, 0xa000, 0x0);

		val32 = rReg32(nSlave, 0xabe4);
		wReg32(nSlave, 0xaba4, val32);
		val32 = rReg32(nSlave, 0xa834);
		wReg32(nSlave, 0xa830, val32);

		wReg32(nMaster, 0xad40, 0x2001f);
		wReg32(nSlave, 0xad40, 0x2001f);


	} else {
		nmi_debug(N_INFO, "dsm off\n");
		wReg32(nMaster, 0xa000, 0x20);
		wReg32(nMaster, 0xad40, 0x9e);

		wReg32(nSlave, 0xa000, 0x20);
		wReg32(nSlave, 0xad40, 0x9e);

	}

}

/******************************************************************************
**
**	Combiner
**
*******************************************************************************/
static void isdbt_switch_combiner(int enable)
{
	uint32_t val32;
	if(enable) {
		val32 = rReg32(nSlave, 0xac40);
		val32 |= (1 << 0);
		val32 &= ~(1 << 2);
		wReg32(nSlave, 0xac40, val32);

		val32 = rReg32(nSlave, 0x641c);
		val32 |= (1 << 13) | (1 << 14);
		wReg32(nSlave, 0x641c, val32);

		val32 = rReg32(nMaster, 0xac40);
		val32 |= (1 << 0);
		val32 &= ~(1 << 2);
		wReg32(nMaster, 0xac40, val32);

		val32 = rReg32(nMaster, 0x641c);
		val32 |= (1 << 13) | (1 << 14);
		wReg32(nMaster, 0x641c, val32);

	} else {
		val32 = rReg32(nMaster, 0xac40);
		val32 &= ~(1 << 0);
		wReg32(nMaster, 0xac40, val32);

		val32 = rReg32(nMaster, 0x641c);
		val32 &= ~((1 << 13) | (1 << 14));
		wReg32(nMaster, 0x641c, val32);

		val32 = rReg32(nSlave,0xac40);
		val32 &= ~(1 << 0);
		wReg32(nSlave, 0xac40, val32);

		val32 = rReg32(nSlave, 0x641c);
		val32 &= ~((1 << 13) | (1 << 14));
		wReg32(nSlave, 0x641c, val32);
	}
}

/******************************************************************************
**
**	DAC Function
**
*******************************************************************************/
static void isdbt_dac_ctl(int enable, uint8_t bits)
{
	uint32_t val32;
	uint8_t val8;
	//tNmiPriv *pp = (tNmiPriv *)&chip.priv[nMaster];

 	val8 = isdbtrfread(nMaster, 0x30);
	val32 = rReg32(nMaster, 0x6448);

	nmi_debug(N_INFO, "Dac - enable (%d), val (%d)\n", enable, bits);
	if (enable) {
		val8 &= ~((1 << 5) | (1 << 6));
		val32 |= (1 << 5);
		val32 &= ~(0x7f << 7);
		val32 |= ((bits & 0x7f) << 7);
	} else {
		val8 |= (1 << 5) | (1 << 6);
		val32 &= ~(1 << 5);
	}
	isdbtrfwrite(nMaster, 0x30, val8);
	wReg32(nMaster, 0x6448, val32);

 	return;	
}

/******************************************************************************
**
**	PWM Function
**
*******************************************************************************/

static void isdbt_pwm_ctl(tNmiIsdbtChip cix, void *pv)
{
	tIsdbtPwmCtl *pwm = (tIsdbtPwmCtl *)pv;
	uint32_t val32;

	val32 = rReg32(cix, 0x6404);
	if (!(val32 & 0x1)) {
 		val32 |= 0x1;
		wReg32(cix, 0x6404, val32);
	}

	val32 = rReg32(cix, 0x6490);
	if (pwm->enable)
		val32 |= 0x1;
	else
		val32 &= ~0x1;
	
	if (pwm->inverse)
		val32 |= 0x2;
	else
		val32 &= ~0x2;

	if (pwm->format)
		val32 |= 0x4;
	else
		val32 &= ~0x4;

	val32 &= ~(0xf << 5);
	val32 |= ((pwm->period & 0xf) << 5);
	
	val32 &= ~(0x3ff << 9);
	val32 |= ((pwm->data & 0x3ff) << 9);
	
	wReg32(cix, 0x6490, val32);

	return;	
}

/******************************************************************************
**
**	Pid filters
**
*******************************************************************************/
static int isdbt_add_pid(tNmiIsdbtChip cix, tIsdbtPid *ppid)
{
	tNmiPriv *pp = (tNmiPriv *)&chip.priv[cix];
	int i, first = 1;
	uint32_t adr, pid;

	/**
		find the empty pid filter
	**/
	for (i = 0; i < 16; i++) {
		adr = 0xa848 + (4 * i);
		pid = rReg32(cix, adr);
		if ((pid & 0x1fff) == 0) {	/* found */
			if (first) {
				first = 0;
				/**
					Check if we have PAT in the filter. If not, then we 
					can use this slot; otherwise, search the next empty slot
				**/
				if (!pp->pidpat)				
					break;
			} else {
				break;
			}
		}
	}

	if (i < 16) {
		adr = 0xa848 + (4 * i);
		pid = ppid->pid & 0x1fff;
		if (pid == 0) {	/* PAT */
			if (!pp->pidpat)
				pp->pidpat = 1;
		}
		if (ppid->remap)
			pid |= ((ppid->rpid & 0x1fff) << 16);
		wReg32(cix, adr, pid);
	} else {
		nmi_debug(N_ERR, "Error, no more filter slot\n");
		return 0;
	}

	return 1;
}

static int isdbt_remove_pid(tNmiIsdbtChip cix, uint32_t pid)
{
	tNmiPriv *pp = (tNmiPriv *)&chip.priv[cix];
	int i;
	uint32_t adr, pid1;

	for (i = 0; i < 16; i++) {
		adr = 0xa848 + (4 * i);
		pid1 = rReg32(cix, adr);
		if ((pid1 & 0x1fff) == pid) {
			pid1 = 0;
			wReg32(cix, adr, pid1);
			break;
		}
	}

	if (i < 16) {
		if ((pid & 0x1fff) == 0)
			pp->pidpat = 0;
	} else {
		nmi_debug(N_ERR, "Error, can't find the matched PID\n");
		return 0;
	}

	return 1;	
} 

static void isdbt_reset_pid(tNmiIsdbtChip cix)
{
	tNmiPriv *pp = (tNmiPriv *)&chip.priv[cix];
	uint32_t adr;
	int i;

	for (i = 0; i < 16; i++) {
		adr = 0xa848 + (i * 4);
		wReg32(cix, adr, 0);
	}

	if (pp->pidpat)
		pp->pidpat = 0;

}


static void isdbt_pid_filter(tNmiIsdbtChip cix, tIsdbtPidFilterCtl *ppfc)
{
	uint32_t val32;

	val32 = rReg32(cix, 0xa804);
	if (ppfc->enfilt) {
		val32 |= (1 << 31);
		if (ppfc->enremapfilt)
			val32 |= (1 << 30);
		else
			val32 &= ~(1 << 30);
	} else {
		val32 &= ~(3 << 30);
	}
	wReg32(cix, 0xa804, val32);
}

/******************************************************************************
**
**	Aes (encryption)
**
*******************************************************************************/

static void isdbt_aes_ctl(tNmiIsdbtChip cix, tIsdbtAes *p)
{
	uint32_t val32, key;

	val32 = rReg32(cix, 0xa8a0);
	if (p->enable) {
		val32 |= 1;
		if (p->hardwire) {
			val32 |= 1 << 1;
		} else {
			key = (p->key[0] << 24) | (p->key[1] << 16) | (p->key[2] << 8) | (p->key[3]);
			wReg32(cix, 0xa8b0, key);
			key = (p->key[4] << 24) | (p->key[5] << 16) | (p->key[6] << 8) | (p->key[7]);
			wReg32(cix, 0xa8ac, key);
			key = (p->key[8] << 24) | (p->key[9] << 16) | (p->key[10] << 8) | (p->key[11]);
			wReg32(cix, 0xa8a8, key);
			key = (p->key[12] << 24) | (p->key[13] << 16) | (p->key[14] << 8) | (p->key[15]);
			wReg32(cix, 0xa8a4, key);
		}		
	} else {
		val32 &= ~0x3;
	}
	wReg32(cix, 0xa8a0, val32);

}

static void isdbt_ts_test_pattern(int pattern)
{
	uint32_t val32;

	val32 = rReg32(nMaster, 0xa804);
	
	val32 |= 1 << 24;
	val32 &= ~(0x0E000000);
	val32 |= pattern << 25;

	wReg32(nMaster, 0xa804, val32);
}

/******************************************************************************
**
**	Virtual Function Set Up
**
*******************************************************************************/

static void isdbt_vtbl_init(tNmiIsdbtVtbl *ptv)
{
	ptv->init					= isdbt_chip_init;
	ptv->probe				= isdbt_probe_chip;
	ptv->r8					= rReg8;
	ptv->w8					= wReg8;
	ptv->r32					= rReg32;
	ptv->w32				= wReg32;
	ptv->rfr					= isdbtrfread;
	ptv->rfw					= isdbtrfwrite;
	ptv->tune				= isdbt_config_tuner;
	ptv->demod			= isdbt_config_demod;
	ptv->decoder			= isdbt_config_decoder;
	ptv->getchipid		= isdbt_get_chipid;
 	ptv->checklock		= isdbt_check_lock;
	ptv->agclock			= isdbt_agc_lock;
	ptv->syrlock			= isdbt_syr_lock;
	ptv->tmcclock		= isdbt_tmcc_lock;
	ptv->feclock			= isdbt_fec_lock;
	ptv->scan				= isdbt_scan;
	ptv->scanpoll			= isdbt_scan_poll;
#ifndef FIX_POINT
	ptv->scantime		= isdbt_scan_time;
#endif
	ptv->reset				= isdbt_software_reset;
	ptv->getsnr			= isdbt_get_snr;
	ptv->getber			= isdbt_get_ber;
	ptv->getper			= isdbt_get_per;
#ifndef FIX_POINT
	ptv->setgainthold	= isdbt_set_agc_thold;
#endif
	ptv->getrssi			= isdbt_get_rssi;
#ifndef FIX_POINT
	ptv->getfreqoffset	= isdbt_get_frequency_offset;
	ptv->gettimeoffset	= isdbt_get_time_offset; 
#endif
	ptv->pwrdown		= isdbt_power_down;
	ptv->pwrup				= isdbt_power_up;
	ptv->sleeptimer		= isdbt_sleep_timer;
	ptv->pwmctl			= isdbt_pwm_ctl;
	ptv->dacctl				= isdbt_dac_ctl;
	ptv->combine			= isdbt_switch_combiner;
	ptv->setgain			= isdbt_set_gain;
	ptv->tscfg				= isdbt_ts_cfg;
	ptv->tsctl				= isdbt_ts_ctl;
	ptv->gettmccinfo	= isdbt_get_tmcc_info;
	ptv->updatepll		= isdbt_update_pll;
	ptv->handleintr		= isdbt_handle_intr;

	ptv->getdoppler		= isdbt_get_doppler_frequency;
#ifndef FIX_POINT
	ptv->getchanlen		= isdbt_get_channel_length;
#endif
	ptv->enableirq		= isdbt_irq_enable;
	ptv->bertimer		= isdbt_set_ber_timer;
	ptv->getberb			= isdbt_get_ber_before;
	ptv->chkpll				= isdbt_check_pll_reset;
	ptv->dsm				= isdbt_switch_dsm;
	ptv->dsmstate		= isdbt_get_dsm_state;
	ptv->dsmlock			= isdbt_get_dsm_master_lock;
	ptv->addpid			= isdbt_add_pid;
	ptv->removepid		= isdbt_remove_pid;
	ptv->pidfilter			= isdbt_pid_filter;
	ptv->resetpid			= isdbt_reset_pid;
	ptv->encryption		= isdbt_aes_ctl;
#ifndef FIX_POINT
	ptv->getacqtime	= isdbt_get_acq_time;
#endif
	ptv->highxtaldrive = isdbt_high_xtal_drive;
	ptv->spidma			= isdbt_spi_dma;
	ptv->configspipad	= isdbt_config_spi_pad;
	ptv->dtvtspattern	= isdbt_ts_test_pattern;
}

