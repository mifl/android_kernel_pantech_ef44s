/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_samsung_oled_premia.h"
extern int get_hw_revision(void);

static struct msm_panel_info pinfo;

#if defined(MIPI_CLOCK_440MBPS)
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
/* 800*1280, RGB888, 4 Lane 60 fps 450Mbps video mode */
  /* regulator */
  {0x03, 0x0a, 0x04, 0x00, 0x20},
  /* timing */
  {0xab, 0x8a, 0x18, 0x00, 0x92, 0x97, 0x1b, 0x8c,
   0x0c, 0x03, 0x04, 0xa0},
  /* phy ctrl */
  {0x5f, 0x00, 0x00, 0x10},
  /* strength */
  {0xff, 0x00, 0x06, 0x00},
  /* pll control */
  {0x0, 0xc7, 0x31, 0xda, 0x00, 0x50, 0x48, 0x63,
   0x31, 0x0f, 0x03,
   0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};
#elif defined(MIPI_CLOCK_441MBPS) //440
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
/* 800*1280, RGB888, 4 Lane 60 fps 450Mbps video mode */
  /* regulator */
  {0x03, 0x0a, 0x04, 0x00, 0x20},
  /* timing */
  {0xab, 0x8a, 0x18, 0x00, 0x92, 0x97, 0x1b, 0x8c,
   0x0c, 0x03, 0x04, 0xa0},
  /* phy ctrl */
  {0x5f, 0x00, 0x00, 0x10},
  /* strength */
  {0xff, 0x00, 0x06, 0x00},
  /* pll control */
  {0x0, 0xb7, 0x31, 0xda, 0x00, 0x50, 0x48, 0x63,
   0x31, 0x0f, 0x03,
   0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};
#elif defined(MIPI_CLOCK_468MBPS)
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
/* 800*1280, RGB888, 4 Lane 60 fps 450Mbps video mode */
  /* regulator */
  {0x03, 0x0a, 0x04, 0x00, 0x20},
  /* timing */
  {0xab, 0x8a, 0x18, 0x00, 0x92, 0x97, 0x1b, 0x8c,
   0x0c, 0x03, 0x04, 0xa0},
  /* phy ctrl */
  {0x5f, 0x00, 0x00, 0x10},
  /* strength */
  {0xff, 0x00, 0x06, 0x00},
  /* pll control */
  {0x0, 0xd3, 0x31, 0xda, 0x00, 0x50, 0x48, 0x63,
   0x31, 0x0f, 0x03,
   0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};
#elif defined(MIPI_CLOCK_494MBPS)
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* 720*1280, RGB888, 4 Lane 60 fps 500Mbps video mode */
  /* regulator */
  {0x03, 0x0a, 0x04, 0x00, 0x20},
  /* timing */
  {0xab, 0x8a, 0x18, 0x00, 0x92, 0x97, 0x1b, 0x8c,
   0x0c, 0x03, 0x04, 0xa0},
  /* phy ctrl */
  {0x5f, 0x00, 0x00, 0x10},
  /* strength */
  {0xff, 0x00, 0x06, 0x00},
  /* pll control */
  {0x0, 0xed, 0x30, 0xda, 0x00, 0x50, 0x48, 0x63,
   0x31, 0x0f, 0x03,
   0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};
#elif defined(MIPI_CLOCK_500MBPS)
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* 720*1280, RGB888, 4 Lane 60 fps 500Mbps video mode */
  /* regulator */
  {0x03, 0x0a, 0x04, 0x00, 0x20},
  /* timing */
  {0xab, 0x8a, 0x18, 0x00, 0x92, 0x97, 0x1b, 0x8c,
   0x0c, 0x03, 0x04, 0xa0},
  /* phy ctrl */
  {0x5f, 0x00, 0x00, 0x10},
  /* strength */
  {0xff, 0x00, 0x06, 0x00},
  /* pll control */
  {0x0, 0xf9, 0x30, 0xda, 0x00, 0x50, 0x48, 0x63,
   0x30, 0x07, 0x01,
   0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};
#elif defined(MIPI_CLOCK_456MBPS)
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* 720*1280, RGB888, 4 Lane 50 fps 320Mbps video mode */
  /* regulator */
  {0x03, 0x0a, 0x04, 0x00, 0x20},
  /* timing */
  {0xab, 0x8a, 0x18, 0x00, 0x92, 0x97, 0x1b, 0x8c,
   0x0c, 0x03, 0x04, 0xa0},
  /* phy ctrl */
  {0x5f, 0x00, 0x00, 0x10},
  /* strength */
  {0xff, 0x00, 0x06, 0x00},
  /* pll control */
  {0x0, 0xc7, 0x31, 0xda, 0x00, 0x50, 0x48, 0x63,
   0x31, 0x0f, 0x03,
   0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};
