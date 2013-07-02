//=============================================================================
// File       : isdbt_dev.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2011/09/29       yschoi         Create
//=============================================================================

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <linux/kthread.h>
//#include <linux/smp_lock.h>

#include "../dmb_interface.h"
#include "../dmb_hw.h"
#include "isdbt_comdef.h"
#include "isdbt_bb.h"
#include "isdbt_dev.h"
#include "isdbt_chip.h"
#include "isdbt_test.h"


/*================================================================== */
/*================    ISDB-T Module Definition     ================= */
/*================================================================== */

#if (defined(FEATURE_TEST_ON_BOOT) || defined(FEATURE_NETBER_TEST_ON_BOOT))
#define FEATURE_TEST_INT
#endif

//#define FEATURE_ISDBT_IGNORE_1ST_INT



struct isdbt_dev {
  struct cdev cdev;
  struct device *dev;
  struct fasync_struct *fasync; // async notification
  wait_queue_head_t wq;
  int irq;
};

static struct isdbt_dev *isdbt_device;
static dev_t isdbt_dev_num;
static struct class *isdbt_dev_class;

static int isdbt_device_major;
static int ISDBT_DEVICE_OPEN;
//static bool power_on_flag = FALSE;

tIsdbtChInfo isdbt_ch_info[ISDBT_1SEG_NUM_OF_CH];
int curr_ch_idx = 50;


#ifdef FEATURE_DMB_TSIF_IF
//extern void tsif_force_stop(void);
#endif

#ifdef CONFIG_SKY_DMB_MICRO_USB_DETECT
int pm8058_is_dmb_ant(void);
#endif
//static int uUSB_ant_detect = 0;

#ifndef FEATURE_TS_PKT_MSG
int g_isdbt_interrup_cnt = 0;
#endif /* FEATURE_TS_PKT_MSG */

//static int play_start = 0;
static int first_dmb_int_flag = 0;

isdbt_ts_data_type ts_data;

tIsdbtSigInfo sig_info;

tIsdbtTunerInfo tuner_info;

#ifdef CONFIG_MSM_BUS_SCALING
struct dmb_platform_data {
  void *bus_scale_table;
};
struct dmb_platform_data *dmb_data;

static uint32_t dmb_bus_scale_handle;
#endif

#ifdef FEATURE_DMB_USE_TASKLET
struct tasklet_struct status_tasklet;
struct tasklet_struct tuner_info_tasklet;
#endif

/*================================================================== */
/*================    ISDB-T Module Functions      ================= */
/*================================================================== */

static int isdbt_open(struct inode *inode, struct file *file);
static int isdbt_release(struct inode *inode, struct file *file);
static ssize_t isdbt_read(struct file *filp, char *buffer, size_t length, loff_t *);
static ssize_t isdbt_write(struct file *filp, const char *buffer, size_t length, loff_t *offset);
static long isdbt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int isdbt_fasync(int fd, struct file *file, int on);

irqreturn_t isdbt_interrupt(int irq, void *dev_id);

#if (defined(FEATURE_TEST_INT) && !defined(FEATURE_ISDBT_THREAD))
static void isdbt_test_interrupt(void);
#endif /* FEATURE_TEST_INT */

static void isdbt_send_sig(void);
static void isdbt_read_data(void);

#ifdef FEATURE_ISDBT_THREAD
static DECLARE_WAIT_QUEUE_HEAD(isdbt_isr_wait);
static u8 isdbt_isr_sig = 0;
static struct task_struct *isdbt_kthread = NULL;
#endif

static struct file_operations isdbt_fops = {
  .owner    = THIS_MODULE,
  .unlocked_ioctl    = isdbt_ioctl,
  .open     = isdbt_open,
  .release  = isdbt_release,
  .read     = isdbt_read,
  .write    = isdbt_write,
  .fasync   = isdbt_fasync,
};

