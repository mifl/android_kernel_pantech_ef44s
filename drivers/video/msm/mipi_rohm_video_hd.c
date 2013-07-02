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
#include "mipi_rohm.h"

static struct msm_panel_info pinfo;

#if defined (MIPI_CLOCK_440MBPS)
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* 720*1280, RGB888, 4 Lane 60 fps 440Mbps video mode */
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
#elif defined (MIPI_CLOCK_500MBPS)
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
#else
#error
#endif

static int __init mipi_video_rohm_wxga_init(void)
{
	int ret;

#ifdef CONFIG_FB_MSM_MIPI_PANEL_DETECT
	if (msm_fb_detect_client("mipi_video_rohm_wxga"))
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
	pinfo.mipi.xres_pad = 0;   //184 : 500Mbps,  68 :440Mbps
	pinfo.mipi.yres_pad = 0;

	pinfo.mipi.xres_pad = 0;
	pinfo.mipi.yres_pad = 0;

	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
    
    	pinfo.lcdc.h_back_porch = 14;
#if defined (MIPI_CLOCK_440MBPS)     
	pinfo.lcdc.h_front_porch = 257;
#elif defined (MIPI_CLOCK_500MBPS)
	pinfo.lcdc.h_front_porch = 393;
#else
#error
#endif
	pinfo.lcdc.h_pulse_width = 7;
	pinfo.lcdc.v_back_porch = 4;
	pinfo.lcdc.v_front_porch = 2;
    	pinfo.lcdc.v_pulse_width = 2;
    
	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 16;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
#if defined (MIPI_CLOCK_440MBPS)     
    	pinfo.clk_rate = 440000000;
#elif defined (MIPI_CLOCK_500MBPS)
    	pinfo.clk_rate = 500000000;
#else
#error
#endif
	pinfo.mipi.mode = DSI_VIDEO_MODE;   
	pinfo.mipi.pulse_mode_hsa_he = TRUE;
	pinfo.mipi.hfp_power_stop = TRUE;
   	pinfo.mipi.hbp_power_stop = FALSE;
   	pinfo.mipi.hsa_power_stop = TRUE;
  	pinfo.mipi.eof_bllp_power_stop = FALSE;
    	pinfo.mipi.bllp_power_stop = TRUE;
    	pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_EVENT;
    	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
    	pinfo.mipi.data_lane0 = TRUE;
    	pinfo.mipi.data_lane1 = TRUE;
    	pinfo.mipi.data_lane2 = TRUE;
    	pinfo.mipi.data_lane3 = TRUE;
#if defined (MIPI_CLOCK_440MBPS)   
   	pinfo.mipi.t_clk_post = 0x21; 
	pinfo.mipi.t_clk_pre = 0x2f;
#elif defined (MIPI_CLOCK_500MBPS)   
	pinfo.mipi.t_clk_post = 0x21; 
	pinfo.mipi.t_clk_pre = 0x30;
#else
#error
#endif
	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = 0;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
#if 0
	pinfo.mipi.frame_rate = 54; //60;
#else
	pinfo.mipi.frame_rate = 57; //60;
#endif
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	pinfo.mipi.tx_eot_append = TRUE;
      //pinfo.mipi.force_clk_lane_hs = 1;
    
	ret = mipi_rohm_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_WVGA_PT);
	if (ret)
		printk(KERN_ERR "%s: failed to register device!\n", __func__);

	return ret;
}
module_init(mipi_video_rohm_wxga_init);