#else
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* 480*800, RGB888, 2 Lane 60 fps 200Mbps video mode */
    /* regulator */
	{0x03, 0x0a, 0x04, 0x00, 0x20},
	/* timing */
	{0x96, 0x85, 0x0c, 0x00, 0x0d, 0x89, 0x10, 0x87,
	0x0d, 0x03, 0x04, 0xa0},
    /* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
    /* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
    {0x0, 0x8f, 0x1, 0x1a, 0x00, 0x50, 0x48, 0x63,
	0x43, 0x1f, 0x0f,
	0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};
#endif

static int __init mipi_video_samsung_oled_hd_pt_init(void)
{
	int ret;

#ifdef CONFIG_FB_MSM_MIPI_PANEL_DETECT
	if (msm_fb_detect_client("mipi_video_samsung_oled_hd"))
		return 0;
#endif

	pinfo.xres = 720;
	pinfo.yres = 1280;
	/*
	 *
	 * Panel's Horizontal input timing requirement is to
	 * include dummy(pad) data of 200 clk in addition to
	 * width and porch/sync width values
	 */

	pinfo.mipi.xres_pad = 0;
	pinfo.mipi.yres_pad = 0;

	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
#if defined(MIPI_CLOCK_440MBPS)
#if 0
	pinfo.lcdc.h_back_porch = 90;//32;
	pinfo.lcdc.h_front_porch = 110;//64;
	pinfo.lcdc.h_pulse_width = 60;//16;
	pinfo.lcdc.v_back_porch = 2;
	pinfo.lcdc.v_front_porch = 13;
	pinfo.lcdc.v_pulse_width = 1;
#else
#if 1  /*2.1us */
	pinfo.lcdc.h_back_porch =48;//32;
	pinfo.lcdc.h_front_porch = 152;//64;
	pinfo.lcdc.h_pulse_width = 24;//16;
	pinfo.lcdc.v_back_porch = 2;
	pinfo.lcdc.v_front_porch = 13;
	pinfo.lcdc.v_pulse_width = 1;
#else
/*  1.6us
	pinfo.lcdc.h_back_porch =76;//32;
	pinfo.lcdc.h_front_porch = 124;//64;
	pinfo.lcdc.h_pulse_width = 24;//16;
	pinfo.lcdc.v_back_porch = 2;
	pinfo.lcdc.v_front_porch = 13;
	pinfo.lcdc.v_pulse_width = 1;
*/
	pinfo.lcdc.h_back_porch =84;//32;
	pinfo.lcdc.h_front_porch = 104;//64;
	pinfo.lcdc.h_pulse_width = 36;//16;
	pinfo.lcdc.v_back_porch = 2;
	pinfo.lcdc.v_front_porch = 13;
	pinfo.lcdc.v_pulse_width = 1;
#endif
#endif

#elif defined(MIPI_CLOCK_456MBPS)
/* 59.6fps
	pinfo.lcdc.h_back_porch = 48;//32;
	pinfo.lcdc.h_front_porch = 184;//64;
	pinfo.lcdc.h_pulse_width = 28;//16;
	pinfo.lcdc.v_back_porch = 2;
	pinfo.lcdc.v_front_porch = 13;
	pinfo.lcdc.v_pulse_width = 1;
*/
/* 456Mhz Ok 59fps 
	pinfo.lcdc.h_back_porch = 48;//32;
	pinfo.lcdc.h_front_porch = 178;//64;
	pinfo.lcdc.h_pulse_width = 32;//16;
	pinfo.lcdc.v_back_porch = 2;
	pinfo.lcdc.v_front_porch = 13;
	pinfo.lcdc.v_pulse_width = 1;
*/
	pinfo.lcdc.h_back_porch = 68;//32;
	pinfo.lcdc.h_front_porch = 138;//64;
	pinfo.lcdc.h_pulse_width = 52;//16;
	pinfo.lcdc.v_back_porch = 2;
	pinfo.lcdc.v_front_porch = 13;
	pinfo.lcdc.v_pulse_width = 1;
#elif defined(MIPI_CLOCK_468MBPS)
	pinfo.lcdc.h_back_porch = 88;//32;
	pinfo.lcdc.h_front_porch = 132;//64;
	pinfo.lcdc.h_pulse_width = 64;//16;
	pinfo.lcdc.v_back_porch = 2;
	pinfo.lcdc.v_front_porch = 13;
	pinfo.lcdc.v_pulse_width = 1;
#elif defined(MIPI_CLOCK_494MBPS)
	pinfo.lcdc.h_back_porch = 92;//32;
	pinfo.lcdc.h_front_porch = 147;//64;
	pinfo.lcdc.h_pulse_width = 100;//16;
	pinfo.lcdc.v_back_porch = 2;
	pinfo.lcdc.v_front_porch = 13;
	pinfo.lcdc.v_pulse_width = 1;
