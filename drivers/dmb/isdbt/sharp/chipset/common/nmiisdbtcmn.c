/******************************************************************************
**
**	Copyright (c) Newport Media Inc.  All rights reserved.
**
** 	Module Name:  nmicmn.c
**		This module is used to wrap the ASIC configuration file.  It exported only one function
**		to the outside world.  The idea is to minimize the global functions usage to simplify
**		the porting efforts.		
**	
**
** 
** 	Version				Date					Author					Descriptions
** -----------		----------	-----------		------------------------------------	
**	1.4.0.12				8.28.2008			K.Yu					Alpha release 
**							9.23.2008									Add AGC gain threshold variables.
*******************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>

#include "../../ntv/inc/nmintvplatform.h"
#include "../inc/nmiisdbtcmn.h"
//#include <stdio.h>
//#include <stdarg.h>
#include <linux/math64.h>
#include <linux/string.h>


#include <linux/unistd.h>
#include <linux/slab.h>

#include "../../../isdbt_comdef.h"
#include "../../../isdbt_bb.h"
#include "../../../isdbt_test.h"

#include "../../../../dmb_interface.h"

#include "../../sharp_bb.h"

/******************************************************************************
**
**	Private Data Structure
**
*******************************************************************************/
typedef struct {
	uint32_t				chipid;
	uint32_t				frequency;

	uint32_t				pll1;
	uint32_t				norm1;
	uint32_t				norm2;
	uint32_t				sx5;
	uint32_t				sx4;
	uint32_t				ddfs1;
	int						scan;
	tNmiIsdbtMode	mode;
	tNmiGuard			guard;
	tNmiMod				modulation;
	tNmiCodeRate		coderate;
	uint32_t 				hi2mi_4;
	uint32_t 				mi2lo_4;
	uint32_t 				lo2by_4;
	uint32_t 				by2lo_4;
	uint32_t 				lo2mi_4;
	uint32_t 				mi2hi_4;

	int						notrack;

	int						pidpat;						

} tNmiPriv;

typedef struct {
	tNmiIsdbtIn		inp;
	tNmiPriv			priv[2];		
	int					buserror;
	int					pwrdown;
} tNmiAsic;

static tNmiAsic chip;

/******************************************************************************
**
**	Debug 
**
*******************************************************************************/

static void nmi_debug(uint32_t zone, char *fmt,...)
{
#if 1 // cys Android porting
	char buf[256];
	va_list args;
	int len;

	if (zone & chip.inp.zone) {
		va_start(args, fmt);
		strcpy(buf, "[Nmi Isdbt]: "); 
		len = vsprintf(&buf[13], fmt, args);
		va_end(args);
    ISDBT_MSG_SHARP_BB("%s",buf);

		//if (chip.inp.hlp.log) {
		//	chip.inp.hlp.log(buf);
		//}
	}
#endif
	return;
}

/******************************************************************************
**
**	Os Help Functions
**
*******************************************************************************/
static void nmi_delay(uint32_t msec)
{
	if (chip.inp.hlp.delay) {
		chip.inp.hlp.delay(msec);
	}
}

static uint32_t nmi_get_tick(void)
{
	uint32_t tick = 0;

	if (chip.inp.hlp.gettick) {
		tick = chip.inp.hlp.gettick();
	}

	return tick;
}

