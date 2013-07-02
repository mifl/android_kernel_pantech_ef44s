//=============================================================================
// File       : Tdmb_dev.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2009/05/06       yschoi         Create
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
#ifdef CONFIG_ARCH_TEGRA
#include <linux/ioctl.h>
#include <linux/file.h>
#include <linux/fs.h>
#endif

#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#ifdef CONFIG_ARCH_MSM
#include <mach/board.h>
#endif
#include <linux/gpio.h>
#include <linux/kthread.h>
//#include <linux/smp_lock.h>  //omit kernel 3.0.1,  BKL was out.  bug??

#include "../dmb_interface.h"
#include "../dmb_hw.h"
#include "../dmb_test.h"

#include "tdmb_comdef.h"
#include "tdmb_chip.h"
#include "tdmb_bb.h"
#include "tdmb_dev.h"
#include "tdmb_test.h"


/*================================================================== */
/*================      TDMB Module Definition     ================= */
/*================================================================== */

#if (defined(FEATURE_TEST_ON_BOOT) || defined(FEATURE_NETBER_TEST_ON_BOOT))
#define FEATURE_TEST_INT
#endif

//#define FEATURE_TDMB_IGNORE_1ST_INT

struct tdmb_dev {
  struct cdev cdev;
  struct device *dev;
  struct fasync_struct *fasync; // async notification
  wait_queue_head_t wq;
  int irq;
};

static struct tdmb_dev *tdmb_device;
static dev_t tdmb_dev_num;
static struct class *tdmb_dev_class;

static int tdmb_device_major;
static int TDMB_DEVICE_OPEN;
bool power_on_flag = FALSE;


ts_data_type ts_data;

fic_data_type fic_data;

#ifdef FEATURE_TDMB_USE_FCI
extern tdmb_bb_int_type fci_int_type;
#endif /* FEATURE_TDMB_USE_FCI */


extern tdmb_mode_type dmb_mode;
extern tdmb_autoscan_state autoscan_state;
extern uint8 gFrequencyBand;
extern uint8 gFreqTableNum;

extern tSignalQuality g_tSigQual;

static int uUSB_ant_detect = 0;

#ifndef FEATURE_TS_PKT_MSG
int g_tdmb_interrupt_cnt = 0;
#endif /* FEATURE_TS_PKT_MSG */

static int play_start = 0;
static int first_dmb_int_flag = 0;

#if defined(FEATURE_DMB_THREAD) && defined(FEATURE_DMB_THREAD_FIC_BUF)
#define FIC_THREAD_BUF_CNT 10

char fic_thread_buf[384 * FIC_THREAD_BUF_CNT + 1];
int fic_thread_size[FIC_THREAD_BUF_CNT + 1];

int fic_buf_w_idx = 0;
int fic_buf_r_idx = 0;
int interrupt_cnt = 0;
#endif

#ifdef CONFIG_MSM_BUS_SCALING
struct dmb_platform_data {
  void *bus_scale_table;
};
struct dmb_platform_data *dmb_data;

static uint32_t tdmb_bus_scale_handle;
#endif


/*================================================================== */
/*================      TDMB Module Functions      ================= */
/*================================================================== */

static int tdmb_open(struct inode *inode, struct file *file);
static int tdmb_release(struct inode *inode, struct file *file);
static ssize_t tdmb_read(struct file *filp, char *buffer, size_t length, loff_t *);
static ssize_t tdmb_write(struct file *filp, const char *buffer, size_t length, loff_t *offset);
#if 0 //from kernel 2.6.36  ioctl(removed) -> unlocked_ioctl changed
static int tdmb_ioctl(struct inode *inodep, struct file *filp, unsigned int cmd, unsigned long arg);
#else
static long tdmb_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#endif

static int tdmb_fasync(int fd, struct file *file, int on);

irqreturn_t tdmb_interrupt(int irq, void *dev_id);

#if (defined(FEATURE_TEST_INT) && !defined(FEATURE_DMB_THREAD))
static void tdmb_test_interrupt(void);
#endif /* FEATURE_TEST_INT */

static void tdmb_send_sig(void);
static void tdmb_read_data(void);

#ifdef FEATURE_DMB_THREAD
static DECLARE_WAIT_QUEUE_HEAD(tdmb_isr_wait);
static u8 tdmb_isr_sig = 0;
static struct task_struct *tdmb_kthread = NULL;
#endif

