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
#include "mipi_samsung_oled_premia.h"
#include <mach/gpio.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <linux/mutex.h>
#include <linux/mfd/pm8xxx/pm8921.h>

#ifdef D_SKY_OLED_TEMP
struct delayed_work panel_therm_detect_work;
unsigned char is_temp,is_high_bk, ptemp_bl,is_set=false;
#endif

struct delayed_work panel_id3_detect_work;
unsigned char changed_id3 = 1;

#ifdef CONFIG_PANTECH_HDMI_PW_CTRL_ON_LCD
extern int hdmi_autodetect_control(int on);
extern int HDMI_Schedule_Control(int on);
extern uint32_t HDMI_Get_Cable_State(void);
#define HDMI_PW_ON 1
#define HDMI_PW_OFF 0
#endif
extern int get_hw_revision(void);

#define LCD_RESET		43
#define LCD_DEBUG_MSG

#ifdef LCD_DEBUG_MSG
#define ENTER_FUNC()        printk(KERN_INFO "[PANTECH_LCD] +%s \n", __FUNCTION__);
#define EXIT_FUNC()         printk(KERN_INFO "[PANTECH_LCD] -%s \n", __FUNCTION__);
//#define ENTER_FUNC2()    printk(KERN_ERR "[PANTECH_LCD] +%s\n", __FUNCTION__);
#define EXIT_FUNC2()       printk(KERN_ERR "[PANTECH_LCD] -%s\n", __FUNCTION__);
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

DEFINE_MUTEX(bl_mutex);
#define D_SKY_BACKLIGHT_CONTROL		1
#if (BOARD_VER > WS10) 
#define OLED_DET	64
#endif

extern int gpio43;
static struct msm_panel_common_pdata *mipi_samsung_oled_hd_pdata;

static struct dsi_buf samsung_oled_hd_tx_buf;
static struct dsi_buf samsung_oled_hd_rx_buf;

#ifdef CONFIG_F_SKYDISP_SMART_DIMMING
void mtp_read(void);
void gamma_300nit_read(unsigned int p_rev);
unsigned char first_read = TRUE;
#endif

//void change_elvss(struct work_struct *work);

struct lcd_state_type {
    boolean disp_powered_up;
    boolean disp_initialized;
    boolean disp_on;
#ifdef CONFIG_ACL_FOR_AMOLED
    boolean acl_flag;
    int acl_data;
#endif
};

static struct lcd_state_type samsung_oled_hd_state = { 0, };

#ifdef D_SKY_OLED_TEMP
//unsigned int is_first_check=1;
#endif

unsigned char is_sleep,is_cont,is_read=FALSE;
unsigned int prev_bl, now_bl;
unsigned int nELVSS,pre_nELVSS; /* ELVSS Flag - 1 : 300~210 , 2: 200~170 , 3: 160~110 , 4:100- (cd2/m) */

uint8 elvss_id3,prev_id2;

char mtp_enable[3]		= {0xF1, 0x5A, 0x5A};
char sleep_out[2]       	= {0x11, 0x00};
char id_read[2]	        	= {0xD1, 0x00};

#if defined(MIPI_CLOCK_494MBPS) || defined(MIPI_CLOCK_500MBPS)
char panel_cond_set[39]		= {0xF8, 0x19, 0x35, 0x00, 0x00, 0x00, 0x95, 0x00, 0x3C, 0x7D, 0x10, 
	                              	 0x27, 0x08, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08, 0x6E, 
                                	 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x23, 0x6E, 0xC0, 0xC1, 
                                 	 0x01, 0x81, 0xC1, 0x00, 0xC3, 0xF6, 0xF6, 0xC1};
#elif defined(MIPI_CLOCK_468MBPS) /* Rev.F */
//char panel_cond_set_tp20[39] 		= {0xF8, 0x19, 0x31, 0x00, 0x00, 0x00, 0x8B, 0x00, 0x39, 0x75, 0x0F,
char panel_cond_set[39] 		= {0xF8, 0x19, 0x31, 0x00, 0x00, 0x00, 0x8B, 0x00, 0x39, 0x75, 0x0F,
					 0x25, 0x07, 0x67, 0x00, 0x00, 0x00, 0x00, 0x04, 0x07, 0x67,
					 0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x20, 0x67, 0xC0, 0xC1,
                                 	 0x01, 0x81, 0xC1, 0x00, 0xC3, 0xF6, 0xF6, 0xC1};
#else /* Rev.E */
char panel_cond_set[39]		= {0xF8, 0x19, 0x30, 0x00, 0x00, 0x00, 0x87, 0x00, 0x37, 0x72, 0x0E, 
	                              	 0x24, 0x07, 0x64, 0x00, 0x00, 0x00, 0x00, 0x04, 0x07, 0x64, 
                                	 0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x1F, 0x64, 0xC0, 0xC1, 
                                 	 0x01, 0x81, 0xC1, 0x00, 0xC3, 0xF6, 0xF6, 0xC1};
#endif
#if 0//defined(MIPI_CLOCK_440MBPS) || defined(MIPI_CLOCK_456MBPS)
#else
#if 0 // BOARD_VER == WS20 || BOARD_VER == TP10 /* Rev. C, Rev. D */
char panel_cond_set[39]		= {0xF8, 0x19, 0x35, 0x00, 0x00, 0x00, 0x95, 0x00, 0x3C, 0x7D, 0x10, 
	                              	 0x27, 0x08, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08, 0x6E, 
                                	 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x23, 0x6E, 0xC0, 0xC1, 
                                 	 0x01, 0x81, 0xC1, 0x00, 0xC3, 0xF6, 0xF6, 0xC1};
//#else // using gpara 
char panel_cond_set[11]		= {0xF8, 0x3D, 0x35, 0x00, 0x00, 0x00, 0x93, 0x00, 0x3C, 0x7D, 0x08}; 
char panel_cond_set_gpara1[13]	= {0xB0, 0x0A, 0xF8, 0x27, 0x7D, 0x3F, 0x00, 0x00, 0x00, 0x20, 0x04, 0x08, 0x6E};
char panel_cond_set_gpara2[13]	= {0xB0, 0x14, 0xF8, 0x00, 0x00, 0x00, 0x02, 0x08, 0x08, 0x23, 0x23, 0xC0, 0xC8 };
char panel_cond_set_gpara3[11]	= {0xB0, 0x1E, 0xF8, 0x08, 0x48, 0xC1, 0x00, 0xC1, 0xFF, 0xFF, 0xC8};

#endif
#endif /* 440 */
char disp_cond_set[4]   	= {0xF2, 0x80, 0x03, 0x0D};
char gamma_update[2] 		= {0xF7, 0x03};
/* etc cond set */
char source_control[4]		= {0xF6, 0x00, 0x02, 0x00};
char pentile_control[10] 	= {0xB6, 0x0C, 0x02, 0x03, 0x32, 0xFF, 0x44, 0x44, 0xC0, 0x00};
char nvm_setting[15]		= {0xD9, 0x14, 0x40, 0x0C, 0xCB, 0xCE, 0x6E, 0xC4, 0x07, 0x40, 0x41,
							  		 0xD0, 0x00, 0x60, 0x19};
#if 0 /* not used */
char mipi_control1[6]		= {0xE1, 0x10, 0x1C, 0x17, 0x08, 0x1D};
char mipi_control2[7] 		= {0xE2, 0xED, 0x07, 0xC3, 0x13, 0x0D, 0x03};
char mipi_control3[2]		= {0xE3, 0x40};
char mipi_control4[8]		= {0xE4, 0x00, 0x00, 0x14, 0x80, 0x00, 0x00, 0x00};
#endif
//#if defined(CONFIG_MACH_MSM8960_VEGAPVW) && (BOARD_VER >= WS10)
char power_control[8] 		= {0xF4, 0xCF, 0x0A, 0x15, 0x10, 0x19, 0x33, 0x02};
//char power_control[8] 		= {0xF4, 0xCF, 0x0A, 0x15, 0x10, 0x1E, 0x33, 0x02};
//#else
//char power_control_pt20[8] 		= {0xF4, 0xCF, 0x0A, 0x0D, 0x10, 0x19, 0x33, 0x02};
//#endif//if defined(CONFIG_MACH_MSM8960_VEGAPVW) && (BOARD_VER >= WS10)
char elvss_control[3] 		= {0xB1, 0x04, 0x9C};
char disp_on[2]             = {0x29, 0x00};
char disp_off[2]            = {0x28, 0x00};
char sleep_in[2]            = {0x10, 0x00};

#ifdef D_SKY_BACKLIGHT_CONTROL
/* Samsung OLED Gamma Setting */
//#if (BOARD_VER == WS20) /* Rev C. */
char gamma_set_300_ws20[26] 	= {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0xCD, 0xD8, 0xBE, 0xBE, 0xC5, 0xB4,
					 0xCC, 0xD1, 0xC5, 0x9D, 0xA6, 0x96, 0xB1, 0xB7, 0xAC, 0x00,
					 0xBE, 0x00, 0xB5, 0x00, 0xDB};
char gamma_set_280_ws20[26] 	= {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0xCF, 0xDA, 0xC1, 0xBF, 0xC6, 0xB5,
					 0xCC, 0xD1, 0xC4, 0x9D, 0xA6, 0x96, 0xB2, 0xB8, 0xAD, 0x00,
					 0xB9, 0x00, 0xB0, 0x00, 0xD5};
char gamma_set_260_ws20[26] 	= {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0xC9, 0xD4, 0xBC, 0xC0, 0xC7, 0xB6,
					 0xCA, 0xCF, 0xC3, 0x9F, 0xA8, 0x98, 0xB2, 0xB8, 0xAD, 0x00,
					 0xB4, 0x00, 0xAB, 0x00, 0xCF};
char gamma_set_240_ws20[26] 	= {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0xCC, 0xD6, 0xBF, 0xC1, 0xC8, 0xB7,
					 0xCA, 0xCF, 0xC3, 0xA0, 0xA8, 0x98, 0xB4, 0xBA, 0xAF, 0x00,
					 0xAE, 0x00, 0xA5, 0x00, 0xC9};
char gamma_set_220_ws20[26] 	= {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0xC9, 0xD2, 0xBD, 0xBF, 0xC6, 0xB5,
					 0xCA, 0xCF, 0xC3, 0xA1, 0xAA, 0x99, 0xB5, 0xBB, 0xB1, 0x00,
					 0xA9, 0x00, 0xA0, 0x00, 0xC2};
char gamma_set_200_ws20[26] 	= {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0xC3, 0xCB, 0xB7, 0xC0, 0xC7, 0xB6,
					 0xCC, 0xD1, 0xC4, 0xA2, 0xAA, 0x99, 0xB6, 0xBC, 0xB1, 0x00,
					 0xA3, 0x00, 0x9A, 0x00, 0xBC};
char gamma_set_180_ws20[26] 	= {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0xC6, 0xCE, 0xBB, 0xC1, 0xC8, 0xB7,
					 0xCC, 0xD1, 0xC4, 0xA3, 0xAC, 0x9B, 0xB8, 0xBE, 0xB3, 0x00,
					 0x9D, 0x00, 0x94, 0x00, 0xB5};
char gamma_set_160_ws20[26] 	= {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0xBE, 0xC5, 0xB4, 0xC1, 0xC8, 0xB7,
					 0xCB, 0xD0, 0xC3, 0xA5, 0xAD, 0x9C, 0xB8, 0xBE, 0xB3, 0x00,
					 0x97, 0x00, 0x8E, 0x00, 0xAE};
char gamma_set_140_ws20[26] 	= {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0xC1, 0xC7, 0xB7, 0xC2, 0xC9, 0xB9,
					 0xCB, 0xD0, 0xC3, 0xA7, 0xB0, 0x9E, 0xBB, 0xC1, 0xB7, 0x00,
					 0x90, 0x00, 0x86, 0x00, 0xA5};
char gamma_set_120_ws20[26] 	= {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0xBA, 0xBF, 0xB2, 0xC0, 0xC7, 0xB6,
					 0xCD, 0xD2, 0xC5, 0xA7, 0xB0, 0x9E, 0xBE, 0xC4, 0xB9, 0x00,
					 0x88, 0x00, 0x7F, 0x00, 0x9D};
char gamma_set_100_ws20[26] 	= {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0xB2, 0xB6, 0xAD, 0xBD, 0xC5, 0xB3,
					 0xCE, 0xD3, 0xC6, 0xA9, 0xB1, 0x9F, 0xBF, 0xC5, 0xBB, 0x00,
					 0x81, 0x00, 0x77, 0x00, 0x94};
char gamma_set_80_ws20[26] 	= {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0xA5, 0xA7, 0xA1, 0xBF, 0xC7, 0xB5,
					 0xCD, 0xD3, 0xC5, 0xAA, 0xB2, 0x9F, 0xC2, 0xC8, 0xBD, 0x00,
					 0x78, 0x00, 0x6E, 0x00, 0x8A};
