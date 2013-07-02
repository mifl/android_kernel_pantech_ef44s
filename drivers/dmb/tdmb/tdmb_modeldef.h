//=============================================================================
// File       : Tdmb_modeldef.h
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2010/12/06       yschoi         Create
//                                              (tdmb_dev.h, tdmb_comdef.h 에서 분리)
//=============================================================================

#ifndef _TDMB_MODELDEF_INCLUDES_H_
#define _TDMB_MODELDEF_INCLUDES_H_

#ifdef CONFIG_ARCH_TEGRA
#include <../gpio-names.h>
#endif

/*================================================================== */
/*================     MODEL FEATURE               ================= */
/*================================================================== */

#if 0
/////////////////////////////////////////////////////////////////////////
// DMB GPIO (depend on model H/W)
// 중복되어 define 된 모델이 있으므로 사용하는 모델을 맨앞으로 가져온다.
/////////////////////////////////////////////////////////////////////////

#elif defined(CONFIG_MACH_APQ8064_EF52S)||defined(CONFIG_MACH_APQ8064_EF52K)||defined(CONFIG_MACH_APQ8064_EF52L)
  #define DMB_RESET     72
  #define DMB_INT       36
  #define DMB_PWR_EN    31
  //#define DMB_ANT_SEL    37
  //#define DMB_ANT_EAR_ACT   1
  //#define DMB_ANT_EAR_ACT2	1
  //#define FEATURE_DMB_SET_ANT_PATH
  //#define FEATURE_DMB_SET_ANT_PATH_POWER
  //#define FEATURE_DMB_PMIC8921
  #define FEATURE_DMB_GPIO_INIT
  #define FEATURE_DMB_THREAD
#ifdef CONFIG_SKY_DMB_I2C_CMD
  #define FEATURE_DMB_THREAD_FIC_BUF
#endif
#ifdef CONFIG_SKY_DMB_PMIC_19200
  #define FEATURE_DMB_CLK_19200
  //  #define DMB_XO_SEL    45
#else
  #define FEATURE_DMB_CLK_24576
#endif

#elif defined(CONFIG_MACH_APQ8064_EF51S)||defined(CONFIG_MACH_APQ8064_EF51K)
  #define FEATURE_DMB_PMIC_GPIO  900
  #define DMB_RESET     6+FEATURE_DMB_PMIC_GPIO
  #define DMB_INT       36
  #define DMB_PWR_EN    31
  #define DMB_ANT_SEL   5+FEATURE_DMB_PMIC_GPIO
  #define DMB_I2C_SCL    54
  #define DMB_I2C_SDA    53
  #define DMB_ANT_EAR_ACT   1
  //#define DMB_ANT_EAR_ACT2	1
  #define FEATURE_DMB_SET_ANT_PATH
  #define FEATURE_DMB_SET_ANT_PATH_POWER
  #define FEATURE_DMB_PMIC8921
  #define FEATURE_DMB_GPIO_INIT
  #define FEATURE_DMB_THREAD
  #define FEATURE_DMB_THREAD_FIC_BUF
#ifdef CONFIG_SKY_DMB_PMIC_19200
  #define FEATURE_DMB_CLK_19200
//  #define DMB_XO_SEL    59
#else
  #define FEATURE_DMB_CLK_24576
#endif

#elif defined(CONFIG_MACH_APQ8064_EF51L)
  //#define DMB_RESET     38
  #define DMB_INT       36
  #define DMB_PWR_EN    31
  #define DMB_I2C_SCL   54
  #define DMB_I2C_SDA    53
  #define FEATURE_DMB_RTV_USE_FM_PATH //only RTV support
#ifndef FEATURE_DMB_RTV_USE_FM_PATH
  #define DMB_ANT_SEL    37
  #define DMB_ANT_EAR_ACT   1
  //#define DMB_ANT_EAR_ACT2	1
  #define FEATURE_DMB_SET_ANT_PATH
  #define FEATURE_DMB_SET_ANT_PATH_POWER
  #define FEATURE_DMB_PMIC8921