#ifdef CONFIG_ARCH_TEGRA
static const struct file_operations tdmb_fops = {
#else
static struct file_operations tdmb_fops = {
#endif
  .owner    = THIS_MODULE,
  .unlocked_ioctl    = tdmb_ioctl,
  //.ioctl    = tdmb_ioctl,
  .open     = tdmb_open,
  .release  = tdmb_release,
  .read     = tdmb_read,
  .write    = tdmb_write,
  .fasync   = tdmb_fasync,
};

/*====================================================================
FUNCTION       tdmb_ioctl  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
#if 0 //from kernel 2.6.36  ioctl(removed) -> unlocked_ioctl changed
static int tdmb_ioctl(struct inode *inodep, struct file *filp, unsigned int cmd, unsigned long arg)
#else
static long tdmb_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
  unsigned long flags;
  //unsigned int data_buffer_length = 0;
  void __user *argp = (void __user *)arg;

  get_frequency_type freq;
  ch_scan_type ch_scan;
  chan_info ch_info;
  g_var_type gVar;

  //TDMB_MSG_DEV("[%s] ioctl cmd_enum[%d]\n", __func__, _IOC_NR(cmd));

#if 0 // not used
  /* First copy down the buffer length */
  if (copy_from_user(&data_buffer_length, argp, sizeof(unsigned int)))
    return -EFAULT;
#endif // 0

  if(_IOC_TYPE(cmd) != IOCTL_TDMB_MAGIC)
  {
    TDMB_MSG_DEV("[%s] invalid Magic Char [%c]\n", __func__, _IOC_TYPE(cmd));
    return -EINVAL;
  }
  if(_IOC_NR(cmd) >= IOCTL_TDMB_MAXNR)
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
#endif /* 0 */

  //lock_kernel(); //ioctl -> unlocked_ioctl 로 변경되어 추가

  switch(cmd)
  {
    case IOCTL_TDMB_BB_DRV_INIT:
#ifdef FEATURE_APP_CALL_TEST_FUNC
      TDMB_MSG_DEV("[%s] blocked for test func\n", __func__);
#else
      tdmb_bb_drv_init();
      power_on_flag = TRUE;
#endif
      play_start = 0;
#ifdef CONFIG_MSM_BUS_SCALING
      if(tdmb_bus_scale_handle > 0)
      {
        TDMB_MSG_DEV("Set DMB bus scale Max.\n");
        msm_bus_scale_client_update_request(tdmb_bus_scale_handle, 1);
      }
#endif

      break;

    case IOCTL_TDMB_BB_INIT:
      tdmb_bb_init();
      break;

    case IOCTL_TDMB_BB_POWER_ON:
      tdmb_bb_power_on();
      break;

    case IOCTL_TDMB_BB_POWER_OFF:
      if(power_on_flag)
      {
        tdmb_bb_power_off();
        power_on_flag = FALSE;
#ifdef CONFIG_MSM_BUS_SCALING
        if(tdmb_bus_scale_handle > 0)
        {
          TDMB_MSG_DEV("Set DMB bus scale Min.\n");
          msm_bus_scale_client_update_request(tdmb_bus_scale_handle, 0);
        }
#endif
      }

      play_start = 0;
      break;

    case IOCTL_TDMB_BB_SET_ANTENNA_PATH:
#ifdef FEATURE_APP_CALL_TEST_FUNC
      TDMB_MSG_DEV("[%s] blocked for test func\n", __func__);
#else
      if(copy_from_user(&flags, argp, sizeof(flags)))
        return -EFAULT;

      dmb_set_ant_path(flags);
      //tdmb_bb_set_antenna_path(TDMB_ANT_EXTERNAL);
#endif
      break;

    case IOCTL_TDMB_BB_CH_SCAN_START:
      if(copy_from_user(&ch_scan, argp, sizeof(ch_scan_type)))
        return -EFAULT;

      tdmb_bb_ch_scan_start(ch_scan.freq, ch_scan.band, ch_scan.freq_offset);
      break;

    case IOCTL_TDMB_BB_GET_FREQUENCY:
      if(copy_from_user(&freq, argp, sizeof(get_frequency_type)))
        return -EFAULT;

      tdmb_bb_get_frequency(&(freq.freq), freq.band, freq.index);
      //TDMB_MSG_DEV("[%s] IOCTL_TDMB_BB_GET_FREQUENCY [%d]\n", __func__, (unsigned int)freq.freq);

      if(copy_to_user(argp, &freq, sizeof(get_frequency_type)))
        return -EFAULT;
      break;

    case IOCTL_TDMB_BB_FIC_PROCESS:
      tdmb_bb_fic_process();
      break;

    case IOCTL_TDMB_BB_SET_FIC_ISR:
      if(copy_from_user(&flags, argp, sizeof(flags)))
        return -EFAULT;

      flags = tdmb_bb_set_fic_isr(flags);

      if(copy_to_user(argp, &flags, sizeof(flags)))
        return -EFAULT;
      break;

    case IOCTL_TDMB_BB_CHW_INTHANDLER2:
      tdmb_bb_chw_IntHandler2();
      break;

    case IOCTL_TDMB_BB_EBI2_CHW_INTHANDLER:
      if(copy_from_user(&flags, argp, sizeof(flags)))
        return -EFAULT;

      flags = tdmb_bb_ebi2_chw_IntHandler((uint8 *)flags);

      if(copy_to_user(argp, &flags, sizeof(flags)))
        return -EFAULT;
      break;

    case IOCTL_TDMB_BB_RESYNC:
      if(copy_from_user(&flags, argp, sizeof(flags)))
        return -EFAULT;

      tdmb_bb_resync(flags);
      break;

    case IOCTL_TDMB_BB_SUBCH_START:
#ifdef FEATURE_APP_CALL_TEST_FUNC
      TDMB_MSG_DEV("[%s] blocked for test func\n", __func__);
      tdmb_bb_ch_test(TDMB_TEST_CH);
#else
      flags = tdmb_bb_subch_start(0, 0);

      if(dmb_mode == TDMB_MODE_NETBER)
      {
        netber_init();
      }
      play_start = 1;

      if(copy_to_user(argp, &flags, sizeof(flags)))
        return -EFAULT;
#endif
      break;

    case IOCTL_TDMB_BB_DRV_START:
      tdmb_bb_drv_start();
      break;

    case IOCTL_TDMB_BB_DRV_STOP:
      tdmb_bb_drv_stop();
      break;

    case IOCTL_TDMB_BB_REPORT_DEBUG_INFO:
      tdmb_bb_report_debug_info();
      break;

    case IOCTL_TDMB_BB_GET_TUNING_STATUS:
      flags = tdmb_bb_get_tuning_status();

      if(copy_to_user(argp, &flags, sizeof(flags)))
        return -EFAULT;
      break;

    case IOCTL_TDMB_BB_SET_FIC_CH_RESULT:
      if(copy_from_user(&flags, argp, sizeof(flags)))
        return -EFAULT;

      tdmb_bb_set_fic_ch_result(flags);
      break;

    case IOCTL_TDMB_BB_GET_FIC_CH_RESULT:
      flags = tdmb_bb_get_fic_ch_result();

      if(copy_to_user(argp, &flags, sizeof(flags)))
        return -EFAULT;
      break;

    case IOCTL_TDMB_BB_READ_INT:
      flags = tdmb_bb_read_int();

      if(copy_to_user(argp, &flags, sizeof(flags)))
        return -EFAULT;
      break;

    case IOCTL_TDMB_BB_GET_SYNC_STATUS:
      flags = tdmb_bb_get_sync_status();

      if(copy_to_user(argp, &flags, sizeof(flags)))
        return -EFAULT;
      break;

    case IOCTL_TDMB_BB_READ_FIB:
#if (defined(FEATURE_TDMB_USE_FCI) && defined(FEATURE_DMB_SPI_IF) && !defined(FEATURE_DMB_THREAD))
      fci_int_type = tdmb_bb_read_int();
#endif

#if defined(FEATURE_DMB_THREAD) && defined(FEATURE_DMB_THREAD_FIC_BUF)
      fic_data.fib_num = fic_thread_size[fic_buf_r_idx];
      memcpy(fic_data.fic_buf, &fic_thread_buf[384 * fic_buf_r_idx], 384);

      fic_buf_r_idx ++;
      if (fic_buf_r_idx >= FIC_THREAD_BUF_CNT)
      {
        fic_buf_r_idx = 0;
      }

      //TDMB_MSG_DEV("[%s] fic_buf_idx w[%d], r[%d], size[%d] int_cnt[%d]\n", __func__, fic_buf_w_idx, fic_buf_r_idx, fic_data.fib_num, interrupt_cnt);
#else
      fic_data.fib_num = tdmb_bb_read_fib(fic_data.fic_buf);
#endif
      if(copy_to_user(argp, &fic_data, sizeof(fic_data)))
        return -EFAULT;
      break;

    case IOCTL_TDMB_BB_SET_SUBCHANNEL_INFO:
#ifdef FEATURE_APP_CALL_TEST_FUNC
      TDMB_MSG_DEV("[%s] blocked for test func\n", __func__);
#else

#ifdef FEAUTRE_USE_FIXED_FIC_DATA
      tdmb_get_fixed_chan_info(TDMB_TEST_CH, &ch_info);
#else
      if(copy_from_user(&ch_info, argp, sizeof(chan_info)))
        return -EFAULT;
#endif /* FEAUTRE_USE_FIXED_FIC_DATA */

#ifdef FEATURE_TDMB_VISUAL_RADIO_SERVICE
      if (ch_info.uiServiceType == TDMB_BB_SVC_VISUALRADIO)
      {
        ch_info.uiServiceType = TDMB_BB_SVC_DMB;
      }
#endif

#if 1
      TDMB_MSG_DEV("[%s] Freq[%d], E-ID[0x%x], SChID[0x%x], SvcType[0x%x], BitRate[%d]\n", __func__, (unsigned int)ch_info.ulRFNum, ch_info.uiEnsumbleID, ch_info.uiSubChID, ch_info.uiServiceType, ch_info.uiBitRate);
#else
      TDMB_MSG_DEV("[%s] ch_info.ulRFNum [%d]\n", __func__, (unsigned int)ch_info.ulRFNum);
      TDMB_MSG_DEV("[%s] ch_info.uiEnsumbleID [0x%x]\n", __func__, ch_info.uiEnsumbleID);
      TDMB_MSG_DEV("[%s] ch_info.uiSubChID [0x%x]\n", __func__, ch_info.uiSubChID);
      TDMB_MSG_DEV("[%s] ch_info.uiServiceType [0x%x]\n", __func__, ch_info.uiServiceType);
      //TDMB_MSG_DEV("[%s] ch_info.uiStarAddr [0x%x]\n", __func__, ch_info.uiStarAddr);
      TDMB_MSG_DEV("[%s] ch_info.uiBitRate [%d]\n", __func__, ch_info.uiBitRate);
      //TDMB_MSG_DEV("[%s] ch_info.uiTmID [%d]\n", __func__, ch_info.uiTmID);

      //TDMB_MSG_DEV("[%s] ch_info.uiSlFlag [%d]\n", __func__, ch_info.uiSlFlag);
      //TDMB_MSG_DEV("[%s] ch_info.ucTableIndex [%d]\n", __func__, ch_info.ucTableIndex);
      //TDMB_MSG_DEV("[%s] ch_info.ucOption [%d]\n", __func__, ch_info.ucOption);
      //TDMB_MSG_DEV("[%s] ch_info.uiProtectionLevel [%d]\n", __func__, ch_info.uiProtectionLevel);
      //TDMB_MSG_DEV("[%s] ch_info.uiDifferentRate [0x%x]\n", __func__, ch_info.uiDifferentRate);
      //TDMB_MSG_DEV("[%s] ch_info.uiSchSize [0x%x]\n", __func__, ch_info.uiSchSize);
#endif
      tdmb_bb_set_subchannel_info(&ch_info);
#endif /* FEATURE_APP_CALL_TEST_FUNC */
      break;

    case IOCTL_TDMB_BB_READ_MSC: // tdmb_read 로 대체
      TDMB_MSG_DEV("[%s] IOCTL_TDMB_BB_READ_MSC\n", __func__);
      break;

    case IOCTL_TDMB_BB_I2C_TEST:
      //tdmb_bb_i2c_test(arg);
      break;

    case IOCTL_TDMB_BB_GET_BER:
      if(!power_on_flag)
      {
        TDMB_MSG_DEV("[%s] TDMB BB power is offed!!!\n", __func__);
        return -EINVAL;
      }

      if (dmb_mode == TDMB_MODE_AIR_PLAY)
      {
        tdmb_bb_get_ber();
      }

#ifndef FEATURE_TS_PKT_MSG
      g_tdmb_interrupt_cnt = 0;
#endif

      if(copy_to_user(argp, &g_tSigQual, sizeof(tSignalQuality)))
        return -EFAULT;
      break;

    case IOCTL_TDMB_BB_CH_STOP:
      tdmb_bb_ch_stop();
      play_start = 0;
      ts_data.type = TYPE_NONE;

#if (defined(FEATURE_TDMB_USE_FCI) && defined(FEATURE_DMB_SPI_IF) && !defined(FEATURE_DMB_THREAD))
      fci_int_type = tdmb_bb_read_int();
#endif
      break;

    case IOCTL_TDMB_BB_CH_TEST:
      if(copy_from_user(&flags, argp, sizeof(flags)))
        return -EFAULT;

      tdmb_bb_ch_test(flags);
      break;

    case IOCTL_TDMB_GVAR_RW:
      if(copy_from_user(&gVar, argp, sizeof(g_var_type)))
        return -EFAULT;

      if(gVar.type == G_READ)
      {
        switch(gVar.name)
        {
        case G_DMB_MODE:
          gVar.data = dmb_mode;
          break;
        case G_GFREQUENCYBAND:
          gVar.data = gFrequencyBand;
          break;
        case G_GFREQTABLENUM:
          gVar.data = gFreqTableNum;
          break;
        case G_AUTOSCAN_STATE:
          gVar.data = autoscan_state; // not used
          break;
        case G_MICRO_USB_ANT:
          uUSB_ant_detect = 0;
#ifdef CONFIG_SKY_TDMB_MICRO_USB_DETECT
          uUSB_ant_detect = dmb_micro_usb_ant_detect();
#endif
          gVar.data = uUSB_ant_detect;
          break;
        default:
          break;
        }

        if(copy_to_user(argp, &gVar, sizeof(g_var_type)))
          return -EFAULT;
      }
      else if(gVar.type == G_WRITE)
      {
        switch(gVar.name)
        {
        case G_DMB_MODE:
          dmb_mode = gVar.data;
          break;
        case G_GFREQUENCYBAND:
          gFrequencyBand = gVar.data;
          break;
        case G_GFREQTABLENUM:
          gFreqTableNum = gVar.data;
          break;
        case G_AUTOSCAN_STATE:
          autoscan_state = gVar.data; // not used
          break;
        default:
          break;
        }
      }

      break;

    default:
      TDMB_MSG_DEV("[%s] unknown command!!!\n", __func__);
      break;
  }

  //unlock_kernel();

  //TDMB_MSG_DEV("[%s] end!!!\n", __func__);
  return 0;
}


