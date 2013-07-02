/*
 * tcbd_test_io.c
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

#include "tcpal_os.h"
#include "tcpal_debug.h"

#include "tcbd_feature.h"
#include "tcbd_api_common.h"
#include "tcbd_drv_ip.h"
#include "tcbd_drv_io.h"

#define DEBUG_DUMP_FILE

#define SIZE_TEST_BUFFER (1024*4)

static u8 test_buffer[SIZE_TEST_BUFFER];

#if defined(DEBUG_DUMP_FILE)
void file_dump(u8 *p, u32 size, char *path)
{
	struct file *flip = NULL;
	mm_segment_t old_fs;

	if (path == NULL) {
		tcbd_debug(DEBUG_ERROR, "invalid filename! %s\n", path);
		return;
	}

	if (p == NULL) {
		tcbd_debug(DEBUG_ERROR, "Invaild pointer! 0x%X\n", (u32)p);
		return;
	}

	flip = filp_open(path, O_CREAT | O_RDWR | O_APPEND | O_LARGEFILE, 0);
	if (IS_ERR(flip)) {
		tcbd_debug(DEBUG_ERROR, "%s open failed\n", path);
		return;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	vfs_write(flip, p, size, &flip->f_pos);
	set_fs(old_fs);

	filp_close(flip, NULL);
}
#endif


static void tcbd_test_memory_io(struct tcbd_device *_device)
{
	s32 ret = 0;
	static u8 tmp_buffer[SIZE_TEST_BUFFER];

	memset(tmp_buffer, 0, SIZE_TEST_BUFFER);

	tcbd_change_memory_view(_device, EP_RAM0_RAM1);
	ret = tcbd_mem_write(_device,
			CODE_MEM_BASE, test_buffer, SIZE_TEST_BUFFER);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "memory write failed %d\n", (int)ret);
		goto exit_test;
	}

	ret = tcbd_mem_read(_device,
			CODE_MEM_BASE, tmp_buffer, SIZE_TEST_BUFFER);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "memory read failed %d\n", (int)ret);
		goto exit_test;
	}

#ifdef DEBUG_DUMP_FILE
	file_dump(test_buffer, SIZE_TEST_BUFFER, "/mnt/sdcard/tflash/org.bin");
	file_dump(tmp_buffer, SIZE_TEST_BUFFER, "/mnt/sdcard/tflash/read.bin");
#endif

	if (memcmp(tmp_buffer, test_buffer, SIZE_TEST_BUFFER))
		goto exit_test;

	tcbd_debug(DEBUG_ERROR, "Memory Read/Write test succeed!\n");
	return;

exit_test:
	tcbd_debug(DEBUG_ERROR, "Memory Read/Write test failed!\n");
}

static void tcbd_test_reg_rw(struct tcbd_device *_device)
{
	s32 ret = 0;
	u8 addr = 0x1B, rdata, wdata = 0xAB;

	ret = tcbd_reg_read(_device, addr, &rdata);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "RegRead failed! %d\n", (int)ret);
		goto exit_rest;
	}
	tcbd_debug(DEBUG_ERROR, "default value : 0x%X\n", rdata);

	tcbd_debug(DEBUG_ERROR, "write 0x%X\n", wdata);
	ret = tcbd_reg_write(_device, addr, wdata);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "reg_write failed! %d\n", (int)ret);
		goto exit_rest;
	}

	ret = tcbd_reg_read(_device, addr, &rdata);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR,
			"reg_read_burst_cont failed! %d\n", (int)ret);
		goto exit_rest;
	}
	tcbd_debug(DEBUG_ERROR,
		"single RW :: W[0x%X] == R[0x%X]\n", wdata, rdata);
	return;

exit_rest:
	tcbd_debug(DEBUG_ERROR, "register burst Write/Read failed!\n");

}

static void tcbd_test_reg_burst_rw_cont(struct tcbd_device *_device)
{
	s32 ret = 0;
	u8 addr;
	u32 rdata, wdata;

	addr = TCBD_CMDDMA_SADDR;
	wdata = SWAP32(CODE_MEM_BASE);

	ret = tcbd_reg_read_burst_cont(_device, addr,
					(u8 *)&rdata, sizeof(u32));
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "reg_read_burst_cont failed! %d\n",
					(int)ret);
		goto exit_test;
	}
	tcbd_debug(DEBUG_ERROR, "default value : 0x%X\n", (u32)SWAP32(rdata));
	tcbd_debug(DEBUG_ERROR, "write 0x%X\n", (u32)wdata);
	ret = tcbd_reg_write_burst_cont(
			_device,
			addr,
			(u8 *)&wdata,
			sizeof(u32));
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "reg_write_burst_cont failed! %d\n",
					(int)ret);
		goto exit_test;
	}

	ret = tcbd_reg_read_burst_cont(
			_device,
			addr,
			(u8 *)&rdata,
			sizeof(u32));
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "reg_read_burst_cont failed! %d\n",
					(int)ret);
		goto exit_test;
	}
	tcbd_debug(DEBUG_ERROR, "multi RW :: W[0x%X] == R[0x%X]\n",
			(u32)wdata, (u32)SWAP32(rdata));
	return;

exit_test:
	tcbd_debug(DEBUG_ERROR, "register burst Write/Read failed!\n");
}

static void tcbd_test_burst_rw_fix(struct tcbd_device *_device)
{

}

#if 0
static void tcbd_test_boot(struct tcbd_device *_device)
{
	s32 ret = 0;
	s32 ver = 0;
	u8 *bin = test_buffer;
	s32 size = SIZE_TEST_BUFFER;

#ifdef FEATURE_DMB_CLK_19200
	ret = tcbd_init_pll(_device, CLOCK_19200KHZ);
#elif defined(FEATURE_DMB_CLK_24576)
	ret = tcbd_init_pll(_device, CLOCK_24576KHZ);
#endif
	ret |= tcbd_init_dsp(_device, bin, size);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "failed to boot! %d\n", (int)ret);
		return;
	}
	tcbd_debug(DEBUG_ERROR, "Download success!!\n");

	ret |= tcbd_get_rom_version(_device, &ver);
	if (ret < 0) {
		tcbd_debug(DEBUG_ERROR, "failed to receive boot version! %d\n",
					(int)ret);
		return;
	}

	tcbd_init_div_io(_device, DIV_IO_TYPE_SINGLE);
	tcbd_debug(DEBUG_ERROR, "received boot version: 0x%X\n", (u32)ver);
}
#endif

static void tcbd_test_freq_set(struct tcbd_device *_device)
{

}

void TcbdTestIo(struct tcbd_device *_device, s32 _testItem)
{
	switch (_testItem) {
	/*case 0:
		tcbd_test_boot(_device);
		break;*/
	case 1:
		tcbd_test_memory_io(_device);
		break;
	case 2:
		tcbd_test_reg_burst_rw_cont(_device);
		break;
	case 3:
		tcbd_test_reg_rw(_device);
		break;
	case 4:
		tcbd_test_freq_set(_device);
		break;
	case 5:
		tcbd_test_burst_rw_fix(_device);
		break;
	}
}