/*====================================================================
FUNCTION       isdbt_ioctl  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
#if 0 //from kernel 2.6.36  ioctl(removed) -> unlocked_ioctl changed
static int isdbt_ioctl(struct inode *inodep, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
static long isdbt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
  unsigned long flags;
  //unsigned int data_buffer_length = 0;
  void __user *argp = (void __user *)arg;

  //get_frequency_type freq;
  //ch_scan_type ch_scan;
  //chan_info ch_info;
  //g_var_type gVar;

  //ISDBT_MSG_DEV("[%s] ioctl cmd_enum[%d]\n", __func__, _IOC_NR(cmd));

#if 0 // not used
  /* First copy down the buffer length */
  if (copy_from_user(&data_buffer_length, argp, sizeof(unsigned int)))
    return -EFAULT;
#endif // 0

  if(_IOC_TYPE(cmd) != IOCTL_ISDBT_MAGIC)
  {
    ISDBT_MSG_DEV("[%s] invalid Magic Char [%c]\n", __func__, _IOC_TYPE(cmd));
    return -EINVAL;
  }

  if(_IOC_NR(cmd) >= IOCTL_ISDBT_MAXNR)
  {
    return -EINVAL;
  }

#if 0 // not used
  size = _IOC_SIZE(cmd);

  if(size)
  {
    err = 0;

    if(_IOC_DIR(cmd) & _IOC_READ)
      err = verify_area(VERIFY_WRITE, (void *) arg, size);
    else if(_IOC_DIR(cmd) & _IOC_WRITE)
      err = verify_area(VERIFY_READ, (void *) arg, size);

    if (err)
      return err;
  }