char gamma_set_60_ws20[26] 	= {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0x96, 0x97, 0x94, 0xC1, 0xC9, 0xB7,
					 0xCE, 0xD3, 0xC6, 0xAC, 0xB4, 0xA0, 0xC4, 0xCA, 0xBF, 0x00,
					 0x6E, 0x00, 0x64, 0x00, 0x7F};
char gamma_set_40_ws20[26] 	= {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0x8C, 0x8C, 0x8C, 0xBB, 0xC3, 0xB1,
					 0xD0, 0xD6, 0xC9, 0xAF, 0xB7, 0xA3, 0xC7, 0xCD, 0xC1, 0x00,
					 0x62, 0x00, 0x58, 0x00, 0x70};
char gamma_set_20_ws20[26] 	= {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0x64, 0x64, 0x64, 0xA9, 0xAB, 0xA7,
					 0xCD, 0xD3, 0xC4, 0xAF, 0xB8, 0xA4, 0xCB, 0xD0, 0xC3, 0x00,
					 0x51, 0x00, 0x47, 0x00, 0x5D};

//#if (BOARD_VER == TP10) /* Rev. C - final center gamma setting (2.2 gamma) */ 
char gamma_set_level_30[26]  = {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0xCD, 0xD8, 0xBE, 0xBE, 0xC5, 0xB4,
                                         0xCC, 0xD1, 0xC5, 0x9D, 0xA6, 0x96, 0xB1, 0xB7, 0xAC, 0x00,
                                         0xBE, 0x00, 0xB5, 0x00, 0xDB}; // 300nit
char gamma_set_level_29[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC6,	0xD0,	0xB9,	0xC0,	0xC7,	0xB6,	0xCB,	0xD0,	0xC3,	0x9F,	0xA8,	0x98,	0xB4,	0xBA,	0xAF,	0x00,	0xAC,	0x00,	0xA3,	0x00,	0xC6	};
char gamma_set_level_28[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC6,	0xD0,	0xB9,	0xC0,	0xC7,	0xB6,	0xCC,	0xD2,	0xC5,	0xA0,	0xA9,	0x98,	0xB5,	0xBB,	0xB0,	0x00,	0xAA,	0x00,	0xA1,	0x00,	0xC4	};
char gamma_set_level_27[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC9,	0xD2,	0xBD,	0xBF,	0xC6,	0xB5,	0xCC,	0xD1,	0xC4,	0xA0,	0xA9,	0x98,	0xB4,	0xBB,	0xB0,	0x00,	0xA9,	0x00,	0xA0,	0x00,	0xC2	};
char gamma_set_level_26[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC9,	0xD2,	0xBD,	0xC1,	0xC8,	0xB7,	0xCB,	0xD0,	0xC3,	0xA1,	0xAA,	0x99,	0xB5,	0xBB,	0xB0,	0x00,	0xA7,	0x00,	0x9E,	0x00,	0xC0	};
char gamma_set_level_25[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xCC,	0xD5,	0xC0,	0xBD,	0xC5,	0xB3,	0xCD,	0xD2,	0xC5,	0xA1,	0xAA,	0x99,	0xB5,	0xBB,	0xB0,	0x00,	0xA5,	0x00,	0x9C,	0x00,	0xBE	};
char gamma_set_level_24[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC3,	0xCB,	0xB7,	0xC0,	0xC7,	0xB6,	0xCC,	0xD1,	0xC4,	0xA2,	0xAA,	0x99,	0xB6,	0xBC,	0xB1,	0x00,	0xA3,	0x00,	0x9A,	0x00,	0xBC	};
char gamma_set_level_23[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC3,	0xCB,	0xB7,	0xC2,	0xC9,	0xB9,	0xCB,	0xD0,	0xC3,	0xA2,	0xAB,	0x9A,	0xB6,	0xBC,	0xB1,	0x00,	0xA1,	0x00,	0x98,	0x00,	0xBA	};
char gamma_set_level_22[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC6,	0xCE,	0xBB,	0xBF,	0xC6,	0xB5,	0xCD,	0xD2,	0xC5,	0xA2,	0xAB,	0x9A,	0xB7,	0xBD,	0xB2,	0x00,	0x9F,	0x00,	0x96,	0x00,	0xB8	};
char gamma_set_level_21[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC6,	0xCE,	0xBB,	0xC1,	0xC8,	0xB7,	0xCC,	0xD1,	0xC4,	0xA3,	0xAC,	0x9B,	0xB7,	0xBD,	0xB2,	0x00,	0x9E,	0x00,	0x94,	0x00,	0xB5	};
char gamma_set_level_20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC9,	0xD1,	0xBE,	0xC0,	0xC7,	0xB6,	0xCB,	0xD0,	0xC3,	0xA3,	0xAC,	0x9B,	0xB8,	0xBE,	0xB3,	0x00,	0x9C,	0x00,	0x93,	0x00,	0xB3	};
char gamma_set_level_19[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC9,	0xD1,	0xBE,	0xC0,	0xC7,	0xB6,	0xCD,	0xD2,	0xC5,	0xA4,	0xAD,	0x9B,	0xB8,	0xBE,	0xB3,	0x00,	0x9A,	0x00,	0x91,	0x00,	0xB1	};
char gamma_set_level_18[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xBB,	0xC2,	0xB0,	0xC2,	0xC9,	0xB9,	0xCC,	0xD1,	0xC4,	0xA4,	0xAD,	0x9C,	0xB9,	0xBF,	0xB4,	0x00,	0x98,	0x00,	0x8F,	0x00,	0xAF	};
char gamma_set_level_17[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xBE,	0xC5,	0xB4,	0xC1,	0xC8,	0xB7,	0xCB,	0xD0,	0xC3,	0xA5,	0xAD,	0x9C,	0xB9,	0xBF,	0xB4,	0x00,	0x96,	0x00,	0x8D,	0x00,	0xAD	};
char gamma_set_level_16[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xBE,	0xC5,	0xB4,	0xC4,	0xCA,	0xBA,	0xCA,	0xD0,	0xC2,	0xA5,	0xAE,	0x9D,	0xBB,	0xC1,	0xB6,	0x00,	0x94,	0x00,	0x8B,	0x00,	0xAA	};
char gamma_set_level_15[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC1,	0xC7,	0xB7,	0xC0,	0xC7,	0xB6,	0xCC,	0xD1,	0xC4,	0xA7,	0xAF,	0x9E,	0xBA,	0xC0,	0xB5,	0x00,	0x92,	0x00,	0x89,	0x00,	0xA8	};
char gamma_set_level_14[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC1,	0xC7,	0xB7,	0xC2,	0xC9,	0xB9,	0xCB,	0xD0,	0xC3,	0xA7,	0xB0,	0x9E,	0xBB,	0xC1,	0xB7,	0x00,	0x90,	0x00,	0x86,	0x00,	0xA5	};
char gamma_set_level_13[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xB6,	0xBC,	0xAD,	0xC0,	0xC7,	0xB6,	0xCC,	0xD1,	0xC4,	0xA6,	0xAF,	0x9D,	0xBC,	0xC2,	0xB7,	0x00,	0x8E,	0x00,	0x84,	0x00,	0xA3	};
char gamma_set_level_12[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xB6,	0xBC,	0xAD,	0xC3,	0xC9,	0xB9,	0xCB,	0xD1,	0xC3,	0xA8,	0xB0,	0x9F,	0xBC,	0xC2,	0xB8,	0x00,	0x8B,	0x00,	0x82,	0x00,	0xA0	};
char gamma_set_level_11[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xBA,	0xBF,	0xB2,	0xC0,	0xC7,	0xB6,	0xCB,	0xD0,	0xC3,	0xA8,	0xB0,	0x9F,	0xBE,	0xC4,	0xB9,	0x00,	0x89,	0x00,	0x80,	0x00,	0x9D	};
char gamma_set_level_10[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xBA,	0xBF,	0xB2,	0xC3,	0xCA,	0xB9,	0xCC,	0xD2,	0xC5,	0xA7,	0xB0,	0x9E,	0xBD,	0xC3,	0xB8,	0x00,	0x87,	0x00,	0x7E,	0x00,	0x9B	};
char gamma_set_level_9[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xAE,	0xB3,	0xA8,	0xC0,	0xC7,	0xB6,	0xCC,	0xD2,	0xC4,	0xA9,	0xB1,	0x9F,	0xBD,	0xC4,	0xB9,	0x00,	0x85,	0x00,	0x7B,	0x00,	0x99	};
char gamma_set_level_8[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xAE,	0xB3,	0xA8,	0xC0,	0xC7,	0xB6,	0xCE,	0xD4,	0xC7,	0xA9,	0xB1,	0x9F,	0xBF,	0xC5,	0xBA,	0x00,	0x83,	0x00,	0x79,	0x00,	0x96	};
char gamma_set_level_7[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xB2,	0xB6,	0xAD,	0xBD,	0xC5,	0xB3,	0xCE,	0xD3,	0xC6,	0xA9,	0xB1,	0x9F,	0xBF,	0xC5,	0xBB,	0x00,	0x81,	0x00,	0x77,	0x00,	0x94	};
char gamma_set_level_6[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xB2,	0xB6,	0xAD,	0xC0,	0xC7,	0xB6,	0xCE,	0xD3,	0xC5,	0xAA,	0xB2,	0xA0,	0xC0,	0xC6,	0xBB,	0x00,	0x7E,	0x00,	0x75,	0x00,	0x91	};
char gamma_set_level_5[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xB7,	0xBA,	0xB2,	0xBE,	0xC5,	0xB4,	0xCF,	0xD4,	0xC7,	0xA8,	0xB1,	0x9E,	0xC0,	0xC6,	0xBB,	0x00,	0x7D,	0x00,	0x73,	0x00,	0x8F	};
char gamma_set_level_4[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xA1,	0xA4,	0x9C,	0xC1,	0xC8,	0xB7,	0xCE,	0xD4,	0xC6,	0xAA,	0xB2,	0x9F,	0xC0,	0xC7,	0xBB,	0x00,	0x7A,	0x00,	0x71,	0x00,	0x8D	};
char gamma_set_level_3[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xA5,	0xA7,	0xA1,	0xBF,	0xC7,	0xB5,	0xCD,	0xD3,	0xC5,	0xAA,	0xB2,	0x9F,	0xC2,	0xC8,	0xBD,	0x00,	0x78,	0x00,	0x6E,	0x00,	0x8A	};
char gamma_set_level_2[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xA5,	0xA7,	0xA1,	0xC2,	0xCA,	0xB8,	0xCE,	0xD4,	0xC7,	0xA9,	0xB1,	0x9E,	0xC2,	0xC9,	0xBD,	0x00,	0x76,	0x00,	0x6C,	0x00,	0x87	};
char gamma_set_level_1[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xA9,	0xAB,	0xA6,	0xC0,	0xC8,	0xB6,	0xCD,	0xD3,	0xC5,	0xAA,	0xB2,	0x9F,	0xC3,	0xC9,	0xBE,	0x00,	0x73,	0x00,	0x6A,	0x00,	0x85	};


//#if (BOARD_VER > TP10) /* Rev. C - final center gamma setting (2.2 gamma) */ 
//char gamma_set_300_tp20[26]  = {0xFA, 0x01, 0x4A, 0x2E, 0x53, 0xCD, 0xD8, 0xBE, 0xBE, 0xC5, 0xB4,
  //                                       0xCC, 0xD1, 0xC5, 0x9D, 0xA6, 0x96, 0xB1, 0xB7, 0xAC, 0x00,
    //                                     0xBE, 0x00, 0xB5, 0x00, 0xDB}; // 300nit
char gamma_set_300_tp20[26]  = {0xFA, 0x01, 0x3D, 0x22, 0x45, 0xCA, 0xD5, 0xBB, 0xBA, 0xC1, 0xB0,
                                         0xC8, 0xCE, 0xC3, 0x9B, 0xA4, 0x94, 0xB0, 0xB6, 0xAC, 0x00,
                                         0xAD, 0x00, 0xA4, 0x00, 0xC9}; // 300nit
