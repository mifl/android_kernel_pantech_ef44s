/*
 * tcbd.c
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>

#include <bsp.h>

#include <linux/uaccess.h>
#include <asm/mach-types.h>

#include "tcbd.h"
#include "tcpal_os.h"
#include "tcpal_debug.h"
#include "tcpal_queue.h"

#include "tcbd_feature.h"
#include "tcbd_api_common.h"
#include "tcbd_drv_io.h"
#include "tcbd_stream_parser.h"

#include "tcbd_hal.h"

#define SUBCH_SELECT  1
#define SUBCH_RELEASE 0

#define I2C_BUS		1
#define I2C_ADDR	(0xA0>>1)

#ifdef __USE_TC_CPU__
volatile PGPIO  RGPIO;
volatile PPIC   RPIC;
#endif /*__USE_TC_CPU__*/

struct tcbd_drv_data {
	s32 open_cnt;
	s32 cmd_if;
	s32 data_if;
	struct tcbd_queue data_q;
	wait_queue_head_t wait_q;
	struct tcbd_device device;
};

static struct tcbd_drv_data tcbd_sample_drv;
static u32 tcbd_osc_clock = CLOCK_38400KHZ;
module_param(tcbd_osc_clock, int, 0644);

static void tcbd_test_something(void)
{

}

u32 tcbd_enqueue_data(void *buffer, u32 size, u32 subch_id, s32 type)
{
	if (tcbd_enqueue(&tcbd_sample_drv.data_q, 
		buffer, size, subch_id, type) == 0)
		wake_up(&tcbd_sample_drv.wait_q);
	return 0;
}

static s32 tcbd_get_stream_if_type(u32 arg)
{
	struct tcbd_device *dev = &tcbd_sample_drv.device;
	s32 peri_type = dev->peri_type;

	if (!arg || !access_ok(VERIFY_WRITE,
		(u8 __user *)(uintptr_t)arg, sizeof(int)))
		return -EFAULT;

	if (copy_to_user((void *)arg, (void *)&peri_type, sizeof(int)))
		return -EFAULT;

	return 0;
}

static s32 tcbd_get_ant_level6(u32 pcber, s32 rssi)
{
	s32 ant_level = 0;

	s32 ant75[] = {300, 450, 600, 800, 950, 1150, };
	s32 ant80[] = {-1,  450, 600, 800, 950, 1150, };
	s32 ant85[] = {-1,  -1,  600, 800, 900, 1150, };
	s32 ant90[] = {-1,  -1,   -1, 800, 950, 1150, };
	s32 ant100[] = {-1, -1,   -1, -1,  950, 1150, };
	s32 *ant = ant75;

	if (rssi > -75)
		ant = ant75;
	else if (rssi > -80)
		ant = ant80;
	else if (rssi > -85)
		ant = ant85;
	else if (rssi > -90)
		ant = ant90;
	else if (rssi > -100)
		ant = ant100;

	if ((s32)pcber <= ant[0])
		ant_level = 6;
	else if ((s32)pcber <= ant[1])
		ant_level = 5;
	else if ((s32)pcber <= ant[2])
		ant_level = 4;
	else if ((s32)pcber <= ant[3])
		ant_level = 3;
	else if ((s32)pcber <= ant[4])
		ant_level = 2;
	else if ((s32)pcber <= ant[5])
		ant_level = 1;
	else if ((s32)pcber > ant[5])
		ant_level = 0;

	return ant_level;
}

