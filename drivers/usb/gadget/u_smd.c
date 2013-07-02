/*
 * u_smd.c - utilities for USB gadget serial over smd
 *
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This code also borrows from drivers/usb/gadget/u_serial.c, which is
 * Copyright (C) 2000 - 2003 Al Borchers (alborchers@steinerpoint.com)
 * Copyright (C) 2008 David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 * Copyright (C) 1999 - 2002 Greg Kroah-Hartman (greg@kroah.com)
 * Copyright (C) 2000 Peter Berger (pberger@brimson.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/termios.h>
#include <mach/msm_smd.h>
#include <linux/debugfs.h>

#include "u_serial.h"

// 20111014, albatros, for PDL IDLE
#ifdef FEATURE_PANTECH_PDL_DLOAD
#include <linux/reboot.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <asm/uaccess.h>
#include <linux/workqueue.h>

#include "sky_rawdata.h"

#ifdef F_PANTECH_SECBOOT
//#include "../../../../../../../boot_images/core/securemsm/secboot/shared/inc/secboot_types.h"
typedef struct
{ 
  #define SECBOOT_FUSE_FLAG_MAGIC_NUM     0xAAFFFF
  #define SECBOOT_FUSE_FLAG_UNSET         0xF0F0F0
  #define SECBOOT_FUSE_FLAG_SET           0xF1F1F1
  #define SECBOOT_FUSE_NOT_BLOWN          0xF2F2F2
  #define SECBOOT_FUSE_BLOWN              0xF3F3F3  

  unsigned int secboot_magic_num;

  unsigned int auth_en;
  unsigned int shk_blow;
  unsigned int shk_rw_dis;
  unsigned int jtag_dis;
} secboot_fuse_flag;
/*
typedef struct
{ 
  #define SECBOOT_SECBOOT_CHECK_MAGIC_NUM     0xCCBBBB
  #define SECBOOT_AUTH_IMG_NUM                16

  uint32 secboot_magic_num;
  uint32 auth_img_result[SECBOOT_AUTH_IMG_NUM];
} secboot_auth_img_result;
*/
#endif

// 20120105, albatros, ?????? ??????? sky_rawdata.h ???? ??????? ??? ?????? ??? ???? ????? o??
// ¢®E¡Ëc¡Ë¢ç¢®¢¯¢®E¡Ëc¡Ë¢ç¢®¢¯?¢®E¡ËcIa??? makefile ????
#if 0
// read rawdata for PDL IMEI
#define SECTOR_SIZE                 512
#define SKY_RAWDATA_MAX             (8192*SECTOR_SIZE) // 4MB
#define SECTOR_SIZE_DEFAULT         1

//----------------------------------------------------------------------------------
// 20110929, albatros, partition size?? ¢®E¡Ëc¡Ë¢ç¢®¢¯¢®E¡Ëc¡Ë¢ç¢®¢¯???? ???? ?¡Ë¢ç¢®¡¿I¡Ë¢ç¢®¡¿¡Ë¢çI???.
// ?????? ¢®E¡Ëc¡Ë¢ç¢®¢¯¢®E¡Ëc¡Ë¢ç¢®¢¯????? ?????? 80?? ???¢®E¡ËcE¢®E¡ËcI?.
// MEMMAP_PAGE_SIZE*32= 16K + backup size 16K = 32K (512*64)
#define GPT_PARTITION_SIZE          (SECTOR_SIZE*80) 

#define DLOAD_SECTOR_START          0
#define DLOAD_INFO_OFFSET           (DLOAD_SECTOR_START)
#define PARTITION_INFO_OFFSET       (DLOAD_SECTOR_START+SECTOR_SIZE)
#define BACKUP_DLOAD_INFO_OFFSET    (DLOAD_SECTOR_START + GPT_PARTITION_SIZE)
#define DLOAD_STATUS_OFFSET         (DLOAD_SECTOR_START + GPT_PARTITION_SIZE*2)
#define DLOAD_HISTORY_OFFSET        (DLOAD_SECTOR_START + GPT_PARTITION_SIZE*2 + SECTOR_SIZE)
#define DLOAD_SECTOR_MAX            (DLOAD_HISTORY_OFFSET + SECTOR_SIZE*20)

//----------------------------------------------------------------------------------

#define BACKUP_SECTOR_START         (DLOAD_SECTOR_START + DLOAD_SECTOR_MAX + SECTOR_SIZE)

#define SECUREESN_START             BACKUP_SECTOR_START
#define SECUREESN_LENGTH            ((SECTOR_SIZE*25) * 2) // use sectors as pages // "*2" means 2 blocks

#define RFCAL_BACKUP_START          (SECUREESN_START+SECUREESN_LENGTH)
#define RFCAL_BACKUP_LENGTH         (SECTOR_SIZE*100)

#define FACTORY_EFS_INIT_START      (RFCAL_BACKUP_START+RFCAL_BACKUP_LENGTH)
#define FACTORY_EFS_INIT_LENGTH     (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define MSEC_BACKUP_START           (FACTORY_EFS_INIT_START+FACTORY_EFS_INIT_LENGTH)
#define MSEC_BACKUP_LENGTH          (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define FUNCTEST_RESULT_INIT_START  (MSEC_BACKUP_START+MSEC_BACKUP_LENGTH)
#define FUNCTEST_RESULT_INIT_LENGTH (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define WIFI_DEVICE_INFO_START      (FUNCTEST_RESULT_INIT_START+FUNCTEST_RESULT_INIT_LENGTH)
#define WIFI_DEVICE_INFO_LENGTH     (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define BT_DEVICE_INFO_START        (WIFI_DEVICE_INFO_START+WIFI_DEVICE_INFO_LENGTH)
#define BT_DEVICE_INFO_LENGTH       (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define PWR_ON_CNT_START            (BT_DEVICE_INFO_START+BT_DEVICE_INFO_LENGTH)
#define PWR_ON_CNT_LENGTH           (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define SDCARD_UPDATE_START         (PWR_ON_CNT_START+PWR_ON_CNT_LENGTH)
#define SDCARD_UPDATE_LENGTH        (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define USB_CHARGING_START          (SDCARD_UPDATE_START+SDCARD_UPDATE_LENGTH)
#define USB_CHARGING_LENGTH         (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define PERMANENTMEMORY_START       (USB_CHARGING_START+USB_CHARGING_LENGTH)
#define PERMANENTMEMORY_LENGTH      (SECTOR_SIZE*2)

//F_PANTECH_MEID_IMEI_ADDR_BACKUP
#define NON_SECURE_IMEI_START       (PERMANENTMEMORY_START+PERMANENTMEMORY_LENGTH)
#define NON_SECURE_IMEI_LENGTH      (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)
#endif