#if 0
char gamma_set_230_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC6,	0xD0,	0xB9,	0xC0,	0xC7,	0xB6,	0xCB,	0xD0,	0xC3,	0x9F,	0xA8,	0x98,	0xB4,	0xBA,	0xAF,	0x00,	0xAC,	0x00,	0xA3,	0x00,	0xC6	};
char gamma_set_224_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC6,	0xD0,	0xB9,	0xC0,	0xC7,	0xB6,	0xCC,	0xD2,	0xC5,	0xA0,	0xA9,	0x98,	0xB5,	0xBB,	0xB0,	0x00,	0xAA,	0x00,	0xA1,	0x00,	0xC4	};
char gamma_set_218_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC9,	0xD2,	0xBD,	0xBF,	0xC6,	0xB5,	0xCC,	0xD1,	0xC4,	0xA0,	0xA9,	0x98,	0xB4,	0xBB,	0xB0,	0x00,	0xA9,	0x00,	0xA0,	0x00,	0xC2	};
char gamma_set_212_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC9,	0xD2,	0xBD,	0xC1,	0xC8,	0xB7,	0xCB,	0xD0,	0xC3,	0xA1,	0xAA,	0x99,	0xB5,	0xBB,	0xB0,	0x00,	0xA7,	0x00,	0x9E,	0x00,	0xC0	};
char gamma_set_206_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xCC,	0xD5,	0xC0,	0xBD,	0xC5,	0xB3,	0xCD,	0xD2,	0xC5,	0xA1,	0xAA,	0x99,	0xB5,	0xBB,	0xB0,	0x00,	0xA5,	0x00,	0x9C,	0x00,	0xBE	};
char gamma_set_200_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC3,	0xCB,	0xB7,	0xC0,	0xC7,	0xB6,	0xCC,	0xD1,	0xC4,	0xA2,	0xAA,	0x99,	0xB6,	0xBC,	0xB1,	0x00,	0xA3,	0x00,	0x9A,	0x00,	0xBC	};
char gamma_set_194_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC3,	0xCB,	0xB7,	0xC2,	0xC9,	0xB9,	0xCB,	0xD0,	0xC3,	0xA2,	0xAB,	0x9A,	0xB6,	0xBC,	0xB1,	0x00,	0xA1,	0x00,	0x98,	0x00,	0xBA	};
char gamma_set_188_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC6,	0xCE,	0xBB,	0xBF,	0xC6,	0xB5,	0xCD,	0xD2,	0xC5,	0xA2,	0xAB,	0x9A,	0xB7,	0xBD,	0xB2,	0x00,	0x9F,	0x00,	0x96,	0x00,	0xB8	};
char gamma_set_182_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC6,	0xCE,	0xBB,	0xC1,	0xC8,	0xB7,	0xCC,	0xD1,	0xC4,	0xA3,	0xAC,	0x9B,	0xB7,	0xBD,	0xB2,	0x00,	0x9E,	0x00,	0x94,	0x00,	0xB5	};
char gamma_set_176_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC9,	0xD1,	0xBE,	0xC0,	0xC7,	0xB6,	0xCB,	0xD0,	0xC3,	0xA3,	0xAC,	0x9B,	0xB8,	0xBE,	0xB3,	0x00,	0x9C,	0x00,	0x93,	0x00,	0xB3	};
char gamma_set_170_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC9,	0xD1,	0xBE,	0xC0,	0xC7,	0xB6,	0xCD,	0xD2,	0xC5,	0xA4,	0xAD,	0x9B,	0xB8,	0xBE,	0xB3,	0x00,	0x9A,	0x00,	0x91,	0x00,	0xB1	};
char gamma_set_164_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xBB,	0xC2,	0xB0,	0xC2,	0xC9,	0xB9,	0xCC,	0xD1,	0xC4,	0xA4,	0xAD,	0x9C,	0xB9,	0xBF,	0xB4,	0x00,	0x98,	0x00,	0x8F,	0x00,	0xAF	};
char gamma_set_158_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xBE,	0xC5,	0xB4,	0xC1,	0xC8,	0xB7,	0xCB,	0xD0,	0xC3,	0xA5,	0xAD,	0x9C,	0xB9,	0xBF,	0xB4,	0x00,	0x96,	0x00,	0x8D,	0x00,	0xAD	};
char gamma_set_152_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xBE,	0xC5,	0xB4,	0xC4,	0xCA,	0xBA,	0xCA,	0xD0,	0xC2,	0xA5,	0xAE,	0x9D,	0xBB,	0xC1,	0xB6,	0x00,	0x94,	0x00,	0x8B,	0x00,	0xAA	};
char gamma_set_146_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC1,	0xC7,	0xB7,	0xC0,	0xC7,	0xB6,	0xCC,	0xD1,	0xC4,	0xA7,	0xAF,	0x9E,	0xBA,	0xC0,	0xB5,	0x00,	0x92,	0x00,	0x89,	0x00,	0xA8	};
char gamma_set_140_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xC1,	0xC7,	0xB7,	0xC2,	0xC9,	0xB9,	0xCB,	0xD0,	0xC3,	0xA7,	0xB0,	0x9E,	0xBB,	0xC1,	0xB7,	0x00,	0x90,	0x00,	0x86,	0x00,	0xA5	};
char gamma_set_134_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xB6,	0xBC,	0xAD,	0xC0,	0xC7,	0xB6,	0xCC,	0xD1,	0xC4,	0xA6,	0xAF,	0x9D,	0xBC,	0xC2,	0xB7,	0x00,	0x8E,	0x00,	0x84,	0x00,	0xA3	};
char gamma_set_128_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xB6,	0xBC,	0xAD,	0xC3,	0xC9,	0xB9,	0xCB,	0xD1,	0xC3,	0xA8,	0xB0,	0x9F,	0xBC,	0xC2,	0xB8,	0x00,	0x8B,	0x00,	0x82,	0x00,	0xA0	};
char gamma_set_122_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xBA,	0xBF,	0xB2,	0xC0,	0xC7,	0xB6,	0xCB,	0xD0,	0xC3,	0xA8,	0xB0,	0x9F,	0xBE,	0xC4,	0xB9,	0x00,	0x89,	0x00,	0x80,	0x00,	0x9D	};
char gamma_set_116_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xBA,	0xBF,	0xB2,	0xC3,	0xCA,	0xB9,	0xCC,	0xD2,	0xC5,	0xA7,	0xB0,	0x9E,	0xBD,	0xC3,	0xB8,	0x00,	0x87,	0x00,	0x7E,	0x00,	0x9B	};
char gamma_set_110_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xAE,	0xB3,	0xA8,	0xC0,	0xC7,	0xB6,	0xCC,	0xD2,	0xC4,	0xA9,	0xB1,	0x9F,	0xBD,	0xC4,	0xB9,	0x00,	0x85,	0x00,	0x7B,	0x00,	0x99	};
char gamma_set_105_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xAE,	0xB3,	0xA8,	0xC0,	0xC7,	0xB6,	0xCE,	0xD4,	0xC7,	0xA9,	0xB1,	0x9F,	0xBF,	0xC5,	0xBA,	0x00,	0x83,	0x00,	0x79,	0x00,	0x96	};
char gamma_set_100_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xB2,	0xB6,	0xAD,	0xBD,	0xC5,	0xB3,	0xCE,	0xD3,	0xC6,	0xA9,	0xB1,	0x9F,	0xBF,	0xC5,	0xBB,	0x00,	0x81,	0x00,	0x77,	0x00,	0x94	};
char gamma_set_95_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xB2,	0xB6,	0xAD,	0xC0,	0xC7,	0xB6,	0xCE,	0xD3,	0xC5,	0xAA,	0xB2,	0xA0,	0xC0,	0xC6,	0xBB,	0x00,	0x7E,	0x00,	0x75,	0x00,	0x91	};
char gamma_set_90_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xB7,	0xBA,	0xB2,	0xBE,	0xC5,	0xB4,	0xCF,	0xD4,	0xC7,	0xA8,	0xB1,	0x9E,	0xC0,	0xC6,	0xBB,	0x00,	0x7D,	0x00,	0x73,	0x00,	0x8F	};
char gamma_set_85_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xA1,	0xA4,	0x9C,	0xC1,	0xC8,	0xB7,	0xCE,	0xD4,	0xC6,	0xAA,	0xB2,	0x9F,	0xC0,	0xC7,	0xBB,	0x00,	0x7A,	0x00,	0x71,	0x00,	0x8D	};
char gamma_set_80_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xA5,	0xA7,	0xA1,	0xBF,	0xC7,	0xB5,	0xCD,	0xD3,	0xC5,	0xAA,	0xB2,	0x9F,	0xC2,	0xC8,	0xBD,	0x00,	0x78,	0x00,	0x6E,	0x00,	0x8A	};
char gamma_set_75_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xA5,	0xA7,	0xA1,	0xC2,	0xCA,	0xB8,	0xCE,	0xD4,	0xC7,	0xA9,	0xB1,	0x9E,	0xC2,	0xC9,	0xBD,	0x00,	0x76,	0x00,	0x6C,	0x00,	0x87	};
char gamma_set_70_tp20[26]  = {	0xFA,	0x01,	0x4A,	0x2E,	0x53,	0xA9,	0xAB,	0xA6,	0xC0,	0xC8,	0xB6,	0xCD,	0xD3,	0xC5,	0xAA,	0xB2,	0x9F,	0xC3,	0xC9,	0xBE,	0x00,	0x73,	0x00,	0x6A,	0x00,	0x85	};
#else
char gamma_set_230_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xC4,   0xCD,   0xB7,   0xBC,   0xC3,   0xB2,   0xC7,   0xCD,   0xC1,   0x9D,   0xA6,   0x95,   0xB3,   0xB9,   0xAF,   0x00,   0x9B,   0x00,   0x92,   0x00,   0xB4    };
char gamma_set_224_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xC4,   0xCD,   0xB7,   0xBC,   0xC3,   0xB2,   0xC9,   0xCF,   0xC3,   0x9D,   0xA6,   0x96,   0xB4,   0xBA,   0xB0,   0x00,   0x99,   0x00,   0x90,   0x00,   0xB2    };
char gamma_set_218_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xC7,   0xD0,   0xBA,   0xBB,   0xC2,   0xB1,   0xC8,   0xCE,   0xC2,   0x9D,   0xA7,   0x96,   0xB3,   0xB9,   0xAF,   0x00,   0x98,   0x00,   0x8F,   0x00,   0xB1    };
char gamma_set_212_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xC7,   0xD0,   0xBA,   0xBE,   0xC5,   0xB4,   0xC7,   0xCD,   0xC1,   0x9E,   0xA7,   0x96,   0xB4,   0xBA,   0xAF,   0x00,   0x96,   0x00,   0x8D,   0x00,   0xAF    };
char gamma_set_206_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xCA,   0xD3,   0xBE,   0xBA,   0xC1,   0xB0,   0xC9,   0xCF,   0xC3,   0x9E,   0xA7,   0x97,   0xB4,   0xBA,   0xB0,   0x00,   0x94,   0x00,   0x8B,   0x00,   0xAC    };
char gamma_set_200_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xC1,   0xC9,   0xB5,   0xBD,   0xC4,   0xB3,   0xC8,   0xCE,   0xC2,   0x9E,   0xA8,   0x97,   0xB5,   0xBB,   0xB0,   0x00,   0x92,   0x00,   0x89,   0x00,   0xAA    };
char gamma_set_194_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xC1,   0xC9,   0xB5,   0xBF,   0xC6,   0xB6,   0xC7,   0xCD,   0xC1,   0x9F,   0xA8,   0x97,   0xB5,   0xBB,   0xB1,   0x00,   0x91,   0x00,   0x87,   0x00,   0xA8    };
char gamma_set_188_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xC4,   0xCC,   0xB8,   0xBB,   0xC3,   0xB1,   0xC9,   0xCF,   0xC3,   0x9F,   0xA8,   0x98,   0xB5,   0xBC,   0xB1,   0x00,   0x8F,   0x00,   0x86,   0x00,   0xA6    };
char gamma_set_182_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xC4,   0xCC,   0xB8,   0xBE,   0xC5,   0xB4,   0xC8,   0xCE,   0xC2,   0x9F,   0xA9,   0x98,   0xB6,   0xBC,   0xB1,   0x00,   0x8D,   0x00,   0x84,   0x00,   0xA4    };
char gamma_set_176_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xC7,   0xCF,   0xBC,   0xBD,   0xC4,   0xB3,   0xC7,   0xCD,   0xC1,   0xA0,   0xA9,   0x99,   0xB6,   0xBD,   0xB2,   0x00,   0x8B,   0x00,   0x82,   0x00,   0xA2    };
char gamma_set_170_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xC7,   0xCF,   0xBC,   0xBD,   0xC4,   0xB3,   0xC9,   0xCF,   0xC3,   0xA0,   0xA9,   0x99,   0xB7,   0xBD,   0xB2,   0x00,   0x89,   0x00,   0x80,   0x00,   0xA0    };
char gamma_set_164_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xB9,   0xC0,   0xAE,   0xBF,   0xC6,   0xB6,   0xC8,   0xCE,   0xC1,   0xA1,   0xAA,   0x99,   0xB7,   0xBD,   0xB3,   0x00,   0x87,   0x00,   0x7E,   0x00,   0x9E    };
char gamma_set_158_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xBC,   0xC3,   0xB2,   0xBE,   0xC5,   0xB5,   0xC8,   0xCD,   0xC0,   0xA1,   0xAA,   0x9A,   0xB8,   0xBE,   0xB3,   0x00,   0x86,   0x00,   0x7C,   0x00,   0x9C    };
char gamma_set_152_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xBC,   0xC3,   0xB2,   0xC1,   0xC8,   0xB8,   0xC7,   0xCC,   0xBF,   0xA1,   0xAB,   0x9A,   0xB9,   0xBF,   0xB5,   0x00,   0x83,   0x00,   0x7A,   0x00,   0x99    };
char gamma_set_146_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xBF,   0xC6,   0xB5,   0xBD,   0xC4,   0xB3,   0xC9,   0xCE,   0xC1,   0xA3,   0xAC,   0x9B,   0xB9,   0xBF,   0xB4,   0x00,   0x81,   0x00,   0x78,   0x00,   0x97    };
char gamma_set_140_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xBF,   0xC6,   0xB5,   0xC0,   0xC7,   0xB6,   0xC8,   0xCD,   0xC0,   0xA3,   0xAC,   0x9C,   0xBA,   0xC0,   0xB6,   0x00,   0x7F,   0x00,   0x76,   0x00,   0x94    };
char gamma_set_134_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xB4,   0xBA,   0xAC,   0xBD,   0xC5,   0xB4,   0xC9,   0xCE,   0xC1,   0xA2,   0xAB,   0x9B,   0xBB,   0xC1,   0xB6,   0x00,   0x7D,   0x00,   0x74,   0x00,   0x92    };
char gamma_set_128_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xB4,   0xBA,   0xAC,   0xC0,   0xC7,   0xB7,   0xC8,   0xCD,   0xC0,   0xA4,   0xAD,   0x9C,   0xBB,   0xC1,   0xB6,   0x00,   0x7B,   0x00,   0x71,   0x00,   0x8F    };
char gamma_set_122_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xB8,   0xBE,   0xB0,   0xBE,   0xC5,   0xB4,   0xC7,   0xCD,   0xBF,   0xA4,   0xAD,   0x9C,   0xBD,   0xC3,   0xB8,   0x00,   0x78,   0x00,   0x6F,   0x00,   0x8C    };
char gamma_set_116_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xB8,   0xBE,   0xB0,   0xC0,   0xC7,   0xB7,   0xC9,   0xCF,   0xC1,   0xA3,   0xAC,   0x9B,   0xBC,   0xC2,   0xB7,   0x00,   0x76,   0x00,   0x6D,   0x00,   0x8A    };
char gamma_set_110_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xAD,   0xB2,   0xA6,   0xBE,   0xC5,   0xB4,   0xC9,   0xCE,   0xC1,   0xA4,   0xAD,   0x9C,   0xBC,   0xC2,   0xB7,   0x00,   0x74,   0x00,   0x6B,   0x00,   0x88    };
char gamma_set_105_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xAD,   0xB2,   0xA6,   0xBE,   0xC5,   0xB4,   0xCB,   0xD1,   0xC4,   0xA4,   0xAD,   0x9C,   0xBD,   0xC4,   0xB9,   0x00,   0x72,   0x00,   0x69,   0x00,   0x85    };
char gamma_set_100_tp20[26]  = {        0xFA,   0x01,   0x3D,   0x22,   0x45,   0xB1,   0xB5,   0xAB,   0xBB,   0xC3,   0xB1,   0xCB,   0xD0,   0xC3,   0xA4,   0xAD,   0x9C,   0xBE,   0xC4,   0xB9,   0x00,   0x70,   0x00,   0x67,   0x00,   0x83    };
char gamma_set_95_tp20[26]  = { 0xFA,   0x01,   0x3D,   0x22,   0x45,   0xB1,   0xB5,   0xAB,   0xBE,   0xC5,   0xB4,   0xCA,   0xD0,   0xC2,   0xA5,   0xAE,   0x9D,   0xBE,   0xC4,   0xB9,   0x00,   0x6E,   0x00,   0x64,   0x00,   0x80    };
char gamma_set_90_tp20[26]  = { 0xFA,   0x01,   0x3D,   0x22,   0x45,   0xB6,   0xB9,   0xB0,   0xBC,   0xC3,   0xB1,   0xCC,   0xD1,   0xC4,   0xA3,   0xAC,   0x9B,   0xBF,   0xC5,   0xBA,   0x00,   0x6C,   0x00,   0x63,   0x00,   0x7E    };
char gamma_set_85_tp20[26]  = { 0xFA,   0x01,   0x3D,   0x22,   0x45,   0xA0,   0xA3,   0x9B,   0xBF,   0xC7,   0xB5,   0xCB,   0xD1,   0xC3,   0xA4,   0xAD,   0x9C,   0xBF,   0xC5,   0xBA,   0x00,   0x6A,   0x00,   0x60,   0x00,   0x7C    };
char gamma_set_80_tp20[26]  = { 0xFA,   0x01,   0x3D,   0x22,   0x45,   0xA4,   0xA7,   0xA0,   0xBD,   0xC5,   0xB2,   0xCA,   0xD0,   0xC2,   0xA5,   0xAE,   0x9C,   0xC1,   0xC7,   0xBC,   0x00,   0x67,   0x00,   0x5E,   0x00,   0x79    };
char gamma_set_75_tp20[26]  = { 0xFA,   0x01,   0x3D,   0x22,   0x45,   0xA4,   0xA7,   0xA0,   0xC0,   0xC8,   0xB6,   0xCC,   0xD1,   0xC4,   0xA4,   0xAD,   0x9A,   0xC1,   0xC7,   0xBC,   0x00,   0x65,   0x00,   0x5C,   0x00,   0x76    };
char gamma_set_70_tp20[26]  = { 0xFA,   0x01,   0x3D,   0x22,   0x45,   0xA8,   0xAA,   0xA5,   0xBE,   0xC6,   0xB3,   0xCB,   0xD0,   0xC3,   0xA5,   0xAE,   0x9C,   0xC1,   0xC8,   0xBC,   0x00,   0x63,   0x00,   0x59,   0x00,   0x74    };
char gamma_set_20_tp20[26]   = {0xFA, 0x01, 0x3D, 0x22, 0x45, 0x64, 0x63, 0x64, 0xA9, 0xAB, 0xA6,
                                         0xCB, 0xD1, 0xC2, 0xAC, 0xB4, 0xA0, 0xC7, 0xCD, 0xC1, 0x00,
                                         0x40, 0x00, 0x36, 0x00, 0x4C};