static s32 tcbd_get_ant_level5(u32 pcber, s32 rssi)
{
    s32 ant_level = 3;

    s32 ant75[] = {300, 500, 700, 900, 1150, };
    s32 ant80[] = {-1,  500, 700, 900, 1150, };
    s32 ant85[] = {-1,  -1,  700, 900, 1150, };
    s32 ant90[] = {-1,  -1,  -1,  900, 1150, };
    s32 ant100[]= {-1,  -1,  -1,  -1,  1150, };
    s32 *ant = ant75;

    if (rssi > -75)
    	ant = ant75;
    else if (rssi > -80)
    	ant = ant80;
    else if (rssi > -85)
    	ant = ant85;
    else if (rssi > -90)
    	ant = ant90;
    else if (rssi > -100)
    	ant = ant100;

    if ((s32)pcber <= ant[0])
    	ant_level = 5;
    else if ((s32)pcber <= ant[1])
    	ant_level = 4;
    else if ((s32)pcber <= ant[2])
    	ant_level = 3;
    else if ((s32)pcber <= ant[3])
    	ant_level = 2;
    else if ((s32)pcber <= ant[4])
    	ant_level = 1;
    else if ((s32)pcber  > ant[4])
    	ant_level = 0;

    return ant_level;
}

static s32 tcbd_get_status_data(u32 arg)
{
	struct tcbd_device *dev = &tcbd_sample_drv.device;
	struct tcbd_signal_quality sig_q;
	struct tcbd_status_data status;

	memset((void *)&sig_q, 0, sizeof(struct tcbd_signal_quality));

	if (!access_ok(VERIFY_WRITE,
		(u8 __user *)(uintptr_t)arg,
		sizeof(struct tcbd_signal_quality)))
		return -EFAULT;

	tcbd_read_signal_info(dev, &status);

	sig_q.PCBER = status.pcber_moving_avg;
	sig_q.SNR   = status.snr_moving_avg;
	sig_q.RSSI  = status.rssi;
	sig_q.RSBER = status.vber_moving_avg;

	printk(KERN_INFO"PCBER:%d, SNR:%d, RSSI:%d, "
		"RSBER:%d, TSPER:%d, ant level:%d\n",
		sig_q.PCBER, sig_q.SNR, sig_q.RSSI, sig_q.RSBER,
		status.tsper, tcbd_get_ant_level6(sig_q.PCBER, sig_q.RSSI));

	if (copy_to_user((void *)arg, (void *)&sig_q,
		sizeof(struct tcbd_signal_quality)))
		return -EFAULT;

	return 0;
}
s32 start_tune = 0;
static s32 tcbd_set_frequency(u32 arg)
{
	s32 ret = 0;
	u32 freq;
	u8 status;
	struct tcbd_device *dev = &tcbd_sample_drv.device;

	if(copy_from_user(&freq, (void *)arg, sizeof(int))) {
		printk("cannot copy from user parameter "
				"in IOCTL_TCBD_SET_FREQ");
		return -EFAULT;
	}
	tcbd_disable_irq(dev, 0);
	ret = tcbd_tune_frequency(dev, freq, 1500);
	if (ret < 0) {
		printk("failed to tune frequency! %d\n", ret);
		return -EFAULT;
	}
	tcbd_reset_queue(&tcbd_sample_drv.data_q);

	/*if scan mode
		tcbd_wait_tune must be called
	  else
		tcbd_wait_tune can be skipped
			to reduce channel switching time*/
	ret = tcbd_wait_tune(dev, &status);
	if (ret < 0) {
		printk("failed to tune frequency! 0x%02X, ret:%d\n",
							status, ret);
		ret =  -EFAULT;
	}
	start_tune = 1;
#if defined(__CSPI_ONLY__)
	tcbd_enable_irq(dev, TCBD_IRQ_EN_DATAINT);
#elif defined(__I2C_STS__)
	tcbd_enable_irq(dev, TCBD_IRQ_EN_FIFOAINIT);
#endif /*__I2C_STS__*/
	return ret;
}

