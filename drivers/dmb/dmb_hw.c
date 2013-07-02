//=============================================================================
// File       : dmb_hw.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2010/11/02       yschoi         Create
//  1.1.0       2011/09/29       yschoi         tdmb_gpio.c => dmb_hw.c
//=============================================================================

#include <linux/kernel.h>
#include <linux/module.h>
#ifdef CONFIG_ARCH_MSM
#include <linux/gpio.h>
#endif
#ifdef CONFIG_ARCH_TEGRA
#include <linux/gpio.h>
#endif

#include <linux/delay.h>

#include "dmb_comdef.h"
#include "dmb_hw.h"

#ifdef FEATURE_DMB_PMIC_19200
#include <mach/msm_xo.h>
#endif

#ifdef FEATURE_DMB_PMIC_POWER
#include <mach/vreg.h>
#include <mach/mpp.h>
#endif /* FEATURE_DMB_PMIC_POWER */

#if defined(FEATURE_DMB_PMIC8058) || defined(FEATURE_DMB_PMIC8921)
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/pmic8058-regulator.h>
#endif

#ifdef FEATURE_DMB_RTV_USE_FM_PATH
#include "tdmb/tdmb_chip.h"
#endif

#ifdef FEATURE_DMB_PMIC_GPIO
#include <mach/irqs.h>
#if 0
// DMB PMIC GPIO (depend on model H/W)
#elif defined(FEATURE_DMB_PMIC8921)
  #define PMIC_GPIO_PM_TO_SYS(pm_gpio)  (pm_gpio - 1 + NR_GPIO_IRQS - FEATURE_DMB_PMIC_GPIO)
#endif
#endif

/*================================================================== */
/*================       DMB HW Driver Definition        ================= */
/*================================================================== */

// #define FEATURE_DMB_GPIO_DEBUG


#ifdef FEATURE_DMB_GPIO_INIT