/*====================================================================
FUNCTION       tdmb_write  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static ssize_t tdmb_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
  TDMB_MSG_DEV("[%s] write\n", __func__);
  return 0;
}


/*====================================================================
FUNCTION       tdmb_read  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static ssize_t tdmb_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
  //TDMB_MSG_DEV("[%s] read\n", __func__);

#if (defined(FEATURE_DMB_SPI_IF) && !defined(FEATURE_DMB_THREAD))
#ifdef FEATURE_TDMB_USE_FCI
  fci_int_type = tdmb_bb_read_int();
#endif

  ts_data.type = TYPE_NONE;
  ts_data.fic_size = 0;
  ts_data.msc_size = 0;

  ts_data.msc_size = tdmb_bb_read_msc(ts_data.msc_buf);
#endif /* FEATURE_DMB_SPI_IF */

  //TDMB_MSG_DEV("[%s] copy_to_user\n", __func__);

  if (copy_to_user((void __user *)buffer, &ts_data, length))
    return -EFAULT;

  //TDMB_MSG_DEV("[%s] tdmb_read end\n", __func__);

  if (dmb_mode == TDMB_MODE_NETBER)
  {
    netber_GetError(ts_data.msc_size, ts_data.msc_buf);
  }

  //TDMB_MSG_DEV("[%s] read end\n", __func__);

  return 0;
}