char gamma_set_20_extra[26]   = {0xFA, 0x01, 0x3D, 0x22, 0x45, 0x64, 0x63, 0x64, 0xA9, 0xAB, 0xA6,
                                         0xCB, 0xD1, 0xC2, 0xAC, 0xB4, 0xA0, 0xC7, 0xCD, 0xC1, 0x00,
                                         0x40, 0x00, 0x36, 0x00, 0x4C};

#endif

//#endif /* TP10 */
//#endif /* WS20 */

#ifdef CONFIG_ACL_FOR_AMOLED
char acl_per55[29] = { 0xC1, 0x47, 0x53, 0x13, 0x53, 0x00, 0x00, 0x02, 0xCF, 0x00, 0x00,
			     0x04, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0A, 0x12,
			     0x1B, 0x23, 0x2C, 0x35, 0x3D, 0x46, 0x4E, 0x57};
char acl_per53[29] = { 0xC1, 0x47, 0x53, 0x13, 0x53, 0x00, 0x00, 0x02, 0xCF, 0x00, 0x00,
			     0x04, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x09, 0x11,
			     0x1A, 0x22, 0x2A, 0x32, 0x3A, 0x43, 0x4B, 0x53};
char acl_per52[29] = { 0xC1, 0x47, 0x53, 0x13, 0x53, 0x00, 0x00, 0x02, 0xCF, 0x00, 0x00,
			     0x04, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x09, 0x11,
			     0x19, 0x21, 0x29, 0x31, 0x39, 0x41, 0x49, 0x51};
char acl_per50[29] = { 0xC1, 0x47, 0x53, 0x13, 0x53, 0x00, 0x00, 0x02, 0xCF, 0x00, 0x00,
			     0x04, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x09, 0x10,
			     0x18, 0x1F, 0x27, 0x2E, 0x36, 0x3D, 0x45, 0x4C};
char acl_per48[29] = { 0xC1, 0x47, 0x53, 0x13, 0x53, 0x00, 0x00, 0x02, 0xCF, 0x00, 0x00,
			     0x04, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x0F,
			     0x17, 0x1E, 0x25, 0x2C, 0x33, 0x3B, 0x42, 0x49};
char acl_per45[29] = { 0xC1, 0x47, 0x53, 0x13, 0x53, 0x00, 0x00, 0x02, 0xCF, 0x00, 0x00,
			     0x04, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x0E,
			     0x14, 0x1B, 0x21, 0x27, 0x2E, 0x34, 0x3B, 0x41};
char acl_per43[29] = { 0xC1, 0x47, 0x53, 0x13, 0x53, 0x00, 0x00, 0x02, 0xCF, 0x00, 0x00,
			     0x04, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x0D,
			     0x14, 0x1A, 0x20, 0x26, 0x2C, 0x33, 0x39, 0x3F};
char acl_per30[29] = { 0xC1, 0x47, 0x53, 0x13, 0x53, 0x00, 0x00, 0x02, 0xCF, 0x00, 0x00,
			     0x04, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			     0x02, 0x06, 0x0A, 0x0F, 0x14, 0x1A, 0x20, 0x27};
char acl_per10[29] = { 0xC1, 0x47, 0x53, 0x13, 0x53, 0x00, 0x00, 0x02, 0xCF, 0x00, 0x00,
			     0x04, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			     0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x0A, 0x0D};
char acl_on[2]	= { 0xC0, 0x01 };
char acl_off[2]	= { 0xC0, 0x00 };
#endif /* D_SKY_OLED_ACL */

#endif /* backlight */

static struct dsi_cmd_desc samsung_oled_hd_display_init_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(mtp_enable),mtp_enable }, 
	{DTYPE_DCS_WRITE, 1, 0, 0, 15, sizeof(sleep_out),sleep_out },  
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(panel_cond_set),panel_cond_set}, 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(disp_cond_set),disp_cond_set}, 
};

static struct dsi_cmd_desc samsung_oled_hd_screen_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_in), sleep_in},
};

/*  499 
static struct dsi_cmd_desc samsung_oled_hd_display_wakeup_cmds[] = {

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out),sleep_out },  
};
*/
static struct dsi_cmd_desc samsung_oled_hd_display_wakeup_cmds[] = {

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out),sleep_out },  
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(mtp_enable),mtp_enable }, 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(panel_cond_set),panel_cond_set}, 
};

static struct dsi_cmd_desc samsung_oled_hd_display_on2_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(source_control),source_control}, 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(pentile_control),pentile_control}, 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nvm_setting),nvm_setting}, 
#if 0
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(mipi_control1),mipi_control1}, 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(mipi_control2),mipi_control2}, 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(mipi_control3),mipi_control3}, 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(mipi_control4),mipi_control4}, 
#endif
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(power_control),power_control}, 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 130, sizeof(elvss_control),elvss_control}, 
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(disp_on), disp_on},
#ifdef CONFIG_ACL_FOR_AMOLED		
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(acl_on), acl_on},//added by ji seung hwa
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(acl_per10),acl_per10 },//added by ji seung hwa
#endif	
};

#ifdef D_SKY_BACKLIGHT_CONTROL
/* backlight */
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_29[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_29), gamma_set_level_29},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_28[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_28), gamma_set_level_28},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_27[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_27), gamma_set_level_27},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_26[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_26), gamma_set_level_26},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_25[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_25), gamma_set_level_25},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_24[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_24), gamma_set_level_24},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_23[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_23), gamma_set_level_23},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_22[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_22), gamma_set_level_22},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_21[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_21), gamma_set_level_21},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_20[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_20), gamma_set_level_20},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_19[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_19), gamma_set_level_19},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_18[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_18), gamma_set_level_18},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_17[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_17), gamma_set_level_17},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_16[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_16), gamma_set_level_16},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_15[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_15), gamma_set_level_15},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_14[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_14), gamma_set_level_14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_13[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_13), gamma_set_level_13},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_12[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_12), gamma_set_level_12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_11[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_11), gamma_set_level_11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_10[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_10), gamma_set_level_10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_9[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_9), gamma_set_level_9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_8[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_8), gamma_set_level_8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_7[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_7), gamma_set_level_7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_6[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_6), gamma_set_level_6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_5[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_5), gamma_set_level_5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_4[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_4), gamma_set_level_4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_3[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_3), gamma_set_level_3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_2[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_2), gamma_set_level_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_gamma_cmds_level_1[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_level_1), gamma_set_level_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_update),gamma_update}, 
};
#ifdef CONFIG_ACL_FOR_AMOLED
static struct dsi_cmd_desc samsung_oled_hd_display_acl_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(acl_on), acl_on},
};
static struct dsi_cmd_desc samsung_oled_hd_display_acl_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(acl_off), acl_off},
};
static struct dsi_cmd_desc samsung_oled_hd_display_acl_control10_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(acl_per10),acl_per10 },
};
static struct dsi_cmd_desc samsung_oled_hd_display_acl_control30_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(acl_per30),acl_per30 },
};
static struct dsi_cmd_desc samsung_oled_hd_display_acl_control43_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(acl_per43),acl_per43 },
};
static struct dsi_cmd_desc samsung_oled_hd_display_acl_control45_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(acl_per45),acl_per45 },
};
static struct dsi_cmd_desc samsung_oled_hd_display_acl_control48_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(acl_per48),acl_per48 },
};
static struct dsi_cmd_desc samsung_oled_hd_display_acl_control50_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(acl_per50),acl_per50 },
};
static struct dsi_cmd_desc samsung_oled_hd_display_acl_control52_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(acl_per52),acl_per52 },
};
static struct dsi_cmd_desc samsung_oled_hd_display_acl_control53_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(acl_per53),acl_per53 },
};
static struct dsi_cmd_desc samsung_oled_hd_display_acl_control55_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(acl_per55),acl_per55 },
};
#endif /* ACL */
#endif
#ifdef D_SKY_DYN_ELVSS 
char elvss_300210_0_tp20[3]	= { 0xB1, 0x04, 0x8D };
char elvss_200170_0_tp20[3]	= { 0xB1, 0x04, 0x94 };
char elvss_160110_0_tp20[3]	= { 0xB1, 0x04, 0x97 };
char elvss_100000_0_tp20[3]	= { 0xB1, 0x04, 0x9C };

