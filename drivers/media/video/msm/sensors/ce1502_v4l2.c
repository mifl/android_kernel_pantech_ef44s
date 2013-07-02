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

#include "msm_sensor.h"
#include <linux/regulator/machine.h> //F_CE1502_POWER
#include "sensor_ctrl.h"

#include <media/v4l2-subdev.h>
#include "msm.h"
#include "msm_ispif.h"
#include "msm_camera_i2c.h"
#if 1 // jjhwang File //yujm
#include <linux/syscalls.h>
#endif

#include <linux/gpio.h>

#include "ce1502_v4l2_cfg.h"

#define CONFIG_PANTECH_CAMERA_IRQ

#ifdef CONFIG_MACH_MSM8960_EF44S
//#define CONFIG_PNTECH_CAMERA_OJT_TEST
#define FULLSIZE_13P0
#endif

#define CONFIG_PNTECH_CAMERA_OJT

#define CONFIG_PNTECH_CAMERA_ZSL_FLASH

#define F_FW_UPDATE

//#define I2C_LOG_PRINT
//#define ISP_LOGEVENT_PRINT

#ifdef ISP_LOGEVENT_PRINT
#include <linux/file.h>
#include <linux/vmalloc.h>
#endif


//#define F_MIPI336

#define NEW_CAPTURE_FW

#define ON  1
#define OFF 0

#if 1 // jjhwang File 
#define CE1502_UPLOADER_INFO_F      "/CE150F00.bin"
#define CE1502_UPLOADER_BIN_F		"/CE150F01.bin"
#define CE1502_FW_INFO_F			"/CE150F02.bin"
#define CE1502_FW_BIN_F			"/CE150F03.bin"

#define CE1502_FW_MAJOR_VER	55
#define CE1502_FW_MINOR_VER	23
#define CE1502_PRM_MAJOR_VER	55
#define CE1502_PRM_MINOR_VER	23
#endif

struct ce1502_ver_t {
	uint8_t fw_major_ver;
	uint8_t fw_minor_ver;
	uint8_t prm_major_ver;
	uint8_t prm_minor_ver;
};

static struct ce1502_ver_t ce1502_ver = {0, };

#define SENSOR_NAME "ce1502"
#define PLATFORM_DRIVER_NAME "msm_camera_ce1502"
#define ce1502_obj ce1502_##obj

/*=============================================================
	SENSOR REGISTER DEFINES
==============================================================*/

/* Sensor Model ID */
#define CE1502_PIDH_REG		0x00
#define CE1502_MODEL_ID		1502

//wsyang_temp
#define F_CE1502_POWER

#ifdef F_CE1502_POWER
#define CAMIO_R_RST_N	0
#define CAMIO_R_STB_N	1
#define CAM1_ISP_CORE_EN 2
#define CAM1_SENSOR_CORE_EN 3
#define CAM1_SYS_EN 4
#define CAM1_RAM_EN 5
#define FLASH_CNTL_EN 6
#define CAM1_INT_N 7
#define CAMIO_MAX	8

#define CAM1_STANDBY     54
#define CAM1_RST_N         107
#define CAM1_ISP_CORE		98
#define CAM1_SENSOR_CORE	97
#define CAM1_SYS	82
#define CAM1_RAM	77
#define FLASH_CNTL	2
#define CAM1_INT	75

static sgpio_ctrl_t sgpios[CAMIO_MAX] = {
	{CAMIO_R_RST_N, "CAM_R_RST_N", CAM1_RST_N},
	{CAMIO_R_STB_N, "CAM_R_STB_N", CAM1_STANDBY},
	{CAM1_ISP_CORE_EN, "CAM_ISP_CORE_N", CAM1_ISP_CORE},
	{CAM1_SENSOR_CORE_EN, "CAM_SENSOR_CORE_N", CAM1_SENSOR_CORE},
	{CAM1_SYS_EN, "CAM_SYS_N", CAM1_SYS},
	{CAM1_RAM_EN, "CAM_RAM_N", CAM1_RAM},
	{FLASH_CNTL_EN, "FLASH_CNTL_N", FLASH_CNTL},
	{CAM1_INT_N, "CAM_INT_N", CAM1_INT},
};

#define CAMV_IO_1P8V	0
#define CAMV_A_2P8V	1
#define CAMV_SENSOR_IO_1P8V	2
#define CAMV_AF_2P8V	3
#define CAMV_MAX	4

static svreg_ctrl_t svregs[CAMV_MAX] = {
	{CAMV_IO_1P8V,   "8921_lvs5", NULL, 0},
	{CAMV_A_2P8V,    "8921_l8",  NULL, 2800},
	{CAMV_SENSOR_IO_1P8V,    "8921_l17",  NULL, 1800},
	{CAMV_AF_2P8V,    "8921_l16",  NULL, 2800},
};
#endif

#ifdef CONFIG_PANTECH_CAMERA_IRQ
static int32_t ce1502_irq_stat = 0;
#endif

DEFINE_MUTEX(ce1502_mut);
static struct msm_sensor_ctrl_t ce1502_s_ctrl;

static int8_t continuous_af_mode = 0;   // 0: no caf, 1: af-c, 2: af-t
#ifdef CONFIG_PNTECH_CAMERA_OJT
static int8_t caf_b_ojt = 0;   // 0: no caf, 1: af-c, 2: af-t
static int8_t current_ojt = 0;   // 0: off, 1: on
#endif
static int8_t sensor_mode = -1;   // 0: full size,  1: qtr size, 2: fullhd size , 3: ZSL
static int32_t x1 = 0, y1=0, x2=0, y2=0;   // 0: full size,  1: qtr size, 2: fullhd size
static int32_t current_fps = 31;
#if 1 //def F_PANTECH_CAMERA_FIX_CFG_AE_AWB_LOCK
static uint8_t aec_awb_lock = 0x02;
#endif
//static int8_t g_sensor_mode = -1;
//static int g_update_type = MSM_SENSOR_REG_INIT;


#if 1 // F_PANTECH_CAMERA_CFG_HDR
static int8_t ev_sft_mode = 0;   // 0:normal capture , 1: ev shift capture
#endif

static struct v4l2_subdev_info ce1502_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_YUYV8_2X8,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