static void nmi_burst_read(void *pv)
{
	int isOverFlow = 0;
	isOverFlow = *(int*)pv;
	
	if (!(chip.inp.hlp.t.burstread &&
			chip.inp.hlp.t.read &&
			chip.inp.hlp.t.write) || chip.pwrdown)
	{
		nmi_debug(N_ERR, "Error, nmi_burst_read, can't access chipset...\n");	// tony
		return;
	}

	nmi_debug(N_INTR, "nmi_burst_read, overflow (%d)\n", isOverFlow);	// tony
	
	if (chip.inp.transporttype == nSPI) {
		int retry;
		uint8_t wb, rb;
		uint32_t count;

		wb = 0xc0;
		chip.inp.hlp.t.write(nMaster, &wb, 1);

		wb = 0x00;
		retry = 100;
  		do {
   			chip.inp.hlp.t.read(nMaster, &wb, 1, &rb, 1);
  		} while ((rb != 0xff) && retry--);	/* try for 100 times */

		if (rb != 0xff) {
	   		nmi_debug(N_ERR, "Burst Read: no response from the chip...\n");
	    	return;
		}

		wb = 0;
		chip.inp.hlp.t.read(nMaster, &wb, 1, &rb, 1);
		count = rb << 16;
		chip.inp.hlp.t.read(nMaster, &wb, 1, &rb, 1);
		count |= rb << 8;
		chip.inp.hlp.t.read(nMaster, &wb, 1, &rb, 1);
		count |= rb;

		nmi_debug(N_INTR, "nmi_burst_read, count (%d)\n", count);	// tony

		if ((count > 0) && (count <= (40 * 188))) {
			chip.inp.hlp.t.burstread(count, isOverFlow);
		}
		wb = 0xff;
		chip.inp.hlp.t.write(nMaster, &wb, 1);
	}

	return;
}

/******************************************************************************
**
**	Bus Read/Write Functions
**
*******************************************************************************/

static uint8_t rReg8(tNmiIsdbtChip cix, uint32_t adr)
{
	uint8_t b[16];
	uint8_t val;
	int len;

	/**
		Check Read/Write pointers. Don't access chip while it is power down.
	**/
	if (!(chip.inp.hlp.write && chip.inp.hlp.read) ||
			chip.pwrdown ||
			chip.buserror) {
		nmi_debug(N_ERR, "Error, read register(8), can't access chipset\n");
		return 0;
	}		 

	if ((chip.inp.bustype == nI2C) || (chip.inp.bustype == nUSB)) {
		if ((adr >= 0x7000) && (adr <= 0x77ff)) {
			b[0] = 0x28;
		} else {
			b[0] = 0x20;							/* byte access */
		}
   		b[1] = (uint8_t)(adr >> 16);
   		b[2] = (uint8_t)(adr >> 8);
   		b[3] = (uint8_t)(adr);
   		b[4] = 0x00;
   		b[5] = 0x01;
		len = 6;

		if (chip.inp.hlp.read(cix, b, len, &val, 1) < 0) {
			nmi_debug(N_ERR, "Failed, bus read...\n");
			chip.buserror = 1;
		}

	} else if (chip.inp.bustype == nSPI) {
		uint8_t retry;
		uint8_t sta;
		uint32_t temp;

		b[0] = 0x50;								/* byte access */
   		b[1] = 0x00;
   		b[2] = 0x01;
   		b[3] = (uint8_t)(adr >> 16);
   		b[4] = (uint8_t)(adr >> 8);
   		b[5] = (uint8_t)(adr);
		len = 6;

		chip.inp.hlp.write(cix, b, len);

		/**
			Wait for complete
		**/
		retry = 100;
		temp = 0;
		do {
			chip.inp.hlp.read(cix, (uint8_t *)&temp, 1, &sta, 1);
			if (sta == 0xff)
				break;
			nmi_delay(1);
		} while (retry--);
		
		if (sta == 0xff) {
			uint32_t count;

			/**
				Read the Count
			**/
			b[0] = b[1] = b[2] = 0x00;
			chip.inp.hlp.read(cix, b, 3, (uint8_t *)&count, 3);
			
			/**
				Read the Data 
			**/
			temp = count;
			count = 0x0;
			count = ((temp >> 16) & 0xff);
			count |= (((temp >> 8) & 0xff) << 8);
			count |= ((temp & 0xff) << 16);
			temp = 0;
			if (count == 1) {
				chip.inp.hlp.read(cix, (uint8_t *)&temp, 1, &val, 1);
				temp = 0xff;
				chip.inp.hlp.write(cix,(uint8_t *)&temp,1);
			} else {
				nmi_debug(N_ERR, "Error, SPI bus, bad count (%d)\n", count);
				chip.buserror = 1;
			}
		} else {
			nmi_debug(N_ERR, "Error, SPI bus, not complete\n");
			chip.buserror = 1;
		}
	} 

	return val; 
}