/*====================================================================
FUNCTION       tdmb_open  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int tdmb_open(struct inode *inode, struct file *file)
{
  TDMB_MSG_DEV("[%s] open\n", __func__);

  //file->private_data = tdmb_device;

  if(TDMB_DEVICE_OPEN)
  {
    TDMB_MSG_DEV("[%s] already opened -> Forced Power off\n", __func__);
    tdmb_bb_power_off();

#ifdef FEATURE_DMB_TSIF_IF
    dmb_tsif_force_stop();
#endif
    return -EBUSY;
  }

  TDMB_DEVICE_OPEN++;

  return 0;
}


/*====================================================================
FUNCTION       tdmb_release  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int tdmb_release(struct inode *inode, struct file *file)
{
  TDMB_MSG_DEV("[%s] release\n", __func__);

  TDMB_DEVICE_OPEN--;

  if(power_on_flag)
  {
    TDMB_MSG_DEV("[%s] TDMB Chip Power Off\n", __func__);
    tdmb_bb_power_off();
  }

  return 0;
}


/*====================================================================
FUNCTION       tdmb_fasync  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int tdmb_fasync(int fd, struct file *file, int on)
{
  int err;

  TDMB_MSG_DEV("[%s]\n", __func__);

  err = fasync_helper(fd, file, on, &tdmb_device->fasync);
  if (err < 0)
    return err;

  //TDMB_MSG_DEV("[%s] tdmb_fasync [0x%8x]\n", __func__, tdmb_device->fasync);

  return 0;
}



/*================================================================== */
/*============      TDMB handler interrupt setting     ============= */
/*================================================================== */