#if 0
static struct msm_camera_i2c_conf_array ce1502_init_conf[] = {
	{&ce1502_recommend_settings[0],
	ARRAY_SIZE(ce1502_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array ce1502_confs[] = {
	{&ce1502_snap_settings[0],
	ARRAY_SIZE(ce1502_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ce1502_prev_settings[0],
	ARRAY_SIZE(ce1502_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};
#endif


#define C_PANTECH_CAMERA_MIN_PREVIEW_FPS	5
#define C_PANTECH_CAMERA_MAX_PREVIEW_FPS	31

#define CE1502_FULL_SIZE_DUMMY_PIXELS     1
#define CE1502_FULL_SIZE_DUMMY_LINES    1
#ifdef FULLSIZE_13P0
#define CE1502_FULL_SIZE_WIDTH    4192 // 4136
#define CE1502_FULL_SIZE_HEIGHT   3104 // 3102
#else
#define CE1502_FULL_SIZE_WIDTH    4096 // 4160 // 4136
#define CE1502_FULL_SIZE_HEIGHT   3072 // 3120 // 3102
#endif

#if 1//#ifdef F_PANTECH_CAMERA_1080P_PREVIEW		
#define CE1502_1080P_SIZE_WIDTH        1920
#define CE1502_1080P_SIZE_HEIGHT        1088
#endif

#define CE1502_QTR_SIZE_DUMMY_PIXELS  1254
#define CE1502_QTR_SIZE_DUMMY_LINES   931
#define CE1502_QTR_SIZE_WIDTH     1280  //1600
#define CE1502_QTR_SIZE_HEIGHT    960   //1200

#define CE1502_HRZ_FULL_BLK_PIXELS   3
#define CE1502_VER_FULL_BLK_LINES     1
#if 1//#ifdef F_PANTECH_CAMERA_1080P_PREVIEW		
#define CE1502_HRZ_1080P_BLK_PIXELS      0
#define CE1502_VER_1080P_BLK_LINES        0
#endif
#define CE1502_HRZ_QTR_BLK_PIXELS    3
#define CE1502_VER_QTR_BLK_LINES      1

#define CE1502_ZSL_SIZE_WIDTH    3264 
#define CE1502_ZSL_SIZE_HEIGHT   2448 


static struct msm_sensor_output_info_t ce1502_dimensions[] = {
	{
		.x_output = CE1502_FULL_SIZE_WIDTH,
		.y_output = CE1502_FULL_SIZE_HEIGHT,
		.line_length_pclk = CE1502_FULL_SIZE_WIDTH + CE1502_HRZ_FULL_BLK_PIXELS+CE1502_FULL_SIZE_DUMMY_PIXELS,
		.frame_length_lines = CE1502_FULL_SIZE_HEIGHT+ CE1502_VER_FULL_BLK_LINES+CE1502_FULL_SIZE_DUMMY_LINES,
#ifdef F_MIPI336
		.vt_pixel_clk = 176160768, //
		.op_pixel_clk = 176160768, //
#else
		.vt_pixel_clk = 264000000, //
		.op_pixel_clk = 264000000, //
#endif
		.binning_factor = 1,
	},
	{
		.x_output = CE1502_QTR_SIZE_WIDTH,
		.y_output = CE1502_QTR_SIZE_HEIGHT,
		.line_length_pclk = CE1502_QTR_SIZE_WIDTH + CE1502_HRZ_QTR_BLK_PIXELS + CE1502_QTR_SIZE_DUMMY_PIXELS,
		.frame_length_lines = CE1502_QTR_SIZE_HEIGHT + CE1502_VER_QTR_BLK_LINES + CE1502_QTR_SIZE_DUMMY_LINES,
#ifdef F_MIPI336
		.vt_pixel_clk = 176160768, //
		.op_pixel_clk = 176160768, //
#else
		.vt_pixel_clk = 264000000, //
		.op_pixel_clk = 264000000, //
#endif
		.binning_factor = 2,
	},


#if 1//#ifdef F_PANTECH_CAMERA_1080P_PREVIEW		
	{
		.x_output = CE1502_1080P_SIZE_WIDTH,
		.y_output = CE1502_1080P_SIZE_HEIGHT,
		.line_length_pclk = CE1502_1080P_SIZE_WIDTH + CE1502_HRZ_1080P_BLK_PIXELS,
		.frame_length_lines = CE1502_1080P_SIZE_HEIGHT+ CE1502_VER_1080P_BLK_LINES,
#ifdef F_MIPI336
		.vt_pixel_clk = 176160768, //
		.op_pixel_clk = 176160768, //
#else
		.vt_pixel_clk = 264000000, //
		.op_pixel_clk = 264000000, //
#endif
		.binning_factor = 2,
	},
#endif
	{
		.x_output = CE1502_ZSL_SIZE_WIDTH,
		.y_output = CE1502_ZSL_SIZE_HEIGHT,
		.line_length_pclk = CE1502_ZSL_SIZE_WIDTH,
		.frame_length_lines = CE1502_ZSL_SIZE_HEIGHT,
#ifdef F_MIPI336
		.vt_pixel_clk = 176160768, //
		.op_pixel_clk = 176160768, //
#else
		.vt_pixel_clk = 264000000, //
		.op_pixel_clk = 264000000, //
#endif
		.binning_factor = 1,
	},
};

#if 1
static struct msm_camera_csid_vc_cfg ce1502_cid_cfg[] = {
	{0, CSI_YUV422_8, CSI_DECODE_8BIT}, 
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
#if 1//#ifdef F_PANTECH_CAMERA_1080P_PREVIEW	
	{2, CSI_RESERVED_DATA, CSI_DECODE_8BIT},
#endif
	{3, CSI_RESERVED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params ce1502_csi_params = {
	.csid_params = {
		//.lane_assign = 0xe4,
		.lane_cnt = 4, 
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = ce1502_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 4,
#ifdef F_MIPI336
		.settle_cnt = 0x0F,
#else	
		.settle_cnt = 0x17, 
#endif
	//	.lane_mask = 0xf,
	},
};

static struct msm_camera_csi2_params *ce1502_csi_params_array[] = {
	&ce1502_csi_params,
	&ce1502_csi_params,
#if 1//#ifdef F_PANTECH_CAMERA_1080P_PREVIEW	
	&ce1502_csi_params,
#endif
	&ce1502_csi_params,
};
#endif




#if 0//
static struct msm_sensor_output_reg_addr_t ce1502_reg_addr = {
	.x_output = 0x034C,
	.y_output = 0x034E,
	.line_length_pclk = REG_LINE_LENGTH_PCK,//0x342
	.frame_length_lines = REG_FRAME_LENGTH_LINES,//0x0340
};
#endif

static struct msm_sensor_id_info_t ce1502_id_info = {
	.sensor_id_reg_addr = CE1502_PIDH_REG,
	.sensor_id = 0,
};

#if 0
#define CE1502_GAIN         0x350B//0x3000
static struct msm_sensor_exp_gain_info_t ce1502_exp_gain_info = {
	.coarse_int_time_addr = REG_COARSE_INTEGRATION_TIME,
	.global_gain_addr = REG_GLOBAL_GAIN,
	.vert_offset = 3,
};
#endif


int32_t ce1502_sensor_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id);


static const struct i2c_device_id ce1502_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ce1502_s_ctrl},
	{ }
};

static struct i2c_driver ce1502_i2c_driver = {
	.id_table = ce1502_i2c_id,
//	.probe  = msm_sensor_i2c_probe,
	.probe  = ce1502_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};


static struct msm_camera_i2c_client ce1502_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static int32_t ce1502_set_led_mode(struct msm_sensor_ctrl_t *s_ctrl ,int8_t led_mode);
static int ce1502_sensor_set_preview_fps(struct msm_sensor_ctrl_t *s_ctrl ,int8_t preview_fps);
#ifdef CONFIG_PNTECH_CAMERA_OJT
static int32_t ce1502_set_ojt_ctrl(struct msm_sensor_ctrl_t *s_ctrl ,int8_t ojt);
#endif
static void ce1502_sensor_start_stream(struct msm_sensor_ctrl_t *s_ctrl);
static void ce1502_sensor_stop_stream(struct msm_sensor_ctrl_t *s_ctrl);

static int32_t ce1502_cmd(struct msm_sensor_ctrl_t *s_ctrl , uint8_t cmd, uint8_t * ydata, int32_t num_data)
{
	int32_t rc = 0;
//	unsigned char buf[10];
	unsigned char buf[130];
	int32_t i = 0;
	
	memset(buf, 0, sizeof(buf));
	buf[0] = cmd;


#ifdef I2C_LOG_PRINT
    SKYCDBG("++++ I2C W : \n");
    SKYCDBG("ISP : 0x%x \n", buf[0]);
#endif

	if(ydata != NULL)
	{
		for(i = 0; i < num_data; i++)
		{
			buf[i+1] = *(ydata+i);
#ifdef I2C_LOG_PRINT
            SKYCDBG("ISP : 0x%x \n", buf[i+1]);	
#endif

		}	
	}
		
#ifdef I2C_LOG_PRINT
    SKYCDBG("++++\n");
#endif

	rc = msm_camera_i2c_txdata(s_ctrl->sensor_i2c_client, buf, num_data+1);
	
	if (rc < 0)
		SKYCERR("ERR:%s FAIL!!!cmd=%d , rc=%d return~~\n", __func__, cmd, rc);	
	
	return rc;
}


static int32_t ce1502_read(struct msm_sensor_ctrl_t *s_ctrl , uint8_t * rdata, uint32_t len)
{
    int32_t rc = 0;

#if defined(I2C_LOG_PRINT) && !defined(ISP_LOGEVENT_PRINT)
    int32_t i = 0;
    SKYCDBG("---- I2C R : \n");
#endif

    rc = msm_camera_i2c_rxdata_2(s_ctrl->sensor_i2c_client, rdata, len);

#if defined(I2C_LOG_PRINT) && !defined(ISP_LOGEVENT_PRINT)
    if(len != 0)
    {
        for(i = 0; i < len; i++)
        {
            SKYCDBG("ISP : 0x%x \n", *(rdata+i));	
        }	
    }

    SKYCDBG("----\n");
#endif

    return rc;
}

static int32_t ce1502_cmd_read(struct msm_sensor_ctrl_t *s_ctrl , unsigned char cmd, uint8_t * rdata, uint32_t len)
{
    int32_t rc = 0;

#ifdef I2C_LOG_PRINT
#if !defined(ISP_LOGEVENT_PRINT)
    int32_t i = 0;
#endif
    SKYCDBG("++++ I2C W :\n");
    SKYCDBG("ISP : 0x%x \n", cmd);
    SKYCDBG("++++\n");
#if !defined(ISP_LOGEVENT_PRINT)
    SKYCDBG("---- I2C R : \n");
#endif
#endif

    *rdata = cmd;
    rc = msm_camera_i2c_rxdata(s_ctrl->sensor_i2c_client, rdata, len);

#if defined(I2C_LOG_PRINT) && !defined(ISP_LOGEVENT_PRINT)
    if(len != 0)
    {
        for(i = 0; i < len; i++)
    {
            SKYCDBG("ISP : 0x%x \n", *(rdata+i));	
    }
    }

    SKYCDBG("----\n");
#endif

    if(rc >= 0)
        rc = 0;

    return rc;
}


static int32_t ce1502_poll(struct msm_sensor_ctrl_t *s_ctrl , unsigned char cmd, uint8_t pdata, 
			   uint8_t mperiod, uint32_t retry)
{
	unsigned char rdata = 0;
	uint32_t i = 0;
	int32_t rc = 0;
	unsigned char tmp_raddr;

	for (i = 0; i < retry; i++) {
		rc = ce1502_cmd_read(s_ctrl, cmd, &tmp_raddr, 1);
//		if (rc < 0)
//			break;
		rdata = tmp_raddr;
#if 0        
              SKYCDBG("%s: (mperiod=%d, retry=%d) <%d>poll data = 0x%x, read = 0x%x\n", 
			__func__, mperiod, retry, i, pdata, rdata);        
#endif
		if (rdata == pdata)
			break;

		msleep(mperiod);
	}

	if (i == retry) {
/*		SKYCERR("%s: -ETIMEDOUT, mperiod=%d, retry=%d\n", 
			__func__, mperiod, retry);
*/
		rc = -ETIMEDOUT;
	}
    if(rc >= 0)
        rc = 0;

	return rc;
}

static int32_t ce1502_poll_bit(struct msm_sensor_ctrl_t *s_ctrl, uint8_t cmd, uint8_t mperiod, uint32_t retry)
{
	uint8_t rdata = 0;
	uint32_t i = 0;
	int32_t rc = 0;

	for (i = 0; i < retry; i++) {
		rc = ce1502_cmd_read(s_ctrl, cmd, &rdata, 1);
#if 0        
              SKYCDBG("%s: (mperiod=%d, retry=%d) <%d> read = 0x%x\n", 
			__func__, mperiod, retry, i, rdata);        
#endif
		if (rc < 0)
			break;
		if (!(rdata & 0x01))
			break;

		msleep(mperiod);
	}

	if (i == retry) {
		SKYCERR("%s: -ETIMEDOUT, mperiod=%d, retry=%d\n", 
			__func__, mperiod, retry);
		rc = -ETIMEDOUT;
	}
    if(rc >= 0)
        rc = 0;

	return rc;
}


#ifdef ISP_LOGEVENT_PRINT
#define CE1502_ISP_EVENT_LOG_DUMP_FILE	"/data/.ce1502_isp_event_log.dat"
static int32_t ce1502_dump_isp_status(uint8_t *log_packet, int32_t frame_packet_byte)
{
    int fd = 0;
    struct file *m_file;
    loff_t pos = 0;
    mm_segment_t old_fs; 

    old_fs = get_fs();
    set_fs(KERNEL_DS); 

    fd = sys_open(CE1502_ISP_EVENT_LOG_DUMP_FILE, O_RDWR|O_CREAT|O_APPEND, 0644);
    if(fd >= 0)
    {
        SKYCDBG("%s: sys_open!!! fd=%d\n", __func__, fd);
        m_file=fget(fd);
        if(m_file)
        {
            vfs_write(m_file, log_packet, frame_packet_byte, &pos);
            fput(m_file);
            SKYCDBG("%s: vfs_write!!! log_packet=%x, frame_packet_byte=%d, pos=%d\n", __func__, (int)log_packet, frame_packet_byte, (int)pos);
        }
        sys_close(fd);
    }
    else
    {
        SKYCERR("%s: Can not open %s\n", __func__, CE1502_ISP_EVENT_LOG_DUMP_FILE);
        set_fs(old_fs);
        return fd;        
    }

    set_fs(old_fs);
    SKYCDBG("%s: complete!!! event log save file~\n", __func__);
    
    return fd;        
}

#define MAX_ISP_PACKET_BYTE 500*1024
static int32_t ce1502_get_isp_event_log(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    int32_t loop = 0;
    int16_t frame_cnt = 0;
    int32_t frame_packet_byte = 0;
    uint8_t *rdata = 0;

    rdata = vmalloc(MAX_ISP_PACKET_BYTE);
    if(rdata == NULL)
    {
    	SKYCERR("ERR:%s vmalloc FAIL!!!rdata=%d \n", __func__, (int)rdata);
       rc = -ENOMEM;
    	goto get_isp_event_log_fail;
    }
    
        
    rc = ce1502_cmd_read(s_ctrl, 0xDB, rdata, 2);
    if (rc < 0)
    {
    	SKYCERR("ERR:%s ce1502_cmd_read[0xDB] FAIL!!!rc=%d \n", __func__, rc);
    	goto get_isp_event_log_fail;
    }

    //get a frame count that is evented by isp
    frame_cnt = rdata[1]<<8 | rdata[0];
    SKYCDBG("%s frame_cnt=%d, rdata[1]=%x, rdata[0]=%x\n",__func__, frame_cnt, rdata[1], rdata[0]);
    
    if((frame_cnt * 130) > MAX_ISP_PACKET_BYTE)
        frame_packet_byte = MAX_ISP_PACKET_BYTE;
    else
        frame_packet_byte = frame_cnt * 130;

    //read event packet. 1frame packet is 2byte+128byte(frame count+1frame packet data) 
    //full event log has a limitation until maximum 500Kbyte
    for(loop=0; loop < frame_packet_byte; loop+= 130)
    {
        rc = ce1502_cmd_read(s_ctrl, 0xDC, rdata+loop, 130);
        if (rc < 0)
        {
        	SKYCERR("ERR:%s ce1502_cmd_read[0xDC] FAIL!!!rc=%d return~~\n", __func__, rc);
        	goto get_isp_event_log_fail;
        }        
    }
    SKYCDBG("%s complete event log read~ loop=%d",__func__, loop);

    rc = ce1502_dump_isp_status(rdata, frame_packet_byte);
    if (rc < 0)
    {
    	SKYCERR("ERR:ce1502_dump_isp_status FAIL!!!rc=%d \n", rc);
    	goto get_isp_event_log_fail;
    }

    if (rdata)
    {
    	vfree(rdata);
    	rdata = NULL;
    }
    SKYCDBG("%s success!!! rc=%d",__func__, rc);
    return rc;

get_isp_event_log_fail:
    if (rdata)
    {
    	vfree(rdata);
    	rdata = NULL;
    }
    SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
    return rc;    
}
#endif

static int32_t ce1502_01_command(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    
    rc = ce1502_cmd(s_ctrl, 0x01, NULL, 0);
    if (rc < 0)
    {
        SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
        return rc;
    }	
    rc = ce1502_poll(s_ctrl, 0x02, 0x00, 10, 400);
    if (rc < 0)
    {
        SKYCERR("ERR:%s 0x02 polling FAIL!!!rc=%d return~~\n", __func__, rc);
        return rc;
    }	
    
    return rc;
}
static int32_t ce1502_42_command(struct msm_sensor_ctrl_t *s_ctrl, uint8_t data)
{
    int32_t rc = 0;


    rc = ce1502_cmd(s_ctrl, 0x42, &data, 1);
    rc = ce1502_poll(s_ctrl, 0x43, data, 10, 100);
    if (rc < 0)
    {
        SKYCERR("%s : preview asistant polling ERROR !!!\n",__func__);
        return rc;
    }
   
    return rc;
}
static int32_t ce1502_lens_stop(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;

    {
#if 1 // AF-T state check
       uint8_t data_buf[4];
       int32_t is_af_t = 0;
    	rc = ce1502_cmd_read(s_ctrl, 0x2D, data_buf, 2);
    	if (rc < 0)
    	{
    		SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
    		return rc;
    	}

    	if(data_buf[0] == 0x01)
    	{
            SKYCDBG("%s AF-T PAUSE~~\n", __func__);
            is_af_t  = 1;
            data_buf[0] = 0x03;
            rc = ce1502_cmd(s_ctrl, 0x2C, data_buf, 1);
            if (rc < 0)
            {
                SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
                return rc;
            }	
            
            rc = ce1502_poll(s_ctrl, 0x2D, 0x02, 10, 40);
            if (rc < 0)
            {
                SKYCERR("ERR:%s AF-T PAUSE POLLING FAIL!!!rc=%d return~~\n", __func__, rc);
#ifdef ISP_LOGEVENT_PRINT
                ce1502_get_isp_event_log(s_ctrl);
#endif
                return rc;
            }
    	}		
        else
#endif
        {
            SKYCDBG("%s LENS STOP ~~\n", __func__);
            rc = ce1502_cmd(s_ctrl, 0x35, 0, 0);	// Lens Stop	
            rc = ce1502_poll_bit(s_ctrl, 0x24, 10, 400);
            if (rc < 0)
            {
                SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
#ifdef ISP_LOGEVENT_PRINT
                ce1502_get_isp_event_log(s_ctrl);
#endif
                return rc;
            }
        }
    }

    return rc;
}

static int32_t ce1502_lens_stop2(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    uint8_t data_buf[4];
    
    rc = ce1502_cmd(s_ctrl, 0x35, 0, 0);	// Lens Stop	
    rc = ce1502_poll_bit(s_ctrl, 0x24, 10, 400);
    if (rc < 0)
    {
        SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
#ifdef ISP_LOGEVENT_PRINT
        ce1502_get_isp_event_log(s_ctrl);
#endif
        return rc;
    }       
    
#if 1 // AF-T state check
    rc = ce1502_cmd_read(s_ctrl, 0x2D, data_buf, 2);
    if (rc < 0)
    {
        SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
        return rc;
    }

    if(data_buf[0] == 0x01)
    {
        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x2C, data_buf, 1);
        rc = ce1502_poll(s_ctrl, 0x2D, 0x00, 10, 400);
        if (rc < 0)
        {
            SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
#ifdef ISP_LOGEVENT_PRINT
            ce1502_get_isp_event_log(s_ctrl);
#endif
            return rc;
        }	
    }		
#endif

    SKYCERR("%s : Lens was stopped\n", __func__);
    return rc;
}

static int32_t ce1502_set_continuous_af(struct msm_sensor_ctrl_t *s_ctrl ,int8_t caf)
{

    uint8_t data_buf[2];	
//    uint8_t read_data[2];
    int rc = 0;

    continuous_af_mode = caf;    
    caf_b_ojt = caf;

    if(!(sensor_mode > 0 && sensor_mode < 4)&&(sensor_mode!=10)) // test
        return 0;


#if 0 //def CONFIG_PNTECH_CAMERA_OJT_TEST
    if(caf == 2)
        caf = 1;
#endif

    SKYCDBG("%s start : caf = %d\n",__func__, caf);

    rc = ce1502_lens_stop2(s_ctrl);

    switch(caf)
    {
    case 1:     //AF_MODE_CONTINUOUS (AF-C)
#if 0
	rc = ce1502_cmd_read(s_ctrl, 0x2D, read_data, 2);	// check AF-T
	if(read_data[0] == 1)
	{
            data_buf[0] = 0x00;
            rc = ce1502_cmd(s_ctrl, 0x2C, data_buf, 1);
	}
#endif
        data_buf[0] = 0x02;
        rc = ce1502_cmd(s_ctrl, 0x20, data_buf, 1);
        rc = ce1502_poll_bit(s_ctrl, 0x24, 10, 400);
        if (rc < 0)
        {
            SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
            return rc;
        }		

        SKYCDBG("AF-C start\n");
        rc = ce1502_cmd(s_ctrl, 0x23, 0, 0);
        if (rc < 0)
        {
            SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
            return rc;
        }
        break;
        
    case 2 :    //AF_MODE_CAF (AF-T)

#ifdef CONFIG_PANTECH_CAMERA_IRQ
        ce1502_irq_stat = 1;	// first trigger start
#endif

        SKYCDBG("AF-T start\n");
        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x20, data_buf, 1); //set focus mode normal
        rc = ce1502_poll_bit(s_ctrl, 0x24, 10, 400);

        data_buf[0] = 0x01;
        rc = ce1502_cmd(s_ctrl, 0x2C, data_buf, 1);
        //rc = ce1502_poll_bit(s_ctrl, 0x24, 10, 400);
        if (rc < 0)
        {
        	SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
        	return rc;
        }	
                
        break;
        
    default :   // continuous AF OFF    
#if 0
	rc = ce1502_cmd_read(s_ctrl, 0x2D, read_data, 2);	// check AF-T
	if(read_data[0] == 1)
	{
            data_buf[0] = 0x00;
            rc = ce1502_cmd(s_ctrl, 0x2C, data_buf, 1);
            rc = ce1502_poll_bit(s_ctrl, 0x24, 10, 400);
            if (rc < 0)
            {
                SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
                return rc;
            }	
	 }
#endif
        
        break;
    }

    SKYCDBG("%s end\n",__func__);
    return rc;
}


static int32_t ce1502_set_led_gpio_set(int32_t led_mode)
{
	int rc;
	int enable_flash_main_gpio = 0;	
	
	if(led_mode != 0)
		enable_flash_main_gpio = ON;
	else
		enable_flash_main_gpio = OFF;

	//control ce1502 led flash main gpio
	rc = sgpio_ctrl(sgpios, FLASH_CNTL_EN, enable_flash_main_gpio);

	return rc;
}
static int32_t ce1502_lens_stability(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    uint8_t rdata = 0;
    uint8_t data_buf[4];

    SKYCDBG("%s  start\n",__func__);
    if(sensor_mode == -1)
        return rc;

    rc = ce1502_cmd_read(s_ctrl, 0x6C, &rdata, 1);

    if((rdata > 0) && (rdata < 8))
    {
    return 0;
    }

    ce1502_set_continuous_af(s_ctrl, 0); // AF-T stop	
    data_buf[0] = 0x01;
    data_buf[1] = 0x00;
    data_buf[2] = 0x00;
    rc = ce1502_cmd(s_ctrl, 0x33, data_buf, 3); //set focus mode normal
    rc = ce1502_poll_bit(s_ctrl, 0x24, 10, 400);

    SKYCDBG("%s END~\n",__func__);

    return rc;
}

#ifdef F_CE1502_POWER
static int ce1502_vreg_init(void)
{
	int rc = 0;
	SKYCDBG("%s:%d\n", __func__, __LINE__);

	rc = sgpio_init(sgpios, CAMIO_MAX);
	if (rc < 0) {
		SKYCERR("%s: sgpio_init failed \n", __func__);
		goto sensor_init_fail;
	}

	rc = svreg_init(svregs, CAMV_MAX);
	if (rc < 0) {
		SKYCERR("%s: svreg_init failed \n", __func__);
		goto sensor_init_fail;
	}

	return rc;

sensor_init_fail:
    return -ENODEV;
}

int32_t ce1502_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    SKYCDBG("%s: %d\n", __func__, __LINE__);

#if 0
    msm_sensor_probe_on(&s_ctrl->sensor_i2c_client->client->dev);   //////////////////
#else
// test    rc = msm_sensor_power_up(s_ctrl);
    SKYCDBG(" %s : msm_sensor_power_up : rc = %d E\n",__func__, rc);  
#endif    
    ce1502_vreg_init();

#ifdef F_AS0260_POWER
    if (sgpio_ctrl(sgpios, CAM2_F_STANDBY, 0) < 0)	rc = -EIO;
    if (sgpio_ctrl(sgpios, CAM2_F_RST_N, 1) < 0)	rc = -EIO;
    mdelay(1);
    if (sgpio_ctrl(sgpios, CAMIO_R_RST_N, 0) < 0)	rc = -EIO;
#endif/*F_AS0260_POWER*/
    
    if (sgpio_ctrl(sgpios, CAM1_ISP_CORE_EN, 1) < 0)	rc = -EIO;	// CORE 1.2V
    mdelay(10);
    
    if (svreg_ctrl(svregs, CAMV_IO_1P8V, 1) < 0)	rc = -EIO;	// HOST IO 1.8V
    if (sgpio_ctrl(sgpios, CAM1_RAM_EN, 1) < 0)	rc = -EIO;	// VDD RAM 1.8V
    if (sgpio_ctrl(sgpios, CAM1_SYS_EN, 1) < 0)	rc = -EIO;	// VDD SYS 2.8V
    mdelay(1);
    
    if (svreg_ctrl(svregs, CAMV_A_2P8V, 1) < 0)	rc = -EIO;	// SENSOR AVDD 2.8V
    if (sgpio_ctrl(sgpios, CAM1_SENSOR_CORE_EN, 1) < 0)	rc = -EIO;	// SENSOR DVDD 1.2V
    if (svreg_ctrl(svregs, CAMV_SENSOR_IO_1P8V, 1) < 0)	rc = -EIO;	// SENSOR VDDIO 1.8V	
    mdelay(1);
    
    if (sgpio_ctrl(sgpios, CAMIO_R_STB_N, 1) < 0)	rc = -EIO;	// ISP Standby
    rc = msm_sensor_power_up(s_ctrl); // test
//    msm_camio_clk_rate_set(MSM_SENSOR_MCLK_24HZ);
    mdelay(1); /* > 500 clk */
    
    if (sgpio_ctrl(sgpios, CAMIO_R_RST_N, 1) < 0)	rc = -EIO;	// ISP Reset
    mdelay(10);
    
    if (svreg_ctrl(svregs, CAMV_AF_2P8V, 1) < 0)	rc = -EIO;	// AF 2.8V
        
    continuous_af_mode = 0;
    sensor_mode = -1;
#if 1 // F_PANTECH_CAMERA_CFG_HDR
    ev_sft_mode = 0;   
#endif
#ifdef CONFIG_PNTECH_CAMERA_OJT
    caf_b_ojt = 0;   // 0: no caf, 1: af-c, 2: af-t
    current_ojt = 0;   // 0: off, 1: on
#endif
    current_fps = 31;
#if 1 //def F_PANTECH_CAMERA_FIX_CFG_AE_AWB_LOCK
    aec_awb_lock = 0x02;
#endif
    
    SKYCDBG("%s X (%d)\n", __func__, rc);
    return rc;

}

int32_t ce1502_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    SKYCDBG("%s\n", __func__);

//    ce1502_lens_stability(s_ctrl);
    ce1502_set_led_gpio_set(0);
    
#if 0
    msm_sensor_probe_off(&s_ctrl->sensor_i2c_client->client->dev);  //////////////
#else
    msm_sensor_power_down(s_ctrl);
    SKYCDBG(" %s : msm_sensor_power_down : rc = %d E\n",__func__, rc);  
#endif
    if (svreg_ctrl(svregs, CAMV_AF_2P8V, 0) < 0)	rc = -EIO;	// AF 2.8V
    