#endif
  #define FEATURE_DMB_GPIO_INIT
  #define FEATURE_DMB_THREAD
  #define FEATURE_DMB_THREAD_FIC_BUF
#ifdef CONFIG_SKY_DMB_PMIC_19200
  #define FEATURE_DMB_CLK_19200
  //  #define DMB_XO_SEL    59
#else
  #define FEATURE_DMB_CLK_24576
#endif

#elif defined(CONFIG_MACH_APQ8064_EF48S)||defined(CONFIG_MACH_APQ8064_EF49K)
  #define DMB_RESET     72
  #define DMB_INT       36
  #define DMB_PWR_EN    31
  #define DMB_ANT_SEL   37
  #define DMB_I2C_SCL    54
  #define DMB_I2C_SDA    53
  #define DMB_ANT_EAR_ACT   1
  //#define DMB_ANT_EAR_ACT2  1
  #define FEATURE_DMB_SET_ANT_PATH
  #define FEATURE_DMB_SET_ANT_PATH_POWER
  #define FEATURE_DMB_PMIC8921
  #define FEATURE_DMB_GPIO_INIT
  #define FEATURE_DMB_THREAD
  #define FEATURE_DMB_THREAD_FIC_BUF
#ifdef CONFIG_SKY_DMB_PMIC_19200
  #define FEATURE_DMB_CLK_19200
  //#define DMB_XO_SEL    45
#else
  #define FEATURE_DMB_CLK_24576
#endif

#elif defined(CONFIG_MACH_APQ8064_EF50L)
  //#define DMB_RESET     38
  #define DMB_INT       36
  #define DMB_PWR_EN    31
  #define DMB_I2C_SCL    54
  #define DMB_I2C_SDA    53
#if CONFIG_BOARD_VER >= CONFIG_WS20
  #define FEATURE_DMB_RTV_USE_FM_PATH //only RTV support
#else
  #define DMB_ANT_SEL   37
  #define DMB_ANT_EAR_ACT   1
  //#define DMB_ANT_EAR_ACT2  1
  #define FEATURE_DMB_SET_ANT_PATH
  #define FEATURE_DMB_SET_ANT_PATH_POWER
  #define FEATURE_DMB_PMIC8921
#endif
  #define FEATURE_DMB_GPIO_INIT
  #define FEATURE_DMB_THREAD
  #define FEATURE_DMB_THREAD_FIC_BUF
  #define FEATURE_DMB_CLK_24576

#elif defined(CONFIG_MACH_TEGRA_MAPLE)
  #define DMB_RESET     TEGRA_GPIO_PQ4 //gpio Q.04 (132)
  #define DMB_INT       TEGRA_GPIO_PQ5 //gpio Q.05 (133)
  #define DMB_PWR_EN    TEGRA_GPIO_PV6 //gpio V.06 (174)
  #define DMB_ANT_SEL   TEGRA_GPIO_PI7 //gpio I.07 (71)
  #define DMB_ANT_EAR_ACT   1 
  #define FEATURE_DMB_SET_ANT_PATH
  //#define FEATURE_DMB_SET_ANT_PATH_POWER
  #define FEATURE_DMB_GPIO_INIT
  #define FEATURE_DMB_THREAD
  #define FEATURE_DMB_CLK_24576

/////////////////////////////////////////////////////////////////////////
#elif defined(CONFIG_MACH_MSM8960_EF44S)
  #define DMB_RESET     38
  #define DMB_INT       39
  #define DMB_PWR_EN    34
#if (BOARD_VER < WS10)
  #define DMB_ANT_SEL   26
#else
  #define DMB_ANT_SEL   6
#endif
  #define DMB_I2C_SCL    37
  #define DMB_I2C_SDA    36
#if (BOARD_VER > WS10)
  #define DMB_ANT_SEL2   56
#else
  #define DMB_ANT_SEL2   64
