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
#include <linux/delay.h>
#include <linux/i2c.h>
#include "sensor_i2c.h"


#undef CDBG
#undef CERR
#if 0//charley2
#define CDBG(fmt, args...) printk(KERN_DEBUG fmt, ##args)
#define CINFO(fmt, args...) printk(KERN_INFO fmt, ##args)
#define CERR(fmt, args...) printk(KERN_ERR fmt, ##args)
#else
#define CDBG(fmt, args...) do{}while(0)
#define CINFO(fmt, args...) do{}while(0)
#define CERR(fmt, args...) do{}while(0)
#endif

#define SI2C_RETRY_CNT		3	// I2C retry count
#define SI2C_RETRY_MS		200	// I2C retry period (ms)
#define SI2C_POLL_MS		10	// I2C polling period (ms)
#define SI2C_BWR_FIFO_SZ	256	// I2C burst write block size

#define si2c_proc_cmd(id, parm1, parm2)	si2c_proc_cmd_##id(parm1, parm2)


static int si2c_tx(uint16_t sa, uint8_t *wd, uint16_t wd_sz);
static int si2c_rx(uint16_t sa, uint8_t *ra, uint16_t ra_sz, 
			uint8_t *rd, uint16_t rd_sz);

static int si2c_proc_cmd_SI2C_WR(uint16_t sa, si2c_cmd_t *cmd);
static int si2c_proc_cmd_SI2C_BWR(uint16_t sa, si2c_cmd_t *cmd);
static int si2c_proc_cmd_SI2C_RD(uint16_t sa, si2c_cmd_t *cmd);
static int si2c_proc_cmd_SI2C_DELAY(uint16_t sa, si2c_cmd_t *cmd);
static int si2c_proc_cmd_SI2C_POLL(uint16_t sa, si2c_cmd_t *cmd);
static int si2c_proc_cmd_SI2C_POLLN(uint16_t sa, si2c_cmd_t *cmd);
static int si2c_proc_cmd_SI2C_POLL_B1AND(uint16_t sa, si2c_cmd_t *cmd);
static int si2c_proc_cmd_SI2C_POLL_B1OR(uint16_t sa, si2c_cmd_t *cmd);
static int si2c_proc_cmd_SI2C_POLL_B0AND(uint16_t sa, si2c_cmd_t *cmd);
static int si2c_proc_cmd_SI2C_POLL_B0OR(uint16_t sa, si2c_cmd_t *cmd);
static int si2c_proc_cmd_SI2C_POLLV(uint16_t sa, si2c_cmd_t *cmd);

static inline uint8_t SI2C_W2BM(uint16_t w)
{
	return (uint8_t)((w & 0xff00) >> 8);
}

static inline uint8_t SI2C_W2BL(uint16_t w)
{
	return (uint8_t)(w & 0x00ff);
}

static inline uint16_t SI2C_B2W(uint8_t m, uint8_t l)
{
	return (((uint16_t)m << 8) | (uint16_t)l);
}


static struct i2c_adapter *si2c_adap = NULL;


int si2c_init(
	struct i2c_adapter *adap,
	si2c_const_param_t *src, si2c_param_t *dst)
{
	si2c_pid_t i = SI2C_PID_MAX;
	si2c_pid_t j = SI2C_PID_MAX;

	CINFO("%s E\n", __func__);

	if (!adap || !src || !dst) {
		CERR("%s err(-EINVAL)\n", __func__); 
		return -EINVAL;
	}

	if (si2c_adap != NULL) {
		CERR("%s err(-EBUSY)\n", __func__); 
		return -EBUSY;
	}

	si2c_adap = adap;

	for (i = 0; i < SI2C_PID_MAX; i++)
		for (j = 0; j < SI2C_PID_MAX; j++)
			if ((src[j].id == i) && (src[j].cmds != NULL)) {
				dst[i].id = src[j].id;
				dst[i].cmds = (si2c_cmd_t *)src[j].cmds;
				break;
			}

	CINFO("%s X\n", __func__);
	return 0;
}


