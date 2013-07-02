/* arch/arm/mach-msm/apanic_pantech.h
 *
 * Copyright (C) 2012 PANTECH, Co. Ltd.
 * based on drivers/misc/apanic.c
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __PANTECH_APANIC_H__
#define __PANTECH_APANIC_H__
extern void pantech_force_dump_key(unsigned int code, int value);
extern void pantech_pm_log(int percent_soc, unsigned int cable_type);
#endif
