#ifndef __ARCH_ARM_MACH_MSM_SKY_SYS_RESET_H
#define __ARCH_ARM_MACH_MSM_SKY_SYS_RESET_H

#define SYS_RESTART_REASON_ADDR 0x2A05F65C

#define SYS_RESET_REASON_NORMAL   0x776655DD
#define SYS_RESET_REASON_EXCEPTION	0x776655E0
#define SYS_RESET_REASON_ASSERT		0x776655E1
#define SYS_RESET_REASON_LINUX    0x776655E2
#define SYS_RESET_REASON_ANDROID  0x776655E3
#define SYS_RESET_REASON_LPASS    0x776655E4
#define SYS_RESET_REASON_DSPS     0x776655E5
#define SYS_RESET_REASON_RIVA     0x776655E6
#define SYS_RESET_REASON_UNKNOWN  0x776655E7
#ifdef CONFIG_PANTECH_FS_AUTO_REPAIR
#define SYS_RESET_REASON_USERDATA_FS 0x776655EA // "FEATURE_PANTECH_AUTO_REPAIR"
#endif /* CONFIG_PANTECH_FS_AUTO_REPAIR */
#define SYS_RESET_REASON_ABNORMAL 0x77236d34
#define SYS_RESET_REASON_DOGBARK  0x776655E9
#define SYS_RESET_BACKLIGHT_OFF_FLAG 0x08000000

#define SYS_RESET_PDL_DOWNLOAD_ENTRY		0xCC33CC33
#define SYS_RESET_PDL_IDLE_DOWNLOAD_ENTRY	0x33CC33CC

#define PANTECH_SHARED_RAM_BASE 0x88B00000 
#define PANTECH_SHARED_RAM_SIZE  0x100000

extern void sky_sys_rst_set_silent_boot_info(void);
extern uint8_t sky_sys_rst_is_silent_boot_mode(void);
extern uint8_t sky_sys_rst_is_backlight_off(void);
extern void  sky_sys_rst_is_silent_boot_backlight(int backlight);
extern void sky_sys_rst_is_silent_boot_for_test(int silent_mode,int backlight);

extern int sky_reset_reason;
extern bool pantech_is_smpl; //p14527 add to fix smpl 
#endif

