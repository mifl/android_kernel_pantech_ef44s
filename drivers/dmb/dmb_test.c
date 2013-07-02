//=============================================================================
// File       : dmb_test.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2012/03/29       yschoi         Create
//=============================================================================

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/syscalls.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "dmb_comdef.h"
#include "dmb_type.h"
#include "dmb_test.h"

#ifdef FEATURE_TEST_ON_BOOT
#include <linux/timer.h>
#include <linux/jiffies.h>

#ifdef FEATURE_DMB_TSIF_IF
#include "dmb_tsif.h"
#endif
#include "dmb_hw.h"

#ifdef CONFIG_SKY_TDMB
#include "tdmb/tdmb_comdef.h"
#include "tdmb/tdmb_chip.h"
#include "tdmb/tdmb_bb.h"
#endif

#ifdef CONFIG_SKY_ISDBT
#include "isdbt/isdbt_comdef.h"
#include "isdbt/isdbt_chip.h"
#include "isdbt/isdbt_bb.h"
#endif
#endif

/*================================================================== */
/*==============        DMB TEST Definition     =============== */
/*================================================================== */
#ifdef FEATURE_TEST_ON_BOOT
#ifdef CONFIG_SKY_TDMB
#define DMB_TEST_CH 1 //TDMB_U1
#endif

#ifdef CONFIG_SKY_ISDBT
#define DMB_TEST_CH 27 //557143
#endif

static struct timer_list dmb_test_tmer;
static struct workqueue_struct *dmb_boot_wq;
static struct work_struct dmb_test_startwq;

#define TDMB_BOOT_TEST_START_TIME 20

//#define FEATURE_BOOTTEST_READ_BER
#ifdef FEATURE_BOOTTEST_READ_BER
static struct timer_list dmb_ber_timer;
static struct work_struct dmb_test_berwq;
#endif
static int dmb_test_cnt = 0;
#endif /* FEATURE_TEST_ON_BOOT */

/*================================================================== */
/*==============        DMB TEST Function     =============== */
/*================================================================== */
/*===========================================================================
FUNCTION       dmb_data_dump
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
void dmb_data_dump(u8 *p, u32 size, char *filename)
{
  struct file *file;
  loff_t pos = 0;
  int fd;
  mm_segment_t old_fs = get_fs();
  set_fs(KERNEL_DS);
  
  fd = sys_open(filename, O_CREAT | O_RDWR | O_APPEND | O_LARGEFILE, 0644);
  if(fd >= 0) 
  {
    file = fget(fd);
    if(file) 
    {
      vfs_write(file, p, size, &pos);
      fput(file);
    }
    sys_close(fd);
  }
  else
  {
    DMB_MSG_TEST("%s open failed  fd [%d]\n", __func__, fd);
  }
  set_fs(old_fs);
}


#ifdef FEATURE_TEST_ON_BOOT
#ifdef FEATURE_BOOTTEST_READ_BER
/*===========================================================================
FUNCTION       dmb_ber_work
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
void dmb_ber_work(struct work_struct *work)
{
  //DMB_MSG_TEST("%s",__func__);

#ifdef  CONFIG_SKY_TDMB
  tdmb_bb_get_ber();
#endif

#ifdef CONFIG_SKY_ISDBT
  isdbt_bb_get_tuner_info();
#endif
}

/*===========================================================================
FUNCTION       dmb_ber_callback
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
void dmb_ber_callback(unsigned long data)
{
  //DMB_MSG_TEST("%s",__func__);

  mod_timer(&dmb_ber_timer, jiffies + msecs_to_jiffies(1000));
  if(dmb_boot_wq != NULL)
  {
    queue_work(dmb_boot_wq, &dmb_test_berwq);
  }
}
#endif

/*===========================================================================
FUNCTION       dmb_test_delete_timer
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
void dmb_test_delete_timer(void)
{
  //DMB_MSG_TEST("%s..\n",__func__);

  dmb_test_cnt = 0;  
  del_timer(&dmb_test_tmer); 
}

/*===========================================================================
FUNCTION       dmb_test_work
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
void dmb_test_work(struct work_struct *work)
{
  //DMB_MSG_TEST("%s",__func__);

  dmb_set_ant_path(DMB_ANT_EARJACK);

#ifdef FEATURE_DMB_TSIF_IF
  dmb_tsif_test();
#endif /* FEATURE_DMB_TSIF_IF */