#endif // 0

  //lock_kernel(); //ioctl -> unlocked_ioctl 로 변경되어 추가

  switch(cmd)
  {
#if 0
    case IOCTL_ISDBT_BB_POWER_ON:
      isdbt_bb_power_on();
      break;

    case IOCTL_ISDBT_BB_POWER_OFF:
      isdbt_bb_power_off();      
      break;

    case IOCTL_ISDBT_BB_SET_ANTENNA_PATH:
      if(copy_from_user(&flags, argp, sizeof(flags)))
        return -EFAULT;

      dmb_set_ant_path(flags)
      break;
#endif

    case IOCTL_ISDBT_BB_START:
      //1. poweron
      isdbt_bb_power_on();

      //dmb_set_ant_path(DMB_ANT_EARJACK);

      //2. chip init
      flags = isdbt_bb_init();

#ifdef CONFIG_MSM_BUS_SCALING
      if(dmb_bus_scale_handle > 0)
      {
        ISDBT_MSG_DEV("Set DMB bus scale Max.\n");
        msm_bus_scale_client_update_request(dmb_bus_scale_handle, 1);
      }
#endif

      if(copy_to_user(argp, &flags, sizeof(flags)))
        return -EFAULT;
      break;

    case IOCTL_ISDBT_BB_STOP:
      //1. power off
      isdbt_bb_power_off();

      flags = ISDBT_RETVAL_SUCCESS;

#ifdef CONFIG_MSM_BUS_SCALING
      if(dmb_bus_scale_handle > 0)
      {
        ISDBT_MSG_DEV("Set DMB bus scale Min.\n");
        msm_bus_scale_client_update_request(dmb_bus_scale_handle, 0);
      }
#endif

      if(copy_to_user(argp, &flags, sizeof(flags)))
        return -EFAULT;
      break;

    case IOCTL_ISDBT_BB_SET_FREQ:
      if(copy_from_user(&flags, argp, sizeof(flags)))
        return -EFAULT;

      if((flags < ISDBT_1SEG_CH_OFFSET)  || (flags > ISDBT_1SEG_MAX_CH))
      {
        ISDBT_MSG_DEV("SET Freq Err!! ch num[%d] > 62\n", (unsigned int)flags);
        flags = ISDBT_RETVAL_PARAMETER_ERROR;
        
        if(copy_to_user(argp, &flags, sizeof(flags)))
          return -EFAULT;
        break;
      }

      if(isdbt_bb_set_freq(flags))
      {
        curr_ch_idx = flags - ISDBT_1SEG_CH_OFFSET;
        flags = ISDBT_RETVAL_SUCCESS;
      }
      else
      {
        curr_ch_idx = flags - ISDBT_1SEG_CH_OFFSET;
        flags = ISDBT_RETVAL_DRIVER_ERROR;
      }

      if(copy_to_user(argp, &flags, sizeof(flags)))
        return -EFAULT;
      break;

    case IOCTL_ISDBT_BB_GET_FREQ:
      flags = curr_ch_idx + ISDBT_1SEG_CH_OFFSET;

      if(copy_to_user(argp, &flags, sizeof(flags)))
        return -EFAULT;
      break;

    case IOCTL_ISDBT_BB_FAST_SEARCH:
      {
      tIsdbtFastSearch ch;
      int index=0,i;

      ch.freq_num = 0;
      ch.num = 10;

      if(copy_from_user(&ch, argp, sizeof(tIsdbtFastSearch)))
        return -EFAULT;

      for(i=0; i < ch.num; i++)
      {
        index = ch.freq_num + index - ISDBT_1SEG_CH_OFFSET;
        isdbt_ch_info[index].sync = (isdbt_bb_set_freq(ch.freq_num)? ISDBT_SYNC_LOCKED: ISDBT_SYNC_UNLOCKED);
        ch.data[i].sync = isdbt_ch_info[index].sync;
        ch.data[i].freq_num = ch.freq_num;
        ISDBT_MSG_DEV("BB_FAST_SEARCH-> freq[%d] num[%d]  isdbt_ch_info[%d].sync=[%d]\n",ch.freq_num, ch.num, index, ch.data[i].sync);
      }

      if(copy_to_user(argp, &ch, sizeof(tIsdbtFastSearch)))
        return -EFAULT;
      }
      break;

    case IOCTL_ISDBT_BB_GET_STATE:
      if (curr_ch_idx < ISDBT_1SEG_NUM_OF_CH)
        flags = isdbt_ch_info[curr_ch_idx].sync;
      else
        flags = ISDBT_RETVAL_OTHERS;

      //ISDBT_MSG_DEV("BB_GET_STATE -> flags[%d]  channel[%d] (idx[%d])\n",(unsigned int)flags,(curr_ch_idx+ISDBT_1SEG_CH_OFFSET),curr_ch_idx);

      if(copy_to_user(argp, &flags, sizeof(flags)))
        return -EFAULT;
      break;

    case IOCTL_ISDBT_BB_GET_SIG_INFO:
      //ISDBT_MSG_DEV("[%s] IOCTL_ISDBT_BB_GET_SIG_INFO start\n", __func__);
#if defined(FEATURE_DMB_USE_TASKLET)
      tasklet_schedule(&status_tasklet);
#elif defined(FEATURE_ISDBT_THREAD)
      isdbt_isr_sig = 1;
      wake_up_interruptible(&isdbt_isr_wait);
#else
      isdbt_bb_get_status(&sig_info);
#endif

#ifndef FEATURE_TS_PKT_MSG
      g_isdbt_interrup_cnt = 0;
#endif

      if(copy_to_user(argp, &sig_info, sizeof(tIsdbtSigInfo)))
        return -EFAULT;

      //ISDBT_MSG_DEV("[%s] IOCTL_ISDBT_BB_GET_SIG_INFO end\n", __func__);
      break;

    case IOCTL_ISDBT_BB_GET_TUNER_INFO:
#if defined( FEATURE_DMB_USE_TASKLET)
      tasklet_schedule(&tuner_info_tasklet);
#elif defined(FEATURE_ISDBT_THREAD)
      isdbt_isr_sig = 2;
      wake_up_interruptible(&isdbt_isr_wait);
#else
      isdbt_bb_get_tuner_info(&tuner_info);
#endif

#ifndef FEATURE_TS_PKT_MSG
      g_isdbt_interrup_cnt = 0;
#endif

      if(copy_to_user(argp, &tuner_info, sizeof(tIsdbtTunerInfo)))
        return -EFAULT;
      break;

    case IOCTL_ISDBT_BB_START_TS:
      isdbt_bb_start_ts(1);

      flags = ISDBT_RETVAL_SUCCESS;

      if(copy_to_user(argp, &flags, sizeof(flags)))
        return -EFAULT;

      break;

    case IOCTL_ISDBT_BB_STOP_TS:
      isdbt_bb_start_ts(0);

      flags = ISDBT_RETVAL_SUCCESS;

      if(copy_to_user(argp, &flags, sizeof(flags)))
        return -EFAULT;

      break;

    case IOCTL_ISDBT_BB_READ_TS:
      break;

    case IOCTL_ISDBT_BB_TEST:
      if(copy_from_user(&flags, argp, sizeof(flags)))
        return -EFAULT;

      sharp_test(flags);
      break;

    default:
      ISDBT_MSG_DEV("[%s] unknown command!!!\n", __func__);
      break;
  }

  //unlock_kernel();

  //ISDBT_MSG_DEV("[%s] end!!!\n", __func__);
  return 0;
}


