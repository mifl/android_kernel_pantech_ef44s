/* Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
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

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_sony_wm.h"
#include <mach/gpio.h>
#include <asm/irq.h>
#include <asm/system.h>

#define GPIO_HIGH_VALUE 1
#define GPIO_LOW_VALUE  0

#define NOP()	do {asm volatile ("NOP");} while(0);
#define DELAY_3NS() do { \
    asm volatile ("NOP"); \
    asm volatile ("NOP"); \
    asm volatile ("NOP");} while(0);

#define LCD_DEBUG_MSG
//test
#ifdef LCD_DEBUG_MSG
#define ENTER_FUNC()        printk(KERN_INFO "[SKY_LCD] +%s \n", __FUNCTION__);
#define EXIT_FUNC()         printk(KERN_INFO "[SKY_LCD] -%s \n", __FUNCTION__);
#define ENTER_FUNC2()       printk(KERN_ERR "[SKY_LCD] +%s\n", __FUNCTION__);
#define EXIT_FUNC2()        printk(KERN_ERR "[SKY_LCD] -%s\n", __FUNCTION__);
#define PRINT(fmt, args...) printk(KERN_INFO fmt, ##args)
#define DEBUG_EN 1
#else
#define PRINT(fmt, args...)
#define ENTER_FUNC2()
#define EXIT_FUNC2()
#define ENTER_FUNC()
#define EXIT_FUNC()
#define DEBUG_EN 0
#endif

#if ((BOARD_VER == WS10) |(BOARD_VER == WS15))
#define FEATURE_WS10_SAMPLE
#elif (BOARD_VER == WS20)
#define FEATURE_WS20_SAMPLE
#elif (BOARD_VER >= TP10)
#define FEATURE_TP10_SAMPLE
#endif

//#define FEATURE_SONYWM_ID_READ
extern int gpio43, gpio16, gpio24; /* gpio43 :reset, gpio16:lcd bl */

#ifndef FEATURE_RENESAS_BL_CTRL_CHG
static int first_enable = 0;
#endif
static int prev_bl_level = 0;

static struct msm_panel_common_pdata *mipi_sony_pdata;

static struct dsi_buf sony_tx_buf;
static struct dsi_buf sony_rx_buf;

struct lcd_state_type {
    boolean disp_powered_up;
    boolean disp_initialized;
    boolean disp_on;
#ifdef CONFIG_LCD_CABC_CONTROL
		int acl_flag;
#endif	
	struct mutex lcd_mutex;
};

static struct lcd_state_type sony_state = { 0, };

char dsctl[2]             = {0x36, 0x40};
char wrdisbv[2]        = {0x51, 0x00};
char wrctrld[2]         = {0x53, 0x2c};
char wrcbc_on[2]     = {0x55, 0x01};
char wrcbc_off[2]     = {0x55, 0x00};
char sleep_out[2]     = {0x11, 0x00};
char sleep_in[2]       = {0x10, 0x00};
char disp_on[2]        = {0x29, 0x00};
char disp_off[2]       = {0x28, 0x00};
#ifdef FEATURE_QUALCOMM_BUG_FIX_LCD_MDP_TIMING_GENERATOR_ON
char nvm_access_en_1[3]       = {0xf0, 0x5a,0x5a};
char nvm_access_en_2[3]       = {0xf1, 0x5a,0x5a};
char panelctl_1[21]       = {0xf6, 0x02,0x11,0x0f,0x25,0x0a,0x00,0x13,0x22,0x1b,0x03,
	                                             0x00,0x00,0x00,0x00,0x00,0x04,0x68,0x09,0x10,0x51};
#endif

static struct dsi_cmd_desc sony_display_off_cmds[] = {
#if defined (FEATURE_WS20_SAMPLE) || defined (FEATURE_TP10_SAMPLE)	
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(disp_off), disp_off},
#endif	
	{DTYPE_DCS_WRITE, 1, 0, 0, 130, sizeof(sleep_in), sleep_in}
};

static struct dsi_cmd_desc sony_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(disp_on), disp_on}
};