    if (sgpio_ctrl(sgpios, CAMIO_R_RST_N, 0) < 0)	rc = -EIO;	// ISP Reset
    mdelay(10);

    if (sgpio_ctrl(sgpios, CAMIO_R_STB_N, 0) < 0)	rc = -EIO;	// ISP Standby
    if (svreg_ctrl(svregs, CAMV_SENSOR_IO_1P8V, 0) < 0)	rc = -EIO;	// SENSOR VDDIO 1.8V	
    if (sgpio_ctrl(sgpios, CAM1_SENSOR_CORE_EN, 0) < 0)	rc = -EIO;	// SENSOR DVDD 1.2V
    if (svreg_ctrl(svregs, CAMV_A_2P8V, 0) < 0)	rc = -EIO;	// SENSOR AVDD 2.8V
    mdelay(5);

    if (sgpio_ctrl(sgpios, CAM1_SYS_EN, 0) < 0)	rc = -EIO;	// VDD SYS 2.8V
    if (sgpio_ctrl(sgpios, CAM1_RAM_EN, 0) < 0)	rc = -EIO;	// VDD RAM 1.8V
    if (svreg_ctrl(svregs, CAMV_IO_1P8V, 0) < 0)	rc = -EIO;	// HOST IO 1.8V
    mdelay(5);

    if (sgpio_ctrl(sgpios, CAM1_ISP_CORE_EN, 0) < 0)	rc = -EIO;	// CORE 1.2V
    
    svreg_release(svregs, CAMV_MAX);
    sgpio_release(sgpios, CAMIO_MAX);
    
    SKYCDBG("%s X (%d)\n", __func__, rc);
    return rc;
}
#endif

#ifdef F_FW_UPDATE
#define CE1502_FW_INFO_W_CMD 0xF2
#define CE1502_FW_DATA_W_CMD 0xF4
#define CE1502_FW_DATA_WP_CMD 0xF3
#endif
static int32_t ce1502_update_fw(struct msm_sensor_ctrl_t *s_ctrl)
{
#define CE1502_NUM_CFG_CMD 4
#define CE1502_NUM_UPDATE_DATA 129
#define CE1502_PACKETS_IN_TABLE 508


    int32_t rc = 0;
#ifdef F_FW_UPDATE
    uint32_t numPacket = 0;
    int fd = 0;

    uint8_t pFW[CE1502_NUM_UPDATE_DATA+1];
    uint32_t i = 0;
    uint8_t rdata = 0xC0;
    uint8_t *pcmd = &pFW[0];
    uint8_t *pdata = &pFW[1];

    mm_segment_t old_fs; 

    old_fs = get_fs();
    set_fs(KERNEL_DS); 
    
    fd = sys_open(CE1502_UPLOADER_INFO_F, O_RDONLY, 0);
    if (fd < 0) {
        SKYCERR("%s: Can not open %s\n", __func__, CE1502_UPLOADER_INFO_F);
        goto fw_update_fail;
    }
      
    if (sys_read(fd, (char *)pdata, CE1502_NUM_CFG_CMD) != CE1502_NUM_CFG_CMD) {
        SKYCERR("%s: Can not read %s\n", __func__, CE1502_UPLOADER_INFO_F);
        sys_close(fd);
        goto fw_update_fail;
    }
    
    numPacket = (*(pdata+1) & 0xFF) << 8;
    numPacket |= *pdata & 0xFF;

    SKYCDBG("%s start : number of uploader packets is 0x%x\n",__func__, numPacket);


    *pcmd= CE1502_FW_INFO_W_CMD;
    //	rc = ce1502_cmd(s_ctrl, cmd, pFW, CE1502_NUM_CFG_CMD);
    rc = msm_camera_i2c_txdata(s_ctrl->sensor_i2c_client, pFW, CE1502_NUM_CFG_CMD+1);
    if (rc < 0)
    {
        SKYCERR("%s : uploader configs write ERROR 0x%x, 0x%x, 0x%x, 0x%x, 0x%x!!!\n",__func__, pFW[0], pFW[1], pFW[2], pFW[3], pFW[4]);
        sys_close(fd);
        goto fw_update_fail;
    }
    sys_close(fd);
    SKYCDBG("%s : fw uploader info write OK !!!!\n",__func__);
    /////////////////////////////////////////////////////////////////////////////////

    fd = sys_open(CE1502_UPLOADER_BIN_F, O_RDONLY, 0);
    
    if (fd < 0) {
        SKYCERR("%s: Can not open %s\n", __func__, CE1502_UPLOADER_BIN_F);
        goto fw_update_fail;
    }

    for(i = 0; i < numPacket; i++)
    {      
        if (sys_read(fd, (char *)pdata, CE1502_NUM_UPDATE_DATA) != CE1502_NUM_UPDATE_DATA) {
            SKYCERR("%s: Can not read %s : %d packet \n", __func__, CE1502_UPLOADER_BIN_F, i);
            sys_close(fd);
            goto fw_update_fail;
        }
        
        *pcmd= CE1502_FW_DATA_W_CMD;
        //	rc = ce1502_cmd(s_ctrl, cmd, pFW, CE1502_NUM_CFG_CMD);
        rc = msm_camera_i2c_txdata(s_ctrl->sensor_i2c_client, pFW, CE1502_NUM_UPDATE_DATA+1);
        if (rc < 0)
        {
            SKYCERR("%s : uploader packet %d write ERROR !!!\n",__func__, i);
            sys_close(fd);
            goto fw_update_fail;
        }
        if(*pcmd == CE1502_FW_DATA_WP_CMD)
        {
            rc = ce1502_read(s_ctrl, &rdata, 1);
            if(rdata != 0)
            {
                SKYCERR("%s : uploader packet %d write ERROR [0xF3 = 0x%x]!!!\n",__func__, i, rdata);
                goto fw_update_fail;
            }
        }
    }
    sys_close(fd);
    SKYCDBG("%s : fw uploader data %d packets write OK !!!\n",__func__, i);
 
    msleep(5);
	
    rc = ce1502_poll(s_ctrl, 0xF5, 0x05, 10, 500);
    if (rc < 0)
    {
        SKYCERR("%s : uploader polling ERROR !!!\n",__func__);
        goto fw_update_fail;
    }
   /////////////////////////////////////////////////////////////////////////////////
   
    fd = sys_open(CE1502_FW_INFO_F, O_RDONLY, 0);
    if (fd < 0) {
        SKYCERR("%s: Can not open %s\n", __func__, CE1502_FW_INFO_F);
        goto fw_update_fail;
    }
      
    if (sys_read(fd, (char *)pdata, CE1502_NUM_CFG_CMD) != CE1502_NUM_CFG_CMD) {
        SKYCERR("%s: Can not read %s\n", __func__, CE1502_FW_INFO_F);
        sys_close(fd);
        goto fw_update_fail;
    }
    
    numPacket = (*(pdata+1) & 0xFF) << 8;
    numPacket |= *pdata & 0xFF;

    SKYCDBG("%s start : number of fw packets is 0x%x\n",__func__, numPacket);


    *pcmd= CE1502_FW_INFO_W_CMD;
    //	rc = ce1502_cmd(s_ctrl, cmd, pFW, CE1502_NUM_CFG_CMD);
    rc = msm_camera_i2c_txdata(s_ctrl->sensor_i2c_client, pFW, CE1502_NUM_CFG_CMD+1);
    if (rc < 0)
    {
        SKYCERR("%s : FW configs write ERROR 0x%x, 0x%x, 0x%x, 0x%x, 0x%x!!!\n",__func__, pFW[0], pFW[1], pFW[2], pFW[3], pFW[4]);
        sys_close(fd);
        goto fw_update_fail;
    }
    sys_close(fd);
    SKYCDBG("%s : fw info write OK !!!!\n",__func__);
    /////////////////////////////////////////////////////////////////////////////////

    fd = sys_open(CE1502_FW_BIN_F, O_RDONLY, 0);
    
    if (fd < 0) {
        SKYCERR("%s: Can not open %s\n", __func__, CE1502_FW_BIN_F);
        goto fw_update_fail;
    }

    for(i = 0; i < numPacket; i++)
    {      
        if (sys_read(fd, (char *)pdata, CE1502_NUM_UPDATE_DATA) != CE1502_NUM_UPDATE_DATA) {
            SKYCERR("%s: Can not read %s : %d packet \n", __func__, CE1502_FW_BIN_F, i);
            sys_close(fd);
            goto fw_update_fail;
        }
        
        *pcmd= CE1502_FW_DATA_W_CMD;
        //	rc = ce1502_cmd(s_ctrl, cmd, pFW, CE1502_NUM_CFG_CMD);
        rc = msm_camera_i2c_txdata(s_ctrl->sensor_i2c_client, pFW, CE1502_NUM_UPDATE_DATA+1);
        if (rc < 0)
        {
            SKYCERR("%s : fw packet %d write ERROR !!!\n",__func__, i);
            sys_close(fd);
            goto fw_update_fail;
        }
        if(*pcmd == CE1502_FW_DATA_WP_CMD)
        {
            rc = ce1502_read(s_ctrl, &rdata, 1);
            if(rdata != 0)
            {
                SKYCERR("%s : fw packet %d write ERROR [0xF3 = 0x%x]!!!\n",__func__, i, rdata);
                goto fw_update_fail;
            }
        }
    }
    sys_close(fd);
    SKYCDBG("%s : fw data %d packets write OK !!!\n",__func__, i);
    /////////////////////////////////////////////////////////////////////////////////
    set_fs(old_fs);

    rc = ce1502_poll(s_ctrl, 0xF5, 0x06, 10, 3000);
    if (rc < 0)
    {
        SKYCERR("%s : fw data upgrade ERROR !!!\n",__func__);
        goto fw_update_fail;
    }

	return rc;	

fw_update_fail:
    set_fs(old_fs);
    SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);	
#endif
    return rc;
}


static int32_t ce1502_update_fw_boot(struct msm_sensor_ctrl_t *s_ctrl, const struct msm_camera_sensor_info *info)
{
	int32_t rc = 0;
	unsigned char data;
	unsigned char rdata[4];
	
#ifdef F_CE1502_POWER	
	rc = ce1502_sensor_power_up(s_ctrl);
	if (rc)
	{		
		SKYCERR(" ce1502_power failed rc=%d\n",rc);
		goto update_fw_boot_done; 
	}
#endif

       SKYCDBG("%s : Boot Start F0 for fw update !!\n", __func__);

	rc = ce1502_cmd(s_ctrl, 0xF0, NULL, 0);
	if (rc < 0)
	{
            goto update_fw_boot_done;             
	}
	msleep(400);

	data = 0x00;
	rc = ce1502_cmd(s_ctrl, 0x00, &data, 1);
	if (rc < 0)
	{
		SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
//		goto update_fw_boot_done; 
              goto update_fw_boot;
	}


	rc = ce1502_read(s_ctrl, rdata, 4);
	if (rc < 0)
	{
		SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
		goto update_fw_boot; 
	}
	ce1502_ver.fw_minor_ver = rdata[0] & 0xFF;	  
	ce1502_ver.fw_major_ver = rdata[1] & 0xFF;
	ce1502_ver.prm_minor_ver = rdata[2] & 0xFF;	  
	ce1502_ver.prm_major_ver = rdata[3] & 0xFF;  
	//SKYCDBG("%s : FW minor version=0x%x, FW major viersion=0x%x\n",__func__, ce1502_ver.fw_minor_ver, ce1502_ver.fw_major_ver);
	printk(KERN_INFO "%s : FW minor version=0x%x, FW major viersion=0x%x\n",__func__, ce1502_ver.fw_minor_ver, ce1502_ver.fw_major_ver);

#if 0   // check AF ver.
		data = 0x05;
		rc = ce1502_cmd(s_ctrl, 0x00, &data, 1);
		if (rc < 0)
		{
			SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
			goto update_fw_boot; 
		}

		rc = ce1502_read(s_ctrl, rdata, 4);
		if (rc < 0)
		{
			SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
			goto update_fw_boot_done; 
		}
		SKYCDBG("%s : AF version[0]=0x%x, version[1]=0x%x, version[2]=0x%x, version[3]= 0x%x\n",__func__, rdata[0]&0xFF, rdata[1]&0xFF, rdata[2]&0xFF, rdata[3]&0xFF);
#endif

	if ((ce1502_ver.fw_major_ver == CE1502_FW_MAJOR_VER) &&
		(ce1502_ver.fw_minor_ver == CE1502_FW_MINOR_VER)) {
		//SKYCDBG("%s : PRM minor version=0x%x, PRM major viersion=0x%x\n",__func__, ce1502_ver.prm_minor_ver, ce1502_ver.prm_major_ver);
		printk(KERN_INFO "%s : PRM minor version=0x%x, PRM major viersion=0x%x\n",__func__, ce1502_ver.prm_minor_ver, ce1502_ver.prm_major_ver);

		if ((ce1502_ver.prm_major_ver == CE1502_PRM_MAJOR_VER) &&
			(ce1502_ver.prm_minor_ver == CE1502_PRM_MINOR_VER)) {						
			//SKYCDBG("%s : PRM minor version=0x%x, PRM major viersion=0x%x\n",__func__, CE1502_PRM_MINOR_VER, CE1502_PRM_MAJOR_VER);
			printk(KERN_INFO "%s : PRM minor version=0x%x, PRM major viersion=0x%x\n",__func__, CE1502_PRM_MINOR_VER, CE1502_PRM_MAJOR_VER);
			goto update_fw_boot_done;
		}
	}
    

update_fw_boot:

#ifdef F_CE1502_POWER	
	rc = ce1502_sensor_power_down(s_ctrl);
	if (rc) {
		SKYCERR(" ce1502_power failed rc=%d\n",rc);		
	}
	rc = ce1502_sensor_power_up(s_ctrl);
	if (rc) {		
		SKYCERR(" ce1502_power failed rc=%d\n",rc);
		goto update_fw_boot_done; 
	}
#endif
	rc = ce1502_update_fw(s_ctrl);

update_fw_boot_done:
#ifdef F_CE1502_POWER	
	SKYCDBG(" ce1502_sensor_release E\n");	
	rc = ce1502_sensor_power_down(s_ctrl);
	if (rc) {
		SKYCERR(" ce1502_power failed rc=%d\n",rc);		
	}
#endif
	
	return rc;
}

