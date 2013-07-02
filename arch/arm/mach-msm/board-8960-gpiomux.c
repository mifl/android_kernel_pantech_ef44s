/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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
 */

#include <linux/gpio.h>
#include <asm/mach-types.h>
#include <mach/gpio.h>	
#include <mach/gpiomux.h>
#include <mach/socinfo.h>
#include "devices.h"
#include "board-8960.h"

/* The SPI configurations apply to GSBI 1*/
static struct gpiomux_setting spi_active = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting spi_suspended_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting spi_active_config2 = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting spi_suspended_config2 = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

#if defined(CONFIG_PANTECH_CAMERA_FLASH) || (defined(CONFIG_MACH_MSM8960_VEGAPVW) && defined(CONFIG_PANTECH_PMIC_MAX17058)) || (defined(CONFIG_MACH_MSM8960_SIRIUSLTE) && defined(CONFIG_PANTECH_PMIC_MAX17058)) 
static struct gpiomux_setting gsbi1_active_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA, //GPIOMUX_DRV_12MA ?
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi1_suspended_config = {
	.func = GPIOMUX_FUNC_1,//GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
#if defined(CONFIG_MACH_MSM8960_VEGAPVW)	
	.pull = GPIOMUX_PULL_UP,//GPIOMUX_PULL_DOWN,//GPIOMUX_PULL_KEEPER,//GPIOMUX_PULL_DOWN,
#else
    .pull = GPIOMUX_PULL_DOWN,//GPIOMUX_PULL_KEEPER,//GPIOMUX_PULL_DOWN,
#endif	
};
#endif

#ifdef CONFIG_PANTECH_GPIO_SLEEP_CONFIG

#if defined(CONFIG_MACH_MSM8960_VEGAPVW) || defined(CONFIG_MACH_MSM8960_EF44S) || defined(CONFIG_MACH_MSM8960_SIRIUSLTE)
static struct gpiomux_setting msm8960_gpio_suspend_in_pd_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};
#if 0
static struct gpiomux_setting msm8960_gpio_suspend_in_pu_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};
static struct gpiomux_setting msm8960_gpio_suspend_out_np_high = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};
#endif
#endif