/*#define __REG_SVC_LONG_ARGS__*/
static s32 tcbd_start_service(u32 arg)
{
	s32 ret = 0;
	u8 data_mode;
	struct tcbd_device *dev = &tcbd_sample_drv.device;
	struct tcbd_service svc = {0, };

	if (copy_from_user(&svc, (void *)arg, sizeof(svc))){
		printk(KERN_ERR"cannot copy from user parameter "
				"in IOCTL_TCBD_SELECT_SUBCH");
		return  -EFAULT;
	}

#if 0
	struct tcc_service_comp_info *_sch =
					tcc_fic_get_svc_comp_info(subch_id);
	svc.bit_rate = tcc_fic_get_bitrate(_sch);
	svc.cu_size = tcc_fic_get_cu_size(_sch);
	svc.cu_start = tcc_fic_get_cu_start(_sch);
	svc.plevel = tcc_fic_get_plevel(_sch);
	svc.ptype = tcc_fic_get_ptype(_sch);
	svc.reconf = 2;
	svc.rs_on = tcc_fic_get_rs(_sch);
	svc.subch_id = tcc_fic_get_subch_id(_sch);
	svc.type = (svc.rs_on) ?  SERVICE_TYPE_DMB: SERVICE_TYPE_DAB; 
#endif

	printk(KERN_INFO"type:%d, ptype:%d, subchid:%d, startcu:%d, "
		   "cusize:%d, plevel:%d, bitrate:%d\n",
		svc.type, svc.ptype, svc.subch_id, svc.start_cu,
		svc.size_cu, svc.plevel, svc.bitrate);

	tcbd_reset_queue(&tcbd_sample_drv.data_q);
#if !defined(__REG_SVC_LONG_ARGS__)
	/*if type is DMB(4) then data mode is '1' else '0' */
	data_mode = (svc.type == 4);
	ret = tcbd_register_service(dev, (u8)svc.subch_id, data_mode);
#else /*!__REG_SVC_LONG_ARGS__*/
	ret = tcbd_register_service_long(dev, &svc);
#endif /*__REG_SVC_LONG_ARGS__*/
	return ret;
}

static s32 tcbd_stop_service(u32 arg)
{
	struct tcbd_device *dev = &tcbd_sample_drv.device;
	struct tcbd_service svc = {0, };

	if (copy_from_user(&svc, (void *)arg, sizeof(svc))){
		printk(KERN_ERR"cannot copy from user parameter "
				"in IOCTL_TCBD_SELECT_SUBCH");
		return  -EFAULT;
	}

	printk(KERN_INFO"type:%d, ptype:%d, subchid:%d, startcu:%d, "
		   "cusize:%d, plevel:%d, bitrate:%d\n",
		svc.type, svc.ptype, svc.subch_id, svc.start_cu,
		svc.size_cu, svc.plevel, svc.bitrate);

	return tcbd_unregister_service(dev, (u8)svc.subch_id);
}

static s32 tcbd_rw_register(u32 arg)
{
	struct tcbd_ioctl_param_t param;
	struct tcbd_device *dev = &tcbd_sample_drv.device;

	if (copy_from_user((void *)&param, (void *)arg, sizeof(param)))
		return -EFAULT;

	switch (param.cmd) {
	case 1:
		tcbd_reg_read(dev, (u8)param.addr, &param.reg_data);
		break;
	case 2:
		tcbd_reg_write(dev, (u8)param.addr, param.reg_data);
		break;
	default:
		return -EINVAL;
		break;
	}

	if (!arg || !access_ok(VERIFY_WRITE,
				(u8 __user*)(uintptr_t)arg, sizeof(param)) )
		return -EFAULT;

	if (copy_to_user((void *)arg, (void *)&param, sizeof(param)))
		return -EFAULT;
	return 0;
}

/* for STS interface only.*/
static s32 tcbd_read_fic(u32 arg)
{
	s32 ret = 0;
	struct tcbd_device *dev = &tcbd_sample_drv.device;

	if (!arg || !access_ok(VERIFY_WRITE, (u8 __user*)(uintptr_t)arg,
							TCBD_FIC_SIZE))
		return -EFAULT;

	ret = tcbd_read_fic_data(dev, (u8 *)arg, TCBD_FIC_SIZE);
	if (ret < 0)
		return -EFAULT;

	return TCBD_FIC_SIZE;
}