#ifdef CONFIG_PANTECH_CAMERA_IRQ
uint32_t ce1502_readirq(struct msm_sensor_ctrl_t *s_ctrl)
{
#if 1
    uint8_t data_buf[10];
    int32_t rc = 0;

    rc = ce1502_cmd_read(s_ctrl, 0xD0, data_buf, 8);

    if (data_buf[2] & 0x04) // bit 2, AF done
    {
        ce1502_irq_stat = 2;
    }
    else if (data_buf[3] & 0x20) // bit 5, AF trigger
    {
        if(ce1502_irq_stat != 1)
        {
            ce1502_irq_stat = 1;
            ce1502_42_command(s_ctrl, 0x00);
        }
    }
#endif

	/* need lock */
	return ce1502_irq_stat;
}

static int ce1502_get_frame_info(struct msm_sensor_ctrl_t *s_ctrl, void __user *argp, int8_t * f_info)
{
#ifdef CONFIG_PNTECH_CAMERA_OJT
    uint8_t data_buf[10];
    int32_t xleft=0, xright=0, ytop=0, ybottom=0;
    int32_t rc = 0;
    int32_t width, height;
    int16_t * p_obj;
    int32_t p_width, p_height;
    int32_t offset_2y=0;
    int32_t offset_y=0;
#endif

    *f_info = ce1502_readirq(s_ctrl);
//  SKYCDBG("%s ce1502_irq_stat=%d\n",__func__, *f_info);

#ifdef CONFIG_PNTECH_CAMERA_OJT
    p_obj = (int16_t *)(f_info+2);
    p_width = *p_obj++;
    p_height = *p_obj;

    rc = ce1502_cmd_read(s_ctrl, 0x51, data_buf, 9);
    *(f_info+1) = data_buf[0];
    if(data_buf[0] == 2)
    {
        p_obj = (int16_t *)(f_info+2);
        xleft = data_buf[1] | (data_buf[2] << 8);
        ytop = data_buf[3] | (data_buf[4] << 8);
        xright = data_buf[5] | (data_buf[6] << 8);
        ybottom = data_buf[7] | (data_buf[8] << 8);

        width = ce1502_dimensions[sensor_mode].x_output;
        height = ce1502_dimensions[sensor_mode].y_output;

        offset_2y = height - (width*p_height)/p_width;
        height = height - offset_2y;
        offset_y = offset_2y/2;
        ytop = ytop - offset_y;
        ybottom = ybottom - offset_y;
        *(p_obj) = ((xleft*2000)/width) & 0xffff;
        *(p_obj+1) = ((ytop*2000)/height) & 0xffff;
        *(p_obj+2) = ((xright*2000)/width) & 0xffff;
        *(p_obj+3) = ((ybottom*2000)/height) & 0xffff;
//        SKYCDBG("%s state=%d :: xleft=%d, ytop=%d, xright=%d , ybottom=%d\n",__func__, data_buf[0], *(p_obj), *(p_obj+1), *(p_obj+2), *(p_obj+3));
    }
#endif

    return 0;
}

#endif

#if 1 //def F_PANTECH_CAMERA_FIX_CFG_AE_AWB_LOCK
static int32_t ce1502_set_aec_lock(struct msm_sensor_ctrl_t *s_ctrl ,int8_t is_lock)
{
    int32_t rc = 0;
    uint8_t data_buf[4];

    if(sensor_mode == 0) // test
        return 0;    

    if(is_lock)
        aec_awb_lock |= 0x01;
    else
        aec_awb_lock &= ~0x01;

    data_buf[0] = aec_awb_lock;
    rc = ce1502_cmd(s_ctrl, 0x11, data_buf, 1);
    
    SKYCDBG("%s end, current ce1502_set_aec_lock(%d)=0x%x\n",__func__, is_lock, aec_awb_lock);

    return rc;
}

static int32_t ce1502_set_awb_lock(struct msm_sensor_ctrl_t *s_ctrl ,int8_t is_lock)
{
    int32_t rc = 0;
    uint8_t data_buf[4];

    if(sensor_mode == 0) // test
        return 0;    

    if(is_lock)
        aec_awb_lock |= 0x10;
    else
        aec_awb_lock &= ~0x10;

    data_buf[0] = aec_awb_lock;
    rc = ce1502_cmd(s_ctrl, 0x11, data_buf, 1);
    
    SKYCDBG("%s end, current aec_awb_lock(%d)=0x%x\n",__func__, is_lock, aec_awb_lock);

    return rc;
}
#endif

#ifdef CONFIG_PNTECH_CAMERA_OJT
static int32_t ce1502_set_object_tracking(struct msm_sensor_ctrl_t *s_ctrl, int32_t ojt)
{
    uint8_t data_buf[10];

    int32_t rc = 0;
    int32_t i = 0;
    int32_t x_c, y_c;

    SKYCDBG("%s E\n",__func__);

    if(!(sensor_mode > 0 && sensor_mode < 4))
        goto set_ojt_end;

    if(!(x1|y1|x2|y2)) {
        goto set_ojt_end;
    }

    if(ojt)
    {        
        ce1502_42_command(s_ctrl, 0x00);
        
#if 0        
        caf_b_ojt = continuous_af_mode;
        rc = ce1502_set_continuous_af(s_ctrl, 1);
#endif

#ifdef CONFIG_PNTECH_CAMERA_OJT_TEST            
        data_buf[0] = 0x00;
        data_buf[1] = 0x01;
        data_buf[2] = 0x00;
        data_buf[3] = 0x00;
        data_buf[4] = 0x00;
        data_buf[5] = 0x00;
        data_buf[6] = 0x00;
        data_buf[7] = 0x00;
        data_buf[8] = 0x00;
        data_buf[9] = 0x00;	
        rc = ce1502_cmd(s_ctrl, 0x4A, data_buf, 10);
#endif

        x_c = (x1+x2)/2;
        y_c = (y1+y2)/2;
        SKYCDBG("%s  x_c = %d, y_c = %d\n",__func__, x_c, y_c);
        data_buf[0] = 0x04;
        data_buf[1] = 0x03; // af & ae interlock
        data_buf[2] = x_c & 0xff;
        data_buf[3] = (x_c >> 8) & 0xff;
        data_buf[4] = y_c & 0xff;
        data_buf[5] = (y_c >> 8) & 0xff;
        data_buf[6] = 0x00;
        data_buf[7] = 0x00;
        data_buf[8] = 0x00;
        data_buf[9] = 0x00;	
        rc = ce1502_cmd(s_ctrl, 0x41, data_buf, 10);
        if (rc < 0)
        {
            SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
            return rc;
        }	

        ce1502_42_command(s_ctrl, 0x04);

        do
        {
            mdelay(10);
            rc = ce1502_cmd_read(s_ctrl, 0x51, data_buf, 9);
        }while(!(data_buf[0] == 2 ||data_buf[0] == 3) && i++ < 20);

        if(data_buf[0] == 3)
        {
            ce1502_42_command(s_ctrl, 0x00);
        }
    }

set_ojt_end:

    SKYCDBG("%s end\n",__func__);

    return rc;	
}
#endif

#if 1
static int32_t ce1502_set_area_interlock(struct msm_sensor_ctrl_t *s_ctrl, int32_t af_interlock, int32_t ae_interlock)
{
    uint8_t data_buf[10];

    int32_t rc = 0;
    int32_t interlock = 0;

    SKYCDBG("%s E\n",__func__);

    if(!(sensor_mode > 0 && sensor_mode < 4))
        goto set_rect_end;

    if(!(x1|y1|x2|y2)) {
        goto set_rect_end;
    }

    SKYCDBG("%s  xleft = %d, ytop = %d, xright = %d, ybottom = %d\n",__func__, x1, y1, x2, y2);

    if(af_interlock)
    {
        interlock = 0x20;
        ce1502_irq_stat = 0;
    }
    if(ae_interlock)
    {
#if 1 //def F_PANTECH_CAMERA_FIX_CFG_AE_AWB_LOCK
        if(aec_awb_lock & 0x01)
            ce1502_set_aec_lock(s_ctrl, 0);
#endif
        interlock = 0x10;
    }

    data_buf[0] = interlock;
    data_buf[1] = 0xB2; //0x03;
    data_buf[2] = x1 & 0xff;
    data_buf[3] = (x1 >> 8) & 0xff;
    data_buf[4] = y1 & 0xff;
    data_buf[5] = (y1 >> 8) & 0xff;
    data_buf[6] = x2 & 0xff;
    data_buf[7] = (x2 >> 8) & 0xff;
    data_buf[8] = y2 & 0xff;
    data_buf[9] = (y2 >> 8) & 0xff;
	
    rc = ce1502_cmd(s_ctrl, 0x41, data_buf, 10);
    if (rc < 0)
    {
        SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
        return rc;
    }	

    ce1502_42_command(s_ctrl, 0x05);

set_rect_end:
    
#if 0 //def CONFIG_PANTECH_CAMERA_IRQ // AF-T state check
    if(af_interlock)
    {
        rc = ce1502_cmd_read(s_ctrl, 0x2D, data_buf, 2);
        if (rc < 0)
        {
            SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
            return rc;
        }

        if(data_buf[0] == 0x01)
        {
            ce1502_irq_stat = 1;	// first trigger start
        }		
    }		
#endif


    SKYCDBG("%s end\n",__func__);

    return rc;	
}

static int32_t ce1502_set_metering_area(struct msm_sensor_ctrl_t *s_ctrl, int32_t metering_area, int8_t * f_info)
{
#define METERING_AREA_SIZE 64
#define METERING_AREA_SIZE_1 63
#define METERING_AREA_SIZE_HALF_1 31
//    uint8_t data_buf[10];
    int32_t x_c, y_c, xleft, xright, ytop, ybottom;
    int32_t width, height, height2;

    int16_t * p_obj;
    int32_t p_width, p_height;
    int32_t offset_2y=0;
    int32_t offset_y=0;
    
    int32_t rc = 0;

#ifdef CONFIG_PNTECH_CAMERA_OJT
    if(current_ojt == 1)
        return rc;
#endif

    SKYCDBG("%s  metering_area = %x\n",__func__, metering_area);

    if(!(sensor_mode > 0 && sensor_mode < 4))
        return rc;

    if (metering_area == 0) {
        ce1502_42_command(s_ctrl, 0x00);

        return rc;
    }

    p_obj = (int16_t *)(f_info);
    p_width = *p_obj++;
    p_height = *p_obj;
    
    width = ce1502_dimensions[sensor_mode].x_output;
    height = ce1502_dimensions[sensor_mode].y_output;

    if(p_width > 0)
        offset_2y = height - (width*p_height)/p_width;
    height2 = height - offset_2y;
    offset_y = offset_2y/2;
    
    x_c = (int32_t)((metering_area & 0xffff0000) >> 16);
    x_c = (x_c*width)/2000;
    y_c = (int32_t)(metering_area & 0xffff);
    y_c = (y_c*height2)/2000;
    y_c = y_c + offset_y;
    
    xleft = x_c - METERING_AREA_SIZE_HALF_1;
    if(xleft < 0)
        xleft = 0;
    if(xleft > width-METERING_AREA_SIZE)
        xleft = width-METERING_AREA_SIZE;

    ytop = y_c - METERING_AREA_SIZE_HALF_1;
    if(ytop < 0)
        ytop = 0;
    if(ytop > height-METERING_AREA_SIZE)
        ytop = height-METERING_AREA_SIZE;

    xright = xleft + METERING_AREA_SIZE_1;
    ybottom = ytop + METERING_AREA_SIZE_1;
    SKYCDBG("%s  p_width = %d, p_height = %d, height = %d, height2 = %d, offset_y = %d\n",__func__, p_width, p_height, height, height2, offset_y);
    SKYCDBG("%s  xleft = %d, ytop = %d, xright = %d, ybottom = %d\n",__func__, xleft, ytop, xright, ybottom);

    x1 = xleft;
    x2 = xright;
    y1 = ytop;
    y2 = ybottom;

    rc = ce1502_set_area_interlock(s_ctrl, 0 ,1);

    SKYCDBG("%s end\n",__func__);

    return rc;	
}

static int32_t ce1502_set_focus_rect(struct msm_sensor_ctrl_t *s_ctrl, int32_t focus_rect, int8_t * f_info)
{
    int32_t focus_rect_size = 128;
    int32_t focus_rect_size_1 = 127;
    int32_t focus_rect_size_half_1 = 63;
//    uint8_t data_buf[10];
    int32_t x_c, y_c, xleft, xright, ytop, ybottom;
    int32_t width, height, height2;

    int16_t * p_obj;
    int32_t p_width, p_height;
    int32_t offset_2y=0;
    int32_t offset_y=0;
    
    int32_t rc = 0;

    SKYCDBG("%s  focus_rect = %x\n",__func__, focus_rect);

    if(!(sensor_mode > 0 && sensor_mode < 4))
        return rc;

    if (focus_rect == 0) {
        ce1502_42_command(s_ctrl, 0x00);

        return rc;
    }

    p_obj = (int16_t *)(f_info);
    p_width = *p_obj++;
    p_height = *p_obj;

    width = ce1502_dimensions[sensor_mode].x_output;
    height = ce1502_dimensions[sensor_mode].y_output;

    if(height == CE1502_ZSL_SIZE_HEIGHT)
    {
        focus_rect_size = 400;
        focus_rect_size_1 = 399;
        focus_rect_size_half_1 = 199;
    }

    if(p_width > 0)
        offset_2y = height - (width*p_height)/p_width;
    height2 = height - offset_2y;
    offset_y = offset_2y/2;
        
    x_c = (int32_t)((focus_rect & 0xffff0000) >> 16);
    x_c = (x_c*width)/2000;
    y_c = (int32_t)(focus_rect & 0xffff);
    y_c = (y_c*height2)/2000;
    y_c = y_c + offset_y;

    xleft = x_c - focus_rect_size_half_1;
    if(xleft < 0)
        xleft = 0;
    if(xleft > width-focus_rect_size)
        xleft = width-focus_rect_size;

    ytop = y_c - focus_rect_size_half_1;
    if(ytop < 0)
        ytop = 0;
    if(ytop > height-focus_rect_size)
        ytop = height-focus_rect_size;

    xright = xleft + focus_rect_size_1;
    ybottom = ytop + focus_rect_size_1;
    SKYCDBG("%s  p_width = %d, p_height = %d, height = %d, height2 = %d, offset_y = %d\n",__func__, p_width, p_height, height, height2, offset_y);
    SKYCDBG("%s  xleft = %d, ytop = %d, xright = %d, ybottom = %d\n",__func__, xleft, ytop, xright, ybottom);

    x1 = xleft;
    x2 = xright;
    y1 = ytop;
    y2 = ybottom;

#ifdef CONFIG_PNTECH_CAMERA_OJT
#if 0 //def CONFIG_PNTECH_CAMERA_OJT_TEST
    current_ojt = 1;
#endif
    if(current_ojt == 1)
    {
        ce1502_set_object_tracking(s_ctrl, current_ojt);
        return 0;
    }    
#endif

    if(continuous_af_mode == 0) // AF-T state check
        rc = ce1502_set_area_interlock(s_ctrl, 1, 0);
    
    SKYCDBG("%s end\n",__func__);

    return rc;	
}
#else
static int32_t ce1502_set_focus_rect(struct msm_sensor_ctrl_t *s_ctrl, int32_t focus_rect)
{
#define FOCUS_RECT_SIZE 64
#define FOCUS_RECT_SIZE_1 63
#define FOCUS_RECT_SIZE_HALF_1 31
    uint8_t data_buf[10];
    int32_t x_c, y_c, xleft, xright, ytop, ybottom;
    int32_t width, height;
    int32_t is_af_t = 0;

    int32_t rc = 0;
    uint8_t read_data =0;

    SKYCDBG("%s  focus_rect = %x\n",__func__, focus_rect);

    if(!(sensor_mode > 0 && sensor_mode < 3))
        goto set_rect_end;

    if (focus_rect == 0) {
        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x42, data_buf, 1);

        return rc;
    }

    rc = ce1502_cmd_read(s_ctrl, 0x24, &read_data, 1);
    if (rc < 0)
    {
        SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
        return rc;
    }

    if(read_data & 0x01)
    {
#if 1 // AF-T state check
    	rc = ce1502_cmd_read(s_ctrl, 0x2D, data_buf, 2);
    	if (rc < 0)
    	{
    		SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
    		return rc;
    	}

    	if(data_buf[0] == 0x01)
    	{
            is_af_t  = 1;
            data_buf[0] = 0x02;
            rc = ce1502_cmd(s_ctrl, 0x2C, data_buf, 1);
            if (rc < 0)
            {
                SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
                return rc;
            }	
    	}		
#endif
        rc = ce1502_cmd(s_ctrl, 0x35, 0, 0);	// Lens Stop	
        rc = ce1502_poll_bit(s_ctrl, 0x24, 10, 400);
        if (rc < 0)
        {
            SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
            return rc;
        }
#if 1 // AF-T state check
    	if(is_af_t  == 1)
    	{
            data_buf[0] = 0x01;
            rc = ce1502_cmd(s_ctrl, 0x2C, data_buf, 1);
            if (rc < 0)
            {
                SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
                return rc;
            }	
	}
#endif
    }

    width = ce1502_dimensions[sensor_mode].x_output;
    height = ce1502_dimensions[sensor_mode].y_output;

    x_c = (int32_t)((focus_rect & 0xffff0000) >> 16);
    x_c = (x_c*width)/2000;
    y_c = (int32_t)(focus_rect & 0xffff);
    y_c = (y_c*height)/2000;

    xleft = x_c - FOCUS_RECT_SIZE_HALF_1;
    if(xleft < 0)
        xleft = 0;
    if(xleft > width-FOCUS_RECT_SIZE)
        xleft = width-FOCUS_RECT_SIZE;

    ytop = y_c - FOCUS_RECT_SIZE_HALF_1;
    if(ytop < 0)
        ytop = 0;
    if(ytop > height-FOCUS_RECT_SIZE)
        ytop = height-FOCUS_RECT_SIZE;

    xright = xleft + FOCUS_RECT_SIZE_1;
    ybottom = ytop + FOCUS_RECT_SIZE_1;

    SKYCDBG("%s  xleft = %d, ytop = %d, xright = %d, ybottom = %d\n",__func__, xleft, ytop, xright, ybottom);

    if(is_af_t == 1)
    {
        x1 = xleft;
        x2 = xright;
        y1 = ytop;
        y2 = ybottom;
        goto set_rect_end;
    }

    data_buf[0] = 0x05;
    data_buf[1] = 0x03;
    data_buf[2] = xleft & 0xff;
    data_buf[3] = (xleft >> 8) & 0xff;
    data_buf[4] = ytop & 0xff;
    data_buf[5] = (ytop >> 8) & 0xff;
    data_buf[6] = xright & 0xff;
    data_buf[7] = (xright >> 8) & 0xff;
    data_buf[8] = ybottom & 0xff;
    data_buf[9] = (ybottom >> 8) & 0xff;
	
    rc = ce1502_cmd(s_ctrl, 0x41, data_buf, 10);
    if (rc < 0)
    {
        SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
        return rc;
    }	

    data_buf[0] = 0x05;
    rc = ce1502_cmd(s_ctrl, 0x42, data_buf, 1);
    rc = ce1502_poll(s_ctrl, 0x43, 0x05, 10, 1000);
    if (rc < 0)
    {
        SKYCERR("%s : uploader polling ERROR !!!\n",__func__);
        return rc;
    }