//	##START #p14527 add NC Pin Setting start
#if defined(CONFIG_MACH_MSM8960_VEGAPVW) 
static struct msm_gpiomux_config msm8960_sleep_gpio_gpio_configs[] __initdata = {

#if (BOARD_VER<WS11)	
	// for WS10 Board
	{
		.gpio = 15,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 63,	 
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 64,	 
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
#endif
#if (BOARD_VER<TP10)	
	// for WS20 and WS11 and WS10 Boards
	{
		.gpio = 35,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 40,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 41,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 115,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
#endif

#if (BOARD_VER>WS10)	
	// for WS11 and WS20 and TP10 Boards
	{
		.gpio = 14,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 27,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 92,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
#endif 
#if (BOARD_VER>WS20)	
	// for TP10 Board
	{
		.gpio = 129,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 151,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
#endif

	// for All Boards
	{
		.gpio = 1,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 6,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 7,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 18,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 19,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 24,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 25,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 26,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 32,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 33,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 34,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 36,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 37,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 38,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 39,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 42,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 43,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 50,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 53,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 55,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 68,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 71,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 72,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 81,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 93,
		.settings = {	
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 94,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 97,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 98,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio =113,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
};

#elif defined(CONFIG_MACH_MSM8960_EF44S) 
static struct msm_gpiomux_config msm8960_sleep_gpio_gpio_configs[] __initdata = {

#if (BOARD_VER<WS20)	
	// for WS15 Board
	{
		.gpio = 40,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 95,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 96,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
#endif

#if (BOARD_VER>WS20)
	// for TP10 Board
	{
		.gpio = 4,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 40,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
#endif

	// for All Boards
	{
		.gpio = 3,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 7,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 8,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 9,	 
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 12,	 
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 13,	 
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 14,	 
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 15,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 18,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 19,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 24,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 25,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 32,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 33,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 41,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 42,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 43,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 47,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 53,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 58,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 71,	 
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 89,	 
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 90,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 91,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 92,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 93,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 94,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 99,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 100,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 101,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 102,
		.settings = {	
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 109,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 110,
		.settings = {	
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 111,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 112,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 113,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 114,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 115,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 128,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 132,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 135,	
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 138,
		.settings = {	
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 139,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 140,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 141,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 144,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 145,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 149,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 151,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},

};
#elif defined(CONFIG_MACH_MSM8960_SIRIUSLTE) /*added by akira... set Pull down GPIO not to be used in SIRIUSLTE */
static struct msm_gpiomux_config msm8960_sleep_gpio_gpio_configs[] __initdata = {
	{
		.gpio = 4,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
#if (BOARD_VER>=WS10)
	{
		.gpio = 18,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 19,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
#endif
	{
		.gpio = 32,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
#if (BOARD_VER<=PT20)
	{
		.gpio = 34,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 35,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
#endif
	{
		.gpio = 40,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 41,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 47,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 53,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 55,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 68,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 92,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 97,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 98,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 111,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 120,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 121,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 124,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 129,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 138,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 144,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
	{
		.gpio = 145,
		.settings = {
			[GPIOMUX_ACTIVE]    = &msm8960_gpio_suspend_in_pd_cfg,
			[GPIOMUX_SUSPENDED] = &msm8960_gpio_suspend_in_pd_cfg,
		},
	},
};
#endif 

#endif /* CONFIG_PANTECH_GPIO_SLEEP_CONFIG */
//	##END #p14527 add NC Pin Setting 
#if defined(CONFIG_PANTECH_PMIC_MAX17058)
#if defined(T_STARQ)
static struct gpiomux_setting gsbi1_active_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct gpiomux_setting gsbi1_suspended_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif
#endif // #if defined(CONFIG_PANTECH_PMIC_MAX17058)
static struct gpiomux_setting gsbi3_suspended_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting gsbi3_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting external_vfr[] = {
	/* Suspended state */
	{
		.func = GPIOMUX_FUNC_3,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_KEEPER,
	},
	/* Active state */
	{
		.func = GPIOMUX_FUNC_3,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_KEEPER,
	},
};

static struct gpiomux_setting gsbi_uart = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

#if defined(CONFIG_PANTECH_PMIC_MAX17058)
#if defined(CONFIG_MACH_MSM8960_EF44S) || defined(T_MAGNUS)
static struct gpiomux_setting gsbi9_active_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct gpiomux_setting gsbi9_suspended_cfg = {
	.func = GPIOMUX_FUNC_2, /*i2c suspend*/
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif
#endif // #if defined(CONFIG_PANTECH_PMIC_MAX17058)
static struct gpiomux_setting gsbi10 = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi12 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting cdc_mclk = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

#if defined(CONFIG_MACH_MSM8960_EF44S)
static struct gpiomux_setting audio_auxpcm[] = {
	/* Suspended state */
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	/* Active state */
	{
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
};

#else

#ifndef CONFIG_PN544
static struct gpiomux_setting audio_auxpcm[] = {
	/* Suspended state */
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	/* Active state */
	{
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
};
#endif
#endif


#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
static struct gpiomux_setting gpio_eth_config = {
	.pull = GPIOMUX_PULL_NONE,
	.drv = GPIOMUX_DRV_8MA,
	.func = GPIOMUX_FUNC_GPIO,
};
#endif

static struct gpiomux_setting slimbus = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

//LS3_LeeYoungHo_120207_chg [ check point -> suspend cfg pull up is right?
#ifndef CONFIG_WIFI_CONTROL_FUNC
static struct gpiomux_setting wcnss_5wire_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting wcnss_5wire_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#else//LS3_LeeYoungHo_120207_chg
static struct gpiomux_setting bcm4334_suspend_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP, //GPIOMUX_PULL_DOWN,
};
static struct gpiomux_setting bcm4334_active_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv  = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};
#endif//LS3_LeeYoungHo_120207_chg_end

static struct gpiomux_setting cyts_resout_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
#ifdef CONFIG_PANTECH_CAMERA
	.pull = GPIOMUX_PULL_DOWN,
#else
	.pull = GPIOMUX_PULL_UP,
#endif
};

static struct gpiomux_setting cyts_resout_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting cyts_sleep_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting cyts_sleep_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting cyts_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting cyts_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

#ifdef CONFIG_USB_EHCI_MSM_HSIC
static struct gpiomux_setting hsic_act_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hsic_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting hsic_hub_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif

static struct gpiomux_setting hap_lvl_shft_suspended_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hap_lvl_shft_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting ap2mdm_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdm2ap_status_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mdm2ap_errfatal_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting ap2mdm_kpdpwr_n_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdp_vsync_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdp_vsync_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
#if !defined (CONFIG_MACH_MSM8960_EF44S)		/// EF44S does not have HDMI.
static struct gpiomux_setting hdmi_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hdmi_active_1_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting hdmi_active_2_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

#if defined(CONFIG_FB_MSM_HDMI_MHL_8334) || defined(CONFIG_FB_MSM_HDMI_MHL_9244)
static struct gpiomux_setting hdmi_active_3_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting hdmi_active_4_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_OUT_HIGH,
};
#endif
#endif	/// EF44S does not have HDMI.
#endif

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
static struct msm_gpiomux_config msm8960_ethernet_configs[] = {
	{
		.gpio = 90,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_eth_config,
		}
	},
	{
		.gpio = 89,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_eth_config,
		}
	},
};
#endif

static struct msm_gpiomux_config msm8960_fusion_gsbi_configs[] = {
	{
		.gpio = 93,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi9_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi9_active_cfg,
		}
	},
	{
		.gpio = 94,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi9_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi9_active_cfg,
		}
	},
	{
		.gpio = 95,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi9_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi9_active_cfg,
		}
	},
	{
		.gpio = 96,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi9_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi9_active_cfg,
		}
	},
};

static struct msm_gpiomux_config msm8960_gsbi_configs[] __initdata = {
	{
		.gpio      = 6,		/* GSBI1 QUP SPI_DATA_MOSI */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spi_suspended_config,
			[GPIOMUX_ACTIVE] = &spi_active,
		},
	},
	{
		.gpio      = 7,		/* GSBI1 QUP SPI_DATA_MISO */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spi_suspended_config,
			[GPIOMUX_ACTIVE] = &spi_active,
		},
	},
#if defined(CONFIG_PANTECH_CAMERA_FLASH) || (defined(CONFIG_MACH_MSM8960_VEGAPVW) && defined(CONFIG_PANTECH_PMIC_MAX17058)) || (defined(CONFIG_MACH_MSM8960_SIRIUSLTE)&& defined(CONFIG_PANTECH_PMIC_MAX17058))
	{
		.gpio      = 8,	/* GSBI1 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi1_suspended_config,
			[GPIOMUX_ACTIVE] = &gsbi1_active_config,                
		},
	},
	{
		.gpio      = 9,	/* GSBI1 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi1_suspended_config,
			[GPIOMUX_ACTIVE] = &gsbi1_active_config,                
		},
	},
#else	
#if defined(CONFIG_PANTECH_PMIC_MAX17058)
	{
		.gpio      = 8,		/* GSBI1 QUP SPI_CS_N */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spi_suspended_config,
			[GPIOMUX_ACTIVE] = &spi_active,
		},
	},
	{
		.gpio      = 9,		/* GSBI1 QUP SPI_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spi_suspended_config,
			[GPIOMUX_ACTIVE] = &spi_active,
		},
	},
#else
	{
		.gpio      = 8,		/* GSBI1 QUP SPI_CS_N */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spi_suspended_config,
			[GPIOMUX_ACTIVE] = &spi_active,
		},
	},
	{
		.gpio      = 9,		/* GSBI1 QUP SPI_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spi_suspended_config,
			[GPIOMUX_ACTIVE] = &spi_active,
		},
	},
#endif
#endif
	{
		.gpio      = 14,		/* GSBI1 SPI_CS_1 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &spi_suspended_config2,
			[GPIOMUX_ACTIVE] = &spi_active_config2,
		},
	},
	{
		.gpio      = 16,	/* GSBI3 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi3_active_cfg,
		},
	},
	{
		.gpio      = 17,	/* GSBI3 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi3_active_cfg,
		},
	},
#if defined(CONFIG_PANTECH_PMIC_MAX17058)
#if defined(CONFIG_MACH_MSM8960_EF44S) || defined(T_MAGNUS)
    {
        .gpio      = 95,    /* GSBI9 I2C QUP SDA */
        .settings = {
            [GPIOMUX_SUSPENDED] = &gsbi9_suspended_cfg,
            [GPIOMUX_ACTIVE] = &gsbi9_active_cfg,
        },
    },
    {
        .gpio      = 96,    /* GSBI9 I2C QUP SCL */
        .settings = {
            [GPIOMUX_SUSPENDED] = &gsbi9_suspended_cfg,
            [GPIOMUX_ACTIVE] = &gsbi9_active_cfg,
        },
    },
#endif
#endif  // #if defined(CONFIG_PANTECH_PMIC_MAX17058)
	{
		.gpio      = 44,	/* GSBI12 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi12,
		},
	},
	{
		.gpio      = 45,	/* GSBI12 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi12,
		},
	},
	{
		.gpio      = 73,	/* GSBI10 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi10,
		},
	},
	{
		.gpio      = 74,	/* GSBI10 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi10,
		},
	},
};

static struct msm_gpiomux_config msm8960_gsbi5_uart_configs[] __initdata = {
	{
		.gpio      = 22,        /* GSBI5 UART2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_uart,
		},
	},
	{
		.gpio      = 23,        /* GSBI5 UART2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_uart,
		},
	},
	{
		.gpio      = 24,        /* GSBI5 UART2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_uart,
		},
	},
	{
		.gpio      = 25,        /* GSBI5 UART2 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_uart,
		},
	},
};

static struct msm_gpiomux_config msm8960_external_vfr_configs[] __initdata = {
	{
		.gpio      = 23,        /* EXTERNAL VFR */
		.settings = {
			[GPIOMUX_SUSPENDED] = &external_vfr[0],
			[GPIOMUX_ACTIVE] = &external_vfr[1],
		},
	},
};

static struct msm_gpiomux_config msm8960_gsbi8_uart_configs[] __initdata = {
	{
		.gpio      = 34,        /* GSBI8 UART3 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_uart,
		},
	},
	{
		.gpio      = 35,        /* GSBI8 UART3 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_uart,
		},
	},
	{
		.gpio      = 36,        /* GSBI8 UART3 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_uart,
		},
	},
	{
		.gpio      = 37,        /* GSBI8 UART3 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_uart,
		},
	},
};

static struct msm_gpiomux_config msm8960_slimbus_config[] __initdata = {
	{
		.gpio	= 60,		/* slimbus data */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
	{
		.gpio	= 61,		/* slimbus clk */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
};

static struct msm_gpiomux_config msm8960_audio_codec_configs[] __initdata = {
	{
		.gpio = 59,
		.settings = {
			[GPIOMUX_SUSPENDED] = &cdc_mclk,
		},
	},
};

#if defined(CONFIG_MACH_MSM8960_EF44S)
static struct msm_gpiomux_config msm8960_audio_auxpcm_configs[] __initdata = {
	{
		.gpio = 63,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
	{
		.gpio = 64,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
	{
		.gpio = 66,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
};

#else
#ifndef CONFIG_PN544
// p11515 - Gpio_66 is used for NFC( PN544 ) Enable pin.

static struct msm_gpiomux_config msm8960_audio_auxpcm_configs[] __initdata = {
	{
		.gpio = 63,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
	{
		.gpio = 64,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
	{
		.gpio = 66,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
};
#endif
#endif
#ifndef CONFIG_WIFI_CONTROL_FUNC
static struct msm_gpiomux_config wcnss_5wire_interface[] = {
	{
		.gpio = 84,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 85,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 87,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 88,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
};
#else//LS3_LeeYoungHo_120207_chg
static struct msm_gpiomux_config bcm4334_interface[] = {
	{
		.gpio = 83,
		.settings = {
			[GPIOMUX_ACTIVE]    = &bcm4334_active_cfg,
			[GPIOMUX_SUSPENDED] = &bcm4334_suspend_cfg,
		},
	},
	{
		.gpio = 84,
		.settings = {
			[GPIOMUX_ACTIVE]    = &bcm4334_active_cfg,
			[GPIOMUX_SUSPENDED] = &bcm4334_suspend_cfg,
		},
	},
	{
		.gpio = 85,
		.settings = {
			[GPIOMUX_ACTIVE]    = &bcm4334_active_cfg,
			[GPIOMUX_SUSPENDED] = &bcm4334_suspend_cfg,
		},
	},
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_ACTIVE]    = &bcm4334_active_cfg,
			[GPIOMUX_SUSPENDED] = &bcm4334_suspend_cfg,
		},
	},
	{
		.gpio = 87,
		.settings = {
			[GPIOMUX_ACTIVE]    = &bcm4334_active_cfg,
			[GPIOMUX_SUSPENDED] = &bcm4334_suspend_cfg,
		},
	},
	{
		.gpio = 88,
		.settings = {
			[GPIOMUX_ACTIVE]    = &bcm4334_active_cfg,
			[GPIOMUX_SUSPENDED] = &bcm4334_suspend_cfg,
		},
	},
};
#endif//LS3_LeeYoungHo_120207_chg_end

static struct msm_gpiomux_config msm8960_cyts_configs[] __initdata = {
	{	/* TS INTERRUPT */
		.gpio = 11,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cyts_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &cyts_int_sus_cfg,
		},
	},
	{	/* TS SLEEP */
		.gpio = 50,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cyts_sleep_act_cfg,
			[GPIOMUX_SUSPENDED] = &cyts_sleep_sus_cfg,
		},
	},
	{	/* TS RESOUT */
		.gpio = 52,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cyts_resout_act_cfg,
			[GPIOMUX_SUSPENDED] = &cyts_resout_sus_cfg,
		},
	},
};

#ifdef CONFIG_USB_EHCI_MSM_HSIC
static struct msm_gpiomux_config msm8960_hsic_configs[] = {
	{
		.gpio = 150,               /*HSIC_STROBE */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
	{
		.gpio = 151,               /* HSIC_DATA */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
};

static struct msm_gpiomux_config msm8960_hsic_hub_configs[] = {
	{
		.gpio = 91,               /* HSIC_HUB_RESET */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_hub_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
};
#endif

#if !defined(CONFIG_WIFI_CONTROL_FUNC)
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct gpiomux_setting sdcc4_clk_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdcc4_cmd_data_0_3_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdcc4_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting sdcc4_data_1_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config msm8960_sdcc4_configs[] __initdata = {
	{
		/* SDC4_DATA_3 */
		.gpio      = 83,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_suspend_cfg,
		},
	},
	{
		/* SDC4_DATA_2 */
		.gpio      = 84,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_suspend_cfg,
		},
	},
	{
		/* SDC4_DATA_1 */
		.gpio      = 85,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_data_1_suspend_cfg,
		},
	},
	{
		/* SDC4_DATA_0 */
		.gpio      = 86,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_suspend_cfg,
		},
	},
	{
		/* SDC4_CMD */
		.gpio      = 87,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_suspend_cfg,
		},
	},
	{
		/* SDC4_CLK */
		.gpio      = 88,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_clk_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_suspend_cfg,
		},
	},
};
#endif
#endif // CONFIG_WIFI_CONTROL_FUNC

static struct msm_gpiomux_config hap_lvl_shft_config[] __initdata = {
	{
		.gpio = 47,
		.settings = {
			[GPIOMUX_SUSPENDED] = &hap_lvl_shft_suspended_config,
			[GPIOMUX_ACTIVE] = &hap_lvl_shft_active_config,
		},
	},
};

static struct msm_gpiomux_config sglte_configs[] __initdata = {
	/* AP2MDM_STATUS */
	{
		.gpio = 77,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* MDM2AP_STATUS */
	{
		.gpio = 24,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_status_cfg,
		}
	},
	/* MDM2AP_ERRFATAL */
	{
		.gpio = 40,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_errfatal_cfg,
		}
	},
	/* AP2MDM_ERRFATAL */
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* AP2MDM_KPDPWR_N */
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_kpdpwr_n_cfg,
		}
	},
	/* AP2MDM_PMIC_PWR_EN */
	{
		.gpio = 22,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_kpdpwr_n_cfg,
		}
	},
	/* AP2MDM_SOFT_RESET */
	{
		.gpio = 78,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
};

static struct msm_gpiomux_config msm8960_mdp_vsync_configs[] __initdata = {
	{
		.gpio = 0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &mdp_vsync_active_cfg,
			[GPIOMUX_SUSPENDED] = &mdp_vsync_suspend_cfg,
		},
	}
};

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
#if !defined (CONFIG_MACH_MSM8960_EF44S)		/// EF44S does not have HDMI.
static struct msm_gpiomux_config msm8960_hdmi_configs[] __initdata = {
	{
		.gpio = 99,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 100,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 101,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 102,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_2_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
#ifdef CONFIG_FB_MSM_HDMI_MHL_9244
		{
		.gpio = 15,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_3_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 66,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_4_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
#endif
#ifdef CONFIG_FB_MSM_HDMI_MHL_8334
		{
		.gpio = 4,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_3_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 15,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_4_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
#endif /* CONFIG_FB_MSM_HDMI_MHL */
};
#endif	/// EF44S does not have HDMI.
#endif

#ifdef CONFIG_PANTECH_CHARGER_WIRELESS
#define W_CHG_FULL 0
#define USB_CHG_DET 1
static struct gpiomux_setting wireless_fulldet_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};
static struct gpiomux_setting wireless_fulldet_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};
static struct gpiomux_setting wireless_usbdet_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};
static struct gpiomux_setting wireless_usbdet_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};
static struct msm_gpiomux_config msm8960_wireless_charger_configs[] __initdata = {
	{
		.gpio = W_CHG_FULL,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wireless_fulldet_active_cfg,
			[GPIOMUX_SUSPENDED] = &wireless_fulldet_suspend_cfg,
		},
	},
	{
		.gpio = USB_CHG_DET,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wireless_usbdet_active_cfg,
			[GPIOMUX_SUSPENDED] = &wireless_usbdet_suspend_cfg,
		},
	},
};
#endif
#ifdef CONFIG_PANTECH_PMIC  // gpio-config for QC recommendation (pull-up) 
static struct gpiomux_setting pm_irq_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};
static struct gpiomux_setting pm_irq_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};
static struct msm_gpiomux_config msm8960_pm_irq_configs[] __initdata = {
	{
		.gpio = 103,
		.settings = {
			[GPIOMUX_ACTIVE]    = &pm_irq_active_cfg,
			[GPIOMUX_SUSPENDED] = &pm_irq_suspend_cfg,
		},
	},
	{
		.gpio = 104,
		.settings = {
			[GPIOMUX_ACTIVE]    = &pm_irq_active_cfg,
			[GPIOMUX_SUSPENDED] = &pm_irq_suspend_cfg,
		},
	},
};
#endif
#ifdef CONFIG_PANTECH_SND //kdkim
#if defined(CONFIG_MACH_MSM8960_EF44S) || defined(T_VEGAPVW)
static struct gpiomux_setting heaset_detect_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting heaset_detect_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config headset_detect_irq_configs[] __initdata = {
	{
		.gpio = 35,
		.settings = {
			[GPIOMUX_ACTIVE]    = &heaset_detect_active_cfg,
			[GPIOMUX_SUSPENDED] = &heaset_detect_suspend_cfg,
		},
	},	
};
#endif
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct gpiomux_setting sdcc2_clk_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdcc2_cmd_data_0_3_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdcc2_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting sdcc2_data_1_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config msm8960_sdcc2_configs[] __initdata = {
	{
		/* DATA_3 */
		.gpio      = 92,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_cfg,
		},
	},
	{
		/* DATA_2 */
		.gpio      = 91,
		.settings = {
		[GPIOMUX_ACTIVE]    = &sdcc2_cmd_data_0_3_actv_cfg,
		[GPIOMUX_SUSPENDED] = &sdcc2_suspend_cfg,
		},
	},
	{
		/* DATA_1 */
		.gpio      = 90,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_data_1_suspend_cfg,
		},
	},
	{
		/* DATA_0 */
		.gpio      = 89,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_cfg,
		},
	},
	{
		/* CMD */
		.gpio      = 97,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_cfg,
		},
	},
	{
		/* CLK */
		.gpio      = 98,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_clk_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_cfg,
		},
	},
};
#endif