char elvss_300210_0[3]	= { 0xB1, 0x04, 0x11 };
char elvss_200170_0[3]	= { 0xB1, 0x04, 0x19 };
char elvss_160110_0[3]	= { 0xB1, 0x04, 0x1B };
char elvss_100000_0[3]	= { 0xB1, 0x04, 0x1F };

char elvss_300210_1[3]	= { 0xB1, 0x04, 0 };
char elvss_200170_1[3]	= { 0xB1, 0x04, 0 };
char elvss_160110_1[3]	= { 0xB1, 0x04, 0 };
char elvss_100000_1[3]	= { 0xB1, 0x04, 0 };

static struct dsi_cmd_desc samsung_oled_hd_display_elvss300_210_id3_0_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(elvss_300210_0),elvss_300210_0}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_elvss200_170_id3_0_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(elvss_200170_0),elvss_200170_0}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_elvss160_110_id3_0_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(elvss_160110_0),elvss_160110_0}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_elvss100_000_id3_0_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(elvss_100000_0),elvss_100000_0}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_elvss300_210_id3_1_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(elvss_300210_1),elvss_300210_1}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_elvss200_170_id3_1_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(elvss_200170_1),elvss_200170_1}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_elvss160_110_id3_1_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(elvss_160110_1),elvss_160110_1}, 
};
static struct dsi_cmd_desc samsung_oled_hd_display_elvss100_000_id3_1_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(elvss_100000_1),elvss_100000_1}, 
};
#endif

int get_board_revision(void)
{
	int rc;
	

	rc = get_hw_revision();
//	printk("[PANTECH_LCD] Read Board H/W Rev : %d \n",rc);

	return rc;
}

static struct dsi_cmd_desc samsung_oled_hd_id_cmd ={
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(id_read), id_read};

static uint32 mipi_samsung_oled_hd_read_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	int i;
	uint8 *lp;

	tp = &samsung_oled_hd_tx_buf;
	rp = &samsung_oled_hd_rx_buf;	
		
	lp = NULL;

	cmd = &samsung_oled_hd_id_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd,4);
	for(i=0; i<3;i++)
	{
		lp = ((uint8 *)rp->data++);
		if(i==0)
		    printk("[PANTECH_LCD]OLED Manufacture ID(ID1) = 0x%x\n",*lp); 
		else if(i==1)
		{
		    prev_id2 = (*lp);
		    printk("[PANTECH_LCD]OLED Rev.(ID2) = 0x%x\n",*lp); 
		}
		else if(i==2){
		    elvss_id3 = (*lp);
		    printk("[PANTECH_LCD]OLED ELVSS Value(ID3) = 0x%x\n",elvss_id3); 
#ifdef D_SKY_DYN_ELVSS 
		    if(elvss_id3 != 0)
		    {	
			    if(elvss_id3 >=0x9f)
				elvss_300210_1[2]= 0x9f;
			    else
				elvss_300210_1[2]= elvss_id3;
			    if((elvss_id3 + 0x7)>=0x9f)
			    	elvss_200170_1[2]= 0x9f;
			    else
			    	elvss_200170_1[2]= (elvss_id3 + 0x7);
			    if((elvss_id3 + 0xa)>=0x9f)
			    	elvss_160110_1[2]= 0x9f;
			    else
			    	elvss_160110_1[2]= (elvss_id3 + 0xa);
				
			    if((elvss_id3 + 0xf)>=0x9f)
		            	elvss_100000_1[2]= 0x9f;
			    else
		            	elvss_100000_1[2]= (elvss_id3 + 0xf);
			 }
#endif
	        }
		msleep(1);
	}
	
	return *lp;
}

#ifdef D_SKY_OLED_TEMP
#if 1
void is_oled_temp_check(struct work_struct *work)
{
	
	struct pm8xxx_adc_chan_result result;
	int rc=0;
	int try_max=0;
	struct fb_info *info; 
	struct msm_fb_data_type *mfd;

    	info = registered_fb[0];
	mfd = (struct msm_fb_data_type *)info->par;

	do
	{
	   rc = pm8xxx_adc_mpp_config_read(PM8XXX_AMUX_MPP_9, ADC_MPP_1_AMUX6, &result);
	   try_max++;
	}while(rc && try_max < 20);

	pm8xxx_adc_scale_lcd_therm(&result);
   	printk("[PANTECH_LCD]: OLED Panel Temperature  %lld(C)\n ", result.physical);
#if 1
	if(result.physical > 42 ) 
	{
		printk("[PANTECH_LCD] PANEL TEMP : PoweSave Mode...(now bl was saved[%d]\n",prev_bl);
		is_temp = 1;
		if((samsung_oled_hd_state.disp_on == true) && (is_high_bk == true) && (is_set == true))
		{
    			mutex_lock(&bl_mutex);
        		mipi_set_tx_power_mode(0);

	   		mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_11,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_11));
			printk("[PANTECH_LCD] PANEL TEMP : PoweSave Mode...(now bl was saved[%d]\n",prev_bl);
			is_set = false;
		//	ptemp_bl = prev_bl;

    			mutex_unlock(&bl_mutex);
        		mipi_set_tx_power_mode(1);
		}
	}else if((result.physical < 38) && (is_temp == 1))
	{
		printk("[PANTECH_LCD] PANEL TEMP : Normal Mode...(saved bl[%d]\n",ptemp_bl);
		if( (samsung_oled_hd_state.disp_on == true) && (is_high_bk == true) && is_set == false)
		{
    			mutex_lock(&bl_mutex);
        		mipi_set_tx_power_mode(0);

			switch(ptemp_bl)
			{
			case 15:
			   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_15,
					ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_15));
			break;
			case 14:
			   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_14,
					ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_14));
			break;
			case 13:
			   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_13,
					ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_13));
			break;
			case 12:
	  		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_12,
					ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_12));
			break;
			case 11:
	  		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_11,
					ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_11));
			break;
			default:
			  printk("[PANTECH_LCD] your bl: %d was not supporteded...\n",ptemp_bl);
			}
			printk("[PANTECH_LCD] PANEL TEMP : Normal Mode...(prev bl is set[%d]\n",ptemp_bl);
			is_temp = 0;
			is_set = true;
			is_high_bk = true;

        		mipi_set_tx_power_mode(1);
    			mutex_unlock(&bl_mutex);
		}
	}
#endif
	schedule_delayed_work(&panel_therm_detect_work, msecs_to_jiffies(20000));
}
#endif
#endif
void set_panel_gamma(unsigned int id2_panel_rev)
{
	int i;

	if(id2_panel_rev == 0xD3 || id2_panel_rev == 0xB3) /* Panel Rev TP20 */
	{
		for(i=0;i<26;i++)
		{
			gamma_set_level_29[i] = gamma_set_230_tp20[i];
			gamma_set_level_28[i] = gamma_set_224_tp20[i];
			gamma_set_level_27[i] = gamma_set_218_tp20[i];
			gamma_set_level_26[i] = gamma_set_212_tp20[i];
			gamma_set_level_25[i] = gamma_set_206_tp20[i];
			gamma_set_level_24[i] = gamma_set_200_tp20[i];
			gamma_set_level_23[i] = gamma_set_194_tp20[i];
			gamma_set_level_22[i] = gamma_set_188_tp20[i];
			gamma_set_level_21[i] = gamma_set_182_tp20[i];
			gamma_set_level_20[i] = gamma_set_176_tp20[i];
			gamma_set_level_19[i] = gamma_set_170_tp20[i];
			gamma_set_level_18[i] = gamma_set_164_tp20[i];
			gamma_set_level_17[i] = gamma_set_158_tp20[i];
			gamma_set_level_16[i] = gamma_set_152_tp20[i];
			gamma_set_level_15[i] = gamma_set_146_tp20[i];
			gamma_set_level_14[i] = gamma_set_140_tp20[i];
			gamma_set_level_13[i] = gamma_set_134_tp20[i];
			gamma_set_level_12[i] = gamma_set_128_tp20[i];
			gamma_set_level_11[i] = gamma_set_122_tp20[i];
			gamma_set_level_10[i] = gamma_set_116_tp20[i];
			gamma_set_level_9[i] = gamma_set_110_tp20[i];
			gamma_set_level_8[i] = gamma_set_105_tp20[i];
			gamma_set_level_7[i] = gamma_set_100_tp20[i];
			gamma_set_level_6[i] = gamma_set_95_tp20[i];
			gamma_set_level_5[i] = gamma_set_90_tp20[i];
			gamma_set_level_4[i] = gamma_set_85_tp20[i];
			gamma_set_level_3[i] = gamma_set_80_tp20[i];
			gamma_set_level_2[i] = gamma_set_75_tp20[i];
			gamma_set_level_1[i] = gamma_set_70_tp20[i];
	      }
#ifdef D_SKY_DYN_ELVSS 
		for(i=0;i<3;i++)
		{
			elvss_300210_0[i] = elvss_300210_0_tp20[i];
			elvss_200170_0[i] = elvss_200170_0_tp20[i];
			elvss_160110_0[i] = elvss_160110_0_tp20[i];
			elvss_100000_0[i] = elvss_100000_0_tp20[i];
		}
#endif
		printk("[PANTECH_LCD] TP20 OLED Setting Finished...\n");
	}

	if(off_charger_flag == true)
	{
		if(id2_panel_rev == 0xD3 || id2_panel_rev== 0xB3) /* Panel Rev TP20 ,TP10*/
		{
	    		for(i=0;i<26;i++)
				gamma_set_level_1[i] = gamma_set_20_tp20[i];
		}else 
		{
	    		for(i=0;i<26;i++)
				gamma_set_level_1[i] = gamma_set_20_extra[i];
		}
			printk("[PANTECH_LCD] Offline Charging Gamma 20cd/m2 Setting Finished...\n");
	}

}

static int mipi_samsung_oled_hd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	u32 tmp;
#if (BOARD_VER > WS10) 
	int noled_det;
#endif

   // 	ENTER_FUNC2();
    
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

       if (samsung_oled_hd_state.disp_initialized == false) {
//	    mutex_lock(&onoff_mutex);
	    mutex_lock(&bl_mutex);
    	    samsung_oled_hd_state.disp_initialized = true;
	   		
//  	    	gpio_set_value_cansleep(gpio43, 0);
// 	    	msleep(10);
 //      	gpio_set_value_cansleep(gpio43, 1);  // lcd panel reset 
// 	    	msleep(10);
		
            mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_init_cmds,
                   ARRAY_SIZE(samsung_oled_hd_display_init_cmds));


#if (BOARD_VER > WS10) 
            noled_det = gpio_get_value(OLED_DET);
	    
	    if(noled_det)
		 printk("[LCD] Connector: Disconnect!\n");
	    else
		 printk("[LCD] Connector: Connect!\n");

	    
	    if((!is_read)&(!noled_det))
#else
 	    if(!is_read)
#endif
		{
			mipi_dsi_cmd_bta_sw_trigger(); 
	    	mipi_samsung_oled_hd_read_id(mfd);
	 	  	mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_15,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_15));
			is_read = TRUE;

			set_panel_gamma(prev_id2);

//			INIT_DELAYED_WORK_DEFERRABLE(&panel_id3_detect_work,change_elvss);
//			schedule_delayed_work(&panel_id3_detect_work, msecs_to_jiffies(0000));
	    }else{
#ifdef D_SKY_OLED_TEMP
		if(!noled_det)
			schedule_delayed_work(&panel_therm_detect_work, msecs_to_jiffies(20000));
#endif
	  		mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_1,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_1));
	    }
		//else{
	  	//	mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_1,
		//		ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_1));
	    	//}
		
		tmp = MIPI_INP(MIPI_DSI_BASE + 0xA8);
		tmp |= (1<<28);
		MIPI_OUTP(MIPI_DSI_BASE + 0xA8, tmp);
