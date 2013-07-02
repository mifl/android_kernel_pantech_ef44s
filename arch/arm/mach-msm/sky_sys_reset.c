/* Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 * MSM architecture driver to reset the modem
 */

#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/io.h>

#include "sky_sys_reset.h"
#include "smd_private.h"
#include <mach/msm_rpcrouter.h>
#include <mach/oem_rapi_client.h>
#include <mach/msm_smsm.h>
#include <mach/msm_iomap.h>

#define RESTART_REASON_ADDR 0x65C

#define DEBUG
/* #undef DEBUG */
#ifdef DEBUG
#define D(x...) printk(x)
#else
#define D(x...) do {} while (0)
#endif

typedef enum { 
	USER_RESET = 0x00000000,
	SW_RESET = 0x00000001,
	PDL_RESET = 0x00000002,
}SYS_RST_RESET_TYPE_E;

typedef enum{
	MAIN_LCD_BL_OFF = 0x00000000,
	MAIN_LCD_BL_ON  = 0x0A0F090F,
}SYS_RST_LCD_BL_STATE_E;
 
typedef enum{
	RST_LCD_BL_OFF=0x00000000,
	RST_LCD_BL_ON =0x00000001,
	RST_LCD_BL_USER=0x00000002, 
}SYS_RST_LCD_BL_E;

//p14527 add to fix smpl
bool pantech_is_smpl=false;

/*
* ** FUNCTION DEFINATION ***
*/
void sky_sys_rst_set_silent_boot_info(void)
{
	void *restart_addr = NULL;
	unsigned reboot_mode = 0;
	
	oem_pm_smem_vendor1_data_type *smem_vendor1_data;
	smem_vendor1_data = smem_alloc(SMEM_ID_VENDOR1, sizeof(oem_pm_smem_vendor1_data_type));
	
	restart_addr = MSM_IMEM_BASE + RESTART_REASON_ADDR;
	reboot_mode = __raw_readl(restart_addr);
	
	smem_vendor1_data->backlight_off = 1;
	
	if(((reboot_mode >> 24)&0xff)==0x7f)
	{
		smem_vendor1_data->backlight_off = 0;
		reboot_mode &= 0xf7ffffff; 
	}
	
	__raw_writel(0x00, restart_addr);
	
	switch(reboot_mode)
	{
		case SYS_RESET_REASON_EXCEPTION:
		case SYS_RESET_REASON_ASSERT:
		case SYS_RESET_REASON_LINUX:
		case SYS_RESET_REASON_ANDROID:
		case SYS_RESET_REASON_LPASS:
		case SYS_RESET_REASON_DSPS:
		case SYS_RESET_REASON_RIVA:
		case SYS_RESET_REASON_UNKNOWN:
		case SYS_RESET_REASON_ABNORMAL:
#ifdef FEATURE_PANTECH_AUTO_REPAIR		
        case SYS_RESET_REASON_USERDATA_FS:
#endif /* FEATURE_PANTECH_AUTO_REPAIR */
		case SYS_RESET_REASON_DOGBARK:
			smem_vendor1_data->silent_boot_mode = 1;
			break;
	
		default :
			if(!smem_vendor1_data->silent_boot_mode)
				smem_vendor1_data->silent_boot_mode = 0;
			else
				pantech_is_smpl = true;
			break;
	} 
	printk(KERN_INFO "reboot_mode: 0x%x\n", reboot_mode);
}
EXPORT_SYMBOL(sky_sys_rst_set_silent_boot_info);

uint8_t sky_sys_rst_is_silent_boot_mode(void)
{
	oem_pm_smem_vendor1_data_type *smem_vendor1_data;
	smem_vendor1_data = smem_alloc(SMEM_ID_VENDOR1, sizeof(oem_pm_smem_vendor1_data_type));
	
	return smem_vendor1_data->silent_boot_mode;
}
EXPORT_SYMBOL(sky_sys_rst_is_silent_boot_mode);

uint8_t sky_sys_rst_is_backlight_off(void)
{
	oem_pm_smem_vendor1_data_type *smem_vendor1_data;
	smem_vendor1_data = smem_alloc(SMEM_ID_VENDOR1, sizeof(oem_pm_smem_vendor1_data_type));
	
	printk(KERN_INFO "[%s] backlight show= %d\n",__func__,smem_vendor1_data->backlight_off);

	//p14527 add to fix smpl
	if(pantech_is_smpl)
		smem_vendor1_data->backlight_off = 1;

	return smem_vendor1_data->backlight_off;
}
EXPORT_SYMBOL(sky_sys_rst_is_backlight_off);

void sky_sys_rst_is_silent_boot_backlight(int backlight)
{
	oem_pm_smem_vendor1_data_type *smem_vendor1_data;
	smem_vendor1_data = smem_alloc(SMEM_ID_VENDOR1, sizeof(oem_pm_smem_vendor1_data_type));
	
	smem_vendor1_data->backlight_off		= backlight;
	printk(KERN_INFO "[%s] backlight store= %d\n",__func__,backlight);
}
EXPORT_SYMBOL(sky_sys_rst_is_silent_boot_backlight);
void sky_sys_rst_is_silent_boot_for_test(int silent_mode,int backlight)
{
	oem_pm_smem_vendor1_data_type *smem_vendor1_data;
	smem_vendor1_data = smem_alloc(SMEM_ID_VENDOR1, sizeof(oem_pm_smem_vendor1_data_type));
	
	smem_vendor1_data->backlight_off		= backlight;
	smem_vendor1_data->silent_boot_mode	= silent_mode;
	printk(KERN_INFO "[%s] backlight store : backlight%d,silent_mode %d\n",__func__,backlight,silent_mode);
}
EXPORT_SYMBOL(sky_sys_rst_is_silent_boot_for_test);