set_rect_end:
    
#if 0 //def CONFIG_PANTECH_CAMERA//IRQ // AF-T state check
    rc = ce1502_cmd_read(0x2D, data_buf, 2);
    if (rc < 0)
    {
        SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
        return rc;
    }

    if(data_buf[0] == 0x01)
    {
        ce1502_irq_stat = 1;	// first trigger start
    }		
#endif


    SKYCDBG("%s end\n",__func__);

    return rc;	
}
#endif


static int32_t ce1502_sensor_set_auto_focus(struct msm_sensor_ctrl_t *s_ctrl, int8_t autofocus)
{
    int32_t rc = 0;
    uint8_t read_data =0;
    uint8_t data_buf[10];

    if(!(sensor_mode > 0 && sensor_mode < 4)) // test
        return 0;

#ifdef CONFIG_PNTECH_CAMERA_OJT
    if(current_ojt == 1)
        return 0;        
#endif

    SKYCDBG("%s  auto_focus = %d\n",__func__, autofocus);
    if ((autofocus < 0) || (autofocus > 6))
    {
        SKYCERR("%s FAIL!!! return~~  autofocus = %d\n",__func__,autofocus);
        return 0;//-EINVAL;
    }
    if(autofocus == 6)  //cancel AF
    {
        rc = ce1502_lens_stop(s_ctrl);
#if 1 // temp // AF-T state check  
    	if(continuous_af_mode == 2)
    	{
            data_buf[0] = 0x01;
            rc = ce1502_cmd(s_ctrl, 0x2C, data_buf, 1);
            if (rc < 0)
            {
                SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
                return rc;
            }	
    	}		
#endif	
        return rc;
    }
    
#if 1 // AF-T state check
    if(continuous_af_mode != 0)
    {
        rc = ce1502_set_area_interlock(s_ctrl, 1, 0);    
        return rc;
    }
#else
    rc = ce1502_cmd_read(s_ctrl, 0x2D, data_buf, 2);
    if (rc < 0)
    {
        SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
        return rc;
    }

    if(data_buf[0] == 0x01)
    {
        rc = ce1502_set_area_interlock(s_ctrl);    
        return rc;
    }
#endif

    SKYCDBG("%s START~ autofocus mode = %d\n",__func__, autofocus);
    if(autofocus == 4)
        return 0;

    rc = ce1502_lens_stop(s_ctrl);

#if 1 // temp // AF-T state check  
    	if(continuous_af_mode == 2)
    	{
            data_buf[0] = 0x01;
            rc = ce1502_cmd(s_ctrl, 0x2C, data_buf, 1);
            if (rc < 0)
            {
                SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
                return rc;
            }		
    	}		
#endif

    rc = ce1502_cmd_read(s_ctrl, 0x43, &read_data, 1);
    if(read_data == 0x05){
        goto start_af;
    }

    switch(autofocus)
    {
    case 1:	// MACRO
        ce1502_42_command(s_ctrl, 0x00);

        data_buf[0] = 0x01;
        rc = ce1502_cmd(s_ctrl, 0x20, data_buf, 1);
        rc = ce1502_poll_bit(s_ctrl, 0x24, 10, 400);
        if (rc < 0)
        {
            SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
            return rc;
        }		


//    default:	// NORMAL
    case 2:	// AUTO
    case 0:	// NORMAL
        ce1502_42_command(s_ctrl, 0x00);

        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x20, data_buf, 1);        
        rc = ce1502_poll_bit(s_ctrl, 0x24, 10, 400);
        if (rc < 0)
        {
            SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
            return rc;
        }		
        break;
    default:
        return 0;
        break;
    }

start_af:
    rc = ce1502_cmd(s_ctrl, 0x23, 0, 0);

    SKYCDBG("%s END~ autofocus mode = %d\n",__func__, autofocus);

    return rc;
}

static int32_t ce1502_sensor_set_focus_mode(struct msm_sensor_ctrl_t *s_ctrl, int8_t focusmode)
{
	uint8_t data_buf[2];	
	//uint8_t read_data[2];
	int caf = 0;
	int rc = 0;
	if(!(sensor_mode > 0 && sensor_mode < 4)&&(sensor_mode!=10) ) // test
		return 0;
		
	SKYCDBG("%s start : focus_mode = %d\n",__func__, focusmode);
	
	//rc = ce1502_lens_stop2(s_ctrl);
	switch(focusmode)
	{
		case 3://AF-C
			caf = 1;
			break;
		case 5://AF-T
			caf = 2;
			break;	
		default:
			caf = 0;
			break;	
	}
	rc = ce1502_set_continuous_af(s_ctrl, caf);
	if (rc < 0)
	{
		SKYCERR("ERR:%s ce1502_set_continuous_af FAIL!!!rc=%d return~~\n", __func__, rc);
		return rc;
	}	
	if(focusmode==4)//infinity
	{
		data_buf[0] = 0x00;
		rc = ce1502_cmd(s_ctrl, 0x20, data_buf, 1); 	   
		rc = ce1502_poll_bit(s_ctrl, 0x24, 10, 400);
		if (rc < 0)
		{
			SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
			return rc;
		}				
	}
        
    SKYCDBG("%s end\n",__func__);
    return rc;
}

static int32_t ce1502_sensor_check_af(struct msm_sensor_ctrl_t *s_ctrl ,int8_t autofocus)
{
    uint8_t rdata = 0;
//    uint8_t data_buf[4];
    int32_t rc = 0;

    if(!(sensor_mode > 0 && sensor_mode < 4)) // test
        return 0;
    if(continuous_af_mode == 1)
        return 0;
    else if(continuous_af_mode == 2)
    {
        if(ce1502_irq_stat == 2)
            return 0;
        else
        {
            uint8_t data_buf[10];

            rc = ce1502_cmd_read(s_ctrl, 0xD0, data_buf, 8);

            if (data_buf[2] & 0x04) // bit 2, AF done
                return 0;
            else 
                return -1;
        }
    }
    rc = ce1502_cmd_read(s_ctrl, 0x24, &rdata, 1);
#if 0        
    SKYCDBG("%s: read = 0x%x\n", __func__, rdata);        
#endif
    if (rc < 0)
        return rc;
    
    if (!(rdata & 0x01))
        rc = 0;
    else
        rc = -1;

    return rc;
}

void ce1502_sensor_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{

#if 1 //F_PANTECH_CAMERA
SKYCDBG("%s: %d\n", __func__, __LINE__);
#endif

}

static int32_t ce1502_ZSL_config(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    uint8_t data_buf[4];
    uint8_t rdata = 0;
    int8_t prev_caf2;

    SKYCDBG("%s: START + \n", __func__);

#if 0 // test cts
    //	if(f_stop_capture == 1)
    {
        //		f_stop_capture = 0;

        rc = ce1502_poll(ce1502_saddr, 0x6C, 0x08, 10, 1000);
    if (rc < 0)
    {
            SKYCERR("%s : Preview Start polling ERROR !!!\n",__func__);
    	return rc;
    }
        goto preview_ok;
    }
#endif	

#if 1 // check isp mode
    rc = ce1502_cmd_read(s_ctrl, 0x6C, &rdata, 1);

    if((rdata > 0) && (rdata < 8))
    {
#if 1
        // stop Capture	
        rc = ce1502_cmd(s_ctrl, 0x75, 0, 0);
        if (rc < 0)
	{
            SKYCERR("ERR:%s Capture Stop command FAIL!!!rc=%d return~~\n", __func__, rc);
            return rc;
	}
#endif
        rc = ce1502_poll(s_ctrl, 0x6C, 0x00, 10, 100);
        if (rc < 0)
        {
            SKYCERR("%s : Capture Stop polling ERROR !!!\n",__func__);
            return rc;
        }		
    }
    else if(rdata == 8)
    {
        goto preview_ok;
    }
    else if(rdata == 9)
    {
        rc = ce1502_poll(s_ctrl, 0x6C, 0x08, 10, 100);
        if (rc < 0)
        {
            SKYCERR("%s : Preview Start polling ERROR !!!\n",__func__);
            return rc;
        }
        goto preview_ok;
    }
#endif
        
    SKYCDBG("%s : ZSL Preview Start CMD !!!\n",__func__);
#ifdef NEW_CAPTURE_FW
        data_buf[0] = 0x00;
    data_buf[1] = 0x05;
    rc = ce1502_cmd(s_ctrl, 0x05, data_buf, 2);
#endif

    data_buf[0] = 0x00;	//
    rc = ce1502_cmd(s_ctrl, 0x40, data_buf, 1);

    data_buf[0] = 0x21;	
#ifdef FULLSIZE_13P0
    data_buf[1] = 0x06;
#else
    data_buf[1] = 0x00;
#endif
    data_buf[2] = 0x00;
    data_buf[3] = 0x00;
    rc = ce1502_cmd(s_ctrl, 0x73, data_buf, 4);

#if 0 //def F_PANTECH_CAMERA_FIX_CFG_AE_AWB_LOCK
    if(aec_awb_lock == 0x02)
#endif        
    {
        // AE/AWB enable
        data_buf[0] = 0x00;	//
        rc = ce1502_cmd(s_ctrl, 0x11, data_buf, 1);
    }

    // ZSL Preview start (PREVIEW)
    data_buf[0] = 0x03;	//
    rc = ce1502_cmd(s_ctrl, 0x7D, data_buf, 1);
    rc = ce1502_poll(s_ctrl, 0x6C, 0x19, 10, 100);  // response 0x13? 0x19?
        if (rc < 0)
        {
        SKYCERR("%s : ZSL Preview Start polling ERROR !!!\n",__func__);
        	return rc;
        }	

preview_ok:
    
    mdelay(30); // test 04.13.

    sensor_mode = 3;
#if 1 // F_PANTECH_CAMERA_CFG_HDR
    ev_sft_mode = 0;   // 0:normal capture , 1: ev shift capture
#endif

    prev_caf2 = caf_b_ojt;
    ce1502_set_continuous_af(s_ctrl, continuous_af_mode);
    caf_b_ojt = prev_caf2;

    //	f_stop_capture = 0;	// test cts
        x1 = 0;
        x2 = 0;
        y1 = 0;
        y2 = 0;

    SKYCDBG("%s end rc = %d\n",__func__, rc);

    return rc;
}

static int32_t ce1502_1080p_config(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    uint8_t data_buf[4];
    uint8_t rdata = 0;
    int8_t prev_caf2;

    SKYCDBG("%s: START + \n", __func__);

#if 0 // test cts
    //	if(f_stop_capture == 1)
    {
        //		f_stop_capture = 0;

        rc = ce1502_poll(ce1502_saddr, 0x6C, 0x08, 10, 1000);
        if (rc < 0)
        {
            SKYCERR("%s : Preview Start polling ERROR !!!\n",__func__);
            return rc;
        }	
        goto preview_ok;
    }
#endif	

#if 1 // check isp mode
    rc = ce1502_cmd_read(s_ctrl, 0x6C, &rdata, 1);

    if((rdata > 0) && (rdata < 8))
    {
#if 1
        // stop Capture	
        rc = ce1502_cmd(s_ctrl, 0x75, 0, 0);
        if (rc < 0)
        {
            SKYCERR("ERR:%s Capture Stop command FAIL!!!rc=%d return~~\n", __func__, rc);
            return rc;
        }	
#endif
        rc = ce1502_poll(s_ctrl, 0x6C, 0x00, 10, 100);
        if (rc < 0)
        {
            SKYCERR("%s : Capture Stop polling ERROR !!!\n",__func__);
            return rc;
        }		
    }
    else if(rdata == 8)
    {
        goto preview_ok;
    }
    else if(rdata == 9)
    {
        rc = ce1502_poll(s_ctrl, 0x6C, 0x08, 10, 100);
        if (rc < 0)
        {
            SKYCERR("%s : Preview Start polling ERROR !!!\n",__func__);
            return rc;
        }	
        goto preview_ok;
    }
#endif

    SKYCDBG("%s : 1080p Preview Start CMD !!!\n",__func__);

#ifdef NEW_CAPTURE_FW
    data_buf[0] = 0x00;
    data_buf[1] = 0x05;
    rc = ce1502_cmd(s_ctrl, 0x05, data_buf, 2);
    data_buf[0] = 0x20;
    data_buf[1] = 0x01;
    rc = ce1502_cmd(s_ctrl, 0x05, data_buf, 2); // continuous capture
#endif

    data_buf[0] = 0x1F;	//1080p(1920x1088)
#ifdef FULLSIZE_13P0
    data_buf[1] = 0x0B;
#else
    data_buf[1] = 0x01;
#endif
    rc = ce1502_cmd(s_ctrl, 0x54, data_buf, 2);

    data_buf[0] = 0x00;	// 01
    rc = ce1502_cmd(s_ctrl, 0x40, data_buf, 1);

#if 0 //def F_PANTECH_CAMERA_FIX_CFG_AE_AWB_LOCK
    if(aec_awb_lock == 0x02)
#endif        
    {
        // AE/AWB enable
        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x11, data_buf, 1);
    }
    // Preview start (PREVIEW)
    data_buf[0] = 0x01;
    rc = ce1502_cmd(s_ctrl, 0x6B, data_buf, 1);

    rc = ce1502_poll(s_ctrl, 0x6C, 0x08, 10, 100);
    if (rc < 0)
    {
        SKYCERR("%s : Preview Start polling ERROR !!!\n",__func__);
        return rc;
    }	