#ifdef FEATURE_SONYWM_ID_READ
static struct dsi_cmd_desc sony_display_init_ws10_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(dsctl), dsctl},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrdisbv), wrdisbv},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrctrld), wrctrld},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrcbc), wrcbc},	
};

static struct dsi_cmd_desc sony_display_init_ws20_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrdisbv), wrdisbv},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrctrld), wrctrld},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrcbc), wrcbc},	
};
#else
static struct dsi_cmd_desc sony_display_init_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(sleep_out), sleep_out},
#ifdef FEATURE_QUALCOMM_BUG_FIX_LCD_MDP_TIMING_GENERATOR_ON	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nvm_access_en_1), nvm_access_en_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nvm_access_en_2), nvm_access_en_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(panelctl_1), panelctl_1},
#endif
#if defined (FEATURE_WS10_SAMPLE) ||defined (FEATURE_TP10_SAMPLE)		
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(dsctl), dsctl},	
#endif	
};
#endif

static struct dsi_cmd_desc sony_display_cabc_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrdisbv), wrdisbv},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrctrld), wrctrld},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrcbc_on), wrcbc_on},	

};

static struct dsi_cmd_desc sony_display_cabc_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrdisbv), wrdisbv},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrctrld), wrctrld},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrcbc_off), wrcbc_off},	
};

static struct dsi_cmd_desc sony_display_cabc_bl_set_cmds[] = {
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(wrdisbv), wrdisbv}
};     

#ifdef FEATURE_SONYWM_ID_READ
static uint8 module_vender;
static char manufacture_id[2] = {0xdc, 0x00}; /* DTYPE_DCS_READ */

static struct dsi_cmd_desc sony_wm_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id};

static uint32 mipi_sonywm_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 i;
	uint8 *lp;

       lp = NULL;
	tp = &sony_tx_buf;
	rp = &sony_rx_buf;
	cmd = &sony_wm_manufacture_id_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 1);
	for(i=0; i<1;i++)
	{
		lp = ((uint8 *)rp->data++);
		module_vender = (*lp);
		pr_info("%s: manufacture_id=0x%x\n", __func__, *lp);
		msleep(5);
	}
	return *lp;
}
#endif

/*
 0. sony_display_init_cmds
 1. dsi_cmd_desc sony_display_on_cmds
 */

#ifdef CONFIG_LCD_CABC_CONTROL
void cabc_contol(struct msm_fb_data_type *mfd, int state)
{
       mutex_lock(&sony_state.lcd_mutex);	
	mipi_set_tx_power_mode(0);
	if(state == true){
		
		mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_cabc_off_cmds,
								ARRAY_SIZE(sony_display_cabc_off_cmds));
		sony_state.acl_flag = true;
	}
	else{ 
		mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_cabc_on_cmds,
							ARRAY_SIZE(sony_display_cabc_on_cmds));
		sony_state.acl_flag = false;
	}
	mipi_set_tx_power_mode(1);	
       mutex_unlock(&sony_state.lcd_mutex);	
	
	printk(KERN_WARNING"mipi_sharp CABC = %d\n",state);

}
#endif

static int mipi_sony_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

    ENTER_FUNC2();

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	
    mutex_lock(&sony_state.lcd_mutex);	
	if (sony_state.disp_initialized == false) {

#ifdef FEATURE_SONYWM_ID_READ
		mipi_sonywm_manufacture_id(mfd);
if (module_vender)
		mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_init_ws20_cmds,
				ARRAY_SIZE(sony_display_init_ws20_cmds));
else
		mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_init_ws10_cmds,
				ARRAY_SIZE(sony_display_init_ws10_cmds));
#else
		mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_init_cmds,
				ARRAY_SIZE(sony_display_init_cmds));
#endif

		sony_state.disp_initialized = true;
	}
	mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_on_cmds,
			ARRAY_SIZE(sony_display_on_cmds));
	sony_state.disp_on = true;