#ifdef CONFIG_SKY_TDMB
#ifdef FEATURE_TDMB_USE_INC  
   t3700_test(DMB_TEST_CH);
#elif defined(FEATURE_TDMB_USE_FCI)
   fc8050_test(DMB_TEST_CH);
#elif defined(FEATURE_TDMB_USE_RTV)
   mtv350_test(DMB_TEST_CH);
#elif defined(FEATURE_TDMB_USE_TCC)
   tcc3170_test(DMB_TEST_CH);
#else
  #error
#endif
#endif /* CONFIG_SKY_TDMB */

#ifdef CONFIG_SKY_ISDBT
#ifdef FEATURE_ISDBT_USE_SHARP  
  sharp_test(DMB_TEST_CH);
#else
  #error
#endif
#endif /* CONFIG_SKY_ISDBT */

#ifdef FEATURE_BOOTTEST_READ_BER
  setup_timer(&dmb_ber_timer, dmb_ber_callback, 0);
  mod_timer(&dmb_ber_timer, jiffies+msecs_to_jiffies(1000));
  INIT_WORK(&dmb_test_berwq, dmb_ber_work);
#endif
}

/*===========================================================================
FUNCTION       dmb_test_callback
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
void dmb_test_callback(unsigned long data)
{  
  DMB_MSG_TEST("%s  cnt [%d]",__func__, dmb_test_cnt);

  dmb_test_cnt++;
  if(dmb_test_cnt == TDMB_BOOT_TEST_START_TIME)
  {
    dmb_test_delete_timer();
    if(dmb_boot_wq != NULL)
    {
      queue_work(dmb_boot_wq, &dmb_test_startwq);
    }
  }
  else
  {
    mod_timer(&dmb_test_tmer, jiffies + msecs_to_jiffies(1000));
  }
}

/*===========================================================================
FUNCTION       dmb_ch_test
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
void dmb_ch_test(void)
{
  DMB_MSG_TEST("[%s] !!\n", __func__);

  dmb_boot_wq = create_singlethread_workqueue("dmb_boot_wq");
  if(dmb_boot_wq == NULL)
  {
    DMB_MSG_TEST("[%s] dmb work queue is NULL !!\n",__func__);
  }

  setup_timer(&dmb_test_tmer, dmb_test_callback, 0);
  mod_timer(&dmb_test_tmer, jiffies+msecs_to_jiffies(1000));
  INIT_WORK(&dmb_test_startwq, dmb_test_work);
}

/*===========================================================================
FUNCTION       dmb_test_on_boot
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
void dmb_test_on_boot(void)
{
  DMB_MSG_TEST("[%s] Test start!!!\n", __func__);

#ifdef CONFIG_SKY_TDMB
  tdmb_bb_power_on();

#ifdef FEATURE_NETBER_TEST_ON_BOOT
  netber_init();
  dmb_mode = TDMB_MODE_NETBER;
#endif

#ifdef FEATURE_COMMAND_TEST_ON_BOOT
 #ifdef FEATURE_TDMB_USE_INC
   t3700_if_test();
 #elif defined(FEATURE_TDMB_USE_FCI)
   fc8050_if_test(1);
 #elif defined(FEATURE_TDMB_USE_TCC)
   tcc3170_rw_test();
 #elif defined(FEATURE_TDMB_USE_RTV)
   mtv350_i2c_test();
 #endif /* FEATURE_TDMB_USE_INC */
#else
  dmb_ch_test();
#if (defined(FEATURE_DMB_EBI_IF) || defined(FEATURE_DMB_SPI_IF))
  tdmb_bb_set_fic_isr(1);
#endif
#endif
#endif /* CONFIG_SKY_TDMB */

#ifdef CONFIG_SKY_ISDBT
  isdbt_bb_power_on();

#ifdef FEATURE_COMMAND_TEST_ON_BOOT  
  #if defined(FEATURE_DMB_SPI_CMD)
      // TO DO
  #else
    #ifdef FEATURE_ISDBT_USE_SHARP
      sharp_i2c_test();
    #endif
  #endif
#else
  dmb_ch_test();
#endif
#endif /* CONFIG_SKY_ISDBT */

  DMB_MSG_TEST("[%s] Test end!!!\n", __func__);
}
#endif /* FEATURE_TEST_ON_BOO T*/
