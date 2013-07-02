/* Copyright (c) 2011, PANTECH. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#ifndef _SENSOR_CTRL_H_
#define _SENSOR_CTRL_H_

#include <linux/types.h>


typedef struct {
	int id;
	const char *label;
	uint32_t nr;
} sgpio_ctrl_t;

typedef struct {
	int id;
	const char *vname;
	struct regulator *vreg;
	uint32_t arg;
} svreg_ctrl_t;


int sgpio_init (sgpio_ctrl_t *gpios, uint32_t sz);
int sgpio_ctrl (sgpio_ctrl_t *gpios, int id, int val);
void sgpio_release (sgpio_ctrl_t *gpios, uint32_t sz);

int svreg_init (svreg_ctrl_t *vregs, uint32_t sz);
int svreg_ctrl (svreg_ctrl_t *vregs, int id, int val);
void svreg_release (svreg_ctrl_t *vregs, uint32_t sz);

#endif /* _SENSOR_CTRL_H_ */