#ifdef CONFIG_LCD_CABC_CONTROL
	if(sony_state.acl_flag == true){
		mipi_dsi_cmds_tx( &sony_tx_buf, sony_display_cabc_off_cmds,
									ARRAY_SIZE(sony_display_cabc_off_cmds));

	}
	else{ 
	
	mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_cabc_on_cmds,
			ARRAY_SIZE(sony_display_cabc_on_cmds));
	}
#else
				
	mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_cabc_on_cmds,
				ARRAY_SIZE(sony_display_cabc_on_cmds));
#endif
    mutex_unlock(&sony_state.lcd_mutex);	
	
	EXIT_FUNC2();
	return 0;
}

static int mipi_sony_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

    ENTER_FUNC2();

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

#ifdef FEATURE_RENESAS_BL_CTRL_CHG
		if(gpio_get_value_cansleep(gpio24))
			gpio_set_value_cansleep(gpio24, GPIO_LOW_VALUE);
		if(gpio_get_value_cansleep(gpio16))
			gpio_set_value_cansleep(gpio16, GPIO_LOW_VALUE);
		msleep(10);		
    wrdisbv[1] =0;
#endif
	
    mutex_lock(&sony_state.lcd_mutex);	
	if (sony_state.disp_on == true) {
		mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_off_cmds,
				ARRAY_SIZE(sony_display_off_cmds));
		sony_state.disp_on = false;
		sony_state.disp_initialized = false;
		
	}
    mutex_unlock(&sony_state.lcd_mutex);	
	
       EXIT_FUNC2();
	return 0;
}

static void mipi_sony_set_backlight(struct msm_fb_data_type *mfd)
{
#if 0
	static int first_enable = 0;
	static int prev_bl_level = 0;
	int cnt, bl_level;
	//int count = 0;
	unsigned long flags;
	bl_level = mfd->bl_level;

	PRINT("[LIVED] set_backlight=%d,prev=%d\n", bl_level, prev_bl_level);
	if (bl_level == prev_bl_level || sony_state.disp_on == 0) {
		PRINT("[LIVED] same! or not disp_on\n");
	} else {
		if (bl_level == 0) {
			//gpio_set_value_cansleep(gpio24 ,GPIO_LOW_VALUE);
			usleep(250);      // Disable hold time
			PRINT("[LIVED] same! or not disp_on\n");
			//gpio_set_value_cansleep(gpio16 ,GPIO_LOW_VALUE);
		} else {
			if (prev_bl_level == 0) {
				//count++;
				gpio_set_value_cansleep(gpio16 ,GPIO_HIGH_VALUE);
				msleep(1);
				gpio_set_value_cansleep(gpio24 ,GPIO_HIGH_VALUE);
				if (first_enable == 0) {
					first_enable = 1;
					msleep(4); // Initial enable time
				} 
				//PRINT("[LIVED] (0) init!\n");
			}

			if (prev_bl_level < bl_level) {
				gpio_set_value_cansleep(gpio24 ,GPIO_LOW_VALUE);
				udelay(70);// TRESET
				gpio_set_value_cansleep(gpio24 ,GPIO_HIGH_VALUE);
				cnt = BL_MAX - bl_level + 1;
			} else {
				cnt = prev_bl_level - bl_level;
			}
			//pr_info("[LIVED] cnt=%d, prev_bl_level=%d, bl_level=%d\n",
			//		cnt, prev_bl_level, bl_level);
			while (cnt) {
				gpio_set_value_cansleep(gpio24 ,GPIO_LOW_VALUE);
				local_save_flags(flags);
				local_irq_disable();
				udelay(5);//DELAY_3NS();//udelay(3);      // Turn off time
				local_irq_restore(flags);
				
				gpio_set_value_cansleep(gpio24 ,GPIO_HIGH_VALUE);
				local_save_flags(flags);
				local_irq_disable();	
				udelay(20);      // Turn on time
				local_irq_restore(flags);
				cnt--;
			}

		}
		prev_bl_level = bl_level;
	}
#else
int bl_level;

#ifndef FEATURE_RENESAS_BL_CTRL_CHG
 if (prev_bl_level == mfd->bl_level)
 	return;
#endif
    //ENTER_FUNC2();
    bl_level = (mfd->bl_level *255)/16;

//20130102_EF44S backlight PWM min duty
if(bl_level < 31 && bl_level > 0)
	bl_level = 31;

printk("mipi_sony_set_backlight prev_bl_level =%d,mfd->bl_level=%d, bl_level =%d\n",prev_bl_level,mfd->bl_level, bl_level);

    wrdisbv[1] = bl_level;

#ifdef FEATURE_RENESAS_BL_CTRL_CHG
	if(!gpio_get_value_cansleep(gpio24))
		gpio_set_value_cansleep(gpio24, GPIO_HIGH_VALUE);
        udelay(10);	
	if(!gpio_get_value_cansleep(gpio16))
		gpio_set_value_cansleep(gpio16, GPIO_HIGH_VALUE);	
#else
    if(first_enable == 0)
    {
        gpio_set_value_cansleep(gpio24, GPIO_HIGH_VALUE);
        udelay(10);
        gpio_set_value_cansleep(gpio16, GPIO_HIGH_VALUE);
        first_enable  = 1;
    }
#endif	
    mutex_lock(&sony_state.lcd_mutex);	
    mipi_set_tx_power_mode(0);
#if 0	
    if(charger_flag == 1 && bl_level > 0){
			mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_init_cmds,
					ARRAY_SIZE(sony_display_init_cmds));
			mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_on_cmds,
					ARRAY_SIZE(sony_display_on_cmds));
	}