#elif defined(MIPI_CLOCK_500MBPS)
#if 0  /* 52 fps */
	pinfo.lcdc.h_back_porch = 108;//32;
	pinfo.lcdc.h_front_porch = 250;//64;
	pinfo.lcdc.h_pulse_width = 140;//16;
#else /* 60fps */
	pinfo.lcdc.h_back_porch = 100;//32;
	pinfo.lcdc.h_front_porch = 148;//64;
	pinfo.lcdc.h_pulse_width = 96;//16;
#endif
	pinfo.lcdc.v_back_porch = 2;
	pinfo.lcdc.v_front_porch = 13;
	pinfo.lcdc.v_pulse_width = 1;
#endif
	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	if(get_hw_revision() == 3)
		pinfo.bl_max = 15;		
	else
		pinfo.bl_max = 29;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
#if defined(MIPI_CLOCK_440MBPS)
 	pinfo.clk_rate = 440000000;
#elif defined(MIPI_CLOCK_456MBPS)
 	pinfo.clk_rate = 456000000;
#elif defined(MIPI_CLOCK_468MBPS)
 	pinfo.clk_rate = 468000000;
#elif defined(MIPI_CLOCK_494MBPS)
 	pinfo.clk_rate = 494000000;
#elif defined(MIPI_CLOCK_500MBPS)
//	pinfo.clk_rate = 500000000;
	pinfo.clk_rate = 499000000;  // 8960 vco bug ( not used from 500 to 600 ) p13447
#elif defined(MIPI_CLOCK_320MBPS)
    	pinfo.clk_rate = 320000000;
#elif defined(MIPI_CLOCK_300MBPS)
    	pinfo.clk_rate = 300000000;
#else
    	pinfo.clk_rate = 200000000;
#endif
	pinfo.mipi.mode = DSI_VIDEO_MODE;    

#if defined(MIPI_CLOCK_440MBPS) || defined(MIPI_CLOCK_456MBPS) || defined(MIPI_CLOCK_468MBPS) || defined(MIPI_CLOCK_494MBPS)
#if 0
	pinfo.mipi.pulse_mode_hsa_he =TRUE;
	pinfo.mipi.hfp_power_stop = TRUE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = TRUE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
#else
	pinfo.mipi.pulse_mode_hsa_he =TRUE;
	pinfo.mipi.hfp_power_stop =FALSE; 
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = TRUE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
#endif
#elif defined(MIPI_CLOCK_500MBPS)
	pinfo.mipi.pulse_mode_hsa_he =TRUE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = TRUE;
	pinfo.mipi.hsa_power_stop = TRUE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
#endif
	pinfo.mipi.traffic_mode = DSI_BURST_MODE;

	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;
#if defined(MIPI_CLOCK_440MBPS)
 	pinfo.mipi.t_clk_post = 0x21; 
	pinfo.mipi.t_clk_pre = 0x2f;
#elif defined(MIPI_CLOCK_456MBPS)
   	pinfo.mipi.t_clk_post = 0x22; 
	pinfo.mipi.t_clk_pre = 0x2f;
#elif defined(MIPI_CLOCK_468MBPS)
   	pinfo.mipi.t_clk_post = 0x21; 
	pinfo.mipi.t_clk_pre = 0x30;
#elif defined(MIPI_CLOCK_300MBPS)
	pinfo.mipi.t_clk_post =0x21;
	pinfo.mipi.t_clk_pre =0x2d;
#elif defined(MIPI_CLOCK_320MBPS)
	pinfo.mipi.t_clk_post =0x21;
    pinfo.mipi.t_clk_pre =0x2d;
#elif defined(MIPI_CLOCK_494MBPS)
	pinfo.mipi.t_clk_post =0x21;
    pinfo.mipi.t_clk_pre =0x30;
#elif defined(MIPI_CLOCK_500MBPS)
//	pinfo.mipi.t_clk_post = 0x21; 
//	pinfo.mipi.t_clk_pre = 0x30;
	pinfo.mipi.t_clk_post = 0x22; 
	pinfo.mipi.t_clk_pre = 0x41;
#else //200
	pinfo.mipi.t_clk_post =0x21;
	pinfo.mipi.t_clk_pre =0x2b;
#endif
	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = 0;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
#if defined(MIPI_CLOCK_320MBPS)
	pinfo.mipi.frame_rate = 50;
#else
	pinfo.mipi.frame_rate = 60;
#endif

	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.force_clk_lane_hs = 1;

	ret = mipi_samsung_oled_hd_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_WVGA_PT);
	if (ret)
		printk(KERN_ERR "%s: failed to register device!\n", __func__);

	return ret;
}

module_init(mipi_video_samsung_oled_hd_pt_init);