#ifdef CONFIG_ARCH_MSM
static uint32_t dmb_gpio_init_table[] = {
  GPIO_CFG(DMB_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),

#ifdef DMB_RESET
  GPIO_CFG(DMB_RESET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
#endif

#ifdef DMB_PWR_EN
  GPIO_CFG(DMB_PWR_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
#endif

#ifdef FEATURE_DMB_SET_ANT_PATH
  GPIO_CFG(DMB_ANT_SEL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
#ifdef DMB_ANT_SEL2
  GPIO_CFG(DMB_ANT_SEL2, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
#endif  
#endif

#ifdef DMB_XO_SEL
  GPIO_CFG(DMB_XO_SEL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
#endif

#if defined(CONFIG_MACH_MSM8960_SIRIUSLTE) && (BOARD_VER == PT11)
  GPIO_CFG(58, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
  GPIO_CFG(77, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
#endif

#ifdef DMB_LNA_EN
  GPIO_CFG(DMB_LNA_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
  GPIO_CFG(DMB_LNA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
#endif

};
#endif /* CONFIG_ARCH_MSM */


#ifdef CONFIG_ARCH_TEGRA
static uint32_t dmb_gpio_init_table[][3] = { //{GPIO, IS_INPUT, VALUE}
  {DMB_INT, 1, 0},
  {DMB_RESET, 0, 0},

#ifndef FEATURE_DMB_PMIC_POWER
  {DMB_PWR_EN, 0, 0},
#endif

#ifdef FEATURE_DMB_SET_ANT_PATH
  {DMB_ANT_SEL, 0, 0},
#endif

#ifdef DMB_XO_SEL
  {DMB_XO_SEL, 0, 0},
#endif
};
#endif /* CONFIG_ARCH_TEGRA */
#endif /* FEATURE_DMB_GPIO_INIT */


#ifdef CONFIG_SKY_TDMB_MICRO_USB_DETECT
extern int pm8058_is_dmb_ant(void);
#endif



/*================================================================== */
/*==============        DMB HW Driver Function      =============== */
/*================================================================== */

#ifdef FEATURE_DMB_GPIO_INIT
/*====================================================================
FUNCTION       dmb_gpio_init
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void dmb_gpio_init(void)
{
  int i;
#ifdef CONFIG_ARCH_MSM
  int rc;
#endif

  DMB_MSG_HW("[%s] dmb_gpio_init!!!\n",__func__);

  for(i = 0; i < ARRAY_SIZE(dmb_gpio_init_table); i ++)
  {
#ifdef CONFIG_ARCH_MSM
#ifdef FEATURE_DMB_PMIC_GPIO
    if(GPIO_PIN(dmb_gpio_init_table[i]) >= FEATURE_DMB_PMIC_GPIO)
      continue;	// PMIC GPIO인 경우 board-8064-pmic.c에서 초기화 한다.
#endif
    rc = gpio_tlmm_config(dmb_gpio_init_table[i], GPIO_CFG_ENABLE);
    if (rc)
    {
      DMB_MSG_HW("[%s] gpio_tlmm_config(%#x)=%d\n",__func__, dmb_gpio_init_table[i], rc);
      break;
    }
#elif defined(CONFIG_ARCH_TEGRA)
    tegra_gpio_enable(dmb_gpio_init_table[i][0]);
    tegra_gpio_init_configure(dmb_gpio_init_table[i][0],dmb_gpio_init_table[i][1],dmb_gpio_init_table[i][2]);
    //DMB_MSG_HW("[%s] dmb_gpio_init gpio[%d], is_input[%d], value[%d]\n", __func__, dmb_gpio_init_table[i][0], dmb_gpio_init_table[i][1], dmb_gpio_init_table[i][2]);
#endif
  }

#if (defined(CONFIG_MACH_APQ8064_EF48S)||defined(CONFIG_MACH_APQ8064_EF49K)||defined(CONFIG_MACH_APQ8064_EF50L)||defined(CONFIG_MACH_APQ8064_EF51S)||defined(CONFIG_MACH_APQ8064_EF51K)||defined(CONFIG_MACH_APQ8064_EF51L))
 #ifdef DMB_ANT_SEL
  dmb_set_gpio(DMB_ANT_SEL, 0);
 #endif
#endif

// EF39S ant_sel low
#if (defined(CONFIG_SKY_EF39S_BOARD) && (BOARD_REV > WS10))
  dmb_set_gpio(DMB_ANT_SEL, 0);
#endif

  DMB_MSG_HW("[%s] end cnt[%d]!!!\n",__func__, i);
}
#endif /* FEATURE_DMB_GPIO_INIT */


/*====================================================================
FUNCTION       dmb_set_gpio
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void dmb_set_gpio(uint gpio, bool value)
{
#if 1
#ifdef FEATURE_DMB_PMIC_GPIO
  if(gpio >= FEATURE_DMB_PMIC_GPIO)
    gpio_set_value_cansleep(PMIC_GPIO_PM_TO_SYS(gpio), value);
  else
#endif
  gpio_set_value(gpio, value);
  
  DMB_MSG_HW("[%s] gpio [%d] set [%d]\n", __func__, gpio, value);

#ifdef FEATURE_DMB_GPIO_DEBUG
#ifdef FEATURE_DMB_PMIC_GPIO
  if(gpio >= FEATURE_DMB_PMIC_GPIO)
    DMB_MSG_HW("[%s] gpio [%d] get [%d]\n", __func__, gpio, gpio_get_value_cansleep(PMIC_GPIO_PM_TO_SYS(gpio)));
  else
#endif
  DMB_MSG_HW("[%s] gpio [%d] get [%d]\n", __func__, gpio, gpio_get_value(gpio));
#endif
#else
  int rc = 0;

  rc = gpio_request(gpio, "dmb_gpio");
  if (!rc)
  {
    rc = gpio_direction_output(gpio, value);
    DMB_MSG_HW("[%s] gpio [%d] set [%d]\n", __func__, gpio, value);
  }
  else
  {
    DMB_MSG_HW("[%s] gpio_request fail!!!\n", __func__);
  }

#ifdef FEATURE_DMB_GPIO_DEBUG
  DMB_MSG_HW("[%s] gpio [%d] get [%d]\n", __func__, gpio, gpio_get_value(gpio));
#endif

  gpio_free(gpio);
#endif // 0
}


#ifdef CONFIG_SKY_TDMB_MICRO_USB_DETECT
/*====================================================================
FUNCTION       dmb_micro_usb_ant_detect
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
int dmb_micro_usb_ant_detect(void)
{
  int ret = 0;

  DMB_MSG_HW("[%s] on[%d]!!!\n", __func__, on);
  
  ret = pm8058_is_dmb_ant();

  return ret;
}
#endif /* CONFIG_SKY_TDMB_MICRO_USB_DETECT */


#if defined(FEATURE_DMB_PMIC8058) || defined(FEATURE_DMB_PMIC8921)
/*====================================================================
FUNCTION       dmb_pmic_8058_onoff
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static boolean dmb_pmic_8058_onoff(int on, const char *id, int min_uV, int max_uV)
{
  static struct regulator *vreg; //If verg value was changed, can not disable regulator
  int ret = 0;
  
  DMB_MSG_HW("[%s] on[%d]!!!\n", __func__, on);

  vreg = regulator_get(NULL, id);

  if ((min_uV > 0) && (max_uV > 0))
  {
    ret = regulator_set_voltage(vreg, min_uV, max_uV);
  }

  if(on)
  {
    if(!ret) ret = regulator_enable(vreg);

    if(ret)
    {
      DMB_MSG_HW("DMB regulator_enable Fail [%d] !!!\n",ret);
      return FALSE;
    }
  }
  else
  {
    ret = regulator_disable(vreg);

    if(ret)
    {
      DMB_MSG_HW("DMB regulator_disable Fail [%d] !!!\n",ret);
      return FALSE;      
    }
  }

  return TRUE;
}
#endif /* FEATURE_DMB_PMIC8058 */


#if defined(FEATURE_DMB_PMIC8921)
/*====================================================================
FUNCTION       dmb_pmic_8921_onoff
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static boolean dmb_pmic_8921_onoff(int on, const char *id, int min_uV, int max_uV)
{
  int ret = 0;

  ret = dmb_pmic_8058_onoff(on, id, min_uV, max_uV);

  return ret;
}
#endif /* FEATURE_DMB_PMIC8921 */


#ifdef FEATURE_DMB_PMIC_POWER
/*====================================================================
FUNCTION       dmb_pmic_power_onoff
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static boolean dmb_pmic_power_onoff(int on)
{
  int ret = 0;

#if defined(CONFIG_MACH_MSM8960_EF46L)
  ret = dmb_pmic_8921_onoff(on, "8921_l18", 1300000, 1300000);
#endif

#if defined(CONFIG_MACH_MSM8960_SIRIUSLTE) && (BOARD_VER != PT11)
  ret = dmb_pmic_8921_onoff(on, "8921_l18", 1200000, 1200000);
  ret = dmb_pmic_8921_onoff(on, "8921_lvs7", 0, 0);
#endif

#if defined(CONFIG_EF31S_32K_BOARD)
  //ret = ;
#endif

#if (defined(CONFIG_EF10_BOARD) || defined(CONFIG_EF12_BOARD))
  //ret = ;
#endif

  return ret;
}
#endif /* FEATURE_DMB_PMIC_POWER */


#ifdef FEATURE_DMB_SET_ANT_PATH_POWER
/*====================================================================
FUNCTION       dmb_pmic_ant_switch_power
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static boolean dmb_pmic_ant_switch_power(int on)
{
  int ret = 0;

#if (defined(CONFIG_MACH_APQ8064_EF51S)||defined(CONFIG_MACH_APQ8064_EF51K))
  ret = dmb_pmic_8921_onoff(on, "8921_l11", 2850000, 2850000);
#endif

#if (defined(CONFIG_MACH_APQ8064_EF48S)||defined(CONFIG_MACH_APQ8064_EF49K)||defined(CONFIG_MACH_APQ8064_EF50L))
  ret = dmb_pmic_8921_onoff(on, "8921_l29", 2800000, 2800000);
#endif

#if (defined(CONFIG_MACH_MSM8960_EF47S) || defined(CONFIG_MACH_MSM8960_EF45K) || defined(CONFIG_MACH_MSM8960_EF46L) || defined(CONFIG_MACH_MSM8960_EF44S))
  ret = dmb_pmic_8921_onoff(on, "8921_l22", 2750000, 2750000);
#endif

#if (defined(CONFIG_SKY_EF39S_BOARD) || defined(CONFIG_SKY_EF40S_BOARD) || defined(CONFIG_SKY_EF40K_BOARD))
  ret = dmb_pmic_8058_onoff(on, "8058_l11", 2600000, 2600000);
#endif

  return ret;
}
#endif /* FEATURE_DMB_SET_ANT_PATH_POWER */


#ifdef FEATURE_DMB_PMIC_19200
static struct msm_xo_voter *xo_handle_a1 = NULL;

static void dmb_pmic_xo_onoff(int on)
{
  int rc =0;

  if(!xo_handle_a1)
  {
    xo_handle_a1 = msm_xo_get(MSM_XO_TCXO_A1, "DMB_XTAL");
  }

  if(on)
  {
    rc = msm_xo_mode_vote(xo_handle_a1, MSM_XO_MODE_ON);
  }
  else
  {
    rc = msm_xo_mode_vote(xo_handle_a1, MSM_XO_MODE_OFF);
  }
  if (rc < 0)
  {
    DMB_MSG_HW("[%s] Configuring MSM_XO_MODE_ON_OFF failed (%d)\n", __func__, rc);
    return;
  }

  return;
}
#endif /* FEATURE_DMB_PMIC_19200 */


/*====================================================================
FUNCTION       dmb_power_on
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void dmb_power_on(void)
{
  DMB_MSG_HW("[%s] start!!!\n", __func__);

#ifdef FEATURE_DMB_SET_ANT_PATH_POWER
  dmb_pmic_ant_switch_power(TRUE);
#endif

#ifdef FEATURE_DMB_PMIC_19200
  dmb_pmic_xo_onoff(1);
#endif

#ifdef DMB_XO_SEL
  dmb_set_gpio(DMB_XO_SEL, 1);
#endif

#ifdef DMB_LNA_EN
  dmb_set_gpio(DMB_LNA_EN, 1);
  dmb_set_gpio(DMB_LNA, 0);
#endif

#if (defined(FEATURE_DMB_TSIF_IF) && defined(FEATURE_DMB_TSIF_CLK_CTL))
  dmb_tsif_clk_enable();
#endif
}


/*====================================================================
FUNCTION       dmb_power_on_chip
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void dmb_power_on_chip(void)
{
  DMB_MSG_HW("[%s] start!!!\n", __func__);

#ifdef FEATURE_DMB_PMIC_POWER
  dmb_pmic_power_onoff(1);
#endif

#ifdef DMB_PWR_EN
  dmb_set_gpio(DMB_PWR_EN, 1);
#endif

#if defined(CONFIG_MACH_MSM8960_SIRIUSLTE) && (BOARD_VER == PT11)
  dmb_set_gpio(58, 1);
  dmb_set_gpio(77, 1);
#endif
}


/*====================================================================
FUNCTION       dmb_power_off
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void dmb_power_off(void)
{
  DMB_MSG_HW("[%s] start!!!\n", __func__);

#ifdef FEATURE_DMB_PMIC_POWER
  dmb_pmic_power_onoff(0);
#endif

#ifdef DMB_PWR_EN
  dmb_set_gpio(DMB_PWR_EN, 0);
#endif
  msleep(1);

#ifdef FEATURE_DMB_SET_ANT_PATH_POWER
  dmb_pmic_ant_switch_power(FALSE);
#endif

#ifdef FEATURE_DMB_SET_ANT_PATH
  dmb_set_gpio(DMB_ANT_SEL, 0);
#ifdef DMB_ANT_SEL2
  dmb_set_gpio(DMB_ANT_SEL2, 0);
#endif
#endif

#ifdef FEATURE_DMB_PMIC_19200
  dmb_pmic_xo_onoff(0);
#endif

#ifdef DMB_XO_SEL
  dmb_set_gpio(DMB_XO_SEL, 0);
#endif

  DMB_MSG_HW("[%s] end!!!\n", __func__);

#if (defined(FEATURE_DMB_TSIF_IF) && defined(FEATURE_DMB_TSIF_CLK_CTL))
  dmb_tsif_clk_disable();
#endif
}


/*====================================================================
FUNCTION       dmb_set_ant_path
DESCRIPTION         
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void dmb_set_ant_path(int ant_type)
{
#ifdef FEATURE_DMB_SET_ANT_PATH
  DMB_MSG_HW("[%s] ant_type[%d]\n", __func__, ant_type);

#if defined(CONFIG_ARCH_MSM8960) && (DMB_ANT_SEL == 26)
  gpio_tlmm_config(GPIO_CFG(DMB_ANT_SEL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#endif

  if (ant_type == DMB_ANT_EARJACK)
  {
    dmb_set_gpio(DMB_ANT_SEL, DMB_ANT_EAR_ACT);
#ifdef DMB_ANT_SEL2
    dmb_set_gpio(DMB_ANT_SEL2, DMB_ANT_EAR_ACT2);
#endif
  }
  else
  {
    dmb_set_gpio(DMB_ANT_SEL, (DMB_ANT_EAR_ACT)?0:1);
#ifdef DMB_ANT_SEL2
    dmb_set_gpio(DMB_ANT_SEL2, (DMB_ANT_EAR_ACT2)?0:1);
#endif
  }
#else
#ifdef FEATURE_DMB_RTV_USE_FM_PATH //only RaonTech support
  mtv350_use_FM_path(ant_type);
#else
  DMB_MSG_HW("[%s]  Do nothing, No ANT. switch\n", __func__);
#endif
#endif /* FEATURE_DMB_SET_ANT_PATH */
}