/*====================================================================
FUNCTION       tdmb_set_isr  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
boolean tdmb_set_isr(int on_off)
{
  int irq = 0;

  if(on_off)
  {
    first_dmb_int_flag = 1;

    irq = gpio_to_irq(DMB_INT);
    if (request_irq(irq, tdmb_interrupt, IRQF_TRIGGER_FALLING, TDMB_DEV_NAME, tdmb_device))
    {
      TDMB_MSG_DEV ("[%s] unable to get IRQ %d.\n", __func__, DMB_INT);
      return FALSE;
    }

#if defined(FEATURE_DMB_THREAD) && defined(FEATURE_DMB_THREAD_FIC_BUF)
    fic_buf_r_idx = 0;
    fic_buf_w_idx = 0;
    interrupt_cnt = 0;
#endif
  }
  else
  {
    first_dmb_int_flag = 0;

    free_irq(gpio_to_irq(DMB_INT), tdmb_device);
    TDMB_MSG_DEV ("[%s] free irq\n", __func__);

    tdmb_bb_set_int(0);

 #if (defined(FEATURE_TDMB_USE_FCI) && !defined(FEATURE_DMB_THREAD))
    fci_int_type = tdmb_bb_read_int();
 #endif
  }

  return TRUE;
}


/*====================================================================
FUNCTION       tdmb_interrupt  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
irqreturn_t tdmb_interrupt(int irq, void *dev_id)
{
#if (defined(FEATURE_DMB_I2C_CMD) && defined(FEATURE_TDMB_USE_TCC_TCC3170))
  if ((play_start == 1) && (irq == gpio_to_irq(DMB_INT))) // play 후 DMB_INT 떠서 예외처리
  {
    //TDMB_MSG_DEV ("[%s] irq[%d], [%d]\n", __func__, irq, gpio_to_irq(DMB_INT));
    return IRQ_HANDLED;
  }
#endif

  //TDMB_MSG_DEV ("[%s] irq[%d]\n", __func__, irq);

#ifndef FEATURE_TS_PKT_MSG
  if(g_tdmb_interrupt_cnt >= 100) // get_ber 을 하지 않는 경우를 위해, 정해진 간격으로 인터럽트가 뜨는지 메시지로 보여줌.
  {
    TDMB_MSG_DEV ("[%s] irq[%d] count[%d]\n", __func__, irq, g_tdmb_interrupt_cnt);
    g_tdmb_interrupt_cnt = 0;
  }
  else
  {
    g_tdmb_interrupt_cnt ++;
  }
#else
  TDMB_MSG_DEV ("[%s] irq[%d]\n", __func__, irq);
#endif /* FEATURE_TS_PKT_MSG */