/*====================================================================
FUNCTION       isdbt_write
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static ssize_t isdbt_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
  ISDBT_MSG_DEV("[%s] write\n", __func__);
  return 0;
}


/*====================================================================
FUNCTION       isdbt_read
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static ssize_t isdbt_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
  ISDBT_MSG_DEV("[%s] read\n", __func__);

#if 0
  if (copy_to_user((void __user *)buffer, &ts_data, length))
    return -EFAULT;
#endif

  ISDBT_MSG_DEV("[%s] read end\n", __func__);

  return 0;
}


/*====================================================================
FUNCTION       isdbt_open  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int isdbt_open(struct inode *inode, struct file *file)
{
  ISDBT_MSG_DEV("[%s] open\n", __func__);

  //file->private_data = isdbt_device;

  if(ISDBT_DEVICE_OPEN){
    ISDBT_MSG_DEV("[%s] already opened -> Forced Power off\n", __func__);
    isdbt_bb_power_off();
#ifdef FEATURE_DMB_TSIF_IF
    //tsif_force_stop();
#endif
    return -EBUSY;
  }
  ISDBT_DEVICE_OPEN++;
  return 0;
}


/*====================================================================
FUNCTION       isdbt_release  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int isdbt_release(struct inode *inode, struct file *file)
{
  ISDBT_MSG_DEV("[%s] release\n", __func__);

  ISDBT_DEVICE_OPEN--;
  return 0;
}


/*====================================================================
FUNCTION       isdbt_fasync  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int isdbt_fasync(int fd, struct file *file, int on)
{
  int err;

  ISDBT_MSG_DEV("[%s]\n", __func__);

  err = fasync_helper(fd, file, on, &isdbt_device->fasync);
  if (err < 0)
    return err;

  //ISDBT_MSG_DEV("[%s] isdbt_fasync [0x%8x]\n", __func__, isdbt_device->fasync);

  return 0;
}



/*================================================================== */
/*============    ISDB-T handler interrupt setting     ============= */
/*================================================================== */

/*====================================================================
FUNCTION       isdbt_set_isr  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean isdbt_set_isr(int on_off)
{
  int irq;

  if(on_off)
  {
    first_dmb_int_flag = 1;

    irq = gpio_to_irq(DMB_INT);
    if (request_irq(irq, isdbt_interrupt, IRQF_TRIGGER_FALLING, ISDBT_DEV_NAME, isdbt_device))
    {
      ISDBT_MSG_DEV("[%s] unable to get IRQ %d.\n", __func__, DMB_INT);
      return FALSE;
    }
  }
  else
  {
    first_dmb_int_flag = 0;

    free_irq(gpio_to_irq(DMB_INT), isdbt_device);
    ISDBT_MSG_DEV("[%s] free irq\n", __func__);
  }

  return TRUE;
}


/*====================================================================
FUNCTION       isdbt_interrupt  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
irqreturn_t isdbt_interrupt(int irq, void *dev_id)
{
#ifndef FEATURE_TS_PKT_MSG
  if(g_isdbt_interrup_cnt >= 100) // get_ber 을 하지 않는 경우를 위해, 정해진 간격으로 인터럽트가 뜨는지 메시지로 보여줌.
  {
    ISDBT_MSG_DEV("[%s] irq[%d] count[%d]\n", __func__, irq, g_isdbt_interrup_cnt);
    g_isdbt_interrup_cnt = 0;
  }
  else
  {
    g_isdbt_interrup_cnt ++;
  }
#else
  ISDBT_MSG_DEV("[%s] irq[%d]\n",__func__, irq);
#endif /* FEATURE_TS_PKT_MSG */