void si2c_release(void)
{
	CINFO("%s E\n", __func__);

	if (si2c_adap != NULL)
		si2c_adap = NULL;

	CINFO("%s X\n", __func__);
}


int si2c_write_param(uint16_t sa, si2c_pid_t pid, si2c_param_t *params)
{
	int rc = 0;

	CINFO("%s E pid=%d\n", __func__, pid);

	if ((pid < 0) || (pid >= SI2C_PID_MAX) || !params) {
		CERR("%s err(-EINVAL)\n", __func__);
		return -EINVAL;
	}

	if (params[pid].cmds == NULL) {
		CERR("%s err(-ENOENT)\n", __func__);
		return -ENOENT;
	}

	rc = si2c_write_cmds(sa, params[pid].cmds);

	CINFO("%s X (%d)\n", __func__, rc);
	return rc;
}


int si2c_write_cmds(uint16_t sa, si2c_cmd_t *cmds)
{
	int rc = 0;

	CINFO("%s E\n", __func__);

	if (!cmds) {
		CERR("%s err(-EINVAL)\n", __func__);
		return -EINVAL;
	}

	while (cmds->id != SI2C_EOC) {
		CDBG("%s id=%d\n", __func__, cmds->id);

		switch (cmds->id) {
		case SI2C_NOP:
			break;
		case SI2C_WR:
			rc = si2c_proc_cmd(SI2C_WR, sa, cmds);
			break;
		case SI2C_BWR:
			rc = si2c_proc_cmd(SI2C_BWR, sa, cmds);
			break;
		case SI2C_RD:
			rc = si2c_proc_cmd(SI2C_RD, sa, cmds);
			break;
		case SI2C_DELAY:
			rc = si2c_proc_cmd(SI2C_DELAY, sa, cmds);
			break;
		case SI2C_POLL:
			rc = si2c_proc_cmd(SI2C_POLL, sa, cmds);
			break;
		case SI2C_POLLN:
			rc = si2c_proc_cmd(SI2C_POLLN, sa, cmds);
			break;
		case SI2C_POLL_B1AND:
			rc = si2c_proc_cmd(SI2C_POLL_B1AND, sa, cmds);
			break;
		case SI2C_POLL_B1OR:
			rc = si2c_proc_cmd(SI2C_POLL_B1OR, sa, cmds);
			break;
		case SI2C_POLL_B0AND:
			rc = si2c_proc_cmd(SI2C_POLL_B0AND, sa, cmds);
			break;
		case SI2C_POLL_B0OR:
			rc = si2c_proc_cmd(SI2C_POLL_B0OR, sa, cmds);
			break;
		case SI2C_POLLV:
			rc = si2c_proc_cmd(SI2C_POLLV, sa, cmds);
			break;
		default:
			CERR("%s err(-EINVAL)\n", __func__);
			rc = -EINVAL;
			break;
		}

		if (rc < 0)
			return rc;

		if (cmds->id == SI2C_POLLV)
			cmds += 2;
		else
			cmds += 1;
	}

	CINFO("%s X\n", __func__);
	return 0;
}


int si2c_write(si2c_cw_t w, uint16_t sa, uint16_t wa, uint16_t wd)
{
	uint8_t b[4] = {0, 0, 0, 0};
	int rc = 0;

	switch (w) {
	case SI2C_A1D1:
		b[0] = SI2C_W2BL(wa);
		b[1] = SI2C_W2BL(wd);
		rc = si2c_tx(sa, b, 2);
		break;
	case SI2C_A1D2:
		b[0] = SI2C_W2BL(wa);
		b[1] = SI2C_W2BM(wd);
		b[2] = SI2C_W2BL(wd);
		rc = si2c_tx(sa, b, 3);
		break;
	case SI2C_A2D1:
		b[0] = SI2C_W2BM(wa);
		b[1] = SI2C_W2BL(wa);
		b[2] = SI2C_W2BL(wd);
		rc = si2c_tx(sa, b, 3);
		break;
	case SI2C_A2D2:
		b[0] = SI2C_W2BM(wa);
		b[1] = SI2C_W2BL(wa);
		b[2] = SI2C_W2BM(wd);
		b[3] = SI2C_W2BL(wd);
		rc = si2c_tx(sa, b, 4);
		break;
	default:
		CERR("%s err(-EINVAL)\n", __func__);
		return -EINVAL;
	}

	return rc;
}