#ifdef CONFIG_BRCM_BT
static struct gpiomux_setting rfkill_active_brcm_bt = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct gpiomux_setting rfkill_suspend_brcm_bt = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct msm_gpiomux_config bt_rfkill_interface[] = {
	{
		.gpio = 120,
		.settings = {
			[GPIOMUX_ACTIVE]    = &rfkill_active_brcm_bt,
			[GPIOMUX_SUSPENDED] = &rfkill_suspend_brcm_bt,
		},
	},
};

static struct gpiomux_setting gsbi6 = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct gpiomux_setting gsbi6_suspend = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};
static struct msm_gpiomux_config gsbi6_uart_configs[] __initdata = {
	{
		.gpio	   = 26,	/* GSBI6 UART */
		.settings = {
			[GPIOMUX_ACTIVE]	= &gsbi6,
			[GPIOMUX_SUSPENDED] = &gsbi6_suspend,
		},
	},
	{
		.gpio	   = 27,	/* GSBI6 UART */
		.settings = {
			[GPIOMUX_ACTIVE]	= &gsbi6,
			[GPIOMUX_SUSPENDED] = &gsbi6_suspend,
		},
	},
	{
		.gpio	   = 28,	/* GSBI6 UART */
		.settings = {
			[GPIOMUX_ACTIVE]	= &gsbi6,
			[GPIOMUX_SUSPENDED] = &gsbi6_suspend,
		},
	},
	{
		.gpio	   = 29,	/* GSBI6 UART */
		.settings = {
			[GPIOMUX_ACTIVE]	= &gsbi6,
			[GPIOMUX_SUSPENDED] = &gsbi6_suspend,
		},
	}
};