preview_ok:
    mdelay(30); // test 04.13.

    sensor_mode = 2;
#if 1 // F_PANTECH_CAMERA_CFG_HDR
    ev_sft_mode = 0;   // 0:normal capture , 1: ev shift capture
#endif

    prev_caf2 = caf_b_ojt;
    ce1502_set_continuous_af(s_ctrl, continuous_af_mode);
    caf_b_ojt = prev_caf2;

    //	f_stop_capture = 0;	// test cts
    x1 = 0;
    x2 = 0;
    y1 = 0;
    y2 = 0;

    SKYCDBG("%s end rc = %d\n",__func__, rc);

    return rc;
}

static int32_t ce1502_video_config(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    uint8_t data_buf[10];
    uint8_t rdata = 0;
    int8_t prev_caf2;

    SKYCDBG("%s: START + \n", __func__);

#if 0 // test cts
    //	if(f_stop_capture == 1)
    {
        //		f_stop_capture = 0;

        rc = ce1502_poll(ce1502_saddr, 0x6C, 0x08, 10, 1000);
        if (rc < 0)
        {
            SKYCERR("%s : Preview Start polling ERROR !!!\n",__func__);
            return rc;
        }	
        goto preview_ok;
    }
#endif	

#if 1 // check isp mode
    rc = ce1502_cmd_read(s_ctrl, 0x6C, &rdata, 1);

    if((rdata > 0) && (rdata < 8))
    {
#if 1
        // stop Capture	
        rc = ce1502_cmd(s_ctrl, 0x75, 0, 0);
        if (rc < 0)
        {
            SKYCERR("ERR:%s Capture Stop command FAIL!!!rc=%d return~~\n", __func__, rc);
            return rc;
        }	
#endif
        rc = ce1502_poll(s_ctrl, 0x6C, 0x00, 10, 100);
        if (rc < 0)
        {
            SKYCERR("%s : Capture Stop polling ERROR !!!\n",__func__);
            return rc;
        }		
    }
    else if(rdata == 8)
    {
        goto preview_ok;
    }
    else if(rdata == 9)
    {
        rc = ce1502_poll(s_ctrl, 0x6C, 0x08, 10, 100);
        if (rc < 0)
        {
            SKYCERR("%s : Preview Start polling ERROR !!!\n",__func__);
            return rc;
        }	
        goto preview_ok;
    }
#endif


    SKYCDBG("%s : Preview Start CMD !!!\n",__func__);

#ifdef NEW_CAPTURE_FW
    data_buf[0] = 0x00;
    data_buf[1] = 0x05;
    rc = ce1502_cmd(s_ctrl, 0x05, data_buf, 2);
    data_buf[0] = 0x20;
    data_buf[1] = 0x01;
    rc = ce1502_cmd(s_ctrl, 0x05, data_buf, 2); // continuous capture
#endif

    data_buf[0] = 0x1C;	//SXGA(1280x960)
#ifdef FULLSIZE_13P0
    data_buf[1] = 0x0B;
#else
    data_buf[1] = 0x01;
#endif
    rc = ce1502_cmd(s_ctrl, 0x54, data_buf, 2);

    data_buf[0] = 0x00;	
    rc = ce1502_cmd(s_ctrl, 0x40, data_buf, 1);

#if 0 //def F_PANTECH_CAMERA_FIX_CFG_AE_AWB_LOCK
    if(aec_awb_lock == 0x02)
#endif        
    {
        // AE/AWB enable
        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x11, data_buf, 1);
    }
    // Preview start (PREVIEW)
    data_buf[0] = 0x01;
    rc = ce1502_cmd(s_ctrl, 0x6B, data_buf, 1);

    rc = ce1502_poll(s_ctrl, 0x6C, 0x08, 10, 100);
    if (rc < 0)
    {
        SKYCERR("%s : Preview Start polling ERROR !!!\n",__func__);
#ifdef ISP_LOGEVENT_PRINT
        ce1502_get_isp_event_log(s_ctrl);
#endif
        return rc;
    }	

preview_ok:

    mdelay(30); // test 04.13.

    sensor_mode = 1;
#if 1 // F_PANTECH_CAMERA_CFG_HDR
    ev_sft_mode = 0;   // 0:normal capture , 1: ev shift capture
#endif

    prev_caf2 = caf_b_ojt;
    ce1502_set_continuous_af(s_ctrl, continuous_af_mode);
    caf_b_ojt = prev_caf2;

    //	f_stop_capture = 0;	// test cts
        x1 = 0;
        x2 = 0;
        y1 = 0;
        y2 = 0;

    if(current_fps != 31)
        ce1502_sensor_set_preview_fps(s_ctrl, current_fps);

    SKYCDBG("%s end rc = %d\n",__func__, rc);

    return rc;
}


//#define SINGLE_CAPTURE
static int32_t ce1502_snapshot_config(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
//	int i=0;
//	uint16_t read_data =0;

	uint8_t data_buf[10];
    int8_t prev_caf;
    int8_t prev_caf2;

    if(sensor_mode == 0)
        goto snapshot_cmd;
    
#if 1
    prev_caf = continuous_af_mode;
    prev_caf2 = caf_b_ojt;
    rc = ce1502_set_continuous_af(s_ctrl, 0);
    continuous_af_mode = prev_caf;
    caf_b_ojt = prev_caf2;
#else
    rc = ce1502_cmd(s_ctrl, 0x35, 0, 0);	// Lens Stop	
    rc = ce1502_poll_bit(s_ctrl, 0x24, 10, 400);
	if (rc < 0)
	{
		SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
		return rc;
	}
    mdelay(10);
#endif
    SKYCDBG("%s start\n",__func__);

    rc = ce1502_set_led_gpio_set(1);

#if 1 // test
#ifndef NEW_CAPTURE_FW 
       // additional setting
	data_buf[0] = 0x00;
	data_buf[1] = 0x04;
	rc = ce1502_cmd(s_ctrl, 0x71, data_buf, 2);
	if (rc < 0)
	{
		SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
		return rc;
	}
#endif


#if 1 // F_PANTECH_CAMERA_CFG_HDR
    if(ev_sft_mode == 1)
    {
#ifdef FULLSIZE_13P0
        data_buf[0] = 0x31;
        data_buf[1] = 0x07;
#else
        data_buf[0] = 0x30;
        data_buf[1] = 0x05;
#endif
        data_buf[2] = 0x56;	
        data_buf[3] = 0x00;	

        rc = ce1502_cmd(s_ctrl, 0x73, data_buf, 4);
        if (rc < 0)
        {
            SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
            return rc;
        }
        data_buf[0] = 0x21;
        data_buf[1] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x07, data_buf, 2);
        data_buf[0] = 0x05;
        data_buf[1] = 0x0A;
        rc = ce1502_cmd(s_ctrl, 0x07, data_buf, 2);
    }
    else
#endif
{

#ifdef NEW_CAPTURE_FW 

#ifdef SINGLE_CAPTURE

#ifdef FULLSIZE_13P0
    data_buf[0] = 0x31;
    data_buf[1] = 0x06;
#else
    data_buf[0] = 0x30;
	data_buf[1] = 0x00;
#endif
    data_buf[2] = 0x00;	
    data_buf[3] = 0x80;	
    
#else

#ifdef FULLSIZE_13P0
    data_buf[0] = 0x31;
    data_buf[1] = 0x07;
#else
    data_buf[0] = 0x30;
    data_buf[1] = 0x05;
#endif
	data_buf[2] = 0x14;	
	data_buf[3] = 0x00;	
    
#endif

#else
    data_buf[1] = 0x00;
    data_buf[2] = 0x14;	
	data_buf[3] = 0x00;	
#endif

	rc = ce1502_cmd(s_ctrl, 0x73, data_buf, 4);
	if (rc < 0)
	{
		SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
		return rc;
	}
#ifdef NEW_CAPTURE_FW 
    data_buf[0] = 0x00;
    data_buf[1] = 0x00;
    rc = ce1502_cmd(s_ctrl, 0x07, data_buf, 2);
#endif
}

#if 0 //def F_PANTECH_CAMERA_FIX_CFG_AE_AWB_LOCK
    if(aec_awb_lock == 0x02)
#endif        
    {
        // AE/AWB Lock
    	data_buf[0] = 0x11;
    	rc = ce1502_cmd(s_ctrl, 0x11, data_buf, 1);
    }

snapshot_cmd:

#ifndef SINGLE_CAPTURE
     rc = ce1502_cmd_read(s_ctrl, 0xD0, data_buf, 8);
#endif

	// Buffering Capture start
	rc = ce1502_cmd(s_ctrl, 0x74, 0, 0);
#else
{

#if 1 // test
        data_buf[0] = 0x1C;	//UXGA(1280x960)
	data_buf[1] = 0x01;
	rc = ce1502_cmd(s_ctrl, 0x54, data_buf, 2);
#endif
      
	// Preview start (PREVIEW)
	data_buf[0] = 0x01;
	rc = ce1502_cmd(s_ctrl, 0x6B, data_buf, 1);
}
#endif

    rc = ce1502_poll(s_ctrl, 0x6C, 0x01, 10, 100);
    if (rc < 0)
    {
        SKYCERR("%s : Capture Start polling ERROR !!!\n",__func__);
#ifdef ISP_LOGEVENT_PRINT
        ce1502_get_isp_event_log(s_ctrl);
#endif
        return rc;
    }	

#if 0 // test
    mdelay(500); // test delay
#else
#if 1 // interrupt status
{
    int i = 0;
    mdelay(20);
    for(i = 0; i < 60; i++)
    {
        rc = ce1502_cmd_read(s_ctrl, 0xD0, data_buf, 8);
        SKYCDBG("%s data_buf[4] = %d\n",__func__, data_buf[4]);
        if (data_buf[4] & 0x4) // bit 2
            break;
        mdelay(10);
    }    
}
#endif
#endif
    SKYCDBG("%s end rc = %d\n",__func__, rc);

    return rc;
}

void ce1502_sensor_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc = 0;
    uint8_t data_buf[2];

    //F_PANTECH_CAMERA	
    SKYCDBG("%s: START + \n", __func__);

//    if(!(sensor_mode > 0 && sensor_mode < 4))
//    if((sensor_mode < 0) || (sensor_mode > 3))
    if(((sensor_mode < 0) || (sensor_mode > 3))&&(sensor_mode != 10))
        return;
    if(sensor_mode == 10) 
        sensor_mode = 1;

    if(sensor_mode != 0)    
        rc = ce1502_lens_stop2(s_ctrl);

    if(sensor_mode == 3)
    {
        data_buf[0] = 0x00;	
        rc = ce1502_cmd(s_ctrl, 0x7D, data_buf, 1);
        rc = ce1502_poll(s_ctrl, 0x6C, 0x00, 10, 100);
        if (rc < 0)
        {
#ifdef ISP_LOGEVENT_PRINT
            ce1502_get_isp_event_log(s_ctrl);
#endif
            SKYCERR("%s : ZSL Preview Stop polling ERROR !!!\n",__func__);
            return;
        }	
    }
    else if(sensor_mode == 0)
    {
        // stop Capture	
        rc = ce1502_cmd(s_ctrl, 0x75, 0, 0);
        if (rc < 0)
        {
            SKYCERR("ERR:%s Capture Stop command FAIL!!!rc=%d return~~\n", __func__, rc);
            return;
        }	

        rc = ce1502_poll(s_ctrl, 0x6C, 0x00, 10, 100);
        if (rc < 0)
        {
#ifdef ISP_LOGEVENT_PRINT
            ce1502_get_isp_event_log(s_ctrl);
#endif
            SKYCERR("%s : Capture Stop polling ERROR !!!\n",__func__);
            return;
        }	
    }
    else
    {
        data_buf[0] = 0x00;	
        rc = ce1502_cmd(s_ctrl, 0x6B, data_buf, 1);
        rc = ce1502_poll(s_ctrl, 0x6C, 0x00, 10, 100);
        if (rc < 0)
        {
#ifdef ISP_LOGEVENT_PRINT
            ce1502_get_isp_event_log(s_ctrl);
#endif
            SKYCERR("%s : Preview Stop polling ERROR !!!\n",__func__);
            return;
        }	
    }

    sensor_mode = -1;
	
    //F_PANTECH_CAMERA	
    SKYCDBG("%s: END -(%d) \n", __func__, rc);	
}

int ce1502_sensor_init(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc = 0;
    uint8_t data_buf[8];

    //F_PANTECH_CAMERA	
    SKYCDBG("%s: START + \n", __func__);

    // ISP FW Boot UP !!
    rc = ce1502_cmd(&ce1502_s_ctrl, 0xF0, NULL, 0);

    mdelay(400);

#if 1 // interrupt status
	rc = ce1502_cmd_read(s_ctrl, 0xD0, data_buf, 8);
	if (rc < 0) {
		SKYCDBG("%s read err\n", __func__);
		return rc;
	}

    	data_buf[0] = 0xFF;
	data_buf[1] = 0xFF;
#ifdef CONFIG_PANTECH_CAMERA_IRQ
	data_buf[2] = 0xFB; // af done(2)
	data_buf[3] = 0xDF; // AF-T start(5)
#else
	data_buf[2] = 0xFF;
	data_buf[3] = 0xFF;
#endif
    	data_buf[4] = 0xFB; // capture ready(2)
	data_buf[5] = 0xFF;
	data_buf[6] = 0xFF;
	data_buf[7] = 0xFF;
	rc = ce1502_cmd(s_ctrl, 0xD1, data_buf, 8);
	if (rc < 0) {
		SKYCDBG("%s bryan err", __func__);
	}
#endif

#ifdef CONFIG_PANTECH_CAMERA_IRQ
	ce1502_irq_stat = 0;
#endif


#ifdef F_MIPI336
    data_buf[0] = 0x0C;	// 336 , PASS2
    rc = ce1502_cmd(s_ctrl, 0x03, data_buf, 1);
    mdelay(10); 
#else
#if 0
    data_buf[0] = 0x05;	// 528 , PASS1
    rc = ce1502_cmd(s_ctrl, 0x03, data_buf, 1);
    mdelay(10); 
#endif
#endif

#if 0 //jjhwang capture delay test
    data_buf[0] = 0x01;	// 0:0ms. 1:50ms, 2:100ms, 3:200ms, 4:300ms
    rc = ce1502_cmd(s_ctrl, 0x05, data_buf, 1);
#endif

#if 0 //def NEW_CAPTURE_FW
    data_buf[0] = 0x00;
    data_buf[1] = 0x05;
    rc = ce1502_cmd(s_ctrl, 0x05, data_buf, 2);
    data_buf[0] = 0x20;
    data_buf[1] = 0x01;
    rc = ce1502_cmd(s_ctrl, 0x05, data_buf, 2); // continuous capture
#endif

//    sensor_mode = 1;
    sensor_mode = 10; // test


#if 0 // test preview
#ifdef NEW_CAPTURE_FW
    data_buf[0] = 0x00;
    data_buf[1] = 0x05;
    rc = ce1502_cmd(s_ctrl, 0x05, data_buf, 2);
    data_buf[0] = 0x20;
    data_buf[1] = 0x01;
    rc = ce1502_cmd(s_ctrl, 0x05, data_buf, 2); // continuous capture
#endif

    data_buf[0] = 0x1C;	//SXGA(1280x960)
    data_buf[1] = 0x01;
    rc = ce1502_cmd(s_ctrl, 0x54, data_buf, 2);

    data_buf[0] = 0x00;
    rc = ce1502_cmd(s_ctrl, 0x40, data_buf, 1);
    
    // AE/AWB enable
    data_buf[0] = 0x00;
    rc = ce1502_cmd(s_ctrl, 0x11, data_buf, 1);

    // Preview start (PREVIEW)
    data_buf[0] = 0x01;
    rc = ce1502_cmd(s_ctrl, 0x6B, data_buf, 1);
#if 1
    mdelay(10);
#else
    rc = ce1502_poll(s_ctrl, 0x6C, 0x08, 10, 100);
    if (rc < 0)
    {
        SKYCERR("%s : Preview Start polling ERROR !!!\n",__func__);
#ifdef ISP_LOGEVENT_PRINT
        ce1502_get_isp_event_log(s_ctrl);
#endif
        return rc;
    }	
#endif

#endif

    //F_PANTECH_CAMERA	
    SKYCDBG("%s: END -(%d) \n", __func__, rc);	
    return rc;	
}

int32_t ce1502_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;