#ifdef FEATURE_ISDBT_IGNORE_1ST_INT
  if(first_dmb_int_flag)
  {
    ISDBT_MSG_DEV("[%s] ignore 1st interrupt after isdbt_set_isr(on)\n", __func__);
    first_dmb_int_flag = 0;

    return IRQ_HANDLED;
  }
#endif

#if (defined(FEATURE_TEST_INT) && !defined(FEATURE_ISDBT_THREAD))
  isdbt_test_interrupt();

  return IRQ_HANDLED;
#endif /* FEATURE_TEST_INT */

#ifdef FEATURE_ISDBT_THREAD
#ifdef FEATURE_DMB_SPI_IF
  isdbt_isr_sig = 1;

  wake_up_interruptible(&isdbt_isr_wait);
#else
  isdbt_send_sig();
#endif
#else
  isdbt_read_data();
#endif

  return IRQ_HANDLED;
}

#ifdef FEATURE_DMB_TSIF_IF
EXPORT_SYMBOL(isdbt_interrupt);
#endif /* FEATURE_DMB_TSIF_IF */


#if (defined(FEATURE_TEST_INT) && !defined(FEATURE_ISDBT_THREAD))
/*====================================================================
FUNCTION       isdbt_test_interrupt  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void isdbt_test_interrupt(void)
{
#ifdef FEATURE_DMB_TSIF_IF
  ISDBT_MSG_DEV("[%s] TSIF test\n", __func__);
#else // EBI2, SPI
  static boolean first_int = FALSE;

  ISDBT_MSG_DEV("[%s] EBI2, SPI test\n", __func__);

  if(first_int)
  {
    isdbt_read_data();
  }

  first_int = TRUE;
#endif /* FEATURE_DMB_TSIF_IF */
}
#endif /* FEATURE_TEST_INT */


/*====================================================================
FUNCTION       isdbt_send_sig  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void isdbt_send_sig(void)
{
  // send signal to framework
  kill_fasync(&isdbt_device->fasync, SIGIO, POLL_IN);

  //ISDBT_MSG_DEV("[%s] kill_fasync[0x%8x]\n", __func__, &isdbt_device->fasync);
  //ISDBT_MSG_DEV("[%s] kill_fasync\n", __func__);
}


#ifdef FEATURE_ISDBT_THREAD
/*====================================================================
FUNCTION       isdbt_thread  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int isdbt_thread(void *x)
{
  ISDBT_MSG_DEV("[%s] \n", __func__);

  set_user_nice(current, -20);

  while(1)
  {
    wait_event_interruptible(isdbt_isr_wait, isdbt_isr_sig || kthread_should_stop());

    isdbt_read_data();
    isdbt_isr_sig = 0;

    if(kthread_should_stop())
      break;
  }

  ISDBT_MSG_DEV("[%s] end\n", __func__);
  
  return 0;
}

#endif


/*====================================================================
FUNCTION       isdbt_tsif_data_parser  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void isdbt_read_data(void)
{
  //ISDBT_MSG_DEV("[%s] \n", __func__);

#if defined(FEATURE_DMB_SPI_IF)
  isdbt_send_sig();
#elif defined(FEATURE_DMB_TSIF_IF)
#ifdef FEATURE_ISDBT_THREAD
  if(isdbt_isr_sig == 1)
  {
    isdbt_bb_get_status(&sig_info);
  }
  else if(isdbt_isr_sig == 2)
  {
    isdbt_bb_get_tuner_info(&tuner_info);
  }
#else
  isdbt_send_sig();
#endif
#endif
  //ISDBT_MSG_DEV("[%s] end\n", __func__);

  return;
}

#ifdef FEATURE_DMB_USE_TASKLET
static void dmb_status_process(unsigned long data)
{
  //ISDBT_MSG_DEV("%s \n", __func__);
  isdbt_bb_get_status(&sig_info);

  //if(copy_to_user(argp, &sig_info, sizeof(tIsdbtSigInfo)))
  //  return;
}

static void dmb_tuner_info_process(unsigned long data)
{
  //ISDBT_MSG_DEV("%s \n", __func__);
  isdbt_bb_get_tuner_info(&tuner_info);
  
  //if(copy_to_user(argp, &tuner_info, sizeof(tIsdbtTunerInfo)))
  //  return;
}
#endif

/*================================================================== */
/*=================     ISDB-T Module setting     ================== */
/*================================================================== */