#endif
  #define DMB_ANT_EAR_ACT   0
  #define DMB_ANT_EAR_ACT2  1
  #define FEATURE_DMB_SET_ANT_PATH
  #define FEATURE_DMB_SET_ANT_PATH_POWER
  #define FEATURE_DMB_PMIC8921
  #define FEATURE_DMB_CLK_19200
  #define FEATURE_DMB_GPIO_INIT

/////////////////////////////////////////////////////////////////////////
#elif defined(CONFIG_MACH_MSM8960_EF47S) || defined(CONFIG_MACH_MSM8960_EF45K) 
  #define DMB_RESET     38
  #define DMB_INT       39
  #define DMB_PWR_EN    34
#if (BOARD_VER < TP10)
  #define DMB_ANT_SEL   26
#else
  #define DMB_ANT_SEL   6
  #define DMB_ANT_SEL_OLD 26
#endif
#if (BOARD_VER > PT10)
  #define DMB_I2C_SCL    37
  #define DMB_I2C_SDA    36
  #define DMB_ANT_SEL2   64
#else
  #define DMB_I2C_SCL    41
  #define DMB_I2C_SDA    40
#endif
  #define DMB_ANT_EAR_ACT   0
  #define DMB_ANT_EAR_ACT2  1
  #define FEATURE_DMB_SET_ANT_PATH
  #define FEATURE_DMB_SET_ANT_PATH_POWER
  #define FEATURE_DMB_PMIC8921
  #define FEATURE_DMB_GPIO_INIT
  #define FEATURE_DMB_THREAD
  #define FEATURE_DMB_THREAD_FIC_BUF
  #define FEATURE_DMB_CLK_24576

/////////////////////////////////////////////////////////////////////////
#elif defined(CONFIG_MACH_MSM8960_EF46L)
  #define DMB_RESET     38
  #define DMB_INT       39
  #define DMB_PWR_EN    34
#if (BOARD_VER < TP10)
  #define DMB_ANT_SEL   26
#else
  #define DMB_ANT_SEL   6
  #define DMB_ANT_SEL_OLD 26
#endif
#if (BOARD_VER > PT10)
  #define DMB_I2C_SCL    37
  #define DMB_I2C_SDA    36
  #define DMB_ANT_SEL2   64
  #define DMB_ANT_EAR_ACT   1
  #define DMB_ANT_EAR_ACT2  0
#else
  #define DMB_I2C_SCL    41
  #define DMB_I2C_SDA    40
  #define DMB_ANT_EAR_ACT   0
#endif
  #define FEATURE_DMB_SET_ANT_PATH
  #define FEATURE_DMB_SET_ANT_PATH_POWER
#if (BOARD_VER < WS10)
  #define FEATURE_DMB_PMIC_POWER
#endif
  #define FEATURE_DMB_PMIC8921
  #define FEATURE_DMB_GPIO_INIT
  #define FEATURE_DMB_THREAD
  #define FEATURE_DMB_THREAD_FIC_BUF
  #define FEATURE_DMB_CLK_24576

/////////////////////////////////////////////////////////////////////////
#elif defined(CONFIG_SKY_EF65L_BOARD)
  #define DMB_RESET     102
  #define DMB_INT       127
  #define DMB_PWR_EN    86
  //#define FEATURE_DMB_SET_ANT_PATH
  //#define DMB_ANT_SEL   42
  #define DMB_ANT_EAR_ACT   1 
  #define FEATURE_DMB_CLK_19200
  #define DMB_XO_SEL 41
  #define FEATURE_DMB_GPIO_INIT
  #define FEATURE_DMB_THREAD

/////////////////////////////////////////////////////////////////////////
#elif (defined(CONFIG_SKY_EF40S_BOARD) || defined(CONFIG_SKY_EF40K_BOARD))
  #define DMB_RESET     102
  #define DMB_INT       127
  #define DMB_PWR_EN    86
  #define DMB_ANT_SEL   42
  #define DMB_ANT_EAR_ACT   1 
#if (BOARD_REV > WS20)
  #define FEATURE_DMB_CLK_19200
  #define DMB_XO_SEL 19
#else
  #define FEATURE_DMB_CLK_24576