static uint32_t rReg32(tNmiIsdbtChip cix, uint32_t adr)
{
	uint8_t b[16];
	uint32_t val;
	int len;

	/**
		Check Read/Write pointers. Don't access chip while it is power down or sleep.
	**/
	if (!(chip.inp.hlp.write && chip.inp.hlp.read) ||
			chip.pwrdown ||
			chip.buserror) {
		nmi_debug(N_ERR, "Error, read register(32), can't access chipset\n");
		return 0;
	}		 

	if ((chip.inp.bustype == nI2C) || (chip.inp.bustype == nUSB)) {
		if ((adr >= 0x7000) && (adr <= 0x77ff)) {
			b[0] = 0x28;
		} else {
			b[0] = 0x80;										/* word access */
		}
   		b[1] = (uint8_t)(adr >> 16);
   		b[2] = (uint8_t)(adr >> 8);
   		b[3] = (uint8_t)(adr);
   		b[4] = 0x00;
   		b[5] = 0x04;
		len = 6;

		if (chip.inp.hlp.read(cix, b, len, (uint8_t *)&val, 4) < 0) {
			nmi_debug(N_ERR, "Failed, bus read...\n");
			chip.buserror = 1;
		}
	} else if (chip.inp.bustype == nSPI) {
		uint8_t retry;
		uint8_t sta;
		uint32_t temp;

		b[0] = 0x70;								/* word access */
   		b[1] = 0x00;
   		b[2] = 0x04;
   		b[3] = (uint8_t)(adr >> 16);
   		b[4] = (uint8_t)(adr >> 8);
   		b[5] = (uint8_t)(adr);
		len = 6;

		chip.inp.hlp.write(cix, b, len);

		/**
			Wait for complete
		**/
		retry = 100;
		temp = 0;
		do {
			chip.inp.hlp.read(cix, (uint8_t *)&temp, 1, &sta, 1);
			if (sta == 0xff)
				break;
			//nmi_delay(1);
		} while (retry--);
		
		if (sta == 0xff) {
			uint32_t count;

			/**
				Read the Count
			**/
			b[0] = b[1] = b[2] = 0x00;
			chip.inp.hlp.read(cix, b, 3, (uint8_t *)&count, 3);
			
			/**
				Read the Data 
			**/
			temp = count;
			count = 0x0;
			count = ((temp >> 16) & 0xff);
			count |= (((temp >> 8) & 0xff) << 8);
			count |= ((temp & 0xff) << 16);
			temp = 0;
			if (count == 4) {
				chip.inp.hlp.read(cix, (uint8_t *)&temp, 4, (uint8_t *)&val, 4);
				temp = 0xff;
				chip.inp.hlp.write(cix,(uint8_t *)&temp,1);
			} else {
				nmi_debug(N_ERR, "Error, SPI bus, bad count (%d)\n", count);
				chip.buserror = 1;
			}
		} else {
			nmi_debug(N_ERR, "Error, SPI bus, not complete\n");
			chip.buserror = 1;
		}
	} 

	return val; 
}