/*====================================================================
FUNCTION       isdbt_probe  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int __devinit isdbt_probe(struct platform_device *pdev)
{
  ISDBT_MSG_DEV("[%s] pdev[0x%8x]\n", __func__, (unsigned int)pdev);

  //isdbt_device->dev = pdev;

#ifdef CONFIG_MSM_BUS_SCALING
  dmb_data = pdev->dev.platform_data;

  if (!dmb_bus_scale_handle && dmb_data && dmb_data->bus_scale_table) 
  {
    ISDBT_MSG_DEV("[%s] Get tdmb_bus_scale_handle\n", __func__);
    dmb_bus_scale_handle = msm_bus_scale_register_client(dmb_data->bus_scale_table);
    if(!dmb_bus_scale_handle) 
    {
      ISDBT_MSG_DEV("[%s] Fail tdmb_bus_scale_handle\n", __func__);
    }
  }
  else
  {
    ISDBT_MSG_DEV("[%s] Fail TDMB bus scale dmb_data\n", __func__);
  }
#endif


  return 0;
}


/*====================================================================
FUNCTION       isdbt_remove  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int __devexit isdbt_remove(struct platform_device *pdev)
{
  ISDBT_MSG_DEV("[%s]\n", __func__);
  //struct isdbt_dev *isdbt_dev = platform_get_drvdata(pdev);

#ifdef CONFIG_MSM_BUS_SCALING
  dmb_data = pdev->dev.platform_data;

  if(dmb_data && dmb_data->bus_scale_table && dmb_bus_scale_handle > 0)
  {
    msm_bus_scale_unregister_client(dmb_bus_scale_handle);
  }
#endif

#ifdef FEATURE_DMB_USE_TASKLET
  tasklet_kill(&status_tasklet);
  tasklet_kill(&tuner_info_tasklet);
#endif

  free_irq(gpio_to_irq(DMB_INT), isdbt_device);
  //unregister device
  return 0;
}


/*====================================================================

======================================================================*/
static struct platform_driver isdbt_driver = {
  .probe    = isdbt_probe,
  .remove   = isdbt_remove,
//  .remove   = __devexit_p(isdbt_remove),
  .driver   = {
    .name   = ISDBT_PLATFORM_DEV_NAME,
    .owner  = THIS_MODULE,
  },
};