//		wmb();
		printk("[MIPI: shinbrad High speed Clk Set(On Sequence) .................................................]\n");
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_on2_cmds,
                  ARRAY_SIZE(samsung_oled_hd_display_on2_cmds));

#ifdef CONFIG_ACL_FOR_AMOLED
		 //if(samsung_oled_hd_state.acl_flag == true)//added by ji seung hwa
		// {

		 //	mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf,samsung_oled_hd_display_acl_on_cmds,
//ARRAY_SIZE(samsung_oled_hd_display_acl_on_cmds));

			switch(samsung_oled_hd_state.acl_data)
			{
                case 55: 
					mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf,samsung_oled_hd_display_acl_control55_cmds ,
	                      ARRAY_SIZE(samsung_oled_hd_display_acl_control55_cmds));
          
                break;
                case 53: 
					mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control53_cmds,
                          ARRAY_SIZE(samsung_oled_hd_display_acl_control53_cmds));
            
                break;
                case 52:
					mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control52_cmds,
                          ARRAY_SIZE(samsung_oled_hd_display_acl_control52_cmds));
          
                break;
                case 50:
					mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control50_cmds,
                          ARRAY_SIZE(samsung_oled_hd_display_acl_control50_cmds));
              
                break;
                case 48:
					mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control48_cmds,
                          ARRAY_SIZE(samsung_oled_hd_display_acl_control48_cmds));
                
                break;
                case 45:
					mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control45_cmds,
                          ARRAY_SIZE(samsung_oled_hd_display_acl_control45_cmds));
               
                break;
                case 43:
					mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control43_cmds,
                          ARRAY_SIZE(samsung_oled_hd_display_acl_control43_cmds));
              
                break;
                	case 30:
					mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control30_cmds,
                          ARRAY_SIZE(samsung_oled_hd_display_acl_control30_cmds));
              
                	break;
                	case 10:
					mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control10_cmds,
                          ARRAY_SIZE(samsung_oled_hd_display_acl_control10_cmds));
              
                	break;

			default:
			  printk("[PANTECH_LCD:%s] your rate: %d was not supported ACL percent rate...\n",__func__,samsung_oled_hd_state.acl_data);
			}
			printk("samsung_oled_hd_display_acl_data = %d\n",samsung_oled_hd_state.acl_data);
		 //}
		 	
#endif
#ifdef CONFIG_F_SKYDISP_SMART_DIMMING
	if(first_read && get_board_revision() != 3) {
		first_read = FALSE;

		mtp_read();
		printk("SMART DIMMING MTP_READ\n");
	}
#endif
	   is_sleep = FALSE;

#if !defined(FEATURE_AARM_RELEASE_MODE) //not user mode
	    printk("[PANTECH_LCD] power on state (oled_hd panel).... \n");
#endif

  	   samsung_oled_hd_state.disp_on = true;
	    mutex_unlock(&bl_mutex);
//	   mutex_unlock(&onoff_mutex);
#ifdef CONFIG_PANTECH_HDMI_PW_CTRL_ON_LCD
		hdmi_autodetect_control(HDMI_PW_ON);
		HDMI_Schedule_Control(1);
#endif	
       }
   
       EXIT_FUNC2();
       return 0;
}

static int mipi_samsung_oled_hd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	u32 tmp;
#ifdef D_SKY_OLED_TEMP
	int noled_det;
#endif
    
    //	ENTER_FUNC2();
    
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
    
	if (samsung_oled_hd_state.disp_on == true) {
	//	is_sleep = TRUE;
//	mutex_lock(&onoff_mutex);
		mutex_lock(&bl_mutex);
        samsung_oled_hd_state.disp_initialized = false;
        samsung_oled_hd_state.disp_on = false;
#if !defined(FEATURE_AARM_RELEASE_MODE) //not user mode
        printk("[PANTECH_LCD] power off state (oled_hd panel).... \n");
#endif

#ifdef D_SKY_OLED_TEMP
        noled_det = gpio_get_value(OLED_DET);
	if(!noled_det)
    	cancel_delayed_work(&panel_therm_detect_work);
#endif

#ifdef CONFIG_PANTECH_HDMI_PW_CTRL_ON_LCD
	if (!HDMI_Get_Cable_State()) {
		hdmi_autodetect_control(HDMI_PW_OFF);
		HDMI_Schedule_Control(0);
	}

#endif	
		tmp = MIPI_INP(MIPI_DSI_BASE + 0xA8);
		tmp &= ~(1<<28);
		MIPI_OUTP(MIPI_DSI_BASE + 0xA8, tmp);
//		wmb();
		printk("[MIPI: Low speed Clk Set(Off) .................................................]\n");
//		mutex_unlock(&onoff_mutex);
	    	mutex_unlock(&bl_mutex);
    	}	   
    
    	EXIT_FUNC2();
	return 0;
}
#ifdef CONFIG_ACL_FOR_AMOLED
void acl_contol(struct msm_fb_data_type *mfd, int state)
{
      //  mutex_lock(&mfd->dma->ov_mutex);
	    mutex_lock(&bl_mutex);

        mipi_set_tx_power_mode(0);

        if(state == true){
                mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf,samsung_oled_hd_display_acl_on_cmds,
                                                        ARRAY_SIZE(samsung_oled_hd_display_acl_on_cmds));
                samsung_oled_hd_state.acl_flag = true;
        }
        else{
                mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_off_cmds,
                                                        ARRAY_SIZE(samsung_oled_hd_display_acl_off_cmds));
                samsung_oled_hd_state.acl_flag = false;
        }

        mipi_set_tx_power_mode(1);
      //  mutex_unlock(&mfd->dma->ov_mutex);
	    mutex_unlock(&bl_mutex);
        printk(KERN_WARNING"[PATECH_LCD]OLED_ACL = %d ( 0: Off , 1: On )\n",state);
}

void acl_data(struct msm_fb_data_type *mfd, int data)
{
      //  mutex_lock(&mfd->dma->ov_mutex);
	     mutex_lock(&bl_mutex);

        mipi_set_tx_power_mode(0);

        switch(data){
                case 55: 
				mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf,samsung_oled_hd_display_acl_control55_cmds ,
                                                       ARRAY_SIZE(samsung_oled_hd_display_acl_control55_cmds));
                samsung_oled_hd_state.acl_data=data;
                break;
                case 53: 
				mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control53_cmds,
                                                       ARRAY_SIZE(samsung_oled_hd_display_acl_control53_cmds));
                samsung_oled_hd_state.acl_data=data;
                break;
                case 52:
				mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control52_cmds,
                                                      ARRAY_SIZE(samsung_oled_hd_display_acl_control52_cmds));
                samsung_oled_hd_state.acl_data=data;
                break;
                case 50:
				mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control50_cmds,
                                                      ARRAY_SIZE(samsung_oled_hd_display_acl_control50_cmds));
                samsung_oled_hd_state.acl_data=data;
                break;
                case 48:
				mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control48_cmds,
                                                     ARRAY_SIZE(samsung_oled_hd_display_acl_control48_cmds));
                samsung_oled_hd_state.acl_data=data;
                break;
                case 45:
				mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control45_cmds,
                                                     ARRAY_SIZE(samsung_oled_hd_display_acl_control45_cmds));
                samsung_oled_hd_state.acl_data=data;
                break;
                case 43:
				mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control43_cmds,
                                                    ARRAY_SIZE(samsung_oled_hd_display_acl_control43_cmds));
                samsung_oled_hd_state.acl_data=data;
                break;
               	case 30:
				mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control30_cmds,
               				         				  ARRAY_SIZE(samsung_oled_hd_display_acl_control30_cmds));
				samsung_oled_hd_state.acl_data=data;//added by ji seung hwa
                break;
               	case 10:
				mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control10_cmds,
               				         				  ARRAY_SIZE(samsung_oled_hd_display_acl_control10_cmds));
				samsung_oled_hd_state.acl_data=data;//added by ji seung hwa
               	break;
				case 0:
				mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_acl_control10_cmds,
               				         				  ARRAY_SIZE(samsung_oled_hd_display_acl_control10_cmds));
        }

        mipi_set_tx_power_mode(1);
        mutex_unlock(&bl_mutex);
        printk("mipi_premia_lcd_acl_data = %d\n",data);
}
#endif

#ifdef D_SKY_DYN_ELVSS_DBG
void read_elvss_array(int flags)
{
	int i;
	
	for(i=0;i<3;i++)
	{
		if(flags == 4)
			printk("elvss_300210_1[index : %d]=0x%x \n",i,elvss_300210_1[i]);
		else if(flags == 3)
			printk("elvss_200170_1[index : %d]=0x%x \n",i,elvss_200170_1[i]);
		else if(flags == 2)
			printk("elvss_160110_1[index : %d]=0x%x \n",i,elvss_160110_1[i]);
		else 
			printk("elvss_100000_1[index : %d]=0x%x \n",i,elvss_100000_1[i]);
	}
}
#endif
//void change_elvss(unsigned char elvss_val)
void change_elvss(struct work_struct *work)
{
	struct fb_info *info; 
	struct msm_fb_data_type *mfd;

    	info = registered_fb[0];
	mfd = (struct msm_fb_data_type *)info->par;

       	mipi_set_tx_power_mode(0);
       	mutex_lock(&bl_mutex);

	if(elvss_id3)
	{
#ifdef D_SKY_DYN_ELVSS_DBG
		read_elvss_array(nELVSS); /* for test */
#endif

		switch(nELVSS){
		case 4:
   			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss300_210_id3_1_cmds,
					ARRAY_SIZE(samsung_oled_hd_display_elvss300_210_id3_1_cmds));
		break;			
		case 3:
   			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss200_170_id3_1_cmds,
					ARRAY_SIZE(samsung_oled_hd_display_elvss200_170_id3_1_cmds));
		break;			
		case 2:
   			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss160_110_id3_1_cmds,
					ARRAY_SIZE(samsung_oled_hd_display_elvss160_110_id3_1_cmds));
		break;			
		case 1:
   			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss100_000_id3_1_cmds,
					ARRAY_SIZE(samsung_oled_hd_display_elvss100_000_id3_1_cmds));
		break;			
		default:
		break;
		}
	}else /* elvss id3 is zero */
	{
		switch(nELVSS){
		case 4:
			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss300_210_id3_0_cmds,
					ARRAY_SIZE(samsung_oled_hd_display_elvss300_210_id3_0_cmds));
		break;			
		case 3:
			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss200_170_id3_0_cmds,
					ARRAY_SIZE(samsung_oled_hd_display_elvss200_170_id3_0_cmds));
		break;			
		case 2:
			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss160_110_id3_0_cmds,
					ARRAY_SIZE(samsung_oled_hd_display_elvss160_110_id3_0_cmds));
		break;			
		case 1:
			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss100_000_id3_0_cmds,
					ARRAY_SIZE(samsung_oled_hd_display_elvss100_000_id3_0_cmds));
		break;			
		default:
		break;
		}
			
	} 

       	mipi_set_tx_power_mode(1);
       	mutex_unlock(&bl_mutex);
	
	printk("[PANTECH_LCD : Set ELVSS Value : %d \n",nELVSS);
	
	changed_id3 = 1;

}

static void mipi_samsung_oled_hd_set_backlight(struct msm_fb_data_type *mfd)
{
//	ENTER_FUNC2();
#if 1
#ifdef D_SKY_BACKLIGHT_CONTROL
#if 0
//	if(prev_bl != mfd->bl_level) {
    		now_bl = mfd->bl_level; 
//	}// else {
	//	printk("[PANTECH_LCD:%s] Equal backlight value ... pass ....(prev_bl = %d, mfd->bl_level = %d)\n",__func__,prev_bl,mfd->bl_level);
	//	return;
//	}
	if((is_sleep == true) && (samsung_oled_hd_state.disp_on == true) && (is_cont == false)) {
		is_sleep= false;
		is_cont = true;
	}
	if (is_sleep == true || samsung_oled_hd_state.disp_on == false )
	{
		printk("[%s]sleep state.. return \n",__func__);
		return;
	}

#if !defined(FEATURE_AARM_RELEASE_MODE) //not user mode
	printk(KERN_ERR "[PANTECH_LCD] OLED_BL Set : %d (Mode:%d)\n", (mfd->bl_level),is_sleep);
#else
	printk(KERN_ERR "[PANTECH_LCD] BL Set : %d (Mode:%d)\n",(mfd->bl_level),is_sleep);
#endif

    	if((samsung_oled_hd_state.disp_initialized != true) || (samsung_oled_hd_state.disp_on == false)) {
			printk("[PANTECH_LCD] Panel is off state....exit\n");
			return;
	}

    	mutex_lock(&bl_mutex);
        mipi_set_tx_power_mode(0);
	
	if((is_cont==true) && (samsung_oled_hd_state.disp_on == true)) {
    //	mutex_lock(&bl_mutex);
     //   mipi_set_tx_power_mode(0);
	   is_cont = false;
           mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_wakeup_cmds,
                   ARRAY_SIZE(samsung_oled_hd_display_wakeup_cmds));
#if !defined(FEATURE_AARM_RELEASE_MODE) //not user mode
		   printk("[PANTECH_LCD:%s] wakeup state (oled_hd panel).... \n",__func__);
#else
		   printk("[PANTECH_LCD]Wake State\n");
#endif
     //   mipi_set_tx_power_mode(1);
   // 	mutex_unlock(&bl_mutex);
		   
	}
    //	mutex_lock(&bl_mutex);
    //    mipi_set_tx_power_mode(0);
