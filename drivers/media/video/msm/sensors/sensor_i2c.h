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

#ifndef _SENSOR_I2C_H_
#define _SENSOR_I2C_H_

#include <linux/types.h>
#include <linux/i2c.h>

//#define F_PANTECH_CAMERA_MIPI_ENABLE

typedef enum {
	SI2C_A1D1 = 0,
	SI2C_A1D2,
	SI2C_A2D1,
	SI2C_A2D2,
	SI2C_WIDTH_MAX,
} si2c_cw_t;

typedef enum {
	SI2C_NOP = 0,
	SI2C_EOC,		// end of commands
	SI2C_WR,
	SI2C_BWR,		// burst write
	SI2C_RD,
	SI2C_DELAY,
	SI2C_POLL, 		// poll until equal
	SI2C_POLLN, 		// poll until not equal
	SI2C_POLL_B1AND,	// poll until all bits become 1
	SI2C_POLL_B1OR,		// poll until any bit become 1
	SI2C_POLL_B0AND,	// poll until all bits become 0
	SI2C_POLL_B0OR,		// poll until any bit become 0
	SI2C_POLLV,		// poll specific variable
	SI2C_CID_MAX,
} si2c_cid_t;


/* alphabetical order. */
typedef enum {
	SI2C_INIT = 0,

	SI2C_PREVIEW,
	SI2C_SNAPSHOT,

	SI2C_AF_TRIGGER,

	SI2C_ANTIBANDING_50HZ,
	SI2C_ANTIBANDING_60HZ,
	SI2C_ANTIBANDING_AUTO,
	SI2C_ANTIBANDING_OFF,

	SI2C_ANTISHAKE_OFF,
	SI2C_ANTISHAKE_ON,

	SI2C_BRIGHTNESS_0,
	SI2C_BRIGHTNESS_M1,
	SI2C_BRIGHTNESS_M2,
	SI2C_BRIGHTNESS_M3,
	SI2C_BRIGHTNESS_M4,
	SI2C_BRIGHTNESS_M5,
	SI2C_BRIGHTNESS_P1,
	SI2C_BRIGHTNESS_P2,
	SI2C_BRIGHTNESS_P3,
	SI2C_BRIGHTNESS_P4,
	SI2C_BRIGHTNESS_P5,

	SI2C_EFFECT_AQUA,
	SI2C_EFFECT_BLACKBOARD,
	SI2C_EFFECT_BLUE,
	SI2C_EFFECT_CBLACKBOARD,	// colorful blackboard, for CamNote
	SI2C_EFFECT_CWHITEBOARD,	// colorful whiteboard, for CamNote
	SI2C_EFFECT_GREEN,
	SI2C_EFFECT_MONO,
	SI2C_EFFECT_NEGATIVE,
	SI2C_EFFECT_OFF,
	SI2C_EFFECT_POSTERIZE,
	SI2C_EFFECT_RED,
	SI2C_EFFECT_SEPIA,
	SI2C_EFFECT_SOLARIZE,
	SI2C_EFFECT_WHITEBOARD,

	SI2C_EXPOSURE_AVERAGE,
	SI2C_EXPOSURE_CENTER,
	SI2C_EXPOSURE_SPOT,

	SI2C_FLASH_AUTO,
	SI2C_FLASH_OFF,
	SI2C_FLASH_ON,
	SI2C_FLASH_TORCH,

	SI2C_FOCUS_AUTO,
	SI2C_FOCUS_MACRO,
	SI2C_FOCUS_INFINITY,
	SI2C_FOCUS_CAF,

	SI2C_FPS_FIXED7,	// for VT
	SI2C_FPS_FIXED8,	// for VT
	SI2C_FPS_FIXED10,
	SI2C_FPS_FIXED14,	// S5K6AAFX13 don't support 15 fps
	SI2C_FPS_FIXED15,	// for MMS recording
	SI2C_FPS_FIXED20,	// for camcorder
	SI2C_FPS_FIXED24,
	SI2C_FPS_FIXED25,
	SI2C_FPS_FIXED30,	// for camcorder
	SI2C_FPS_VARIABLE,	// variable fps (sensor-dependent)

	SI2C_REFLECT_OFF,
	SI2C_REFLECT_MIRROR,
	SI2C_REFLECT_MIRROR_WATER,
	SI2C_REFLECT_WATER,

	SI2C_SCENE_NORMAL,
	SI2C_SCENE_ACTION,
	SI2C_SCENE_AUTO,
	SI2C_SCENE_BARCODE,
	SI2C_SCENE_BEACH,
	SI2C_SCENE_CANDLELIGHT,
	SI2C_SCENE_FIREWORKS,
	SI2C_SCENE_INDOOR,
	SI2C_SCENE_LANDSCAPE,
	SI2C_SCENE_PARTY_INDOOR,
	SI2C_SCENE_NIGHT,
	SI2C_SCENE_NIGHT_PORTRAIT,
	SI2C_SCENE_OFF,
	SI2C_SCENE_PARTY,
	SI2C_SCENE_PORTRAIT,
	SI2C_SCENE_SNOW,
	SI2C_SCENE_SPORTS,
	SI2C_SCENE_STEADYPHOTO,
	SI2C_SCENE_SUNSET,
	SI2C_SCENE_TEXT,
	SI2C_SCENE_THEATRE,
	SI2C_SCENE_WINTER,

	SI2C_WB_AUTO,
	SI2C_WB_CLOUDY,
	SI2C_WB_DAYLIGHT,
	SI2C_WB_FLUORESCENT,
	SI2C_WB_INCANDESCENT,

#ifdef F_PANTECH_CAMERA_MIPI_ENABLE
	SI2C_MIPI_ENABLE,
	SI2C_MIPI_DISABLE,
#endif
	SI2C_PID_MAX,
} si2c_pid_t;


typedef struct {
	si2c_cid_t id;	// command id
	si2c_cw_t w;	// command width (addr/data)
	uint16_t a;	// addr
	uint16_t d;	// data
	uint32_t arg;	// multi-purpose argument
} si2c_cmd_t;

typedef struct {
	si2c_pid_t id;		// parameter id
	const si2c_cmd_t *cmds;	// const command table
} si2c_const_param_t;

/* PANTECH_CAMERA_TODO, id 가 필요한가? */

typedef struct {
	si2c_pid_t id;		// parameter id
	si2c_cmd_t *cmds;	// command table
} si2c_param_t;

typedef struct {
	si2c_pid_t id;
	const char *name;
} si2c_param_name_t;

int si2c_init(struct i2c_adapter *adap, si2c_const_param_t *src,
		si2c_param_t *dst);
void si2c_release(void);

int si2c_write_param(uint16_t sa, si2c_pid_t pid, si2c_param_t *params);
int si2c_write_cmds(uint16_t sa, si2c_cmd_t *cmds);
int si2c_write(si2c_cw_t w, uint16_t sa, uint16_t wa, uint16_t wd);
int si2c_read(si2c_cw_t w, uint16_t sa, uint16_t ra, uint16_t *rd);

#endif /* _SENSOR_I2C_H_ */