int si2c_read(si2c_cw_t w, uint16_t sa, uint16_t ra, uint16_t *rd)
{
	uint8_t a[2] = {0, 0}, d[2] = {0, 0};
	int rc = 0;

	if (!rd) {
		CERR("%s err(-EINVAL)\n", __func__);
		return -EINVAL;
	}

	switch (w) {
	case SI2C_A1D1:
		a[0] = SI2C_W2BL(ra);
		rc = si2c_rx(sa, a, 1, d, 1);
		*rd = SI2C_B2W(0, d[0]);
		break;
	case SI2C_A1D2:
		a[0] = SI2C_W2BL(ra);
		rc = si2c_rx(sa, a, 1, d, 2);
		*rd = SI2C_B2W(d[0], d[1]);
		break;
	case SI2C_A2D1:
		a[0] = SI2C_W2BM(ra);
		a[1] = SI2C_W2BL(ra);
		rc = si2c_rx(sa, a, 2, d, 1);
		*rd = SI2C_B2W(0, d[0]);
		break;
	case SI2C_A2D2:
		a[0] = SI2C_W2BM(ra);
		a[1] = SI2C_W2BL(ra);
		rc = si2c_rx(sa, a, 2, d, 2);
		*rd = SI2C_B2W(d[0], d[1]);
		break;
	default:
		CERR("%s err(-EINVAL)\n", __func__);
		return -EINVAL;
	}
	
	return rc;
}