//F_PANTECH_CAMERA	
SKYCDBG("%s: %d, %d res=%d\n", __func__, __LINE__,update_type,res);
	
#if 0//def F_PANTECH_CAMERA
   if (s_ctrl->func_tbl->sensor_stop_stream)
	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
#endif

#if 1
    if(res != 0)
       ce1502_sensor_stop_stream(s_ctrl);
    else if(update_type == MSM_SENSOR_UPDATE_PERIODIC)
        ce1502_snapshot_config(s_ctrl);
#else
    g_sensor_mode = res;
    g_update_type = update_type;
    if(g_sensor_mode != 0) {
       ce1502_sensor_ideal_stream(s_ctrl);
    }
    else if(g_update_type == MSM_SENSOR_UPDATE_PERIODIC) {
        ce1502_snapshot_config(s_ctrl);
    }
#endif
        
    msleep(30);
    if (update_type == MSM_SENSOR_REG_INIT) {
		SKYCDBG("Register INIT\n");
        s_ctrl->curr_csi_params = NULL;
        msm_sensor_enable_debugfs(s_ctrl);
		
        ce1502_sensor_init(s_ctrl);

    } else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		SKYCDBG("PERIODIC : %d\n", res);
//        msm_sensor_write_res_settings(s_ctrl, res);
		if (s_ctrl->curr_csi_params != s_ctrl->csi_params[res]) {
			SKYCDBG("CSI config in progress  ==> LP11 state.\n");
			s_ctrl->curr_csi_params = s_ctrl->csi_params[res];

			s_ctrl->curr_csi_params->csid_params.lane_assign =
				s_ctrl->sensordata->sensor_platform_info->
				csi_lane_params->csi_lane_assign;
			s_ctrl->curr_csi_params->csiphy_params.lane_mask =
				s_ctrl->sensordata->sensor_platform_info->
				csi_lane_params->csi_lane_mask;

			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSID_CFG,
				&s_ctrl->curr_csi_params->csid_params);

			mb();
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIPHY_CFG,
				&s_ctrl->curr_csi_params->csiphy_params);
			mb();
			msleep(20);
			SKYCDBG("CSI config END.\n");		
		}

		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_PCLK_CHANGE, &s_ctrl->msm_sensor_reg->
			output_settings[res].op_pixel_clk);

#if 0//def F_PANTECH_CAMERA
        if (s_ctrl->func_tbl->sensor_start_stream)
		s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
#else
            switch (res) {
            case 0:
//                rc = ce1502_snapshot_config(s_ctrl);	
                break;
            case 1:
                rc = ce1502_video_config(s_ctrl);	
                break;
            case 2: 
                rc = ce1502_1080p_config(s_ctrl);	
                break;
            case 3: 
                rc = ce1502_ZSL_config(s_ctrl);	
                break;
            default:
                rc = ce1502_video_config(s_ctrl);	
                SKYCDBG("%s fail res=%d\n", __func__, res);
                break;
            }
            sensor_mode = res;
#endif
//		msleep(30);
	}
	SKYCDBG("%s: %d x\n", __func__, __LINE__);
	return rc;
}

int32_t ce1502_sensor_set_fps(struct msm_sensor_ctrl_t *s_ctrl,
						struct fps_cfg *fps)
{
	//uint16_t total_lines_per_frame;
	int32_t rc = 0;
	SKYCDBG("%s: %d\n", __func__, __LINE__);
#if 0	
	s_ctrl->fps_divider = fps->fps_div;


	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines,
			total_lines_per_frame, MSM_CAMERA_I2C_WORD_DATA);
#endif	
	return rc;
}

#ifdef CONFIG_PANTECH_CAMERA
static int ce1502_sensor_set_brightness(struct msm_sensor_ctrl_t *s_ctrl ,int8_t brightness)
{
    uint8_t data_buf[2];
    int rc = 0;

    if(sensor_mode == 0) // test
        return 0;
    
    SKYCDBG("%s start\n",__func__);

    if(brightness < 0 || brightness >= CE1502_BRIGHTNESS_MAX){
        SKYCERR("%s error. brightness=%d\n", __func__, brightness);
        return -EINVAL;
    }

    data_buf[0] = 0x02;
    data_buf[1] = ce1502_bright_data[brightness];
    rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
    if (rc < 0)
    {
        SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
        return rc;
    }	

    rc  = ce1502_01_command(s_ctrl);

    SKYCDBG("%s end\n",__func__);
    return rc;
}

static int ce1502_sensor_set_effect(struct msm_sensor_ctrl_t *s_ctrl ,int8_t effect)
{
    uint8_t data_buf[2];
    int rc = 0;

    if(sensor_mode == 0) // test
        return 0;
    
    SKYCDBG("%s start\n",__func__);

    if(effect < 0 || effect >= CE1502_EFFECT_MAX){
        SKYCERR("%s error. effect=%d\n", __func__, effect);
        return -EINVAL;
    }

    data_buf[0] = 0x05;
    data_buf[1] = ce1502_effect_data[effect];
    rc = ce1502_cmd(s_ctrl, 0x3D, data_buf, 2);	//effect off
    if (rc < 0)
    {
        SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
        return rc;
    }	

    data_buf[0] = 0x07;
    if(effect == 5) //posterize
    {
        data_buf[1] = 0x10;    
    } else {
        data_buf[1] = 0x06;    
    }     
    rc = ce1502_cmd(s_ctrl, 0x3D, data_buf, 2);	//effect off
    if (rc < 0)
    {
        SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
        return rc;
    }	
    
    rc  = ce1502_01_command(s_ctrl);

    SKYCDBG("%s end\n",__func__);
	
    return rc;
}

static int ce1502_sensor_set_exposure_mode(struct msm_sensor_ctrl_t *s_ctrl ,int8_t exposure)
{
    uint8_t data_buf[2];
    int32_t rc = 0;

    if(sensor_mode == 0) // test
        return 0;
    
    SKYCDBG("%s  exposure = %d\n",__func__, exposure);

    if ((exposure < 0) || (exposure >= CE1502_EXPOSURE_MAX))
    {
        SKYCERR("%s FAIL!!! return~~  exposure = %d\n",__func__,exposure);
        return 0;//-EINVAL;
    }

#if 1 //def F_PANTECH_CAMERA_FIX_CFG_AE_AWB_LOCK
    if(aec_awb_lock & 0x01)
        ce1502_set_aec_lock(s_ctrl, 0);
#endif

    data_buf[0] = 0x00;
    data_buf[1] = ce1502_exposure_data[exposure];
    rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
    if (rc < 0)
    {
        SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
        return rc;
    }	

    rc  = ce1502_01_command(s_ctrl);

    SKYCDBG("%s end\n",__func__);

    return rc;
}

static int ce1502_sensor_set_wb(struct msm_sensor_ctrl_t *s_ctrl ,int8_t wb)
{
    uint8_t data_buf[2];
    int rc = 0;

    if(sensor_mode == 0) // test
        return 0;
    
    SKYCDBG("%s start\n",__func__);

    if(wb < 1 || wb > CE1502_WHITEBALANCE_MAX){
        SKYCERR("%s error. whitebalance=%d\n", __func__, wb);
        return -EINVAL;
    }

#if 1 //def F_PANTECH_CAMERA_FIX_CFG_AE_AWB_LOCK
    ce1502_set_awb_lock(s_ctrl, 0);
#endif

    if(wb == 1 || wb == 2)		// auto
    {
        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x1A, data_buf, 1);
        if (rc < 0)
        {
            SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
            return rc;
        }	

        data_buf[0] = 0x11;
        data_buf[1] =  0x00;
    }	
    else
    {
        data_buf[0] = 0x10;
        data_buf[1] =  ce1502_wb_data[wb-1];
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);	

        data_buf[0] = 0x01;
        rc = ce1502_cmd(s_ctrl, 0x1A, data_buf, 1);
        if (rc < 0)
        {
            SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
            return rc;
        }	
    }	

    rc  = ce1502_01_command(s_ctrl);

    SKYCDBG("%s end\n",__func__);

    return rc;
}

static int32_t ce1502_set_scene_mode(struct msm_sensor_ctrl_t *s_ctrl, int8_t scene_mode)
{
    uint8_t data_buf[2];
    int rc = 0;

    if(sensor_mode == 0) // test
        return 0;    

    SKYCDBG("%s start\n",__func__);

    if(scene_mode < 0 || scene_mode >= CE1502_SCENE_MAX+1){
        SKYCERR("%s error. scene_mode=%d\n", __func__, scene_mode);
        return 0; //-EINVAL;
    }

#if 1 //def F_PANTECH_CAMERA_FIX_CFG_AE_AWB_LOCK
    if(aec_awb_lock & 0x01)
        ce1502_set_aec_lock(s_ctrl, 0);
    if(aec_awb_lock & 0x10)
        ce1502_set_awb_lock(s_ctrl, 0);
#endif

    switch (scene_mode)
    {
    case 0: //OFF
        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x1A, data_buf, 1);
        data_buf[0] = 0x00;
        data_buf[1] = 0x01;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x01;
        data_buf[1] = 0x00;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x02;
        data_buf[1] = 0x03;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        data_buf[0] = 0x06;
        data_buf[1] = 0x08;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        break;
    case 7: //Potrait
        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x1A, data_buf, 1);
        data_buf[0] = 0x00;
        data_buf[1] = 0x01;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x01;
        data_buf[1] = 0x00;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x02;
        data_buf[1] = 0x02;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        data_buf[0] = 0x06;
        data_buf[1] = 0x08;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        break;
    case 2: //LandScape
        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x1A, data_buf, 1);
        data_buf[0] = 0x00;
        data_buf[1] = 0x03;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x01;
        data_buf[1] = 0x00;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x02;
        data_buf[1] = 0x04;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        data_buf[0] = 0x06;
        data_buf[1] = 0x09;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        break;
    case 19: //Indoor
    case 14: //Party
        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x1A, data_buf, 1);
        data_buf[0] = 0x00;
        data_buf[1] = 0x01;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x01;
        data_buf[1] = 0x03;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x02;
        data_buf[1] = 0x03;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        data_buf[0] = 0x06;
        data_buf[1] = 0x09;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        break;
    case 9: //Sports
        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x1A, data_buf, 1);
        data_buf[0] = 0x00;
        data_buf[1] = 0x01;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x01;
        data_buf[1] = 0x08;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x02;
        data_buf[1] = 0x03;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        data_buf[0] = 0x06;
        data_buf[1] = 0x08;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        break;
    case 6: //Night
        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x1A, data_buf, 1);
        data_buf[0] = 0x00;
        data_buf[1] = 0x01;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x01;
        data_buf[1] = 0x09;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x02;
        data_buf[1] = 0x03;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        data_buf[0] = 0x06;
        data_buf[1] = 0x08;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        break;
    case 4: //Beach
    case 3: //Snow
        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x1A, data_buf, 1);
        data_buf[0] = 0x00;
        data_buf[1] = 0x01;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x01;
        data_buf[1] = 0x01;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x02;
        data_buf[1] = 0x03;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        data_buf[0] = 0x06;
        data_buf[1] = 0x09;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        break;
    case 5: //Sunset
        data_buf[0] = 0x10;
        data_buf[1] = 0x01;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x01;
        rc = ce1502_cmd(s_ctrl, 0x1A, data_buf, 1);
        data_buf[0] = 0x00;
        data_buf[1] = 0x01;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x01;
        data_buf[1] = 0x00;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x02;
        data_buf[1] = 0x03;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        data_buf[0] = 0x06;
        data_buf[1] = 0x08;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        break;
    case 20: //TEXT
        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x1A, data_buf, 1);
        data_buf[0] = 0x00;
        data_buf[1] = 0x01;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x01;
        data_buf[1] = 0x00;           
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        data_buf[0] = 0x02;
        data_buf[1] = 0x05;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        data_buf[0] = 0x06;
        data_buf[1] = 0x08;           
        rc = ce1502_cmd(s_ctrl, 0x3d, data_buf, 2);
        break;

    case CE1502_SCENE_MAX: // AUTO
            break;
    }

    rc  = ce1502_01_command(s_ctrl);

    SKYCDBG("%s end\n",__func__);

    return rc;
}

static int ce1502_sensor_set_preview_fps(struct msm_sensor_ctrl_t *s_ctrl ,int8_t preview_fps)
{
	/* 0 : variable 30fps, 1 ~ 30 : fixed fps */
	/* default: variable 8 ~ 30fps */
	uint8_t data_buf[4];
	int32_t rc = 0;

	if ((preview_fps < C_PANTECH_CAMERA_MIN_PREVIEW_FPS) || 
		(preview_fps > C_PANTECH_CAMERA_MAX_PREVIEW_FPS)) {
		SKYCERR("%s: -EINVAL, preview_fps=%d\n", 
			__func__, preview_fps);
		return 0; //-EINVAL;
	}

	SKYCDBG("%s: preview_fps=%d\n", __func__, preview_fps);

	if(preview_fps == C_PANTECH_CAMERA_MAX_PREVIEW_FPS)
	{
		data_buf[0] = 0xFF;
        data_buf[1] = 0xFF;
	}
	else
	{
		data_buf[0] = preview_fps;
		data_buf[1] = 0x00;
        	rc = ce1502_cmd(s_ctrl, 0x5A, data_buf, 2);
	}
	
	if (rc < 0)
	{
		SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
		return rc;
	}	
    current_fps = preview_fps;

	SKYCDBG("%s end rc = %d\n",__func__, rc);

	return rc;
}

static int ce1502_sensor_set_reflect(struct msm_sensor_ctrl_t *s_ctrl ,int8_t reflect)
{
	int rc = 0;

	return rc;
}

static int32_t ce1502_set_antishake(struct msm_sensor_ctrl_t *s_ctrl ,int8_t antishake)
{
    uint8_t data_buf[2];
    int32_t rc = 0;

    if(sensor_mode == 0) // test
        return 0;    

    switch(antishake)
    {
        case 0 :
        case 1 :
            data_buf[0] = antishake;
            rc = ce1502_cmd(s_ctrl, 0x5B, data_buf, 1);
            if (rc < 0)
            {
                SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
                return rc;
            }	
            break;

        case 2 :
            break;
        default :
            SKYCERR("%s FAIL!!! return~~  antishake = %d\n",__func__,antishake);
            break;        
    }


    SKYCDBG("%s end\n",__func__);

    return rc;
}

#ifdef CONFIG_PNTECH_CAMERA_OJT
static int32_t ce1502_set_ojt_ctrl(struct msm_sensor_ctrl_t *s_ctrl ,int8_t ojt)
{
    int32_t rc = 0;
    uint8_t data_buf[10];
    
    if(sensor_mode == 0) // test
        return 0;    

    if(ojt == 0)
    {
//        rc = ce1502_set_object_tracking(s_ctrl, ojt);
#ifdef CONFIG_PNTECH_CAMERA_OJT_TEST
        data_buf[0] = 0x00;
        data_buf[1] = 0x00;
        data_buf[2] = 0x00;
        data_buf[3] = 0x00;
        data_buf[4] = 0x00;
        data_buf[5] = 0x00;
        data_buf[6] = 0x00;
        data_buf[7] = 0x00;
        data_buf[8] = 0x00;
        data_buf[9] = 0x00;	
        rc = ce1502_cmd(s_ctrl, 0x4A, data_buf, 10);
#endif
        ce1502_42_command(s_ctrl, 0x00);
        
        rc = ce1502_set_continuous_af(s_ctrl, caf_b_ojt);
        if(caf_b_ojt == 0)
        {
            data_buf[0] = 0x00;
            rc = ce1502_cmd(s_ctrl, 0x20, data_buf, 1); //set focus mode normal
            rc = ce1502_poll_bit(s_ctrl, 0x24, 10, 400);
        }        
    }
    else if(current_ojt == 0)
    {
        int8_t prev_caf = continuous_af_mode;
        rc = ce1502_set_continuous_af(s_ctrl, 1);        
        caf_b_ojt = prev_caf;
        SKYCDBG("%s : caf_b_ojt=%d\n",__func__, caf_b_ojt);
    }

    current_ojt = ojt;
    
    SKYCDBG("%s end, current_ojt=%d\n",__func__, current_ojt);

    return rc;
}
#endif