enum {
  DLOADINFO_NONE_STATE = 0,
  DLOADINFO_AT_RESPONSE_STATE,
  DLOADINFO_PHONE_INFO_STATE,
  DLOADINFO_HASH_TABLE_STATE,
  DLOADINFO_PARTI_TABLE_STATE,
  DLOADINFO_FINISH_STATE,
  DLOADINFO_MAX_STATE
};

enum {
  FINISH_CMD = 0,
  PHONE_INFO_CMD,
  HASH_TABLE_CMD,
  PARTI_TABLE_CMD,
  MAX_PHONEINFO_CMD
};

typedef struct {
  unsigned int partition_size_;
  char   partition_name_[ 8 ];
}partition_info_type;

typedef  unsigned long int  uint32;      /* Unsigned 32 bit value */
typedef  unsigned char      uint8;       /* Unsigned 8  bit value */

typedef struct {
  uint32 version_;
  char   model_name_    [ 16 ];
  char   binary_version_[ 16 ];
  uint32 fs_version_;
  uint32 nv_version_;
  char   build_date_    [ 16 ];
  char   build_time_    [ 16 ];

  //???? ????¢®E¡ËcIa? ????
  uint32 boot_loader_version_;                // ???? ???? ???????? ?¢®E¡ËcE¢®E¡ËcI? ????¢®E¡ËcIa??? ????
  uint32 boot_section_id_[4];                 // ????¢®E¡ËcIa? ?????? ????? ???? ????? ???? ???? ??? section id
  
  // ???? ???? EFS ????
  uint32              efs_size_;                // ??u EFS ??????
  uint32              partition_num_;           // EFS?? ???? ??????? ????
  partition_info_type partition_info_[ 6 ];     // EFS?? ??????? ????

  uint32 FusionID;
  uint8  Imei[15];
  /* F_PANTECH_SECBOOT - DO NOT erase this comment! */
  char   secure_magic_[ 7 ];

  uint8  reserved_2[ 54 ];
  
} phoneinfo_type;

static struct delayed_work phoneinfo_read_wqst;
static char pantech_phoneinfo_buff[SECTOR_SIZE]={0,};

#define NV_UE_IMEI_SIZE             9
#define IMEI_ADDR_MAGIC_NUM         0x88776655

typedef struct
{
  uint32 imei_magic_num;
  uint8 backup_imei[NV_UE_IMEI_SIZE];
  uint8 emptspace[51];
} imei_backup_info_type;


static void load_phoneinfo_with_imei(struct work_struct *work_s);
static unsigned fill_writereq(int *dloadinfo_state, struct usb_request *writereq);
static unsigned int fill_phoneinfo(char *buff);
static unsigned int check_phoneinfo(void);
#endif 


#define SMD_RX_QUEUE_SIZE		8
#define SMD_RX_BUF_SIZE			2048

#define SMD_TX_QUEUE_SIZE		8
#define SMD_TX_BUF_SIZE			2048

static struct workqueue_struct *gsmd_wq;

#define SMD_N_PORTS	2
#define CH_OPENED	0
#define CH_READY	1
struct smd_port_info {
	struct smd_channel	*ch;
	char			*name;
	unsigned long		flags;
};

struct smd_port_info smd_pi[SMD_N_PORTS] = {
	{
		.name = "DS",
	},
	{
		.name = "UNUSED",
	},
};

struct gsmd_port {
	unsigned		port_num;
	spinlock_t		port_lock;

	unsigned		n_read;
	struct list_head	read_pool;
	struct list_head	read_queue;
	struct work_struct	push;

	struct list_head	write_pool;
	struct work_struct	pull;

	struct gserial		*port_usb;

	struct smd_port_info	*pi;
	struct delayed_work	connect_work;

	/* At present, smd does not notify
	 * control bit change info from modem
	 */
	struct work_struct	update_modem_ctrl_sig;

#define SMD_ACM_CTRL_DTR		0x01
#define SMD_ACM_CTRL_RTS		0x02
	unsigned		cbits_to_modem;

#define SMD_ACM_CTRL_DCD		0x01
#define SMD_ACM_CTRL_DSR		0x02
#define SMD_ACM_CTRL_BRK		0x04
#define SMD_ACM_CTRL_RI		0x08
	unsigned		cbits_to_laptop;

	/* pkt counters */
	unsigned long		nbytes_tomodem;
	unsigned long		nbytes_tolaptop;
};

static struct smd_portmaster {
	struct mutex lock;
	struct gsmd_port *port;
	struct platform_driver pdrv;
} smd_ports[SMD_N_PORTS];
static unsigned n_smd_ports;

static void gsmd_free_req(struct usb_ep *ep, struct usb_request *req)
{
	kfree(req->buf);
	usb_ep_free_request(ep, req);
}

static void gsmd_free_requests(struct usb_ep *ep, struct list_head *head)
{
	struct usb_request	*req;

	while (!list_empty(head)) {
		req = list_entry(head->next, struct usb_request, list);
		list_del(&req->list);
		gsmd_free_req(ep, req);
	}
}

static struct usb_request *
gsmd_alloc_req(struct usb_ep *ep, unsigned len, gfp_t flags)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, flags);
	if (!req) {
		pr_err("%s: usb alloc request failed\n", __func__);
		return 0;
	}

	req->length = len;
	req->buf = kmalloc(len, flags);
	if (!req->buf) {
		pr_err("%s: request buf allocation failed\n", __func__);
		usb_ep_free_request(ep, req);
		return 0;
	}

	return req;
}

static int gsmd_alloc_requests(struct usb_ep *ep, struct list_head *head,
		int num, int size,
		void (*cb)(struct usb_ep *ep, struct usb_request *))
{
	int i;
	struct usb_request *req;

	pr_debug("%s: ep:%p head:%p num:%d size:%d cb:%p", __func__,
			ep, head, num, size, cb);

	for (i = 0; i < num; i++) {
		req = gsmd_alloc_req(ep, size, GFP_ATOMIC);
		if (!req) {
			pr_debug("%s: req allocated:%d\n", __func__, i);
			return list_empty(head) ? -ENOMEM : 0;
		}
		req->complete = cb;
		list_add(&req->list, head);
	}

	return 0;
}