#if defined(FEATURE_DMB_THREAD) && defined(FEATURE_DMB_THREAD_FIC_BUF)
  interrupt_cnt ++;
#endif

#ifdef FEATURE_TDMB_IGNORE_1ST_INT
  if(first_dmb_int_flag)
  {
    TDMB_MSG_DEV ("[%s] ignore 1st interrupt after tdmb_set_isr(on)\n", __func__);
    first_dmb_int_flag = 0;

    return IRQ_HANDLED;
  }
#endif

#if (defined(FEATURE_TEST_INT) && !defined(FEATURE_DMB_THREAD))
  tdmb_test_interrupt();

  return IRQ_HANDLED;
#endif /* FEATURE_TEST_INT */

#ifdef FEATURE_DMB_THREAD
  tdmb_isr_sig = 1;

  wake_up_interruptible(&tdmb_isr_wait);
#else
  tdmb_read_data();
#endif

  return IRQ_HANDLED;
}

#ifdef FEATURE_DMB_TSIF_IF
EXPORT_SYMBOL(tdmb_interrupt);
#endif /* FEATURE_DMB_TSIF_IF */


#if (defined(FEATURE_TEST_INT) && !defined(FEATURE_DMB_THREAD))
/*====================================================================
FUNCTION       tdmb_test_interrupt  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void tdmb_test_interrupt(void)
{
#if defined(FEATURE_DMB_TSIF_IF) || defined(FEATURE_DMB_SPI_IF)
  TDMB_MSG_DEV ("[%s] TSIF, SPI test\n", __func__);
#else // EBI2
  static boolean first_int = TRUE;

  TDMB_MSG_DEV ("[%s] EBI2 test\n", __func__);

  if(!first_int)
  {
    tdmb_read_data();
  }

  first_int = FALSE;
#endif /* FEATURE_DMB_TSIF_IF */
}
#endif /* FEATURE_TEST_INT */


/*====================================================================
FUNCTION       tdmb_send_sig  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void tdmb_send_sig(void)
{
  // send signal to framework
  kill_fasync(&tdmb_device->fasync, SIGIO, POLL_IN);

  //TDMB_MSG_DEV ("[%s] kill_fasync[0x%8x]\n", __func__, &tdmb_device->fasync);
  //TDMB_MSG_DEV ("[%s] kill_fasync\n", __func__);
}


#ifdef FEATURE_DMB_THREAD
/*====================================================================
FUNCTION       tdmb_thread  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int tdmb_thread(void *x)
{
  TDMB_MSG_DEV ("[%s] \n", __func__);

#if (defined(CONFIG_SKY_EF39S_BOARD) || defined(CONFIG_SKY_EF40S_BOARD) || defined(CONFIG_SKY_EF40K_BOARD))
  set_user_nice(current, 0);
#else
  set_user_nice(current, -20);
#endif

  while(1)
  {
    wait_event_interruptible(tdmb_isr_wait, tdmb_isr_sig || kthread_should_stop());

    tdmb_isr_sig = 0;

#if (defined(FEATURE_DMB_SPI_IF) && defined(FEATURE_TDMB_USE_FCI))
    if (dmb_mode == TDMB_MODE_AIR_PLAY)
    {
      tdmb_spi_clear_int();
    }
#endif

    tdmb_read_data();

    if(kthread_should_stop())
      break;
  }

  TDMB_MSG_DEV ("[%s] end\n", __func__);
  
  return 0;
}

#endif


#if defined(FEATURE_DMB_THREAD) && defined(FEATURE_DMB_THREAD_FIC_BUF)
/*====================================================================
FUNCTION       tdmb_read_fic_data  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void tdmb_read_fic_data(void)
{
  fic_thread_size[fic_buf_w_idx] = tdmb_bb_read_fib(&fic_thread_buf[384 * fic_buf_w_idx]);
  fic_buf_w_idx ++;
  if (fic_buf_w_idx >= FIC_THREAD_BUF_CNT)
  {
    fic_buf_w_idx = 0;
  }
}
#endif


/*====================================================================
FUNCTION       tdmb_read_data  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static void tdmb_read_data(void)
{
  bool read_msc = FALSE;

  //TDMB_MSG_DEV ("[%s] \n", __func__);

#if (defined(FEATURE_DMB_EBI_IF) || defined(FEATURE_DMB_SPI_IF))
  ts_data.type = TYPE_NONE;
  ts_data.fic_size = 0;
  ts_data.msc_size = 0;

 #if defined(FEATURE_TDMB_USE_INC)
  if(play_start == 1)
  {
    read_msc = TRUE;
  }
 #elif defined(FEATURE_TDMB_USE_FCI)
  fci_int_type = tdmb_bb_read_int();
  
  if(play_start == 1)
  {
    if(fci_int_type == TDMB_BB_INT_MSC)
    {
      read_msc = TRUE;
    }
    else // TDMB_BB_INT_FIC 인 경우 리턴
    {
      return;
      //read_msc = FALSE;
    }
  }
 #elif defined(FEATURE_TDMB_USE_TCC)
  tdmb_bb_read_int();
  if(play_start == 1)
  {
    read_msc = TRUE;
  }
 #else
  read_msc = TRUE;
 #endif

 #ifndef FEATURE_TEST_READ_DATA_ON_BOOT
  if(read_msc)
 #endif
  {
    ts_data.msc_size = tdmb_bb_read_msc(ts_data.msc_buf);
  }

#elif defined(FEATURE_DMB_TSIF_IF)
  read_msc = FALSE;

 #if defined(FEATURE_TDMB_USE_FCI)
  if(play_start == 0)
  {
    fci_int_type = tdmb_bb_read_int();
  #if defined(FEATURE_DMB_THREAD) && defined(FEATURE_DMB_THREAD_FIC_BUF)
    tdmb_read_fic_data();
  #endif
  }
 #elif defined(FEATURE_TDMB_USE_TCC)
  //tdmb_bb_read_int();
 #elif defined(FEATURE_TDMB_USE_INC)
  #if defined(FEATURE_DMB_THREAD) && defined(FEATURE_DMB_THREAD_FIC_BUF)
  if(play_start == 0)
  {
    tdmb_read_fic_data();
  }
  #endif
 #elif defined(FEATURE_TDMB_USE_RTV)
  #if defined(FEATURE_DMB_THREAD) && defined(FEATURE_DMB_THREAD_FIC_BUF)
   if(play_start == 0)
   {
     tdmb_read_fic_data();
   }
  #endif 
 #else // INC, RAONTech
  // Do nothing..
 #endif
#endif

  tdmb_send_sig();

  //TDMB_MSG_DEV ("[%s] end\n", __func__);

  return;
}



/*================================================================== */
/*=================      TDMB Module setting      ================== */
/*================================================================== */