static void wReg8(tNmiIsdbtChip cix, uint32_t adr, uint8_t val)
{
	uint8_t b[16];
	int len;

	/**
		Check Read/Write pointers. Don't access chip while it is sleep.
	**/
	if (!(chip.inp.hlp.write && chip.inp.hlp.read) ||
			(chip.pwrdown && (adr != 0x7000) && (adr != 0x7001)) ||
			chip.buserror) {
		nmi_debug(N_ERR, "Error, write register(8), can't access chipset\n");
		return;
	}		 

	if (chip.pwrdown) {
		chip.pwrdown = 0;
	}

	if ((chip.inp.bustype == nI2C) || (chip.inp.bustype == nUSB)) {
		if ((adr >= 0x7000) && (adr <= 0x77ff)) {
			b[0] = 0x38;
		} else {
			b[0] = 0x30;
		}
   		b[1] = (uint8_t)(adr >> 16);
   		b[2] = (uint8_t)(adr >> 8);
   		b[3] = (uint8_t)(adr);
   		b[4] = 0x00;
   		b[5] = 0x01;
		b[6] = val;
		len = 7;
	} else if (chip.inp.bustype == nSPI) {
		b[0] = 0x90;
   		b[1] = 0;
   		b[2] = 1;
   		b[3] = (uint8_t)(adr >> 16);
   		b[4] = (uint8_t)(adr >> 8);
   		b[5] = (uint8_t)(adr);
		b[6] = val;
		len = 7;
	} 
	
	if (chip.inp.hlp.write(cix, b, len) < 0) {
		nmi_debug(N_ERR, "Failed, bus write...\n");
		chip.buserror = 1;
	}

	return;
}


static void wReg32(tNmiIsdbtChip cix, uint32_t adr, uint32_t val)
{
	uint8_t b[16];
	int len;

	/**
		Check Read/Write pointers. Don't access chip while it is sleep.
	**/
	if (!(chip.inp.hlp.write && chip.inp.hlp.read) ||
			chip.pwrdown ||
			chip.buserror) {
		nmi_debug(N_ERR, "Error, write register(32), can't access chipset\n");
		return;
	}		 

	if ((chip.inp.bustype == nI2C) || (chip.inp.bustype == nUSB)) {
		b[0] = 0x90;
		b[1] = (uint8_t)(adr >> 16);
		b[2] = (uint8_t)(adr >> 8);
		b[3] = (uint8_t)adr;
		b[4] = 0x00;
		b[5] = 0x04;
		b[6] = (uint8_t)val;
		b[7] = (uint8_t)(val >> 8);
		b[8] = (uint8_t)(val >> 16);
		b[9] = (uint8_t)(val >> 24);
		len = 10;
	} else if (chip.inp.bustype == nSPI) {
		b[0] = 0xb0;
   		b[1] = 0;
   		b[2] = 4;
   		b[3] = (uint8_t)(adr >> 16);
   		b[4] = (uint8_t)(adr >> 8);
   		b[5] = (uint8_t)(adr);
		b[6] = (uint8_t)val;
		b[7] = (uint8_t)(val >> 8);
		b[8] = (uint8_t)(val >> 16);
		b[9] = (uint8_t)(val >> 24);
		len = 10;
	} 
	
	if (chip.inp.hlp.write(cix, b, len) < 0) {
		nmi_debug(N_ERR, "Failed, bus write...\n");
		chip.buserror = 1;
	}
	
	return; 
}