static void gsmd_start_rx(struct gsmd_port *port)
{
	struct list_head	*pool;
	struct usb_ep		*out;
	unsigned long	flags;
	int ret;

	if (!port) {
		pr_err("%s: port is null\n", __func__);
		return;
	}

	spin_lock_irqsave(&port->port_lock, flags);

	if (!port->port_usb) {
		pr_debug("%s: USB disconnected\n", __func__);
		goto start_rx_end;
	}

	pool = &port->read_pool;
	out = port->port_usb->out;

	while (test_bit(CH_OPENED, &port->pi->flags) && !list_empty(pool)) {
		struct usb_request	*req;

		req = list_entry(pool->next, struct usb_request, list);
		list_del(&req->list);
		req->length = SMD_RX_BUF_SIZE;

		spin_unlock_irqrestore(&port->port_lock, flags);
		ret = usb_ep_queue(out, req, GFP_KERNEL);
		spin_lock_irqsave(&port->port_lock, flags);
		if (ret) {
			pr_err("%s: usb ep out queue failed"
					"port:%p, port#%d\n",
					 __func__, port, port->port_num);
			list_add_tail(&req->list, pool);
			break;
		}
	}
start_rx_end:
	spin_unlock_irqrestore(&port->port_lock, flags);
}

// 20111014, albatros, for PDL IDLE
#ifdef FEATURE_PANTECH_PDL_DLOAD
static unsigned int fill_phoneinfo(char *buff)
{
  phoneinfo_type *pantech_phoneinfo_buff_ptr;

  pantech_phoneinfo_buff_ptr = (phoneinfo_type *)&pantech_phoneinfo_buff[16];
  
  if(pantech_phoneinfo_buff_ptr->version_== 0) 
  {
    printk(KERN_ERR "%s: phoneinfo is broken or empty\n", __func__);
    return 0;
  }

  memcpy(buff, pantech_phoneinfo_buff, 16 + sizeof(phoneinfo_type));
  printk(KERN_INFO "%s: phoneinfo is OK\n", __func__);  
  return (16 + sizeof(phoneinfo_type));
}

static unsigned int check_phoneinfo(void)
{
  phoneinfo_type *pantech_phoneinfo_buff_ptr;

  pantech_phoneinfo_buff_ptr = (phoneinfo_type *)&pantech_phoneinfo_buff[16];
  
  if(pantech_phoneinfo_buff_ptr->version_== 0) 
  {
// 20120314, albatros, packet communication performance..
//    printk(KERN_ERR "%s: phoneinfo is broken or empty\n", __func__);
    return 0;
  }

// 20120314, albatros, packet communication performance..
//  printk(KERN_INFO "%s: phoneinfo is OK\n", __func__);  
  return 1;
}

static void load_phoneinfo_with_imei(struct work_struct *work_s)
{
  struct file *rawdata_filp;
  char read_buf[SECTOR_SIZE];
  mm_segment_t oldfs;
  int rc;
  phoneinfo_type *pantech_phoneinfo_buff_ptr;

  static int read_count = 0;

#if defined(T_OSCAR) || defined(T_MAGNUS) || defined(T_STARQ) || defined(T_VEGAPVW) 
  imei_backup_info_type *imei_backup_info_buf;
#endif
  
  printk(KERN_INFO "%s: read phone info start\n", __func__);

  // phoneinfo buffer init
  memset( pantech_phoneinfo_buff, 0x0, SECTOR_SIZE );  

  // phoneinfo packet ?????? ?¡Ë¢çE¢®Ec¡Ë¢ç¢®¡¿¢®E¡Ëc? ?? ???? ??????
  pantech_phoneinfo_buff[0] = 1;
  pantech_phoneinfo_buff[9] = 1;
  pantech_phoneinfo_buff_ptr = (phoneinfo_type *)&pantech_phoneinfo_buff[16];

  // phoneinfo ??????? ????.
  oldfs = get_fs();
  set_fs(KERNEL_DS);
  rawdata_filp = filp_open("/dev/block/mmcblk0p10", O_RDONLY | O_SYNC, 0);
  if( IS_ERR(rawdata_filp) )
  {
    set_fs(oldfs);
    printk(KERN_ERR "%s: filp_open error\n",__func__);
		return;
  }
  set_fs(oldfs);
  printk(KERN_INFO "%s: file open OK\n", __func__);

  // ??????? ??????? ?¡Ë¢ç¢®¡¿I¢®E¡ËcI??? phoneinfo ?? ?¢®E¡Ëc¡§I¢®¨ÏO¢®E¡ËcE¢®E¡ËcI?.
  rawdata_filp->f_pos = 0;
  memset( read_buf, 0x0, SECTOR_SIZE );
  // read
	if(((rawdata_filp->f_flags & O_ACCMODE) & O_RDONLY) != 0)
  {
    printk(KERN_ERR "%s: read permission denied\n",__func__);
    return;
	}

  oldfs = get_fs();
  set_fs(KERNEL_DS);
  rc = rawdata_filp->f_op->read(rawdata_filp, read_buf, SECTOR_SIZE, &rawdata_filp->f_pos);
  if (rc < 0) 
  {
    set_fs(oldfs);
    printk(KERN_ERR "%s: read phoneinfo error = %d \n",__func__,rc);
    filp_close(rawdata_filp, NULL);
		return;
  }
  set_fs(oldfs);
  memcpy(pantech_phoneinfo_buff_ptr, &read_buf[32], sizeof(phoneinfo_type));

  printk(KERN_INFO "%s: read Phoneinfo OK\n", __func__);

  // ???? phoneinfo ??? imei ???? a???? ????? rawdata ?? imei ???? ?¢®E¡Ëc¡§I¢®¨ÏO¢®E¡ËcE¢®E¡ËcI?.
  // lseek
#if defined(T_OSCAR) || defined(T_MAGNUS) || defined(T_STARQ) || defined(T_VEGAPVW) 
  rawdata_filp->f_pos = NON_SECURE_IMEI_START;
  memset( read_buf, 0x0, SECTOR_SIZE );
  printk(KERN_ERR "%s: rawdata_filp->f_pos = %x \n",__func__,NON_SECURE_IMEI_START);

  // read
	if(((rawdata_filp->f_flags & O_ACCMODE) & O_RDONLY) != 0)
  {
    printk(KERN_ERR "%s: read permission denied\n",__func__);
    return;
	}
  oldfs = get_fs();
  set_fs(KERNEL_DS);
  rc = rawdata_filp->f_op->read(rawdata_filp, read_buf, 16, &rawdata_filp->f_pos);
  if (rc < 0) {
    set_fs(oldfs);
    printk(KERN_ERR "%s: read imei error = %d \n",__func__,rc);
    filp_close(rawdata_filp, NULL);
		return;
  }
  set_fs(oldfs);

  #if 0 //test
    printk(KERN_ERR "%s : imei <%x> %x %x %x %x , %x %x %x %x, %x %x %x %x, %x %x %x \n",__func__,0,read_buf[0],read_buf[1],read_buf[2],read_buf[3],read_buf[4],read_buf[5],read_buf[6],read_buf[7],read_buf[8],read_buf[9],read_buf[10],read_buf[11],read_buf[12],read_buf[13],read_buf[14]);
  #endif

  printk(KERN_INFO "%s: read IMEI OK\n", __func__);

  imei_backup_info_buf = (imei_backup_info_type *)&read_buf[0];
  if(imei_backup_info_buf->imei_magic_num & IMEI_ADDR_MAGIC_NUM) 
  {
    memcpy(pantech_phoneinfo_buff_ptr->Imei, read_buf+4, NV_UE_IMEI_SIZE);
  }
#endif

#ifdef F_PANTECH_SECBOOT
  {
    secboot_fuse_flag *secboot_flag_ptr = NULL;
    const char str_secure[] = "$3(UR3"; // secure target
    const char str_non_secure[] = "3ru(3$"; // non-secure target

    rawdata_filp->f_pos = PANTECH_SECBOOT_FLAG_START;
    memset(read_buf, 0, SECTOR_SIZE);
    printk(KERN_ERR "%s: PANTECH_SECBOOT_FLAG pos=%x\n", __func__, PANTECH_SECBOOT_FLAG_START);

    if (((rawdata_filp->f_flags & O_ACCMODE) & O_RDONLY) != 0)
    {
      printk(KERN_ERR "%s: PANTECH_SECBOOT_FLAG permission denied!\n", __func__);
      printk(KERN_ERR "%s: secure target (for safety)\n", __func__);
      strcpy(pantech_phoneinfo_buff_ptr->secure_magic_, str_secure);
      return;
    }

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    rc = rawdata_filp->f_op->read(rawdata_filp, read_buf, SECTOR_SIZE, &rawdata_filp->f_pos);
    if (rc < 0) {
      set_fs(oldfs);
      printk(KERN_ERR "%s: PANTECH_SECBOOT_FLAG read failed! (%d)\n", __func__, rc);
      printk(KERN_ERR "%s: secure target (for safety)\n", __func__);
      strcpy(pantech_phoneinfo_buff_ptr->secure_magic_, str_secure);
      filp_close(rawdata_filp, NULL);
      return;
    }

    set_fs(oldfs);

    printk(KERN_INFO "%s: PANTECH_SECBOOT_FLAG read done\n", __func__);

    secboot_flag_ptr = (secboot_fuse_flag *)&read_buf[0];

    if (secboot_flag_ptr->secboot_magic_num == SECBOOT_FUSE_FLAG_MAGIC_NUM)
    {
      printk(KERN_ERR "%s: PANTECH_SECBOOT_FLAG valid magic\n", __func__);

      if (secboot_flag_ptr->auth_en == SECBOOT_FUSE_NOT_BLOWN)
      {
        printk(KERN_ERR "%s: PANTECH_SECBOOT_FLAG non-secure target\n", __func__);
        strcpy(pantech_phoneinfo_buff_ptr->secure_magic_, str_non_secure);
      }
      else if (secboot_flag_ptr->auth_en == SECBOOT_FUSE_BLOWN)
      {
        printk(KERN_ERR "%s: PANTECH_SECBOOT_FLAG secure target\n", __func__);
        strcpy(pantech_phoneinfo_buff_ptr->secure_magic_, str_secure);
      }
      else
      {
        printk(KERN_ERR "%s: PANTECH_SECBOOT_FLAG secure target (for safety)\n", __func__);
        strcpy(pantech_phoneinfo_buff_ptr->secure_magic_, str_secure);
      }
    }
    else
    {
      printk(KERN_ERR "%s: PANTECH_SECBOOT_FLAG invalid magic!\n", __func__);
      printk(KERN_ERR "%s: secure target (for safety)\n", __func__);
      strcpy(pantech_phoneinfo_buff_ptr->secure_magic_, str_secure);
    }
  }
#endif

  // close(
  filp_close(rawdata_filp, NULL);

  if(check_phoneinfo() != 1 && read_count < 5)
  {
    schedule_delayed_work(&phoneinfo_read_wqst, HZ*10);
    read_count++;
  }

  printk(KERN_INFO "%s: read phone info end : read_count = %d\n", __func__, read_count);
  return;

}

