/* * Gadget Driver for Android * * Copyright (C) 2009 Motorola, Inc.  * Author: * * This software is licensed under the terms of the GNU General Public * License version 2, as published by the Free Software Foundation, and * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __F_PANTECH_ANDROID_H
#define __F_PANTECH_ANDROID_H

#define MSC_TYPE_FLAG         0x01
#define ADB_TYPE_FLAG         0x02
#define ETH_TYPE_FLAG         0x04
#define MTP_TYPE_FLAG         0x08
#define ACM_TYPE_FLAG         0x10
#define CDROM_TYPE_FLAG       0x20
#define DIAG_TYPE_FLAG        0x40
#define RNDIS_TYPE_FLAG       0x80
#define RMNET_TYPE_FLAG       0x100
#define ACCESSORY_TYPE_FLAG   0x200
#define OBEX_TYPE_FLAG        0x400
#define PTP_TYPE_FLAG	      0x800
#define DEFAULT_TYPE_FLAG     0x1000

#define PC_MODE				0
#define WINDOW_MEDIA_SYNC	1
#define USB_MASS_STORAGE	2
#define CHARGE_ONLY			3
#define FIELD_MODE			4
#define FACTORY_MODE		5
#define PTP_MODE		6

#define MAX_USB_TYPE_NUM	8	




void usb_interface_enum_cb(int flag);
void usb_data_transfer_callback(void);
void mode_switch_cb(int mode);
int type_switch_cb(int type);
int adb_enable_access(void);

#ifdef CONFIG_ANDROID_PANTECH_USB_CDFREE
void pst_req_mode_switch_cb(int pst_mode);
int pst_rsp_current_mode(void);
#endif

#endif /* __F_PANTECH_ANDROID_H */