/*====================================================================
FUNCTION       tdmb_probe  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int __devinit tdmb_probe(struct platform_device *pdev)
{
  TDMB_MSG_DEV ("[%s] pdev[0x%8x]\n", __func__, (unsigned int)pdev);

  //tdmb_device->dev = pdev;
#ifdef CONFIG_MSM_BUS_SCALING
  dmb_data = pdev->dev.platform_data;

  if (!tdmb_bus_scale_handle && dmb_data && dmb_data->bus_scale_table) 
  {
    TDMB_MSG_DEV("[%s] Get tdmb_bus_scale_handle\n", __func__);
    tdmb_bus_scale_handle = msm_bus_scale_register_client(dmb_data->bus_scale_table);
    if(!tdmb_bus_scale_handle) 
    {
      TDMB_MSG_DEV("[%s] Fail tdmb_bus_scale_handle\n", __func__);
    }
  }
  else
  {
    TDMB_MSG_DEV("[%s] Fail TDMB bus scale dmb_data\n", __func__);
  }
#endif

#ifdef FEATURE_DMB_EBI_IF
  if (pdev->id == 0)
  {
    ebi2_tdmb_base = ioremap(pdev->resource[0].start,
    pdev->resource[0].end -
    pdev->resource[0].start + 1);
    if (!ebi2_tdmb_base)
    {
      TDMB_MSG_DEV("[%s] ebi2_tdmb_base ioremap failed!\n", __func__);
      return -ENOMEM;
    }
    else
    {
      TDMB_MSG_DEV("[%s] ebi2_tdmb_base[0x%x], start[0x%x], end[0x%x]\n", __func__, ebi2_tdmb_base, pdev->resource[0].start, pdev->resource[0].end);
    }
  }
  else
  {
      TDMB_MSG_DEV("[%s] pdev->id is not zero!\n", __func__);
  }
#endif /* FEATURE_DMB_EBI_IF */
  
  return 0;
}


/*====================================================================
FUNCTION       tdmb_remove  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int __devexit tdmb_remove(struct platform_device *pdev)
{
  TDMB_MSG_DEV ("[%s]\n", __func__);
  //struct tdmb_dev *tdmb_dev = platform_get_drvdata(pdev);

#ifdef CONFIG_MSM_BUS_SCALING
  dmb_data = pdev->dev.platform_data;

  if(dmb_data && dmb_data->bus_scale_table && tdmb_bus_scale_handle > 0)
  {
    msm_bus_scale_unregister_client(tdmb_bus_scale_handle);
  }
#endif

  free_irq(gpio_to_irq(DMB_INT), tdmb_device);
  //unregister device
  return 0;
}


/*====================================================================

======================================================================*/
static struct platform_driver tdmb_driver = {
  .probe    = tdmb_probe,
  .remove   = tdmb_remove,
//  .remove   = __devexit_p(tdmb_remove),
  .driver   = {
    .name   = TDMB_PLATFORM_DEV_NAME,
    .owner  = THIS_MODULE,
  },
};