#endif


int __init msm8960_init_gpiomux(void)
{
	int rc = msm_gpiomux_init(NR_GPIO_IRQS);
	if (rc) {
		pr_err(KERN_ERR "msm_gpiomux_init failed %d\n", rc);
		return rc;
	}

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	msm_gpiomux_install(msm8960_ethernet_configs,
			ARRAY_SIZE(msm8960_ethernet_configs));
#endif

	msm_gpiomux_install(msm8960_gsbi_configs,
			ARRAY_SIZE(msm8960_gsbi_configs));

	msm_gpiomux_install(msm8960_cyts_configs,
			ARRAY_SIZE(msm8960_cyts_configs));

	msm_gpiomux_install(msm8960_slimbus_config,
			ARRAY_SIZE(msm8960_slimbus_config));

	msm_gpiomux_install(msm8960_audio_codec_configs,
			ARRAY_SIZE(msm8960_audio_codec_configs));

#if defined(CONFIG_MACH_MSM8960_EF44S)
	  msm_gpiomux_install(msm8960_audio_auxpcm_configs,
		ARRAY_SIZE(msm8960_audio_auxpcm_configs));
#else
#ifndef CONFIG_PN544
		msm_gpiomux_install(msm8960_audio_auxpcm_configs,
				ARRAY_SIZE(msm8960_audio_auxpcm_configs));
#endif
#endif
//LS3_LeeYoungHo_120207_chg [ BCM SDIO gpio config
#ifndef CONFIG_WIFI_CONTROL_FUNC
	msm_gpiomux_install(wcnss_5wire_interface,
			ARRAY_SIZE(wcnss_5wire_interface));
#else//LS3_LeeYoungHo_120207_chg
	msm_gpiomux_install(bcm4334_interface,
			ARRAY_SIZE(bcm4334_interface));
#endif//LS3_LeeYoungHo_120207_chg_end

#if !defined(CONFIG_WIFI_CONTROL_FUNC)
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	msm_gpiomux_install(msm8960_sdcc4_configs,
		ARRAY_SIZE(msm8960_sdcc4_configs));
#endif
#endif // !CONFIG_WIFI_CONTROL_FUNC

	if (machine_is_msm8960_mtp() || machine_is_msm8960_fluid() ||
		machine_is_msm8960_liquid() || machine_is_msm8960_cdp())
		msm_gpiomux_install(hap_lvl_shft_config,
			ARRAY_SIZE(hap_lvl_shft_config));

#ifdef CONFIG_USB_EHCI_MSM_HSIC
	if ((SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 1) &&
		machine_is_msm8960_liquid())
		msm_gpiomux_install(msm8960_hsic_configs,
			ARRAY_SIZE(msm8960_hsic_configs));

	if ((SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 1) &&
			machine_is_msm8960_liquid())
		msm_gpiomux_install(msm8960_hsic_hub_configs,
			ARRAY_SIZE(msm8960_hsic_hub_configs));
#endif

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
#if !defined (CONFIG_MACH_MSM8960_EF44S)		/// EF44S does not have HDMI.
	msm_gpiomux_install(msm8960_hdmi_configs,
			ARRAY_SIZE(msm8960_hdmi_configs));
#endif
#endif

	msm_gpiomux_install(msm8960_mdp_vsync_configs,
			ARRAY_SIZE(msm8960_mdp_vsync_configs));

#ifdef CONFIG_PANTECH_CHARGER_WIRELESS
	msm_gpiomux_install(msm8960_wireless_charger_configs,
			ARRAY_SIZE(msm8960_wireless_charger_configs));
#endif

//p14527 add
#ifdef CONFIG_PANTECH_GPIO_SLEEP_CONFIG
#if defined(CONFIG_MACH_MSM8960_VEGAPVW) || defined(CONFIG_MACH_MSM8960_EF44S) || defined(CONFIG_MACH_MSM8960_SIRIUSLTE)
	msm_gpiomux_install(msm8960_sleep_gpio_gpio_configs,
			ARRAY_SIZE(msm8960_sleep_gpio_gpio_configs));
#endif

#endif
#ifdef CONFIG_PANTECH_PMIC // gpio-config for QC recommendation (pull-up) 
	msm_gpiomux_install(msm8960_pm_irq_configs,
			ARRAY_SIZE(msm8960_pm_irq_configs));
#endif
#ifdef CONFIG_PANTECH_SND //kdkim
#if defined(CONFIG_MACH_MSM8960_EF44S) || defined(T_VEGAPVW)
	msm_gpiomux_install(headset_detect_irq_configs,
			ARRAY_SIZE(headset_detect_irq_configs));
#endif
#endif
	if (socinfo_get_platform_subtype() == PLATFORM_SUBTYPE_SGLTE)
		msm_gpiomux_install(msm8960_gsbi8_uart_configs,
			ARRAY_SIZE(msm8960_gsbi8_uart_configs));
	else
		msm_gpiomux_install(msm8960_gsbi5_uart_configs,
			ARRAY_SIZE(msm8960_gsbi5_uart_configs));

	if (socinfo_get_platform_subtype() == PLATFORM_SUBTYPE_SGLTE) {
		/* For 8960 Fusion 2.2 Primary IPC */
		msm_gpiomux_install(msm8960_fusion_gsbi_configs,
			ARRAY_SIZE(msm8960_fusion_gsbi_configs));
		/* For SGLTE 8960 Fusion External VFR */
		msm_gpiomux_install(msm8960_external_vfr_configs,
			ARRAY_SIZE(msm8960_external_vfr_configs));
	}

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	msm_gpiomux_install(msm8960_sdcc2_configs,
		ARRAY_SIZE(msm8960_sdcc2_configs));
#endif

#ifdef CONFIG_BRCM_BT
	printk(KERN_INFO"krstnd_test : %s \n",__func__);
	msm_gpiomux_install(bt_rfkill_interface,
			ARRAY_SIZE(bt_rfkill_interface));

	msm_gpiomux_install(gsbi6_uart_configs,
		 	ARRAY_SIZE(gsbi6_uart_configs));

#endif


	if (socinfo_get_platform_subtype() == PLATFORM_SUBTYPE_SGLTE)
		msm_gpiomux_install(sglte_configs,
			ARRAY_SIZE(sglte_configs));

	return 0;
}
