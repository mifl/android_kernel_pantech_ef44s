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

#include <linux/types.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include "sensor_ctrl.h"


#undef CDBG
#undef CERR
#if defined(CONFIG_PANTECH_CAMERA) && defined(FEATURE_AARM_RELEASE_MODE)
#define CDBG(fmt, args...) do { } while (0)
#define CINFO(fmt, args...) do { } while (0)
#else
#define CDBG(fmt, args...) printk(KERN_DEBUG fmt, ##args)
#define CINFO(fmt, args...) printk(KERN_INFO fmt, ##args)
#endif
#define CERR(fmt, args...) printk(KERN_ERR fmt, ##args)


int sgpio_init(sgpio_ctrl_t *gpios, uint32_t sz)
{
	uint32_t i = 0;
	int rc = 0;

	CINFO("%s E\n", __func__);

	for (i = 0; i < sz; i++) {
		rc = gpio_request(gpios[i].nr, gpios[i].label);
		if (rc < 0) {
			CERR("%s req err(%d, %s)\n", __func__, rc, gpios[i].label);
			return rc;
		}
		rc = gpio_direction_output(gpios[i].nr, 0);
		if (rc < 0) {
			CERR("%s dir_out err(%d, %s)\n", __func__, rc, gpios[i].label);
			return rc;
		}
	}

	CINFO("%s X\n", __func__);
	return 0;
}


int sgpio_ctrl(sgpio_ctrl_t *gpios, int id, int val)
{
	int rc = 0;

	/* PANTECH_CAMERA_TODO, How to verify id? */

	if (val != 0)
		val = 1;

	rc = gpio_direction_output(gpios[id].nr, val);
	if (rc < 0) {
		CERR("%s err(%d, %s)\n", __func__, rc, gpios[id].label);
		return rc;
	}

	CINFO("%s set %s to %d\n", __func__, gpios[id].label, val);
	return 0;
}


void sgpio_release(sgpio_ctrl_t *gpios, uint32_t sz)
{
	uint32_t i = 0;

	CINFO("%s E\n", __func__);

	for (i = 0; i < sz; i++)
		gpio_free(gpios[i].nr);

	CINFO("%s X\n", __func__);
}


int svreg_init(svreg_ctrl_t *vregs, uint32_t sz)
{
	struct regulator *vreg = NULL;
	int uvolt = 0;
	uint32_t i = 0;
	int rc = 0;

	CINFO("%s E\n", __func__);

	for (i = 0; i < sz; i++) {
		vreg = regulator_get(NULL, vregs[i].vname);
		if (IS_ERR(vreg)) {
			CERR("%s err(-EBUSY, %s)\n", __func__, vregs[i].vname);
			return -EBUSY;
		}

		vregs[i].vreg = vreg;

		/* Some voltage sources don't have interface to set voltage 
		 * levels. You can check these limitations in appropriate 
		 * regulator platform drivers.
		 * (e.g. pmic8901-regulator.c, pmic8058-regulator.c,...)
		 * So we don't call 'regulator_set_voltage' for this kind 
		 * of voltage sources. IOW you should set voltage level
		 * correctly in board initialization code.
		 * (e.g. static struct rpm_vreg_pdata rpm_vreg_init_pdata[]) */
		if (vregs[i].arg == 0)
			continue;

		uvolt = vregs[i].arg * 1000;

		rc = regulator_set_voltage(vreg, uvolt, uvolt);
		if (rc < 0) {
			CERR("%s err(%d, %s)\n", __func__, rc, vregs[i].vname);
			return rc;
		}
	}

	CINFO("%s X\n", __func__);
	return 0;
}


int svreg_ctrl(svreg_ctrl_t *vregs, int id, int val)
{
	int rc = 0;

	/* PANTECH_CAMERA_TODO, How to verify id? */

	if (!vregs[id].vreg) {
		CERR("%s (-ENOENT, %d)\n", __func__, id);
		return -ENOENT;
	}

	if (val) {
		rc = regulator_enable(vregs[id].vreg);
		if (rc) {
			CERR("%s on err(%d, %s)\n", __func__, rc,
				vregs[id].vname);
			return rc;
		}
	} else {
		rc = regulator_disable(vregs[id].vreg);
		if (rc) {
			CERR("%s off err(%d, %s)\n", __func__, rc,
				vregs[id].vname);
			return rc;
		}
	}

	CINFO("%s set %s to %d\n", __func__, vregs[id].vname, val);
	return 0;
}


void svreg_release(svreg_ctrl_t *vregs, uint32_t sz)
{
	uint32_t i = 0;

	CINFO("%s E\n", __func__);

	for (i = 0; i < sz; i++)
		if (vregs[i].vreg)
			regulator_put(vregs[i].vreg);

	CINFO("%s X\n", __func__);
}