/*====================================================================
FUNCTION       tdmb_init  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int __init tdmb_init(void)
{
  int rc = 0;
  
  TDMB_MSG_DEV("[%s]\n", __func__);

#if 1
  // PLATFORM Driver
  platform_driver_register(&tdmb_driver);
  TDMB_MSG_DEV("[%s] platform_driver_register tdmb_driver\n", __func__);
#endif

  tdmb_device = kzalloc(sizeof(struct tdmb_dev), GFP_KERNEL);
  if (!tdmb_device)
  {
    TDMB_MSG_DEV("[%s] Unable to allocate memory for tdmb_dev\n", __func__);
    return -ENOMEM;
  }
  else
  {
    //TDMB_MSG_DEV("[%s] tdmb_device [0x%8x]\n", __func__, (unsigned int)tdmb_device);
  }

  // allocation device number
  rc = alloc_chrdev_region(&tdmb_dev_num, 0, 1, TDMB_DEV_NAME);
  if(rc < 0)
  {
    TDMB_MSG_DEV("[%s] alloc_chrdev_region Failed rc = %d\n", __func__, rc);
    return 0;
  }
  else
  {
    tdmb_device_major = MAJOR(tdmb_dev_num);
    TDMB_MSG_DEV("[%s] registered with DeviceNum [0x%8x] Major [%d], Minor [%d]\n"
      , __func__, tdmb_dev_num, MAJOR(tdmb_dev_num), MINOR(tdmb_dev_num));
  }

  // create class
  tdmb_dev_class = class_create(THIS_MODULE, TDMB_DEV_NAME);
  if (!tdmb_dev_class)
  {
    rc = PTR_ERR(tdmb_dev_class);
    TDMB_MSG_DEV("[%s] couldn't create tdmb_dev_class rc = %d\n", __func__, rc);
    return 0;
  }
  else
  {
    //TDMB_MSG_DEV("[%s] class_create [0x%8x]\n", __func__, (unsigned int)tdmb_dev_class);
  }

  // create device
  tdmb_device->dev = device_create(tdmb_dev_class, NULL, tdmb_dev_num, NULL, TDMB_DEV_NAME);
  if (!tdmb_device->dev)
  {
    rc = PTR_ERR(tdmb_device->dev);
    TDMB_MSG_DEV("[%s] class_device_create failed %d\n", __func__, rc);
    return 0;
  }
  else
  {
    //TDMB_MSG_DEV("[%s] device_create, tdmb_device->dev[0x%8x]\n", __func__, (unsigned int)tdmb_device->dev);
  }

  // add character device
  cdev_init(&tdmb_device->cdev, &tdmb_fops);
  tdmb_device->cdev.owner = THIS_MODULE;

  rc = cdev_add(&(tdmb_device->cdev), tdmb_dev_num, 1);
  if (rc < 0) {
    TDMB_MSG_DEV("[%s] cdev_add failed\n", __func__);
  }
  else
  {
    //TDMB_MSG_DEV("[%s] cdev_add\n", __func__);
  }

  init_waitqueue_head(&tdmb_device->wq);

  // init tdmb bb function table
  tdmb_bb_func_tbl_init();

  // 20101102 cys
#ifdef FEATURE_DMB_GPIO_INIT
  dmb_gpio_init();
#endif /* FEATURE_DMB_GPIO_INIT */

#ifdef FEATURE_DMB_I2C_CMD
  dmb_i2c_api_Init();
#endif /* FEATURE_DMB_I2C_CMD */

#ifdef FEATURE_DMB_SPI_IF
  dmb_spi_init();
#endif

// TEST_ON_BOOT
#if (defined(FEATURE_TEST_ON_BOOT) || defined(FEATURE_NETBER_TEST_ON_BOOT))// 부팅중에 테스트
  dmb_test_on_boot();
  play_start = 1;
#endif /* TEST_ON_BOOT */

#ifdef FEATURE_DMB_THREAD
  if(!tdmb_kthread)
  {
    tdmb_kthread = kthread_run(tdmb_thread, NULL, "tdmb_thread");

  }
#endif

  TDMB_MSG_DEV("[%s] tdmb_init end!!!\n", __func__);

  return 0;
}


/*====================================================================
FUNCTION       tdmb_exit  
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void __exit tdmb_exit(void)
{
  TDMB_MSG_DEV("[%s] exit!!!\n", __func__);

  cdev_del(&(tdmb_device->cdev));
  device_destroy(tdmb_dev_class, tdmb_dev_num);  
  class_destroy(tdmb_dev_class);
  kfree(tdmb_device);

  platform_driver_unregister(&tdmb_driver);
#ifdef CONFIG_ARCH_MSM
  unregister_chrdev_region(tdmb_device_major, 0);
#endif

#ifdef FEATURE_DMB_THREAD
  kthread_stop(tdmb_kthread);
#endif

  TDMB_MSG_DEV("[%s] unregister tdmb_driver\n", __func__);
}


module_init(tdmb_init);
module_exit(tdmb_exit);


MODULE_LICENSE("GPL");