#endif
  //#define FEATURE_DMB_PMIC_POWER
  #define FEATURE_DMB_SET_ANT_PATH
  #define FEATURE_DMB_PMIC8058
  #define FEATURE_DMB_GPIO_INIT
  #define FEATURE_DMB_THREAD

/////////////////////////////////////////////////////////////////////////
#elif defined(CONFIG_SKY_EF39S_BOARD)
  #define DMB_RESET     102
  #define DMB_INT       127
  #define DMB_PWR_EN    86
  #define DMB_ANT_SEL   42
#if (BOARD_REV > WS10)
  #define DMB_ANT_EAR_ACT   0
#else
  #define DMB_ANT_EAR_ACT   1
#endif
  #define FEATURE_DMB_SET_ANT_PATH
  #define FEATURE_DMB_SET_ANT_PATH_POWER

#if (BOARD_REV > WS20)
  #define FEATURE_DMB_CLK_19200
  #define DMB_XO_SEL 19
#else
  #define FEATURE_DMB_CLK_24576
#endif
  //#define FEATURE_DMB_PMIC_POWER // TP20 에서 잠시 검토
  #define FEATURE_DMB_GPIO_INIT
  #define FEATURE_DMB_THREAD

/////////////////////////////////////////////////////////////////////////
#elif defined(CONFIG_EF35_BOARD)
  #define DMB_RESET     129
  #define DMB_INT       127
  #define DMB_PWR_EN    136
  #define DMB_I2C_SCL    73
  #define DMB_I2C_SDA    72
//#if EF35L_BDVER_L(WS20)
  #define DMB_ANT_SEL   42
  #define DMB_ANT_EAR_ACT   1
  #define FEATURE_DMB_SET_ANT_PATH
//#endif
  //#define FEATURE_DMB_PMIC_POWER
  #define FEATURE_DMB_GPIO_INIT

/////////////////////////////////////////////////////////////////////////
#elif (defined(CONFIG_EF33_BOARD) || defined(CONFIG_EF34_BOARD))
  #define DMB_RESET     129
  #define DMB_INT       127
  #define DMB_PWR_EN    136
  #define DMB_I2C_SCL    73
  #define DMB_I2C_SDA    72
#if (EF33S_BDVER_L(WS20) || EF34K_BDVER_L(WS20)) // Since WS20 use only retractable Antenna
  #define DMB_ANT_SEL   42
  #define DMB_ANT_EAR_ACT   1
  #define FEATURE_DMB_SET_ANT_PATH
#endif
  //#define FEATURE_DMB_PMIC_POWER
  #define FEATURE_DMB_GPIO_INIT
  #define FEATURE_DMB_CLK_24576

/////////////////////////////////////////////////////////////////////////
#elif defined(CONFIG_EF31S_32K_BOARD)
  #define DMB_RESET     26
  #define DMB_INT       30
  #define DMB_PWR_EN    106
  #define DMB_I2C_SDA   28
  #define DMB_I2C_SCL   29
  #define FEATURE_DMB_PMIC_POWER
  //#define FEATURE_DMB_TSIF_CLK_CTL
  #define FEATURE_DMB_GPIO_INIT
  #define FEATURE_DMB_CLK_24576

/////////////////////////////////////////////////////////////////////////
#elif defined(CONFIG_EF18_BOARD)
  #define DMB_RESET     163
  #define DMB_INT       40
  #define DMB_PWR_EN    164
  #define DMB_ANT_SEL   172
  #define DMB_ANT_EAR_ACT   0
  #define FEATURE_DMB_SET_ANT_PATH 
  #define FEATURE_DMB_CLK_24576

/////////////////////////////////////////////////////////////////////////
#elif defined(CONFIG_EF14_BOARD)
  #define DMB_RESET     155
  #define DMB_INT       152
  #define DMB_PWR_EN    151
  #define DMB_ANT_SEL   21
  #define DMB_ANT_EAR_ACT   1 
  #define FEATURE_DMB_SET_ANT_PATH
  #define FEATURE_DMB_CLK_24576

