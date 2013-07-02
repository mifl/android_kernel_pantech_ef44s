/*
 * tcbd.h
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

#ifndef _TCC317x_DXB_H_
#define _TCC317x_DXB_H_

#include <linux/types.h>
#if 0
#define TCBD_DEV_FILE		"/dev/tcbd"
#define TCBD_DEV_NAME		"tcbd"
#define TCBD_DEV_MAJOR		244
#define TCBD_DEV_MINOR		0
#else
#define TCBD_DEV_FILE		"/dev/tc317x_dxb"
#define TCBD_DEV_NAME		"tc317x_dxb"
#define TCBD_DEV_MAJOR		243
#define TCBD_DEV_MINOR		0

#endif

/*#define IOCTL_TCBD_OFF	 _IO(TCBD_DEV_MAJOR, 1)
#define IOCTL_TCBD_ON	  _IO(TCBD_DEV_MAJOR, 2)*/
#define IOCTL_TCBD_RESET	  _IO(TCBD_DEV_MAJOR, 3)

#define IOCTL_TCBD_SET_FREQ	  _IO(TCBD_DEV_MAJOR, 5)
#define IOCTL_TCBD_SELECT_SUBCH  _IO(TCBD_DEV_MAJOR, 6)
#define IOCTL_TCBD_RELEASE_SUBCH _IO(TCBD_DEV_MAJOR, 7)

#define IOCTL_TCBD_TEST		  _IO(TCBD_DEV_MAJOR, 10)
#define IOCTL_TCBD_GET_MON_STAT  _IO(TCBD_DEV_MAJOR, 11)
#define IOCTL_TCBD_SET_INTR_THRESHOLD_SIZE _IO(TCBD_DEV_MAJOR, 12)
#define IOCTL_TCBD_RW_REG	_IO(TCBD_DEV_MAJOR, 13)
#define IOCTL_TCBD_READ_FIC	  _IO(TCBD_DEV_MAJOR, 14)
#define IOCTL_TCBD_GET_STREAM_IF _IO(TCBD_DEV_MAJOR, 15)
struct tcbd_ioctl_param_t {
  s32 cmd;
  u32 addr;
  s32 size;
  u8 reg_data;
};


struct tcbd_signal_quality {
  u32 SNR;
  u32 PCBER;
  u32 RSBER;
  u32 RSSI;
};

#endif
