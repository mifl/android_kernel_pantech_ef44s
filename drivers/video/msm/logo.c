/* drivers/video/msm/logo.c
 *
 * Show Logo in RLE 565 format
 *
 * Copyright (C) 2008 Google Incorporated
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fb.h>
#include <linux/vt_kern.h>
#include <linux/unistd.h>
#ifdef CONFIG_FB_MSM_CLEARUP_READ_SYSCALLS
#include <linux/uaccess.h>
#else
#include <linux/syscalls.h>
#endif

#include <linux/irq.h>
#include <asm/system.h>

#if defined(CONFIG_MACH_MSM8960_VEGAPVW) || defined(CONFIG_MACH_MSM8960_SIRIUSLTE) || defined(CONFIG_MACH_MSM8960_MAGNUS)  
#define fb_width(fb)	((fb)->var.xres + 16) // 720 + 16 for 32bit align , shinbrad ( shinjg ) 20110322
#else
#define fb_width(fb)	((fb)->var.xres)
#endif

#define fb_height(fb)	((fb)->var.yres)
#define fb_size(fb)	((fb)->var.xres * (fb)->var.yres * 2)

//#define CONFIG_PANTECH_FB_24BPP_RGB888 		1

typedef unsigned int IBUF_TYPE;
#if defined(CONFIG_FB_MSM_DEFAULT_DEPTH_RGBA8888)
static void memset32(void *_ptr, unsigned int val, unsigned count)
{
        register unsigned int *ptr = _ptr;
        count >>= 2;
        while (count--)
                *ptr++ = val;
}
#else
static void memset16(void *_ptr, unsigned short val, unsigned count)
{
	unsigned short *ptr = _ptr;
	count >>= 1;
	while (count--)
		*ptr++ = val;
}
#endif

#ifdef CONFIG_FB_MSM_CLEARUP_READ_SYSCALLS
/* 565RLE image format: [count(2 bytes), rle(2 bytes)] */
int load_565rle_image(char *filename, bool bf_supported)
{
	struct fb_info *info;
	unsigned count,max;
	struct file *filp;
	int err = 0;
	IBUF_TYPE *data, *bits, *ptr;
	mm_segment_t fs;
	loff_t pos;

	fs = get_fs();
	set_fs(get_ds());

	info = registered_fb[0];
	if (!info) {
		printk(KERN_WARNING "%s: Can not access framebuffer\n",
			__func__);
		return -ENODEV;
	}

	filp = filp_open(filename, O_RDONLY,0);
	if(IS_ERR(filp)){
		printk(KERN_WARNING "%s: Can not open %s\n",
			__func__, filename);
			err = PTR_ERR(filp);
			goto err_logo_set_fs;
	}

	count = (unsigned)i_size_read(filp->f_path.dentry->d_inode);
	if (count == 0) {
		err = -EIO;
		goto err_logo_close_file;
	}
	data = kmalloc(count, GFP_KERNEL);
	if (!data) {
		printk(KERN_WARNING "%s: Can not alloc data\n", __func__);
		err = -ENOMEM;
		goto err_logo_close_file;
	}
	pos = 0;
	if(vfs_read(filp, (char __user *)data, count, &pos) != count) {
		err = -EIO;
		goto err_logo_free_data;
	}

	max = fb_width(info) * fb_height(info);
	ptr = data;
	if (bf_supported && (info->node == 1 || info->node == 2)) {
		err = -EPERM;
		pr_err("%s:%d no info->creen_base on fb%d!\n",
		       __func__, __LINE__, info->node);
		goto err_logo_free_data;
	}	
	bits = (IBUF_TYPE *)(info->screen_base);
	printk("[%s] logo CONFIG_PANTECH_FB_24BPP_RGB888 test _ shinbrad \n",__func__);

	while (count > 3) {
		unsigned n = ptr[0];
		if (n > max)
			break;

		memset32((unsigned int *)bits, ptr[1], n << 2);
		bits += n;
		max -= n;
		ptr += 2;
		count -= 4;
	}

err_logo_free_data:
	kfree(data);
err_logo_close_file:
	filp_close(filp, current->files);
err_logo_set_fs:
	set_fs(fs);
	return err;
}
#else /* CONFIG_FB_MSM_CLEARUP_READ_SYSCALLS */
/* 565RLE image format: [count(2 bytes), rle(2 bytes)] */
int load_565rle_image(char *filename, bool bf_supported)
{
	struct fb_info *info;
	int fd, count, err = 0;
	unsigned max;
#if defined(CONFIG_FB_MSM_DEFAULT_DEPTH_RGBA8888)
	IBUF_TYPE *data, *bits, *ptr;
#else
	unsigned short *data, *bits, *ptr;
#endif

	info = registered_fb[0];
	if (!info) {
		printk(KERN_WARNING "%s: Can not access framebuffer\n",
			__func__);
		return -ENODEV;
	}

	fd = sys_open(filename, O_RDONLY, 0);
	if (fd < 0) {
		printk(KERN_WARNING "%s: Can not open %s\n",
			__func__, filename);
		return -ENOENT;
	}
	count = sys_lseek(fd, (off_t)0, 2);
	if (count <= 0) {
		err = -EIO;
		goto err_logo_close_file;
	}
	sys_lseek(fd, (off_t)0, 0);
	data = kmalloc(count, GFP_KERNEL);
	if (!data) {
		printk(KERN_WARNING "%s: Can not alloc data\n", __func__);
		err = -ENOMEM;
		goto err_logo_close_file;
	}
	if (sys_read(fd, (char *)data, count) != count) {
		err = -EIO;
		goto err_logo_free_data;
	}

	max = fb_width(info) * fb_height(info);
	ptr = data;
	if (bf_supported && (info->node == 1 || info->node == 2)) {
		err = -EPERM;
		pr_err("%s:%d no info->creen_base on fb%d!\n",
		       __func__, __LINE__, info->node);
		goto err_logo_free_data;
	}
#if defined(CONFIG_FB_MSM_DEFAULT_DEPTH_RGBA8888)
	bits = (IBUF_TYPE *)(info->screen_base);
	printk("[%s] logo CONFIG_PANTECH_FB_24BPP_RGB888 test _ shinbrad \n",__func__);
#else
	bits = (unsigned short *)(info->screen_base);
	printk("[%s] logo CONFIG_PANTECH_FB_24BPP_NOT_RGB888 test _ shinbrad \n",__func__);
#endif

	while (count > 3) {
		unsigned n = ptr[0];
		if (n > max)
			break;
#if defined(CONFIG_FB_MSM_DEFAULT_DEPTH_RGBA8888)
		memset32((unsigned int *)bits, ptr[1], n << 2);
#else
		memset16(bits, ptr[1], n << 1);
#endif
		bits += n;
		max -= n;
		ptr += 2;
		count -= 4;
	}

err_logo_free_data:
	kfree(data);
err_logo_close_file:
	sys_close(fd);
	return err;
}
#endif /* CONFIG_FB_MSM_CLEARUP_READ_SYSCALLS */
EXPORT_SYMBOL(load_565rle_image);