static s32 tcbd_init_device(struct tcbd_device *_dev)
{
	u32 ret;
	u8 chip_id = 0;

	/* power on */
	tchal_power_on_device();

	/* command io open */
	ret = tcbd_io_open(_dev);
	if (ret < 0) {
		printk(KERN_ERR"failed io open %d \n", (int)ret);
		goto exit_init;
	}

	/* check chip id */
	tcbd_reg_read(_dev, TCBD_CHIPID, &chip_id);
	printk(KERN_INFO"chip id : 0x%X \n", chip_id);
	if (chip_id != TCBD_CHIPID_VALUE) {
		printk(KERN_ERR"invalid chip id !!! exit!\n");
		goto exit_init;
	}

#if defined(__I2C_STS__)
	tcbd_select_peri(_dev, PERI_TYPE_STS);
	tcbd_change_irq_mode(_dev, INTR_MODE_LEVEL_LOW);
#elif defined(__CSPI_ONLY__)
	tcbd_select_peri(_dev, PERI_TYPE_SPI_ONLY);
	tcbd_change_irq_mode(_dev, INTR_MODE_LEVEL_LOW);
#endif /*__CSPI_ONLY__*/
	if (tcbd_device_start(_dev, tcbd_osc_clock) < 0) {
		printk(KERN_ERR"could not start device!!\n");
		goto exit_init;
	}
	return 0;

exit_init:
	tcbd_io_close(&tcbd_sample_drv.device);
	tchal_power_down_device();
	return -EFAULT;
}


static int tcbd_ioctl(
	struct inode *inode,
	struct file *filp,
	unsigned int cmd,
	unsigned long arg)
{
	s32 ret = 0;

	switch (cmd) {
	case IOCTL_TCBD_SET_FREQ:
		ret = tcbd_set_frequency(arg);
		break;
	case IOCTL_TCBD_SELECT_SUBCH:
		ret = tcbd_start_service(arg);
		break;
	case IOCTL_TCBD_RELEASE_SUBCH:
		ret = tcbd_stop_service(arg);
		break;
	case IOCTL_TCBD_GET_MON_STAT:
		ret = tcbd_get_status_data(arg);
		break;
	case IOCTL_TCBD_RW_REG:
		ret = tcbd_rw_register(arg);
		break;
	case IOCTL_TCBD_READ_FIC:
		ret = tcbd_read_fic(arg);
		break;
	case IOCTL_TCBD_TEST:
		tcbd_test_something();
		break;
	case IOCTL_TCBD_GET_STREAM_IF:
		ret = tcbd_get_stream_if_type(arg);
		break;
	default:
		printk(KERN_ERR"bl: unrecognized ioctl (0x%x)\n", cmd);
		ret = -EINVAL;
	}

	return ret;
}

static int tcbd_release(struct inode *inode, struct file *filp)
{
	tcbd_disable_irq(&tcbd_sample_drv.device, 0);
	tcbd_deinit_queue(&tcbd_sample_drv.data_q);
	tcbd_io_close(&tcbd_sample_drv.device);
	tchal_power_down_device();

	tcbd_sample_drv.open_cnt--;
	printk(KERN_INFO"[%s:%d]\n", __func__, __LINE__);
	return 0;
}

static s32 tcbd_open(struct inode *inode, struct file *filp)
{
	struct tcbd_device *dev = &tcbd_sample_drv.device;
	static u8 mscBuffer[TCBD_MAX_FIFO_SIZE*6];

	printk(KERN_INFO"[%s:%d]\n", __func__, __LINE__);
	if (tcbd_sample_drv.open_cnt) {
		printk(KERN_ERR"driver aready opened!\n");
		return -EIO;
	}

	if (tcbd_init_device(dev) < 0)
		return -EIO;

	tcbd_init_queue(&tcbd_sample_drv.data_q, mscBuffer,
						TCBD_MAX_FIFO_SIZE*5);
	tcbd_sample_drv.open_cnt++;

	return 0;
}