unsigned fill_writereq(int *dloadinfo_state, struct usb_request *writereq)
{
  unsigned len = TX_BUF_SIZE;

  switch( *dloadinfo_state )
  {
    case DLOADINFO_AT_RESPONSE_STATE:
    {
      memcpy(writereq->buf, "AT*PHONEINFO*WAIT", sizeof("AT*PHONEINFO*WAIT")-1);
      writereq->length = sizeof("AT*PHONEINFO*WAIT")-1;
      len = writereq->length;
      printk(KERN_ERR "%s: AT*PHONEINFO*WAIT", __func__);
    }
    break;
  
    case DLOADINFO_PHONE_INFO_STATE:
    {
      //int i;

      // header
      //    command       2byte
      //    ack_nack      2byte
      //    error_code    4byte
      //    data_length   4byte
      //    reserved      4byte

      printk(KERN_ERR "%s: case DLOADINFO_PHONE_INFO_STATE", __func__);
      memset( writereq->buf, 0x0, 16 + sizeof(phoneinfo_type) );
      len = writereq->length = fill_phoneinfo((char *)writereq->buf); 

      #if 0 //test
        for( i=3; i < len/16; i++ )
        {
          printk(KERN_ERR "%s : phoneinfo <%x> %x %x %x %x, %x %x %x %x, %x %x %x %x, %x %x %x %x\n",__func__,i*16,tx_buf[i*16],tx_buf[i*16+1],tx_buf[i*16+2],tx_buf[i*16+3],tx_buf[i*16+4],tx_buf[i*16+5],tx_buf[i*16+6],tx_buf[i*16+7],tx_buf[i*16+8],tx_buf[i*16+9],tx_buf[i*16+10],tx_buf[i*16+11],tx_buf[i*16+12],tx_buf[i*16+13],tx_buf[i*16+14],tx_buf[i*16+15]);
        }
      #endif 

      printk(KERN_ERR "%s: packet make DLOADINFO_PHONE_INFO_STATE", __func__);
    }
    break;
  
    case DLOADINFO_FINISH_STATE:
    {
      // header
      //    command       2byte
      //    ack_nack      2byte
      //    error_code    4byte
      //    data_length   4byte
      //    reserved      4byte

      printk(KERN_ERR "%s: case DLOADINFO_FINISH_STATE", __func__);
      memset( writereq->buf, 0x0, 16 );
      writereq->length = 16;
      len = writereq->length;

      *dloadinfo_state = DLOADINFO_NONE_STATE;
      printk(KERN_ERR "%s: set DLOADINFO_NONE_STATE", __func__);
    }
    break;
  }
  return len;
}
#endif 