/*====================================================================
FUNCTION       isdbt_init  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int __init isdbt_init(void)
{
  int rc = 0;
  
  ISDBT_MSG_DEV("[%s]\n", __func__);

#if 1
  // PLATFORM Driver
  platform_driver_register(&isdbt_driver);
  ISDBT_MSG_DEV("[%s] platform_driver_register isdbt_driver\n", __func__);
#endif

  isdbt_device = kzalloc(sizeof(struct isdbt_dev), GFP_KERNEL);
  if (!isdbt_device)
  {
    ISDBT_MSG_DEV("[%s] Unable to allocate memory for isdbt_dev\n", __func__);
    return -ENOMEM;
  }
  else
  {
    //ISDBT_MSG_DEV("[%s] isdbt_device [0x%8x]\n", __func__, (unsigned int)isdbt_device);
  }

  // allocation device number
  rc = alloc_chrdev_region(&isdbt_dev_num, 0, 1, ISDBT_DEV_NAME);
  if(rc < 0)
  {
    ISDBT_MSG_DEV("[%s] alloc_chrdev_region Failed rc = %d\n", __func__, rc);
    return 0;
  }
  else
  {
    isdbt_device_major = MAJOR(isdbt_dev_num);
    ISDBT_MSG_DEV("[%s] registered with DeviceNum [0x%8x] Major [%d], Minor [%d]\n"
      , __func__, isdbt_dev_num, MAJOR(isdbt_dev_num), MINOR(isdbt_dev_num));
  }

  // create class
  isdbt_dev_class = class_create(THIS_MODULE, ISDBT_DEV_NAME);
  if (!isdbt_dev_class)
  {
    rc = PTR_ERR(isdbt_dev_class);
    ISDBT_MSG_DEV("[%s] couldn't create isdbt_dev_class rc = %d\n", __func__, rc);
    return 0;
  }
  else
  {
    //ISDBT_MSG_DEV("[%s] class_create [0x%8x]\n", __func__, (unsigned int)isdbt_dev_class);
  }

  // create device
  isdbt_device->dev = device_create(isdbt_dev_class, NULL, isdbt_dev_num, NULL, ISDBT_DEV_NAME);
  if (!isdbt_device->dev)
  {
    rc = PTR_ERR(isdbt_device->dev);
    ISDBT_MSG_DEV("[%s] class_device_create failed %d\n", __func__, rc);
    return 0;
  }
  else
  {
    //ISDBT_MSG_DEV("[%s] device_create, isdbt_device->dev[0x%8x]\n", __func__, (unsigned int)isdbt_device->dev);
  }

  // add character device
  cdev_init(&isdbt_device->cdev, &isdbt_fops);
  isdbt_device->cdev.owner = THIS_MODULE;

  rc = cdev_add(&(isdbt_device->cdev), isdbt_dev_num, 1);
  if (rc < 0) {
    ISDBT_MSG_DEV("[%s] cdev_add failed\n", __func__);
  }
  else
  {
    //ISDBT_MSG_DEV("[%s] cdev_add\n", __func__);
  }

  init_waitqueue_head(&isdbt_device->wq);

  // init isdbt bb function table
  isdbt_bb_func_tbl_init();

  // 20101102 cys
#ifdef FEATURE_DMB_GPIO_INIT
  dmb_gpio_init();
#endif /* FEATURE_DMB_GPIO_INIT */

#ifdef FEATURE_DMB_I2C_CMD
  dmb_i2c_api_Init();
#endif /* FEATURE_DMB_I2C_CMD */

#ifdef FEATURE_DMB_SPI_CMD
  dmb_spi_init();
#endif /* FEATURE_DMB_SPI_CMD */

// TEST_ON_BOOT
#if (defined(FEATURE_TEST_ON_BOOT) || defined(FEATURE_NETBER_TEST_ON_BOOT))// 부팅중에 테스트
  isdbt_test_on_boot();
#endif /* TEST_ON_BOOT */

#ifdef FEATURE_ISDBT_THREAD
  if(!isdbt_kthread)
  {
    isdbt_kthread = kthread_run(isdbt_thread, NULL, "isdbt_thread");

  }
#endif

#ifdef FEATURE_DMB_USE_TASKLET
  tasklet_init(&status_tasklet, dmb_status_process, (unsigned long)isdbt_device->dev);
  tasklet_init(&tuner_info_tasklet, dmb_tuner_info_process, (unsigned long)isdbt_device->dev);
#endif

  ISDBT_MSG_DEV("[%s] isdbt_init end!!!\n", __func__);

  return 0;
}


/*====================================================================
FUNCTION       isdbt_exit  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void __exit isdbt_exit(void)
{
  ISDBT_MSG_DEV("[%s] exit!!!\n", __func__);

  cdev_del(&(isdbt_device->cdev));
  device_destroy(isdbt_dev_class, isdbt_dev_num);  
  class_destroy(isdbt_dev_class);
  kfree(isdbt_device);

  platform_driver_unregister(&isdbt_driver);
  unregister_chrdev_region(isdbt_device_major, 0);

#ifdef FEATURE_ISDBT_THREAD
  kthread_stop(isdbt_kthread);
#endif

  ISDBT_MSG_DEV("[%s] unregister isdbt_driver\n", __func__);
}


module_init(isdbt_init);
module_exit(isdbt_exit);


MODULE_LICENSE("GPL");