#ifdef D_SKY_OLED_TEMP
	if(is_temp == 1)
	{
		if(now_bl >= 11)
		{
			ptemp_bl= now_bl;
			now_bl = 11;
			printk("[PANTECH_LCD] Panel temp was high...set to low..220cd/m\n");
		} 
	}
#endif
#else
	now_bl = mfd->bl_level; 

//	if(prev_bl == now_bl && samsung_oled_hd_state.disp_on) {
//		printk("[PANTECH_LCD] prev_bl = now_bl return... \n");
//		return;
//	}
		
#ifdef D_SKY_OLED_TEMP
	if(is_temp == 1)
	{
		if(now_bl >= 11)
		{
			ptemp_bl= now_bl;
			now_bl = 11;
			printk("[PANTECH_LCD] Panel temp was high...set to low..220cd/m\n");
		} 
	}
#endif
   	mipi_set_tx_power_mode(0);
   	mutex_lock(&bl_mutex);		

	printk(KERN_ERR "[PANTECH_LCD] OLED_BL : %d now_bl : %d (Mode:%d)\n", (mfd->bl_level), now_bl ,is_sleep);

	if(is_sleep && samsung_oled_hd_state.disp_on )
	{
          mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_wakeup_cmds,
                   ARRAY_SIZE(samsung_oled_hd_display_wakeup_cmds));
		   printk("[PANTECH_LCD:%s] wakeup state (oled_hd panel).... \n",__func__);
	}
	

#endif

	if(get_board_revision() == 3) {
		switch (now_bl) {
		case 15:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_15,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_15));
			nELVSS = 4;
			break;
		case 14:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_14,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_14));
			nELVSS = 4;
			break;
		case 13:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_13,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_13));
			nELVSS = 4;
			break;
		case 12:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_12,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_12));
			nELVSS = 4;
			break;
		case 11:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_11,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_11));
			nELVSS = 4;
			break;
		case 10:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_10,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_10));
			nELVSS = 4;
			break;
		case 9:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_9,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_9));
			nELVSS = 3;
			break;
		case 8:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_8,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_8));
			nELVSS = 3;
			break;
		case 7:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_7,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_7));
			nELVSS = 2;
			break;
		case 6:
			default:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_6,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_6));
			nELVSS = 2;
			break;
		case 5:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_5,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_5));
			nELVSS = 2;
			break;
		case 4:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_4,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_4));
			nELVSS = 2;
			break;
		case 3:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_3,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_3));
			nELVSS = 2;
			break;
		case 2:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_2,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_2));
			nELVSS = 1;
			break;
		case 1:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_1,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_1));
			nELVSS = 1;
			break;
		case 0:
	            mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_screen_off_cmds,
	             	      ARRAY_SIZE(samsung_oled_hd_screen_off_cmds));
		    is_sleep = TRUE;
#if !defined(FEATURE_AARM_RELEASE_MODE) //not user mode
		    printk("[PANTECH_LCD] sleep state (oled_hd panel).... \n");
#else
		    printk("[PANTECH_LCD]Sleep State\n");
#endif
			nELVSS = 1;
			break;
		}
	}
	else {
		switch (now_bl) {
		case 29:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_29,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_29));
			nELVSS = 4;
			break;
		case 28:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_28,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_28));
			nELVSS = 4;
			break;
		case 27:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_27,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_27));
			nELVSS = 4;
			break;
		case 26:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_26,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_26));
			nELVSS = 4;
			break;
		case 25:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_25,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_25));
			nELVSS = 4;
			break;
		case 24:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_24,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_24));
			nELVSS = 3;
			break;
		case 23:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_23,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_23));
			nELVSS = 3;
			break;
		case 22:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_22,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_22));
			nELVSS = 3;
			break;
		case 21:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_21,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_21));
			nELVSS = 3;
			break;
		case 20:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_20,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_20));
			nELVSS = 3;
			break;
		case 19:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_19,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_19));
			nELVSS = 3;
			break;
		case 18:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_18,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_18));
			nELVSS = 3;
			break;
		case 17:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_17,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_17));
			nELVSS = 2;
			break;
		case 16:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_16,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_16));
			nELVSS = 2;
			break;
		case 15:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_15,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_15));
			nELVSS = 2;
			break;
		case 14:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_14,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_14));
			nELVSS = 2;
			break;
		case 13:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_13,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_13));
			nELVSS = 2;
			break;
		case 12:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_12,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_12));
			nELVSS = 2;
			break;
		case 11:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_11,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_11));
			nELVSS = 2;
			break;
		case 10:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_10,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_10));
			nELVSS = 2;
			break;
		case 9:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_9,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_9));
			nELVSS = 2;
			break;
		case 8:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_8,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_8));
			nELVSS = 2;
			break;
		case 7:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_7,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_7));
			nELVSS = 1;
			break;
		case 6:
			default:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_6,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_6));
			nELVSS = 1;
			break;
		case 5:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_5,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_5));
			nELVSS = 1;
			break;
		case 4:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_4,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_4));
			nELVSS = 1;
			break;
		case 3:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_3,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_3));
			nELVSS = 1;
			break;
		case 2:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_2,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_2));
			nELVSS = 1;
			break;
		case 1:
		   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_1,
				ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_1));
			nELVSS = 1;
			break;
		case 0:
	            mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_screen_off_cmds,
	             	      ARRAY_SIZE(samsung_oled_hd_screen_off_cmds));
		    is_sleep = TRUE;
#if !defined(FEATURE_AARM_RELEASE_MODE) //not user mode
		    printk("[PANTECH_LCD] sleep state (oled_hd panel).... \n");
#else
		    printk("[PANTECH_LCD]Sleep State\n");
#endif
			nELVSS = 1;
			break;
		}	
	}

#ifdef D_SKY_DYN_ELVSS 
	if(pre_nELVSS != nELVSS)
	{
		if(elvss_id3)
		{
#ifdef D_SKY_DYN_ELVSS_DBG
			read_elvss_array(nELVSS); /* for test */
#endif
			switch(nELVSS){
			case 4:
	   			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss300_210_id3_1_cmds,
						ARRAY_SIZE(samsung_oled_hd_display_elvss300_210_id3_1_cmds));
			break;			
			case 3:
	   			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss200_170_id3_1_cmds,
						ARRAY_SIZE(samsung_oled_hd_display_elvss200_170_id3_1_cmds));
			break;			
			case 2:
	   			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss160_110_id3_1_cmds,
						ARRAY_SIZE(samsung_oled_hd_display_elvss160_110_id3_1_cmds));
			break;			
			case 1:
	   			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss100_000_id3_1_cmds,
						ARRAY_SIZE(samsung_oled_hd_display_elvss100_000_id3_1_cmds));
			break;			
			default:
			break;
			}
		}else /* elvss id3 is zero */
		{
			switch(nELVSS){
			case 4:
	   			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss300_210_id3_0_cmds,
						ARRAY_SIZE(samsung_oled_hd_display_elvss300_210_id3_0_cmds));
			break;			
			case 3:
	   			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss200_170_id3_0_cmds,
						ARRAY_SIZE(samsung_oled_hd_display_elvss200_170_id3_0_cmds));
			break;			
			case 2:
	   			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss160_110_id3_0_cmds,
						ARRAY_SIZE(samsung_oled_hd_display_elvss160_110_id3_0_cmds));
			break;			
			case 1:
	   			mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_elvss100_000_id3_0_cmds,
						ARRAY_SIZE(samsung_oled_hd_display_elvss100_000_id3_0_cmds));
			break;			
			default:
			break;
			}
			
		} 
		printk(" [PANTECH_LCD] Set ELVSS Value(in backlight) : %d\n",nELVSS);
	}

	pre_nELVSS = nELVSS;
#endif
	prev_bl = now_bl;

#ifdef D_SKY_OLED_TEMP	
	if(now_bl >= 11)
		is_high_bk = true;
	else
		is_high_bk = false;
#endif
	

   	mipi_set_tx_power_mode(1);
    	mutex_unlock(&bl_mutex);

#endif
	EXIT_FUNC2();
#else
	now_bl = mfd->bl_level; 
/*
	if(prev_bl == now_bl && samsung_oled_hd_state.disp_on) {
		printk("[PANTECH_LCD] prev_bl = now_bl return... \n");
		return;
	}
*/
		
#ifdef D_SKY_OLED_TEMP
	if(is_temp == 1)
	{
		if(now_bl >= 11)
		{
			ptemp_bl= now_bl;
			now_bl = 11;
			printk("[PANTECH_LCD] Panel temp was high...set to low..220cd/m\n");
		} 
	}
#endif
   	mipi_set_tx_power_mode(0);
   	mutex_lock(&bl_mutex);		

	printk(KERN_ERR "[PANTECH_LCD] OLED_BL Set : %d now_bl : %d (Mode:%d)\n", (mfd->bl_level), now_bl ,is_sleep);

	if(is_sleep && samsung_oled_hd_state.disp_on )
	{
          mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_wakeup_cmds,
                   ARRAY_SIZE(samsung_oled_hd_display_wakeup_cmds));
		   printk("[PANTECH_LCD:%s] wakeup state (oled_hd panel).... \n",__func__);
	}
	
		switch (now_bl) {			
	case 29:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_29,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_29));
		nELVSS = 4;
		break;
	case 28:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_28,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_28));
		nELVSS = 4;
		break;
	case 27:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_27,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_27));
		nELVSS = 4;
		break;
	case 26:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_26,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_26));
		nELVSS = 4;
		break;
	case 25:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_25,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_25));
		nELVSS = 4;
		break;
	case 24:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_24,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_24));
		nELVSS = 3;
		break;
	case 23:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_23,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_23));
		nELVSS = 3;
		break;
	case 22:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_22,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_22));
		nELVSS = 3;
		break;
	case 21:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_21,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_21));
		nELVSS = 3;
		break;
	case 20:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_20,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_20));
		nELVSS = 3;
		break;
	case 19:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_19,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_19));
		nELVSS = 3;
		break;
	case 18:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_18,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_18));
		nELVSS = 3;
		break;
	case 17:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_17,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_17));
		nELVSS = 2;
		break;
	case 16:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_16,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_16));
		nELVSS = 2;
		break;
	case 15:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_15,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_15));
		nELVSS = 2;
		break;
	case 14:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_14,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_14));
		nELVSS = 2;
		break;
	case 13:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_13,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_13));
		nELVSS = 2;
		break;
	case 12:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_12,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_12));
		nELVSS = 2;
		break;
	case 11:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_11,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_11));
		nELVSS = 2;
		break;
	case 10:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_10,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_10));
		nELVSS = 2;
		break;
	case 9:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_9,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_9));
		nELVSS = 2;
		break;
	case 8:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_8,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_8));
		nELVSS = 2;
		break;
	case 7:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_7,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_7));
		nELVSS = 1;
		break;
	case 6:
		default:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_6,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_6));
		nELVSS = 1;
		break;
	case 5:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_5,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_5));
		nELVSS = 1;
		break;
	case 4:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_4,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_4));
		nELVSS = 1;
		break;
	case 3:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_3,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_3));
		nELVSS = 1;
		break;
	case 2:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_2,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_2));
		nELVSS = 1;
		break;
	case 1:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_display_gamma_cmds_level_1,
			ARRAY_SIZE(samsung_oled_hd_display_gamma_cmds_level_1));
		nELVSS = 1;
//		printk("now_bl %d\n", now_bl);
		break;
	case 0:
            mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_screen_off_cmds,
             	      ARRAY_SIZE(samsung_oled_hd_screen_off_cmds));
	    is_sleep = TRUE;
#if !defined(FEATURE_AARM_RELEASE_MODE) //not user mode
	    printk("[PANTECH_LCD:%s] sleep state (oled_hd panel).... \n",__func__);
#else
	    printk("[PANTECH_LCD]Sleep State\n");
#endif
		nELVSS = 1;
		break;
	}

	mipi_set_tx_power_mode(1);
    mutex_unlock(&bl_mutex);	

	
#ifdef D_SKY_OLED_TEMP	
		if(now_bl >= 11) // 231 cd/m2 
			is_high_bk = true;
		else
			is_high_bk = false;