static void gsmd_rx_push(struct work_struct *w)
{
	struct gsmd_port *port = container_of(w, struct gsmd_port, push);
	struct smd_port_info *pi = port->pi;
	struct list_head *q;

// 20111014, albatros, for PDL IDLE
#ifdef FEATURE_PANTECH_PDL_DLOAD
	struct list_head *pool_write = &port->write_pool;
	static int dloadinfo_state = DLOADINFO_NONE_STATE;
	const unsigned short DLOADINFO_PACKET_VERSION = 0;
#endif

	pr_debug("%s: port:%p port#%d", __func__, port, port->port_num);

	spin_lock_irq(&port->port_lock);

	q = &port->read_queue;
	while (pi->ch && !list_empty(q)) {
		struct usb_request *req;
		int avail;

		req = list_first_entry(q, struct usb_request, list);

		switch (req->status) {
		case -ESHUTDOWN:
			pr_debug("%s: req status shutdown portno#%d port:%p\n",
					__func__, port->port_num, port);
			goto rx_push_end;
		default:
			pr_warning("%s: port:%p port#%d"
					" Unexpected Rx Status:%d\n", __func__,
					port, port->port_num, req->status);
		case 0:
			/* normal completion */
			break;
		}

//20110721 choiseulkee add, reboot for PDL IDLE download
#ifdef FEATURE_PANTECH_PDL_DLOAD
    if(check_phoneinfo() == 1)
    {
      if( memcmp( req->buf, "AT*PHONEINFO*RESET", sizeof("AT*PHONEINFO*RESET")-1) == 0 )
      {
        printk(KERN_ERR "%s: PDL IDLE DLOAD REBOOT", __func__);
        // albatros, ????????? ???y? ?????? ?¢®E¡ËcIi????? ????? ???Ao ??? ??? ?????? ???
        //machine_restart("oem-33");
        kernel_restart("oem-33");
        return;
      }
      else if(memcmp( req->buf, "AT*PHONEINFO", sizeof("AT*PHONEINFO")-1) == 0 )
      {
        printk(KERN_ERR "%s: go DLOADINFO_AT_RESPONSE_STATE", __func__);
        dloadinfo_state = DLOADINFO_AT_RESPONSE_STATE;
      }
      else if( dloadinfo_state == DLOADINFO_AT_RESPONSE_STATE || dloadinfo_state == DLOADINFO_PHONE_INFO_STATE )
      {
        printk(KERN_ERR "%s: if %d", __func__, dloadinfo_state);
        if( *(unsigned int *)(req->buf) == (PHONE_INFO_CMD|(DLOADINFO_PACKET_VERSION<<16)) )
        {
          dloadinfo_state = DLOADINFO_PHONE_INFO_STATE;
          printk(KERN_ERR "%s: go DLOADINFO_PHONE_INFO_STATE", __func__);
        }
        if( *(unsigned int *)(req->buf) == (FINISH_CMD|(DLOADINFO_PACKET_VERSION<<16)) )
        {
          dloadinfo_state = DLOADINFO_FINISH_STATE;
          printk(KERN_ERR "%s: go DLOADINFO_FINISH_STATE", __func__);
        }
      }

      if( dloadinfo_state != DLOADINFO_NONE_STATE )
      {
        printk(KERN_ERR "%s: run cmd send_dload_packet", __func__);

        if (!list_empty(pool_write))
        {
          struct usb_ep *in = port->port_usb->in;
          struct usb_request *writereq;
          unsigned len = TX_BUF_SIZE;
          int ret;

          printk(KERN_ERR "%s: if start, before list_entry", __func__);
          writereq = list_entry(pool_write->next, struct usb_request, list);

          list_del(&writereq->list);

          len = fill_writereq(&dloadinfo_state, writereq);

          printk(KERN_ERR "%s: before usb_ep_queue, len= %d", __func__, len);
      		spin_unlock_irq(&port->port_lock);
      		ret = usb_ep_queue(in, writereq, GFP_KERNEL);
      		spin_lock_irq(&port->port_lock);
          printk(KERN_ERR "%s: ret=%d", __func__,ret);
      		if (ret) 
          {
      			pr_err("%s: usb ep out queue failed"
      					"port:%p, port#%d err:%d\n",
      					__func__, port, port->port_num, ret);
      			/* could be usb disconnected */
      			if (!port->port_usb)
      			{
              printk(KERN_ERR "%s: before gsmd_free_req", __func__);
      				gsmd_free_req(in, writereq);
      			}
      		}
          port->nbytes_tolaptop += len;
      		port->n_read = 0;
      		list_move(&req->list, &port->read_pool);
          goto rx_push_end;
        }
      }
    }
#endif

		avail = smd_write_avail(pi->ch);
		if (!avail)
			goto rx_push_end;

		if (req->actual) {
			char		*packet = req->buf;
			unsigned	size = req->actual;
			unsigned	n;
			int		count;

			n = port->n_read;
			if (n) {
				packet += n;
				size -= n;
			}

			count = smd_write(pi->ch, packet, size);
			if (count < 0) {
				pr_err("%s: smd write failed err:%d\n",
						__func__, count);
				goto rx_push_end;
			}

			if (count != size) {
				port->n_read += count;
				goto rx_push_end;
			}

			port->nbytes_tomodem += count;
		}

		port->n_read = 0;
		list_move(&req->list, &port->read_pool);
	}

rx_push_end:
	spin_unlock_irq(&port->port_lock);

	gsmd_start_rx(port);
}

static void gsmd_read_pending(struct gsmd_port *port)
{
	int avail;

	if (!port || !port->pi->ch)
		return;

	/* passing null buffer discards the data */
	while ((avail = smd_read_avail(port->pi->ch)))
		smd_read(port->pi->ch, 0, avail);

	return;
}