static int32_t ce1502_set_led_mode(struct msm_sensor_ctrl_t *s_ctrl ,int8_t led_mode)
{
    /* off, auto, on, movie */	
    int rc;
    uint8_t data_buf[10];

    if(sensor_mode == 0) // test
        return 0;    

    SKYCDBG("%s: led_mode=%d\n", __func__, led_mode);
    if ((led_mode < 0) || (led_mode > 7)) {
        SKYCERR("%s: -EINVAL, led_mode=%d\n", __func__, led_mode);
        return -EINVAL;
    }

    if(led_mode != 6)
        rc = ce1502_lens_stop(s_ctrl);

    //control ce1502 isp gpio
    switch(led_mode)
    {
    case 0: //off
        SKYCDBG("CE1502_CFG_LED_MODE_OFF SET\n");

			data_buf[0] = 0x00;
			data_buf[1] = 0x00;
			rc = ce1502_cmd(s_ctrl, 0x06, data_buf, 2);
			
			data_buf[0] = 0x01;
			data_buf[1] = 0x00;
			rc = ce1502_cmd(s_ctrl, 0xB2, data_buf, 2);

        data_buf[0] = 0x03;
        data_buf[1] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0xB2, data_buf, 2);
	
        mdelay(10);
        rc = ce1502_set_led_gpio_set(led_mode);
        break;
        
    case 1: // auto
        SKYCDBG("CE1502_CFG_LED_MODE_AUTO SET\n");

        data_buf[0] = 0x00;
        data_buf[1] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x06, data_buf, 2);			

        data_buf[0] = 0x03;
        data_buf[1] = 0x01;
        data_buf[2] = 0x15;
        data_buf[3] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0xB3, data_buf, 4);

			data_buf[0] = 0x01;
			data_buf[1] = 0x00; //0x02;  //AF flash off
			rc = ce1502_cmd(s_ctrl, 0xB2, data_buf, 2);

        data_buf[0] = 0x03;
        data_buf[1] = 0x02;
        rc = ce1502_cmd(s_ctrl, 0xB2, data_buf, 2);

#if 0 //def CONFIG_PNTECH_CAMERA_ZSL_FLASH
        data_buf[0] = 0x04;
        data_buf[1] = 0x00;
        data_buf[2] = 0x15;
        data_buf[3] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0xB3, data_buf, 4);

        data_buf[0] = 0x04;
        data_buf[1] = 0x02;
        rc = ce1502_cmd(s_ctrl, 0xB2, data_buf, 2);
#else
        data_buf[0] = 0x01;
        data_buf[1] = 0x0A;
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        rc = ce1502_01_command(s_ctrl);
#endif	
    break;	

    case 2: // on
        SKYCDBG("CE1502_CFG_LED_MODE_ON SET\n");

        data_buf[0] = 0x00;
        data_buf[1] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x06, data_buf, 2);

        data_buf[0] = 0x03;
        data_buf[1] = 0x01;
        data_buf[2] = 0x15;
        data_buf[3] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0xB3, data_buf, 4);

        data_buf[0] = 0x01;
        data_buf[1] = 0x00; //0x01;  //AF flash off
        rc = ce1502_cmd(s_ctrl, 0xB2, data_buf, 2);

        data_buf[0] = 0x03;
        data_buf[1] = 0x01;
        rc = ce1502_cmd(s_ctrl, 0xB2, data_buf, 2);

#if 0 //def CONFIG_PNTECH_CAMERA_ZSL_FLASH
        data_buf[0] = 0x04;
        data_buf[1] = 0x00;
        data_buf[2] = 0x15;
        data_buf[3] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0xB3, data_buf, 4);

        data_buf[0] = 0x04;
        data_buf[1] = 0x01;
        rc = ce1502_cmd(s_ctrl, 0xB2, data_buf, 2);
#else
        data_buf[0] = 0x01;
        data_buf[1] = 0x0A;
        rc = ce1502_cmd(s_ctrl, 0x04, data_buf, 2);
        rc = ce1502_01_command(s_ctrl);
#endif
        break;

    case 3: // torch
        SKYCDBG("CE1502_CFG_LED_MODE_MOVIE SET\n");
        rc = ce1502_set_led_gpio_set(led_mode);

        data_buf[0] = 0x01;
        data_buf[1] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0xB2, data_buf, 2);

        data_buf[0] = 0x03;
        data_buf[1] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0xB2, data_buf, 2);

        data_buf[0] = 0x00;
        data_buf[1] = 0x01;
        data_buf[2] = 0x03;
        data_buf[3] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0xB3, data_buf, 4);


        data_buf[0] = 0x00;
        data_buf[1] = 0x01;
        rc = ce1502_cmd(s_ctrl, 0x06, data_buf, 2);
        break;

    case 6: // LED_MODE_ZSL_FLASH_OFF for ZSL flash
        SKYCDBG("LED_MODE_ZSL_FLASH_OFF SET\n");
        data_buf[0] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0x11, data_buf, 1);

        mdelay(10);
        rc = ce1502_set_led_gpio_set(0);
        rc = 0;
        
        break;
        
    case 5: // LED_MODE_ZSL_FLASH_ON for ZSL flash
        SKYCDBG("LED_MODE_ZSL_FLASH_ON SET\n");

        rc = ce1502_set_led_gpio_set(1);
        data_buf[0] = 0x13;
        rc = ce1502_cmd(s_ctrl, 0x11, data_buf, 1);

        rc = ce1502_cmd_read(s_ctrl, 0xBD, data_buf, 1);
        if(data_buf[0] == 1)
        {
            data_buf[0] = 0x11;
            rc = ce1502_cmd(s_ctrl, 0x11, data_buf, 1);
            rc = 0;
        }
        else
            rc = -1;
        
        break;

    case 7: // LED_MODE_ZSL_TORCH_AUTO for ZSL flash
        SKYCDBG("LED_MODE_ZSL_TORCH_AUTO SET\n");

        rc = ce1502_set_led_gpio_set(1);
        data_buf[0] = 0x13;
        rc = ce1502_cmd(s_ctrl, 0x11, data_buf, 1);

        rc = ce1502_cmd_read(s_ctrl, 0xBD, data_buf, 1);
        if(data_buf[0] == 1)
        {
            data_buf[0] = 0x00;
            rc = ce1502_cmd(s_ctrl, 0x11, data_buf, 1);
            mdelay(10);
            
            data_buf[0] = 0x01;
            data_buf[1] = 0x00;
            rc = ce1502_cmd(s_ctrl, 0xB2, data_buf, 2);

            data_buf[0] = 0x03;
            data_buf[1] = 0x00;
            rc = ce1502_cmd(s_ctrl, 0xB2, data_buf, 2);

            data_buf[0] = 0x00;
            data_buf[1] = 0x01;
            data_buf[2] = 0x03;
            data_buf[3] = 0x00;
            rc = ce1502_cmd(s_ctrl, 0xB3, data_buf, 4);


            data_buf[0] = 0x00;
            data_buf[1] = 0x01;
            rc = ce1502_cmd(s_ctrl, 0x06, data_buf, 2);
        }
        else
        {
            data_buf[0] = 0x00;
            rc = ce1502_cmd(s_ctrl, 0x11, data_buf, 1);
            mdelay(10);
            rc = ce1502_set_led_gpio_set(0);
        }        
        rc = 0;
        break;
        
    case 4: // torch flash for 4648 test mode
        SKYCDBG("CE1502_CFG_LED_MODE_MOVIE SET\n");
        rc = ce1502_set_led_gpio_set(led_mode);

        data_buf[0] = 0x01;
        data_buf[1] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0xB2, data_buf, 2);

        data_buf[0] = 0x03;
        data_buf[1] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0xB2, data_buf, 2);

        data_buf[0] = 0x00;
        data_buf[1] = 0x01;
        data_buf[2] = 0x15;
        data_buf[3] = 0x00;
        rc = ce1502_cmd(s_ctrl, 0xB3, data_buf, 4);


        data_buf[0] = 0x00;
        data_buf[1] = 0x01;
        rc = ce1502_cmd(s_ctrl, 0x06, data_buf, 2);

        break;
        
    default:		
        break;			
    }

#if 1 // temp // AF-T state check  
    if(led_mode != 5)
    {
        if(continuous_af_mode == 1)
        {
            data_buf[0] = 0x02;
            ce1502_cmd(s_ctrl, 0x20, data_buf, 1);
            ce1502_poll_bit(s_ctrl, 0x24, 10, 400);

            SKYCDBG("AF-C start\n");
            ce1502_cmd(s_ctrl, 0x23, 0, 0);
        }
        if(continuous_af_mode == 2)
        {
            data_buf[0] = 0x01;
            SKYCDBG("AF-T resume\n");
            ce1502_cmd(s_ctrl, 0x2C, data_buf, 1);
        }
    }		
#endif

    return rc;
}

static int ce1502_set_hdr(struct msm_sensor_ctrl_t *s_ctrl)
{    
    int rc = 0;

    if(ev_sft_mode == 0)
    {
        ev_sft_mode = 1;
    }
    else
    {
        rc = ce1502_cmd(s_ctrl, 0x74, 0, 0);
    }

    SKYCDBG("%s end\n",__func__);
    return rc;
}

#if 0
static int32_t ce1502_set_wdr(struct msm_sensor_ctrl_t *s_ctrl ,int32_t wdr)
{
    uint8_t data_buf[2];
    int32_t rc = 0;

    if ((wdr != 0) && (wdr != 1))
    {
        SKYCERR("%s FAIL!!! return~~  wdr = %d\n",__func__,wdr);
        return 0;//-EINVAL;
    }

    data_buf[0] = wdr;
    rc = ce1502_cmd(s_ctrl, 0x88, data_buf, 1);
    if (rc < 0)
    {
        SKYCERR("ERR:%s FAIL!!!rc=%d return~~\n", __func__, rc);
        return rc;
    }	

    SKYCDBG("%s end\n",__func__);

    return rc;
}
#endif
#endif

int32_t ce1502_sensor_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;
	SKYCDBG("%s_i2c_probe called\n", client->name);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		SKYCDBG("i2c_check_functionality failed\n");
		rc = -EFAULT;
		//return rc;
		goto probe_fail;
	}

	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
	if (s_ctrl->sensor_i2c_client != NULL) {

		s_ctrl->sensor_i2c_client->client = client;
		if (s_ctrl->sensor_i2c_addr != 0)
			s_ctrl->sensor_i2c_client->client->addr =
				s_ctrl->sensor_i2c_addr;

	SKYCDBG("s_ctrl->sensor_i2c_addr =%d\n", s_ctrl->sensor_i2c_client->client->addr);

	} else {
		rc = -EFAULT;
		//return rc;
		goto probe_fail;
	}

	s_ctrl->sensordata = client->dev.platform_data;

       // ISP firmware update on probing
       ce1502_update_fw_boot(s_ctrl, s_ctrl->sensordata);

	snprintf(s_ctrl->sensor_v4l2_subdev.name,
		sizeof(s_ctrl->sensor_v4l2_subdev.name), "%s", id->name);
	v4l2_i2c_subdev_init(&s_ctrl->sensor_v4l2_subdev, client,
		s_ctrl->sensor_v4l2_subdev_ops);

	msm_sensor_register(&s_ctrl->sensor_v4l2_subdev);
	goto i2c_probe_end;
probe_fail:
	CDBG("%s_i2c_probe failed\n", client->name);
i2c_probe_end:

	return rc;
}

static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&ce1502_i2c_driver);
}

static struct v4l2_subdev_core_ops ce1502_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};


static struct v4l2_subdev_core_ops ce1502_subdev_core_ops;
static struct v4l2_subdev_video_ops ce1502_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ce1502_subdev_ops = {
	.core = &ce1502_subdev_core_ops,
	.video  = &ce1502_subdev_video_ops,
};

static struct msm_sensor_fn_t ce1502_func_tbl = {
    
	.sensor_start_stream = ce1502_sensor_start_stream,
	.sensor_stop_stream = ce1502_sensor_stop_stream,
//	.sensor_group_hold_on = msm_sensor_group_hold_on,
//	.sensor_group_hold_off = msm_sensor_group_hold_off,
//	.sensor_get_prev_lines_pf = msm_sensor_get_prev_lines_pf,
//	.sensor_get_prev_pixels_pl = msm_sensor_get_prev_pixels_pl,
//	.sensor_get_pict_lines_pf = msm_sensor_get_pict_lines_pf,
//	.sensor_get_pict_pixels_pl = msm_sensor_get_pict_pixels_pl,
//	.sensor_get_pict_max_exp_lc = msm_sensor_get_pict_max_exp_lc,
//	.sensor_get_pict_fps = msm_sensor_get_pict_fps,
//	.sensor_set_fps = ce1502_sensor_set_fps,//msm_sensor_set_fps,
//	.sensor_write_exp_gain = msm_sensor_write_exp_gain_ce1502, //return 0
//	.sensor_setting = msm_sensor_setting,
	.sensor_setting = ce1502_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
//	.sensor_open_init = ce1502_sensor_open_init,
//	.sensor_release = ce1502_sensor_release,
#ifdef F_CE1502_POWER
	.sensor_power_up = ce1502_sensor_power_up,//msm_sensor_power_up,
	.sensor_power_down = ce1502_sensor_power_down,//msm_sensor_power_down,
#else
    .sensor_power_up = msm_sensor_power_up,
    .sensor_power_down = msm_sensor_power_down,
#endif
	.sensor_get_csi_params = msm_sensor_get_csi_params,  //yujm_temp
//	.sensor_probe = msm_sensor_probe,
//	.sensor_probe = ce1502_sensor_probe,
#ifdef CONFIG_PANTECH_CAMERA
    .sensor_set_brightness = ce1502_sensor_set_brightness,
    .sensor_set_effect = ce1502_sensor_set_effect,
    .sensor_set_exposure_mode = ce1502_sensor_set_exposure_mode,
    .sensor_set_wb = ce1502_sensor_set_wb,
    .sensor_set_scene_mode = ce1502_set_scene_mode,
    .sensor_set_preview_fps = ce1502_sensor_set_preview_fps,
    .sensor_set_reflect = ce1502_sensor_set_reflect,    
    .sensor_set_auto_focus = ce1502_sensor_set_auto_focus,
    .sensor_set_antishake = ce1502_set_antishake,
    .sensor_check_af = ce1502_sensor_check_af,    
    .sensor_set_continuous_af = ce1502_set_continuous_af,
    .sensor_set_focus_rect = ce1502_set_focus_rect,
    .sensor_set_led_mode = ce1502_set_led_mode,
    .sensor_set_hdr = ce1502_set_hdr,
    .sensor_set_metering_area = ce1502_set_metering_area,
	.sensor_set_focus_mode = ce1502_sensor_set_focus_mode, 
#ifdef CONFIG_PNTECH_CAMERA_OJT
    .sensor_set_ojt_ctrl = ce1502_set_ojt_ctrl,
#endif
#if 1 //def F_PANTECH_CAMERA_FIX_CFG_AE_AWB_LOCK
    .sensor_set_aec_lock = ce1502_set_aec_lock,
    .sensor_set_awb_lock = ce1502_set_awb_lock,
#endif
#ifdef CONFIG_PANTECH_CAMERA_IRQ
    .sensor_get_frame_info = ce1502_get_frame_info,
#endif
    .sensor_lens_stability = ce1502_lens_stability,
#endif
};

static struct msm_sensor_reg_t ce1502_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
#if 0
	.start_stream_conf = ce1502_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ce1502_start_settings),
	.stop_stream_conf = ce1502_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ce1502_stop_settings),
#endif
	.init_settings = NULL,//&ce1502_init_conf[0],
	.init_size = 0, //ARRAY_SIZE(ce1502_init_conf),
	.mode_settings = NULL, //&ce1502_confs[0],
	.output_settings = &ce1502_dimensions[0],
#if 1
	.num_conf =4,// ARRAY_SIZE(ce1502_cid_cfg),
#endif
	//.num_conf = 2,
};

static struct msm_sensor_ctrl_t ce1502_s_ctrl = {
	.msm_sensor_reg = &ce1502_regs,
	.sensor_i2c_client = &ce1502_sensor_i2c_client,
	.sensor_i2c_addr = 0x78,
	.sensor_id_info = &ce1502_id_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
#if 1
	.csi_params = &ce1502_csi_params_array[0],
#endif
	.msm_sensor_mutex = &ce1502_mut,
	.sensor_i2c_driver = &ce1502_i2c_driver,
	.sensor_v4l2_subdev_info = ce1502_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ce1502_subdev_info),
	.sensor_v4l2_subdev_ops = &ce1502_subdev_ops,
	.func_tbl = &ce1502_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

//module_init(msm_sensor_init_module);
late_initcall(msm_sensor_init_module);
MODULE_DESCRIPTION("CE1502 13MP YUV sensor driver");
MODULE_LICENSE("GPL v2");