#endif
	
		if(pre_nELVSS != nELVSS)
		{
	//		change_elvss(nELVSS);
			printk("[PANTECH_LCD] pre_nELVSS != nELVSS.....\n");
			pre_nELVSS = nELVSS;
//			if(changed_id3)
//			{
//				changed_id3 = 0;
//				printk("[PANTECH_LCD] Ready to ID3 Value.....\n");
//				schedule_delayed_work(&panel_id3_detect_work, msecs_to_jiffies(0000));
//			}
			
		}

	prev_bl = now_bl;

	EXIT_FUNC2();
#endif
}

#ifdef CONFIG_F_SKYDISP_SMART_DIMMING
char read_panel[2]	= {0xD3, 0x00};
char write_gpara1[2]	= {0xB0, 0x0A};
char write_gpara2[2]	= {0xB0, 0x14};

static struct dsi_cmd_desc samsung_oled_hd_read_panel[] ={
	{DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(read_panel), read_panel},
};

static struct dsi_cmd_desc samsung_oled_hd_gpara11_cmd[] ={
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_gpara1), write_gpara1},
};
static struct dsi_cmd_desc samsung_oled_hd_gpara21_cmd[] ={
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(write_gpara2), write_gpara2},
};

char mtp_buffer[24];
char gamma_buffer[29][25];

void mipi_samsung_oled_hd_read(struct msm_fb_data_type *mfd, int index)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	int i;
	uint8 *lp;

	tp = &samsung_oled_hd_tx_buf;
	rp = &samsung_oled_hd_rx_buf;	
		
	lp = NULL;

	cmd = samsung_oled_hd_read_panel;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 10);
	
	for(i = index; i < index + 10; i++)
	{
		if(i > 23)
			break;
		mtp_buffer[i] = (*(char *)rp->data++);
		//printk("[PANTECH_LCD] mtp_buffer[%d] = %d\n", i, mtp_buffer[i]); 
	}
	msleep(1); 
}

void mtp_read(void)
{
	struct fb_info *info; 
	struct msm_fb_data_type *mfd;

    	info = registered_fb[0];
	mfd = (struct msm_fb_data_type *)info->par;

        //mipi_set_tx_power_mode(0);

	mipi_dsi_cmd_bta_sw_trigger(); 
	mipi_samsung_oled_hd_read(mfd, 0);
	
	mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_gpara11_cmd, 
						ARRAY_SIZE(samsung_oled_hd_gpara11_cmd));
	
	mipi_dsi_cmd_bta_sw_trigger(); 
	mipi_samsung_oled_hd_read(mfd, 10);
	
	mipi_dsi_cmds_tx(mfd, &samsung_oled_hd_tx_buf, samsung_oled_hd_gpara21_cmd, 
						ARRAY_SIZE(samsung_oled_hd_gpara21_cmd));
	
	mipi_dsi_cmd_bta_sw_trigger(); 
	mipi_samsung_oled_hd_read(mfd, 20);

        //mipi_set_tx_power_mode(1);

	mtp_buffer[6] = mtp_buffer[6] & 0x01;
	mtp_buffer[14] = mtp_buffer[14] & 0x01;
	mtp_buffer[22] = mtp_buffer[22] & 0x01;
}

void gamma_300nit_read(unsigned int p_rev)
{
	int i;
	
//	if(p_rev == 0xD3 ) {
	for(i = 0; i < 26; i++)
		gamma_set_level_30[i] = gamma_set_300_tp20[i];	
}

void mtp_write(void)
{
	//int ii;
	int jj;
	for(jj=1;jj<25;jj++){
		gamma_set_level_29[jj+1] = gamma_buffer[0][jj]; 
		gamma_set_level_28[jj+1] = gamma_buffer[1][jj];
		gamma_set_level_27[jj+1] = gamma_buffer[2][jj];
		gamma_set_level_26[jj+1] = gamma_buffer[3][jj];
		gamma_set_level_25[jj+1] = gamma_buffer[4][jj];
		gamma_set_level_24[jj+1] = gamma_buffer[5][jj];
		gamma_set_level_23[jj+1] = gamma_buffer[6][jj];
		gamma_set_level_22[jj+1] = gamma_buffer[7][jj];
		gamma_set_level_21[jj+1] = gamma_buffer[8][jj];
		gamma_set_level_20[jj+1] = gamma_buffer[9][jj];
		gamma_set_level_19[jj+1] = gamma_buffer[10][jj];
		gamma_set_level_18[jj+1] = gamma_buffer[11][jj];				
		gamma_set_level_17[jj+1] = gamma_buffer[12][jj];				
		gamma_set_level_16[jj+1] = gamma_buffer[13][jj];
		gamma_set_level_15[jj+1] = gamma_buffer[14][jj]; 
		gamma_set_level_14[jj+1] = gamma_buffer[15][jj];
		gamma_set_level_13[jj+1] = gamma_buffer[16][jj];
		gamma_set_level_12[jj+1] = gamma_buffer[17][jj];
		gamma_set_level_11[jj+1] = gamma_buffer[18][jj];
		gamma_set_level_10[jj+1] = gamma_buffer[19][jj];
		gamma_set_level_9[jj+1] = gamma_buffer[20][jj];
		gamma_set_level_8[jj+1] = gamma_buffer[21][jj];
		gamma_set_level_7[jj+1] = gamma_buffer[22][jj];
		gamma_set_level_6[jj+1] = gamma_buffer[23][jj];
		gamma_set_level_5[jj+1] = gamma_buffer[24][jj];
		gamma_set_level_4[jj+1] = gamma_buffer[25][jj];
		gamma_set_level_3[jj+1] = gamma_buffer[26][jj];				
		gamma_set_level_2[jj+1] = gamma_buffer[27][jj];				
		gamma_set_level_1[jj+1] = gamma_buffer[28][jj];				
	}
	
	/*for(ii = 0;ii < 29; ii++){
		printk(KERN_WARNING"\n");
		for(jj=1;jj<25;jj++) {
			printk(KERN_WARNING"[%s]gamma val[%d][%d] = 0x%x\n", __func__, ii, jj, gamma_buffer[ii][jj]);
		}
		printk(KERN_WARNING"\n\n");
	}*/

	printk(KERN_WARNING "[PANTECH_LCD] Smart Dimming 0n\n");
}
#endif

static struct msm_fb_panel_data samsung_oled_hd_panel_data = {
       .on             = mipi_samsung_oled_hd_on,
       .off            = mipi_samsung_oled_hd_off,
       .set_backlight  = mipi_samsung_oled_hd_set_backlight,
};

static int __devinit mipi_samsung_oled_hd_probe(struct platform_device *pdev)
{
#ifdef D_SKY_OLED_TEMP
	int noled_det;
#endif

    if (pdev->id == 0) {
        mipi_samsung_oled_hd_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

#ifdef D_SKY_OLED_TEMP
        noled_det = gpio_get_value(OLED_DET);

	if(!noled_det)
	{
		INIT_DELAYED_WORK_DEFERRABLE(&panel_therm_detect_work, is_oled_temp_check);
		schedule_delayed_work(&panel_therm_detect_work, msecs_to_jiffies(20000));
		printk("[PANTECH_LCD] Panel Therm Check Start..\n");
	}
#endif
#ifdef CONFIG_F_SKYDISP_SMART_DIMMING
	if(get_board_revision() != 3) 
		gamma_300nit_read(prev_id2);
#endif
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_samsung_oled_hd_probe,
	.driver = {
		.name   = "mipi_samsung_oled_hd",
	},
};


static int ch_used[3];

int mipi_samsung_oled_hd_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_samsung_oled_hd", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	samsung_oled_hd_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &samsung_oled_hd_panel_data,
		sizeof(samsung_oled_hd_panel_data));
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

unsigned char gamma_flag = false;
unsigned char power_flag = false;
unsigned char id3_flag = false;
unsigned char init_flag = false;


static int __init mipi_samsung_oled_hd_init(void)
{

    int i;

   // ENTER_FUNC2();
	
    samsung_oled_hd_state.disp_powered_up = true;

    mipi_dsi_buf_alloc(&samsung_oled_hd_tx_buf, DSI_BUF_SIZE);
    mipi_dsi_buf_alloc(&samsung_oled_hd_rx_buf, DSI_BUF_SIZE);

    if(get_board_revision() >= 6) /* tp20  456Mbps , OLED inital ver4 -> 468Mbps inital ver5*/
    {
#if 0
	if(!init_flag)
	{
		for(i=0;i<39;i++)
		{
			 panel_cond_set[i] = panel_cond_set_tp20[i];
			 if(i<26 && (!gamma_flag))
			 {
				gamma_set_level_29[i] = gamma_set_230_tp20[i];
				gamma_set_level_28[i] = gamma_set_224_tp20[i];
				gamma_set_level_27[i] = gamma_set_218_tp20[i];
				gamma_set_level_26[i] = gamma_set_212_tp20[i];
				gamma_set_level_25[i] = gamma_set_206_tp20[i];
				gamma_set_level_24[i] = gamma_set_200_tp20[i];
				gamma_set_level_23[i] = gamma_set_194_tp20[i];
				gamma_set_level_22[i] = gamma_set_188_tp20[i];
				gamma_set_level_21[i] = gamma_set_182_tp20[i];
				gamma_set_level_20[i] = gamma_set_176_tp20[i];
				gamma_set_level_19[i] = gamma_set_170_tp20[i];
				gamma_set_level_18[i] = gamma_set_164_tp20[i];
				gamma_set_level_17[i] = gamma_set_158_tp20[i];
				gamma_set_level_16[i] = gamma_set_152_tp20[i];
				gamma_set_level_15[i] = gamma_set_146_tp20[i];
				gamma_set_level_14[i] = gamma_set_140_tp20[i];
				gamma_set_level_13[i] = gamma_set_134_tp20[i];
				gamma_set_level_12[i] = gamma_set_128_tp20[i];
				gamma_set_level_11[i] = gamma_set_122_tp20[i];
				gamma_set_level_10[i] = gamma_set_116_tp20[i];
				gamma_set_level_9[i] = gamma_set_110_tp20[i];
				gamma_set_level_8[i] = gamma_set_105_tp20[i];
				gamma_set_level_7[i] = gamma_set_100_tp20[i];
				gamma_set_level_6[i] = gamma_set_95_tp20[i];
				gamma_set_level_5[i] = gamma_set_90_tp20[i];
				gamma_set_level_4[i] = gamma_set_85_tp20[i];
				gamma_set_level_3[i] = gamma_set_80_tp20[i];
				gamma_set_level_2[i] = gamma_set_75_tp20[i];
				gamma_set_level_1[i] = gamma_set_70_tp20[i];
			 }
			 
			if(i<8 && (!power_flag))
			{
				power_control[i] = power_control_tp20[i];
				power_flag = true;
			}
	
#ifdef D_SKY_DYN_ELVSS 
			if(i<3 && (!id3_flag))
			{
				elvss_300210_0[i] = elvss_300210_0_tp20[i];
				elvss_200170_0[i] = elvss_200170_0_tp20[i];
				elvss_160110_0[i] = elvss_160110_0_tp20[i];
				elvss_100000_0[i] = elvss_100000_0_tp20[i];
				id3_flag = true;
			}
#endif
		}
		gamma_flag = true;
		init_flag = true;
		printk("[PANTECH_LCD] TP20 OLED Setting Finished...\n");
	}
		printk("[PANTECH_LCD] TP20 OLED Setting Finished...\n");
#endif
		init_flag = true;
    }else if(get_board_revision() == 3) /* ws20 */
    {
	if(!init_flag)
	{
		for(i=0;i<26;i++)
		{
		  	if(!gamma_flag)
			 {
				gamma_set_level_15[i] = gamma_set_300_ws20[i];
				gamma_set_level_14[i] = gamma_set_280_ws20[i];
				gamma_set_level_13[i] = gamma_set_260_ws20[i];
				gamma_set_level_12[i] = gamma_set_240_ws20[i];
				gamma_set_level_11[i] = gamma_set_220_ws20[i];
				gamma_set_level_10[i] = gamma_set_200_ws20[i];
				gamma_set_level_9[i] = gamma_set_180_ws20[i];
				gamma_set_level_8[i] = gamma_set_160_ws20[i];
				gamma_set_level_7[i] = gamma_set_140_ws20[i];
				gamma_set_level_6[i] = gamma_set_120_ws20[i];
				gamma_set_level_5[i] = gamma_set_100_ws20[i];
				gamma_set_level_4[i] = gamma_set_80_ws20[i];
				gamma_set_level_3[i] = gamma_set_60_ws20[i];
				gamma_set_level_2[i] = gamma_set_40_ws20[i];
				gamma_set_level_1[i] = gamma_set_20_ws20[i];

			}
		}	
		gamma_flag = true;

		init_flag = true;
		printk("[PANTECH_LCD] WS20 OLED Setting Finished...\n");
	}
    }else
		printk("[PANTECH_LCD] TP10 OLED Setting Finished...\n");
		
    mutex_init(&bl_mutex);
	
    EXIT_FUNC2();

    return platform_driver_register(&this_driver);
}
module_init(mipi_samsung_oled_hd_init);