static void gsmd_tx_pull(struct work_struct *w)
{
	struct gsmd_port *port = container_of(w, struct gsmd_port, pull);
	struct list_head *pool = &port->write_pool;
	struct smd_port_info *pi = port->pi;
	struct usb_ep *in;

	pr_debug("%s: port:%p port#%d pool:%p\n", __func__,
			port, port->port_num, pool);

	spin_lock_irq(&port->port_lock);

	if (!port->port_usb) {
		pr_debug("%s: usb is disconnected\n", __func__);
		spin_unlock_irq(&port->port_lock);
		gsmd_read_pending(port);
		return;
	}

	in = port->port_usb->in;
	while (pi->ch && !list_empty(pool)) {
		struct usb_request *req;
		int avail;
		int ret;

		avail = smd_read_avail(pi->ch);
		if (!avail)
			break;

		avail = avail > SMD_TX_BUF_SIZE ? SMD_TX_BUF_SIZE : avail;

		req = list_entry(pool->next, struct usb_request, list);
		list_del(&req->list);
		req->length = smd_read(pi->ch, req->buf, avail);

		spin_unlock_irq(&port->port_lock);
		ret = usb_ep_queue(in, req, GFP_KERNEL);
		spin_lock_irq(&port->port_lock);
		if (ret) {
			pr_err("%s: usb ep out queue failed"
					"port:%p, port#%d err:%d\n",
					__func__, port, port->port_num, ret);
			/* could be usb disconnected */
			if (!port->port_usb)
				gsmd_free_req(in, req);
			else
				list_add(&req->list, pool);
			goto tx_pull_end;
		}

		port->nbytes_tolaptop += req->length;
	}

tx_pull_end:
	/* TBD: Check how code behaves on USB bus suspend */
	if (port->port_usb && smd_read_avail(port->pi->ch) && !list_empty(pool))
		queue_work(gsmd_wq, &port->pull);

	spin_unlock_irq(&port->port_lock);

	return;
}

static void gsmd_read_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct gsmd_port *port = ep->driver_data;

	pr_debug("%s: ep:%p port:%p\n", __func__, ep, port);

	if (!port) {
		pr_err("%s: port is null\n", __func__);
		return;
	}

	spin_lock(&port->port_lock);
	if (!test_bit(CH_OPENED, &port->pi->flags) ||
			req->status == -ESHUTDOWN) {
		spin_unlock(&port->port_lock);
		gsmd_free_req(ep, req);
		return;
	}

	list_add_tail(&req->list, &port->read_queue);
	queue_work(gsmd_wq, &port->push);
	spin_unlock(&port->port_lock);

	return;
}

static void gsmd_write_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct gsmd_port *port = ep->driver_data;

	pr_debug("%s: ep:%p port:%p\n", __func__, ep, port);

	if (!port) {
		pr_err("%s: port is null\n", __func__);
		return;
	}

	spin_lock(&port->port_lock);
	if (!test_bit(CH_OPENED, &port->pi->flags) ||
			req->status == -ESHUTDOWN) {
		spin_unlock(&port->port_lock);
		gsmd_free_req(ep, req);
		return;
	}

	if (req->status)
		pr_warning("%s: port:%p port#%d unexpected %s status %d\n",
				__func__, port, port->port_num,
				ep->name, req->status);

	list_add(&req->list, &port->write_pool);
	queue_work(gsmd_wq, &port->pull);
	spin_unlock(&port->port_lock);

	return;
}

static void gsmd_start_io(struct gsmd_port *port)
{
	int		ret = -ENODEV;

	pr_debug("%s: port: %p\n", __func__, port);

	spin_lock(&port->port_lock);

	if (!port->port_usb)
		goto start_io_out;

	smd_tiocmset_from_cb(port->pi->ch,
			port->cbits_to_modem,
			~port->cbits_to_modem);

	ret = gsmd_alloc_requests(port->port_usb->out,
				&port->read_pool,
				SMD_RX_QUEUE_SIZE, SMD_RX_BUF_SIZE,
				gsmd_read_complete);
	if (ret) {
		pr_err("%s: unable to allocate out requests\n",
				__func__);
		goto start_io_out;
	}

	ret = gsmd_alloc_requests(port->port_usb->in,
				&port->write_pool,
				SMD_TX_QUEUE_SIZE, SMD_TX_BUF_SIZE,
				gsmd_write_complete);
	if (ret) {
		gsmd_free_requests(port->port_usb->out, &port->read_pool);
		pr_err("%s: unable to allocate IN requests\n",
				__func__);
		goto start_io_out;
	}

start_io_out:
	spin_unlock(&port->port_lock);

	if (ret)
		return;

	gsmd_start_rx(port);
}

static unsigned int convert_uart_sigs_to_acm(unsigned uart_sig)
{
	unsigned int acm_sig = 0;

	/* should this needs to be in calling functions ??? */
	uart_sig &= (TIOCM_RI | TIOCM_CD | TIOCM_DSR);

	if (uart_sig & TIOCM_RI)
		acm_sig |= SMD_ACM_CTRL_RI;
	if (uart_sig & TIOCM_CD)
		acm_sig |= SMD_ACM_CTRL_DCD;
	if (uart_sig & TIOCM_DSR)
		acm_sig |= SMD_ACM_CTRL_DSR;

	return acm_sig;
}

static unsigned int convert_acm_sigs_to_uart(unsigned acm_sig)
{
	unsigned int uart_sig = 0;

	/* should this needs to be in calling functions ??? */
	acm_sig &= (SMD_ACM_CTRL_DTR | SMD_ACM_CTRL_RTS);

	if (acm_sig & SMD_ACM_CTRL_DTR)
		uart_sig |= TIOCM_DTR;
	if (acm_sig & SMD_ACM_CTRL_RTS)
		uart_sig |= TIOCM_RTS;

	return uart_sig;
}


static void gsmd_stop_io(struct gsmd_port *port)
{
	struct usb_ep	*in;
	struct usb_ep	*out;
	unsigned long	flags;

	spin_lock_irqsave(&port->port_lock, flags);
	if (!port->port_usb) {
		spin_unlock_irqrestore(&port->port_lock, flags);
		return;
	}
	in = port->port_usb->in;
	out = port->port_usb->out;
	spin_unlock_irqrestore(&port->port_lock, flags);

	usb_ep_fifo_flush(in);
	usb_ep_fifo_flush(out);

	spin_lock(&port->port_lock);
	if (port->port_usb) {
		gsmd_free_requests(out, &port->read_pool);
		gsmd_free_requests(out, &port->read_queue);
		gsmd_free_requests(in, &port->write_pool);
		port->n_read = 0;
		port->cbits_to_laptop = 0;
	}

	if (port->port_usb->send_modem_ctrl_bits)
		port->port_usb->send_modem_ctrl_bits(
					port->port_usb,
					port->cbits_to_laptop);
	spin_unlock(&port->port_lock);

}