static ssize_t tcbd_write(struct file *flip, const char *buf,
				size_t len, loff_t *ppos)
{
	return 0;
}

static ssize_t tcbd_read(struct file *flip, char *buf,
				size_t len, loff_t *ppos)
{
	s32 ret;
	s32 copied_size, type;
	u8 subch_id;

	if (!access_ok(VERIFY_WRITE,
				(u8 __user*)(uintptr_t)buf, len))
		return -EFAULT;

	copied_size = len;
	ret = tcbd_dequeue(&tcbd_sample_drv.data_q, (u8 *)(buf + 4),
				&copied_size, &subch_id, &type);

	buf[0] = type;
	buf[1] = subch_id;
	*((u16*)(buf + 2)) = (u16)copied_size;
	copied_size += 4;

	if (ret < 0)
		return -EFAULT;

	return copied_size;
}

static u32 tcbd_poll(struct file *flip, struct poll_table_struct *wait)
{
	if (!tcbd_queue_is_empty(&tcbd_sample_drv.data_q))
		return (POLLIN | POLLRDNORM);

	poll_wait(flip, &(tcbd_sample_drv.wait_q), wait);

	if (!tcbd_queue_is_empty(&tcbd_sample_drv.data_q))
		return (POLLIN | POLLRDNORM);

	return 0;
}

struct file_operations tcbd_fops = {
	.owner = THIS_MODULE,
	.open = tcbd_open,
	.release = tcbd_release,
	.ioctl = tcbd_ioctl,
	.read = tcbd_read,
	.write = tcbd_write,
	.poll = tcbd_poll,
};

#if defined(__I2C_STS__)
static struct i2c_board_info i2c_device[] = {
	{I2C_BOARD_INFO("tcbd", I2C_ADDR ), NULL}
};
#endif /*__I2C_STS__*/

static struct class *tcbd_class;
static s32 __init tcbd_init(void)
{
	s32 res;

	tchal_init();
#if defined(__I2C_STS__)
	i2c_register_board_info(I2C_BUS, i2c_device, ARRAY_SIZE(i2c_device));
	tcpal_set_i2c_io_function();
#elif defined(__CSPI_ONLY__)
	tcpal_set_cspi_io_function();
#endif /*__CSPI_ONLY_*/
	tcpal_irq_register_handler((void*)&tcbd_sample_drv.device);

	res = register_chrdev(TCBD_DEV_MAJOR, TCBD_DEV_NAME, &tcbd_fops);
	if (res < 0) return res;

	tcbd_class = class_create(THIS_MODULE, TCBD_DEV_NAME);
	if (NULL == device_create(tcbd_class, NULL,
				MKDEV(TCBD_DEV_MAJOR, TCBD_DEV_MINOR),
				NULL, TCBD_DEV_NAME))
		printk(KERN_ERR"device_create failed\n");
	else
		printk(KERN_INFO"[%s:%d] device_created!!\n",
					__func__, __LINE__);

	memset(&tcbd_sample_drv, 0, sizeof(tcbd_sample_drv));
	init_waitqueue_head(&(tcbd_sample_drv.wait_q));

	return 0;
}

static void __exit tcbd_exit(void)
{
	tcpal_irq_unregister_handler();

	device_destroy(tcbd_class, MKDEV(TCBD_DEV_MAJOR, TCBD_DEV_MINOR));
	class_destroy(tcbd_class);
	unregister_chrdev(TCBD_DEV_MAJOR, TCBD_DEV_NAME);
	printk(KERN_INFO "%s\n", __func__);
}

module_init(tcbd_init);
module_exit(tcbd_exit);

MODULE_AUTHOR("Telechips Inc.");
MODULE_DESCRIPTION("tcbd control");
MODULE_LICENSE("GPL");