static int si2c_tx(uint16_t sa, uint8_t *wd, uint16_t wd_sz)
{
	int i = 0;
	int rc = 0;

	struct i2c_msg msg[] = {
		{
			.addr = sa,
			.flags = 0,
			.len = wd_sz,
			.buf = wd,
		},
	};

	if (!si2c_adap) {
		CERR("%s err(-EFAULT)\n", __func__);
		return -EFAULT;
	}

	if (!wd || (wd_sz == 0)) {
		CERR("%s err(-EINVAL)\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < SI2C_RETRY_CNT; i++) {
		rc = i2c_transfer(si2c_adap, msg, 1);
		if (rc > 0) {
			CDBG("%s <%d> %02x.%02x.%02x.%02x.%02x\n", 
				__func__, wd_sz, sa,
				(wd_sz >= 1) ? *wd : 0,
				(wd_sz >= 2) ? *(wd + 1) : 0,
				(wd_sz >= 3) ? *(wd + 2) : 0,
				(wd_sz >= 4) ? *(wd + 3) : 0);
			return 0;
		}
		CERR("%s err(%d), retry tx\n", __func__, rc);
		msleep(SI2C_RETRY_MS);
	}

	CERR("%s err(%d) <%d> %02x.%02x.%02x.%02x.%02x\n", 
		__func__, rc, wd_sz, sa,
		(wd_sz >= 1) ? *wd : 0, (wd_sz >= 2) ? *(wd + 1) : 0,
		(wd_sz >= 3) ? *(wd + 2) : 0, (wd_sz >= 4) ? *(wd + 3) : 0);
	return rc;
}


static int si2c_rx(
	uint16_t sa, uint8_t *ra, uint16_t ra_sz, 
	uint8_t *rd, uint16_t rd_sz)
{
	int i = 0;
	int rc = 0;

	struct i2c_msg msgs[] = {
	{
		.addr  = sa,
		.flags = 0,
		.len   = ra_sz,
		.buf   = ra,
	},
	{
		.addr  = sa,
		.flags = I2C_M_RD,
		.len   = rd_sz,
		.buf   = rd,
	},
	};

	if (!si2c_adap) {
		CERR("%s err(-EFAULT)\n", __func__);
		return -EFAULT;
	}

	if (!ra || (ra_sz == 0) || !rd || (rd_sz == 0)) {
		CERR("%s err(-EINVAL)\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < SI2C_RETRY_CNT; i++) {
		rc = i2c_transfer(si2c_adap, msgs, 2);
		if (rc > 0) {
			CDBG("%s <%d/%d> %02x.%02x.%02x.%02x.%02x\n", 
				__func__, ra_sz, rd_sz, sa,
				(ra_sz >= 1) ? *ra : 0,
				(ra_sz >= 2) ? *(ra + 1) : 0,
				(rd_sz >= 1) ? *rd : 0,
				(rd_sz >= 2) ? *(rd + 1) : 0);
			return 0;
		}
		CERR("%s err(%d), retry rx\n", __func__, rc);
		msleep(SI2C_RETRY_MS);
	}

	CERR("%s err(%d) <%d> %02x.%02x.%02x\n", 
		__func__, rc, ra_sz, sa,
		(ra_sz >= 1) ? *ra : 0, (ra_sz >= 2) ? *(ra + 1) : 0);
	return rc;
}


static int si2c_proc_cmd_SI2C_WR(uint16_t sa, si2c_cmd_t *cmd)
{
	return si2c_write(cmd->w, sa, cmd->a, cmd->d);
}


static int si2c_proc_cmd_SI2C_BWR(uint16_t sa, si2c_cmd_t *cmd)
{
	static uint8_t b[SI2C_BWR_FIFO_SZ];
	static uint32_t cmd_cnt = 0;
	static uint16_t i = 0;
	int rc = 0;

	if ((cmd->w < 0) || (cmd->w >= SI2C_WIDTH_MAX)) {
		CERR("%s err(-EINVAL)\n", __func__);
		return -EINVAL;
	}

	/* first command, so put address first */
	if (cmd_cnt == 0) {
		if ((cmd->w == SI2C_A1D1) || (cmd->w == SI2C_A1D2))
			b[i++] = SI2C_W2BL(cmd->a);
		else {
			b[i++] = SI2C_W2BM(cmd->a);
			b[i++] = SI2C_W2BL(cmd->a);
		}
	}

	if ((cmd->w == SI2C_A1D1) || (cmd->w == SI2C_A2D1))
		b[i++] = SI2C_W2BL(cmd->d);
	else {
		b[i++] = SI2C_W2BM(cmd->d);
		b[i++] = SI2C_W2BL(cmd->d);
	}

	cmd_cnt++;

	/* end of BWR commands */
	if ((cmd + 1)->id != SI2C_BWR)
		goto start_burst_write;
	/* next cmd is BWR, but FIFO is full */
	else if (i >= SI2C_BWR_FIFO_SZ - 2 - 1)
		goto start_burst_write;

	return 0;

start_burst_write:

	CDBG("%s %d BWR commands, %d bytes\n", __func__, cmd_cnt, i);
	rc = si2c_tx(sa, b, i);

	cmd_cnt = 0;
	i = 0;

	return rc;
}


static int si2c_proc_cmd_SI2C_RD(uint16_t sa, si2c_cmd_t *cmd)
{
	uint16_t d = 0;
	/* PANTECH_CAMERA_TODO, do something with read data */
	return si2c_read(cmd->w, sa, cmd->a, &d);
}

static int si2c_proc_cmd_SI2C_DELAY(uint16_t sa, si2c_cmd_t *cmd)
{
	msleep(cmd->arg);
	return 0;
}


static int si2c_proc_cmd_SI2C_POLL(uint16_t sa, si2c_cmd_t *cmd)
{
	uint16_t d = 0;
	uint32_t i = 0;
	int rc = 0;

	for (i = 0; i < cmd->arg; i++) {
		rc = si2c_read(cmd->w, sa, cmd->a, &d);
		if (rc < 0)
			return rc;
		if (d == cmd->d)
			break;
		msleep(SI2C_POLL_MS);
	}

	if (i == cmd->arg) {
		CERR("%s err(-ETIMEDOUT)\n", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}


static int si2c_proc_cmd_SI2C_POLLN(uint16_t sa, si2c_cmd_t *cmd)
{
	uint16_t d = 0;
	uint32_t i = 0;
	int rc = 0;

	for (i = 0; i < cmd->arg; i++) {
		rc = si2c_read(cmd->w, sa, cmd->a, &d);
		if (rc < 0)
			return rc;
		if (d != cmd->d)
			break;
		msleep(SI2C_POLL_MS);
	}

	if (i == cmd->arg) {
		CERR("%s err(-ETIMEDOUT)\n", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}


static int si2c_proc_cmd_SI2C_POLL_B1AND(uint16_t sa, si2c_cmd_t *cmd)
{
	uint16_t d = 0;
	uint32_t i = 0;
	int rc = 0;

	for (i = 0; i < cmd->arg; i++) {
		rc = si2c_read(cmd->w, sa, cmd->a, &d);
		if (rc < 0)
			return rc;
		if ((cmd->d & d) == cmd->d)
			break;
		msleep(SI2C_POLL_MS);
	}

	if (i == cmd->arg) {
		CERR("%s err(-ETIMEDOUT)\n", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}


static int si2c_proc_cmd_SI2C_POLL_B1OR(uint16_t sa, si2c_cmd_t *cmd)
{
/*
	uint16_t d = 0;
	uint32_t i = 0;
	int rc = 0;

	for (i = 0; i < cmd->arg; i++) {
		rc = si2c_read(cmd->w, sa, cmd->a, &d);
		if (rc < 0)
			return rc;
		if (cmd->d & d)
			break;
		msleep(SI2C_POLL_MS);
	}

	if (i == cmd->arg)
		return -EIO;
*/
	return 0;
}


static int si2c_proc_cmd_SI2C_POLL_B0AND(uint16_t sa, si2c_cmd_t *cmd)
{
	uint16_t d = 0;
	uint32_t i = 0;
	int rc = 0;

	for (i = 0; i < cmd->arg; i++) {
		rc = si2c_read(cmd->w, sa, cmd->a, &d);
		if (rc < 0)
			return rc;
		if ((cmd->d & d) == 0)
			break;
		msleep(SI2C_POLL_MS);
	}

	if (i == cmd->arg) {
		CERR("%s err(-ETIMEDOUT)\n", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}


static int si2c_proc_cmd_SI2C_POLL_B0OR(uint16_t sa, si2c_cmd_t *cmd)
{
/*
	uint16_t d = 0;
	uint32_t i = 0;
	int rc = 0;

	for (i = 0; i < cmd->arg; i++) {
		rc = si2c_read(cmd->w, sa, cmd->a, &d);
		if (rc < 0)
			return rc;
		if (!(cmd->d & d))
			break;
		msleep(SI2C_POLL_MS);
	}

	if (i == cmd->arg)
		return -EIO;
*/
	return 0;
}


static int si2c_proc_cmd_SI2C_POLLV(uint16_t sa, si2c_cmd_t *cmd)
{
	si2c_cmd_t *cmd1 = NULL;
	si2c_cmd_t *cmd2 = NULL;
	uint16_t mask = 0;
	uint16_t poll_ms = 0;
	uint16_t poll_cnt = 0;
	uint16_t d = 0;
	uint16_t i = 0;
	int rc = 0;

/*PANTECH_CAMERA_TODO, How to verify next command? */

	BUG_ON(!cmd);

	cmd1 = cmd++;
	cmd2 = cmd;

	BUG_ON(cmd2->id != SI2C_POLLV);

	mask = (uint16_t)(cmd1->arg);
	poll_ms = (uint16_t)((cmd2->arg & 0xffff0000) >> 16);
	poll_cnt = (uint16_t)(cmd2->arg);

	BUG_ON((poll_ms == 0) || (poll_cnt == 0));

	for (i = 0; i < poll_cnt; i++) {
		if (si2c_write(cmd1->w, sa, cmd1->a, cmd1->d) < 0) rc = -EIO;
		if (si2c_read(cmd2->w, sa, cmd2->a, &d) < 0) rc = -EIO;
		if (rc < 0) {
			CERR("%s err(%d)\n", __func__, rc);
			return rc;
		}

		if ((d & mask) == cmd2->d)
			break;

		msleep(poll_ms);
	}

	if (i == poll_cnt) {
		CERR("%s err(-ETIMEDOUT)\n", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}