static void gsmd_notify(void *priv, unsigned event)
{
	struct gsmd_port *port = priv;
	struct smd_port_info *pi = port->pi;
	int i;

	switch (event) {
	case SMD_EVENT_DATA:
		pr_debug("%s: Event data\n", __func__);
		if (smd_read_avail(pi->ch))
			queue_work(gsmd_wq, &port->pull);
		if (smd_write_avail(pi->ch))
			queue_work(gsmd_wq, &port->push);
		break;
	case SMD_EVENT_OPEN:
		pr_debug("%s: Event Open\n", __func__);
		set_bit(CH_OPENED, &pi->flags);
		gsmd_start_io(port);
		break;
	case SMD_EVENT_CLOSE:
		pr_debug("%s: Event Close\n", __func__);
		clear_bit(CH_OPENED, &pi->flags);
		gsmd_stop_io(port);
		break;
	case SMD_EVENT_STATUS:
		i = smd_tiocmget(port->pi->ch);
		port->cbits_to_laptop = convert_uart_sigs_to_acm(i);
		if (port->port_usb && port->port_usb->send_modem_ctrl_bits)
			port->port_usb->send_modem_ctrl_bits(port->port_usb,
						port->cbits_to_laptop);
		break;
	}
}

static void gsmd_connect_work(struct work_struct *w)
{
	struct gsmd_port *port;
	struct smd_port_info *pi;
	int ret;

	port = container_of(w, struct gsmd_port, connect_work.work);
	pi = port->pi;

	pr_debug("%s: port:%p port#%d\n", __func__, port, port->port_num);

	if (!test_bit(CH_READY, &pi->flags))
		return;

	ret = smd_named_open_on_edge(pi->name, SMD_APPS_MODEM,
				&pi->ch, port, gsmd_notify);
	if (ret) {
		if (ret == -EAGAIN) {
			/* port not ready  - retry */
			pr_debug("%s: SMD port not ready - rescheduling:%s err:%d\n",
					__func__, pi->name, ret);
			queue_delayed_work(gsmd_wq, &port->connect_work,
				msecs_to_jiffies(250));
		} else {
			pr_err("%s: unable to open smd port:%s err:%d\n",
					__func__, pi->name, ret);
		}
	}
}

static void gsmd_notify_modem(void *gptr, u8 portno, int ctrl_bits)
{
	struct gsmd_port *port;
	int temp;
	struct gserial *gser = gptr;

	if (portno >= n_smd_ports) {
		pr_err("%s: invalid portno#%d\n", __func__, portno);
		return;
	}

	if (!gser) {
		pr_err("%s: gser is null\n", __func__);
		return;
	}

	port = smd_ports[portno].port;

	temp = convert_acm_sigs_to_uart(ctrl_bits);

	if (temp == port->cbits_to_modem)
		return;

	port->cbits_to_modem = temp;

	/* usb could send control signal before smd is ready */
	if (!test_bit(CH_OPENED, &port->pi->flags))
		return;

	/* if DTR is high, update latest modem info to laptop */
	if (port->cbits_to_modem & TIOCM_DTR) {
		unsigned i;

		i = smd_tiocmget(port->pi->ch);
		port->cbits_to_laptop = convert_uart_sigs_to_acm(i);

		if (gser->send_modem_ctrl_bits)
			gser->send_modem_ctrl_bits(
					port->port_usb,
					port->cbits_to_laptop);
	}

	smd_tiocmset(port->pi->ch,
			port->cbits_to_modem,
			~port->cbits_to_modem);
}

int gsmd_connect(struct gserial *gser, u8 portno)
{
	unsigned long flags;
	int ret;
	struct gsmd_port *port;

	pr_debug("%s: gserial:%p portno:%u\n", __func__, gser, portno);

	if (portno >= n_smd_ports) {
		pr_err("%s: Invalid port no#%d", __func__, portno);
		return -EINVAL;
	}

	if (!gser) {
		pr_err("%s: gser is null\n", __func__);
		return -EINVAL;
	}

	port = smd_ports[portno].port;

	spin_lock_irqsave(&port->port_lock, flags);
	port->port_usb = gser;
	gser->notify_modem = gsmd_notify_modem;
	port->nbytes_tomodem = 0;
	port->nbytes_tolaptop = 0;
	spin_unlock_irqrestore(&port->port_lock, flags);

	ret = usb_ep_enable(gser->in);
	if (ret) {
		pr_err("%s: usb_ep_enable failed eptype:IN ep:%p",
				__func__, gser->in);
		port->port_usb = 0;
		return ret;
	}
	gser->in->driver_data = port;

	ret = usb_ep_enable(gser->out);
	if (ret) {
		pr_err("%s: usb_ep_enable failed eptype:OUT ep:%p",
				__func__, gser->out);
		port->port_usb = 0;
		gser->in->driver_data = 0;
		return ret;
	}
	gser->out->driver_data = port;

	queue_delayed_work(gsmd_wq, &port->connect_work, msecs_to_jiffies(0));

	return 0;
}

void gsmd_disconnect(struct gserial *gser, u8 portno)
{
	unsigned long flags;
	struct gsmd_port *port;

	pr_debug("%s: gserial:%p portno:%u\n", __func__, gser, portno);

	if (portno >= n_smd_ports) {
		pr_err("%s: invalid portno#%d\n", __func__, portno);
		return;
	}

	if (!gser) {
		pr_err("%s: gser is null\n", __func__);
		return;
	}

	port = smd_ports[portno].port;

	spin_lock_irqsave(&port->port_lock, flags);
	port->port_usb = 0;
	spin_unlock_irqrestore(&port->port_lock, flags);

	/* disable endpoints, aborting down any active I/O */
	usb_ep_disable(gser->out);
	usb_ep_disable(gser->in);

	spin_lock_irqsave(&port->port_lock, flags);
	gsmd_free_requests(gser->out, &port->read_pool);
	gsmd_free_requests(gser->out, &port->read_queue);
	gsmd_free_requests(gser->in, &port->write_pool);
	port->n_read = 0;
	spin_unlock_irqrestore(&port->port_lock, flags);

	if (test_and_clear_bit(CH_OPENED, &port->pi->flags)) {
		/* lower the dtr */
		port->cbits_to_modem = 0;
		smd_tiocmset(port->pi->ch,
				port->cbits_to_modem,
				~port->cbits_to_modem);
	}

	if (port->pi->ch) {
		smd_close(port->pi->ch);
		port->pi->ch = NULL;
	}
}

#define SMD_CH_MAX_LEN	20
static int gsmd_ch_probe(struct platform_device *pdev)
{
	struct gsmd_port *port;
	struct smd_port_info *pi;
	int i;
	unsigned long flags;

	pr_debug("%s: name:%s\n", __func__, pdev->name);

	for (i = 0; i < n_smd_ports; i++) {
		port = smd_ports[i].port;
		pi = port->pi;

		if (!strncmp(pi->name, pdev->name, SMD_CH_MAX_LEN)) {
			set_bit(CH_READY, &pi->flags);
			spin_lock_irqsave(&port->port_lock, flags);
			if (port->port_usb)
				queue_delayed_work(gsmd_wq, &port->connect_work,
					msecs_to_jiffies(0));
			spin_unlock_irqrestore(&port->port_lock, flags);
			break;
		}
	}
	return 0;
}