#if 0 // cys not used
static uint32_t rReg32_t(tNmiIsdbtChip cix, uint32_t adr)
{
	uint8_t b[16];
	uint32_t val;
	int len;

	/**
		Check Read/Write pointers. Don't access chip while it is power down or sleep.
	**/
	if (!(chip.inp.hlp.write && chip.inp.hlp.read) ||
			chip.pwrdown ||
			chip.buserror) {
		nmi_debug(N_ERR, "Error, read register(32), can't access chipset\n");
		return 0;
	}		 

 	{
		uint8_t retry;
		uint8_t sta;
		uint32_t temp;

		b[0] = 0x70;								/* word access */
   		b[1] = 0x00;
   		b[2] = 0x04;
   		b[3] = (uint8_t)(adr >> 16);
   		b[4] = (uint8_t)(adr >> 8);
   		b[5] = (uint8_t)(adr);
		len = 6;

		chip.inp.hlp.t.write(cix, b, len);

		/**
			Wait for complete
		**/
		retry = 100;
		temp = 0;
		do {
			chip.inp.hlp.t.read(cix, (uint8_t *)&temp, 1, &sta, 1);
			if (sta == 0xff)
				break;
			//nmi_delay(1);
		} while (retry--);
		
		if (sta == 0xff) {
			uint32_t count;

			/**
				Read the Count
			**/
			b[0] = b[1] = b[2] = 0x00;
			chip.inp.hlp.t.read(cix, b, 3, (uint8_t *)&count, 3);
			
			/**
				Read the Data 
			**/
			temp = count;
			count = 0x0;
			count = ((temp >> 16) & 0xff);
			count |= (((temp >> 8) & 0xff) << 8);
			count |= ((temp & 0xff) << 16);
			temp = 0;
			if (count == 4) {
				chip.inp.hlp.t.read(cix, (uint8_t *)&temp, 4, (uint8_t *)&val, 4);
				temp = 0xff;
				chip.inp.hlp.t.write(cix,(uint8_t *)&temp,1);
			} else {
				nmi_debug(N_ERR, "Error, SPI bus, bad count (%d)\n", count);
				chip.buserror = 1;
			}
		} else {
			nmi_debug(N_ERR, "Error, SPI bus, not complete\n");
			chip.buserror = 1;
		}
	} 

	return val; 
}

static void wReg32_t(tNmiIsdbtChip cix, uint32_t adr, uint32_t val)
{
	uint8_t b[16];
	int len;

	/**
		Check Read/Write pointers. Don't access chip while it is sleep.
	**/
	if (!(chip.inp.hlp.write && chip.inp.hlp.read) ||
			chip.pwrdown ||
			chip.buserror) {
		nmi_debug(N_ERR, "Error, write register(32), can't access chipset\n");
		return;
	}		 
	
	{
		b[0] = 0xb0;
   		b[1] = 0;
   		b[2] = 4;
   		b[3] = (uint8_t)(adr >> 16);
   		b[4] = (uint8_t)(adr >> 8);
   		b[5] = (uint8_t)(adr);
		b[6] = (uint8_t)val;
		b[7] = (uint8_t)(val >> 8);
		b[8] = (uint8_t)(val >> 16);
		b[9] = (uint8_t)(val >> 24);
		len = 10;
	} 
	
	if (chip.inp.hlp.t.write(cix, b, len) < 0) {
		nmi_debug(N_ERR, "Failed, bus write...\n");
		chip.buserror = 1;
	}
	
	return; 
}
#endif

/******************************************************************************
**
**	Includes Asic implementation
**
*******************************************************************************/

#if (NMI_TV_MODE == __ISDBT_ONLY_MODE__)
#include "nmiisdbt.c"
#else
#include "nmi625isdbt.c"
#endif

/******************************************************************************
**
**	Global Exported Initialization Function
**
*******************************************************************************/

typedef struct {
	tNmiBus type;
	int (*read)(tNmiIsdbtChip, uint8_t *, int, uint8_t *, int);
	int (*write)(tNmiIsdbtChip, uint8_t *, int);
} tNmiSpiWorkaround;

void nmi_spi_workaround(tNmiSpiWorkaround *inp)
{
	uint32_t val32;

	if (inp != NULL)
		memcpy((void *)&chip.inp, (void *)inp, sizeof(tNmiIsdbtIn));



	val32 = rReg32(nMaster, 0x644c);
	val32 &= ~(1 << 14);
	wReg32(nMaster, 0x644c, val32);

	val32 = rReg32(nMaster, 0x6408);
	val32 &= ~(0x7 << 24);
	val32 |= (0x6 << 24);
	wReg32(nMaster, 0x6408, val32);
	
}

void nmi_isdbt_common_init(tNmiIsdbtIn *inp, tNmiIsdbtVtbl *ptv)
{
	memset((void *)&chip, 0, sizeof(tNmiAsic));

	if (inp != NULL)
		memcpy((void *)&chip.inp, (void *)inp, sizeof(tNmiIsdbtIn));

	if (ptv != NULL) {
		isdbt_vtbl_init(ptv);
	}
	
	return;
}

