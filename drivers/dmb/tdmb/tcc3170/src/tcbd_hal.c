/*
 * tcbd_hal.c
 *
 * Author:  <linux@telechips.com>
 * Description: Telechips broadcast driver
 *
 * Copyright (c) Telechips, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/delay.h>

//#include <bsp.h>

#include "tcpal_os.h"
#include "tcpal_debug.h"
#include "tcbd_hal.h"
#include "tcbd_feature.h"


void tchal_init(void)
{
	tchal_power_down_device();
}

void tchal_reset_device(void)
{
#ifdef __USE_TC_CPU__
	tcbd_debug(DEBUG_TCHAL, "\n");
	/* reset */
	BITCLR(RGPIO->GPDFN1, Hw4 - Hw0); /* DXB0_RST# Set GPIO mode */
	BITSET(RGPIO->GPDEN,  Hw8); /* DXB0_RST# Set GPIO Output mode*/
	BITCLR(RGPIO->GPDDAT, Hw8); /* DXB0_RST# Clear */
	tcpal_msleep(10);
	BITSET(RGPIO->GPDDAT, Hw8); /* DXB0_RST# Set*/
#endif
}

void tchal_power_on_device(void)
{
#ifdef __USE_TC_CPU__
	tcbd_debug(DEBUG_TCHAL, "\n");
	BITCLR(RGPIO->GPEFN0, Hw16 - Hw12);/* DXB1_PD Set GPIO mode*/

	BITSET(RGPIO->GPEEN,  Hw3);/* DXB1_PD Set GPIO Output mode*/
	BITCLR(RGPIO->GPEDAT, Hw3);/* DXB1_PD Clear*/
	tcpal_msleep(10);
	BITSET(RGPIO->GPEDAT, Hw3);/* DXB1_PD Set*/
	tcpal_msleep(10);

	tchal_reset_device();
	tchal_irq_setup();
#endif
}


void tchal_power_down_device(void)
{
#ifdef __USE_TC_CPU__
	tcbd_debug(DEBUG_TCHAL, "\n");
	BITCLR(RGPIO->GPEFN0, Hw16 - Hw12);
	BITSET(RGPIO->GPEEN,  Hw3);/* DXB1_PD Set GPIO Output mode*/
	BITCLR(RGPIO->GPEDAT, Hw3);/* DXB1_PD Clear*/
	BITCLR(RGPIO->GPDDAT, Hw8);/* DXB0_RST# Clear */
#endif
}

void tchal_irq_setup(void)
{
#ifdef __USE_TC_CPU__
	BITCLR(RGPIO->GPDFN1, Hw8 - Hw4); /* DXB0_IRQ Set GPIO mode*/
	BITCLR(RGPIO->GPDEN, Hw9);        /* DXB0_IRQ input mode*/

	BITCSET(RGPIO->EINTSEL0, Hw32 - Hw24, 20<<24); /*GPIO_D9*/
	BITSET(RPIC->POL0, 1<<IRQ_TC317X);
#endif
}