#endif	
    mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_cabc_bl_set_cmds,
			ARRAY_SIZE(sony_display_cabc_bl_set_cmds));
#if 0	
   if(charger_flag == 1 && bl_level == 0){
			mipi_dsi_cmds_tx(&sony_tx_buf, sony_display_off_cmds,
					ARRAY_SIZE(sony_display_off_cmds));
	}
#endif	
    prev_bl_level = mfd->bl_level;
    mipi_set_tx_power_mode(1);
    mutex_unlock(&sony_state.lcd_mutex);		
    if(bl_level == 0)
    {
#ifdef FEATURE_RENESAS_BL_CTRL_CHG
		if(gpio_get_value_cansleep(gpio16))
			gpio_set_value_cansleep(gpio16, GPIO_LOW_VALUE);
		if(gpio_get_value_cansleep(gpio24))
			gpio_set_value_cansleep(gpio24, GPIO_LOW_VALUE);
		
#else
          gpio_set_value_cansleep(gpio16, GPIO_LOW_VALUE);
          gpio_set_value_cansleep(gpio24, GPIO_LOW_VALUE);
          first_enable = 0;
#endif		      
    }	
    //EXIT_FUNC2();
#endif	
}


static int __devinit mipi_sony_lcd_probe(struct platform_device *pdev)
{

    if (pdev->id == 0) {
        mipi_sony_pdata = pdev->dev.platform_data;
		return 0;
	}
       mutex_init(&sony_state.lcd_mutex);	
	msm_fb_add_device(pdev);
	
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_sony_lcd_probe,
	.driver = {
		.name   = "mipi_sony_wm",
	},
};

static struct msm_fb_panel_data sony_panel_data = {
       .on             = mipi_sony_lcd_on,
       .off            = mipi_sony_lcd_off,
       .set_backlight  = mipi_sony_set_backlight,
};

static int ch_used[3];

int mipi_sony_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_sony_wm", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	sony_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &sony_panel_data,
		sizeof(sony_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_sony_lcd_init(void)
{
    ENTER_FUNC2();

    sony_state.disp_powered_up = true;

    mipi_dsi_buf_alloc(&sony_tx_buf, DSI_BUF_SIZE);
    mipi_dsi_buf_alloc(&sony_rx_buf, DSI_BUF_SIZE);

    EXIT_FUNC2();

    return platform_driver_register(&this_driver);
}

module_init(mipi_sony_lcd_init);