static int gsmd_ch_remove(struct platform_device *pdev)
{
	struct gsmd_port *port;
	struct smd_port_info *pi;
	int i;

	pr_debug("%s: name:%s\n", __func__, pdev->name);

	for (i = 0; i < n_smd_ports; i++) {
		port = smd_ports[i].port;
		pi = port->pi;

		if (!strncmp(pi->name, pdev->name, SMD_CH_MAX_LEN)) {
			clear_bit(CH_READY, &pi->flags);
			clear_bit(CH_OPENED, &pi->flags);
			if (pi->ch) {
				smd_close(pi->ch);
				pi->ch = NULL;
			}
			break;
		}
	}
	return 0;
}

static void gsmd_port_free(int portno)
{
	struct gsmd_port *port = smd_ports[portno].port;

	if (!port)
		kfree(port);
}

static int gsmd_port_alloc(int portno, struct usb_cdc_line_coding *coding)
{
	struct gsmd_port *port;
	struct platform_driver *pdrv;

	port = kzalloc(sizeof(struct gsmd_port), GFP_KERNEL);
	if (!port)
		return -ENOMEM;

	port->port_num = portno;
	port->pi = &smd_pi[portno];

	spin_lock_init(&port->port_lock);

	INIT_LIST_HEAD(&port->read_pool);
	INIT_LIST_HEAD(&port->read_queue);
	INIT_WORK(&port->push, gsmd_rx_push);

	INIT_LIST_HEAD(&port->write_pool);
	INIT_WORK(&port->pull, gsmd_tx_pull);

	INIT_DELAYED_WORK(&port->connect_work, gsmd_connect_work);

	smd_ports[portno].port = port;
	pdrv = &smd_ports[portno].pdrv;
	pdrv->probe = gsmd_ch_probe;
	pdrv->remove = gsmd_ch_remove;
	pdrv->driver.name = port->pi->name;
	pdrv->driver.owner = THIS_MODULE;
	platform_driver_register(pdrv);

	pr_debug("%s: port:%p portno:%d\n", __func__, port, portno);

	return 0;
}

#if defined(CONFIG_DEBUG_FS)
static ssize_t debug_smd_read_stats(struct file *file, char __user *ubuf,
		size_t count, loff_t *ppos)
{
	struct gsmd_port *port;
	struct smd_port_info *pi;
	char *buf;
	unsigned long flags;
	int temp = 0;
	int i;
	int ret;

	buf = kzalloc(sizeof(char) * 512, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	for (i = 0; i < n_smd_ports; i++) {
		port = smd_ports[i].port;
		pi = port->pi;
		spin_lock_irqsave(&port->port_lock, flags);
		temp += scnprintf(buf + temp, 512 - temp,
				"###PORT:%d###\n"
				"nbytes_tolaptop: %lu\n"
				"nbytes_tomodem:  %lu\n"
				"cbits_to_modem:  %u\n"
				"cbits_to_laptop: %u\n"
				"n_read: %u\n"
				"smd_read_avail: %d\n"
				"smd_write_avail: %d\n"
				"CH_OPENED: %d\n"
				"CH_READY: %d\n",
				i, port->nbytes_tolaptop, port->nbytes_tomodem,
				port->cbits_to_modem, port->cbits_to_laptop,
				port->n_read,
				pi->ch ? smd_read_avail(pi->ch) : 0,
				pi->ch ? smd_write_avail(pi->ch) : 0,
				test_bit(CH_OPENED, &pi->flags),
				test_bit(CH_READY, &pi->flags));
		spin_unlock_irqrestore(&port->port_lock, flags);
	}

	ret = simple_read_from_buffer(ubuf, count, ppos, buf, temp);

	kfree(buf);

	return ret;

}

static ssize_t debug_smd_reset_stats(struct file *file, const char __user *buf,
				 size_t count, loff_t *ppos)
{
	struct gsmd_port *port;
	unsigned long flags;
	int i;

	for (i = 0; i < n_smd_ports; i++) {
		port = smd_ports[i].port;

		spin_lock_irqsave(&port->port_lock, flags);
		port->nbytes_tolaptop = 0;
		port->nbytes_tomodem = 0;
		spin_unlock_irqrestore(&port->port_lock, flags);
	}

	return count;
}

static int debug_smd_open(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations debug_gsmd_ops = {
	.open = debug_smd_open,
	.read = debug_smd_read_stats,
	.write = debug_smd_reset_stats,
};

static void gsmd_debugfs_init(void)
{
	struct dentry *dent;

	dent = debugfs_create_dir("usb_gsmd", 0);
	if (IS_ERR(dent))
		return;

	debugfs_create_file("status", 0444, dent, 0, &debug_gsmd_ops);
}
#else
static void gsmd_debugfs_init(void) {}
#endif

int gsmd_setup(struct usb_gadget *g, unsigned count)
{
	struct usb_cdc_line_coding	coding;
	int ret;
	int i;

	pr_debug("%s: g:%p count: %d\n", __func__, g, count);

	if (!count || count > SMD_N_PORTS) {
		pr_err("%s: Invalid num of ports count:%d gadget:%p\n",
				__func__, count, g);
		return -EINVAL;
	}

	coding.dwDTERate = cpu_to_le32(9600);
	coding.bCharFormat = 8;
	coding.bParityType = USB_CDC_NO_PARITY;
	coding.bDataBits = USB_CDC_1_STOP_BITS;

// 20111014, albatros, for PDL IDLE
#ifdef FEATURE_PANTECH_PDL_DLOAD
  INIT_DELAYED_WORK(&phoneinfo_read_wqst, load_phoneinfo_with_imei);
	schedule_delayed_work(&phoneinfo_read_wqst, HZ*10);
#endif

	gsmd_wq = create_singlethread_workqueue("k_gsmd");
	if (!gsmd_wq) {
		pr_err("%s: Unable to create workqueue gsmd_wq\n",
				__func__);
		return -ENOMEM;
	}

	for (i = 0; i < count; i++) {
		mutex_init(&smd_ports[i].lock);
		n_smd_ports++;
		ret = gsmd_port_alloc(i, &coding);
		if (ret) {
			n_smd_ports--;
			pr_err("%s: Unable to alloc port:%d\n", __func__, i);
			goto free_smd_ports;
		}
	}

	gsmd_debugfs_init();

	return 0;
free_smd_ports:
	for (i = 0; i < n_smd_ports; i++)
		gsmd_port_free(i);

	destroy_workqueue(gsmd_wq);

	return ret;
}

void gsmd_cleanup(struct usb_gadget *g, unsigned count)
{
	/* TBD */
}