/////////////////////////////////////////////////////////////////////////
#elif defined(CONFIG_EF13_BOARD)
  #define DMB_RESET     87
  #define DMB_INT       84
  #define DMB_PWR_EN    85
  #define DMB_ANT_SEL   79
  #define DMB_ANT_EAR_ACT   0 // Earjack Low active
  #define FEATURE_DMB_SET_ANT_PATH
  #define FEATURE_DMB_CLK_24576

/////////////////////////////////////////////////////////////////////////
#elif defined(CONFIG_EF12_BOARD)
  #define DMB_RESET     155
  #define DMB_INT       98
  //#define DMB_PWR_EN    57 // not used
  //#define DMB_ANT_SEL   58 // not used
  //#define DMB_ANT_EAR_ACT   1 // not used
  //#define FEATURE_DMB_SET_ANT_PATH
  #define FEATURE_DMB_PMIC_POWER
  #define FEATURE_DMB_CLK_24576

/////////////////////////////////////////////////////////////////////////
#elif defined(CONFIG_SP33_BOARD)
  #define DMB_RESET     23
  #define DMB_INT       18
  #define DMB_PWR_EN    26
  #define DMB_ANT_SEL   72
  #define DMB_ANT_EAR_ACT   1 // Earjack High active
  #define FEATURE_DMB_SET_ANT_PATH
  #define FEATURE_DMB_CLK_24576

/////////////////////////////////////////////////////////////////////////
#elif defined(CONFIG_EF10_BOARD)
  #define DMB_RESET     155
  #define DMB_INT       98
  //#define DMB_PWR_EN    57 // not used
  #define DMB_ANT_SEL   58
  #define DMB_ANT_EAR_ACT   1 // Earjack active (1 : High, 0 : low)
  #define FEATURE_DMB_SET_ANT_PATH
  #define FEATURE_DMB_PMIC_POWER
  #define FEATURE_DMB_CLK_24576

#else
  #error
#endif


/*================================================================== */
/*================     TEST FEATURE                ================= */
/*================================================================== */

//#define FEATURE_TS_PKT_MSG // Single CH : 모든 패킷을 보여줌, Mulch CH : 읽은 데이터의 첫 패킷만 보여줌.
//#define FEATURE_TEST_ON_BOOT
//#define FEATURE_TEST_READ_DATA_ON_BOOT
//#define FEATURE_NETBER_TEST_ON_BOOT
//#define FEATURE_NETBER_TEST_ON_AIR
//#define FEATURE_DMB_DUMP_FILE
//#define FEATURE_APP_CALL_TEST_FUNC
//#define FEAUTRE_USE_FIXED_FIC_DATA
//#define FEATURE_DMB_GPIO_DEBUG => dmb_hw.c 로 이동 (빌드 단축)
//#define FEATURE_COMMAND_TEST_ON_BOOT
//#define FEATURE_DMB_I2C_WRITE_CHECK => dmb_i2c.c 로 이동 (빌드 단축)
//#define FEATURE_DMB_I2C_DBG_MSG => dmb_i2c.c 로 이동 (빌드 단축)
//#define FEATURE_EBI_WRITE_CHECK
//#define FEATURE_HW_INPUT_MATCHING_TEST

#ifndef CONFIG_SKY_DMB_TSIF_IF
#if (defined(FEATURE_TEST_ON_BOOT) && !defined(FEATURE_DMB_THREAD)) // 부팅테스트시 쓰레드로 돌려 데이터 계속 읽음
#define FEATURE_DMB_THREAD
#endif
#endif

#ifdef CONFIG_SKY_TDMB_INC_BB
#if (defined(FEATURE_TEST_ON_BOOT) || defined(FEATURE_NETBER_TEST_ON_BOOT) || defined(FEATURE_APP_CALL_TEST_FUNC))
#define INC_FICDEC_USE // 테스트시 채널정보들이 정확하지 않을때 필요
#define FEATURE_INC_FIC_UPDATE_MSG
#endif
#endif /* CONFIG_SKY_TDMB_INC_BB */


#endif /* _TDMB_MODELDEF_INCLUDES_H_ */
