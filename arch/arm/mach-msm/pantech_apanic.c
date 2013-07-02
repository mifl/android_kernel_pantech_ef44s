/* arch/arm/mach-msm/apanic_pantech.c
 *
 * Copyright (C) 2011 PANTECH, Co. Ltd.
 * based on drivers/misc/apanic.c
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.      See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/wakelock.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/notifier.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/preempt.h>
#include <asm/cacheflush.h>
#include <asm/system.h>
#include <linux/fb.h>
#include <linux/time.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/nmi.h>
#include <mach/msm_iomap.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <mach/pantech_apanic.h>
#include <linux/console.h>
#include "smd_private.h"
#include "modem_notifier.h"
#include "sky_sys_reset.h"

#define MAX_CORES 2
#define MMU_BUF_SIZE 256

#define POS(x) (x > 0 ? x : 0)
#define WRITE_LOG(...) \
  if(bufsize != 0) { \
    n = snprintf(s, POS(bufsize), __VA_ARGS__); \
    s+=n; \
    total +=n; \
    bufsize-=n;\
  }

#define MMU_SCRIPT_BUF_SIZE 512
#define EXCP_SCRIPT_BUF_SIZE 512

#define PANIC_MAGIC 0xDAEDDAED
#define PHDR_VERSION 0x01

struct painc_info_date {
	unsigned short year;
	unsigned short month;
	unsigned short day;
	unsigned short hour;
	unsigned short minute;
	unsigned short second;
};

struct apanic_config {
	unsigned int initialized;
	unsigned buf_size;
	unsigned int writesize;
	void *bounce; 
};

static struct apanic_config driver_context;


typedef unsigned int mmu_t;

struct pt_mmu {
	mmu_t cp15_sctlr;
	mmu_t cp15_ttb0;
	mmu_t cp15_ttb1;	
	mmu_t cp15_dacr;
	mmu_t cp15_prrr;
	mmu_t cp15_nmrr;
};

struct cores_info {
	struct pt_regs core_info[MAX_CORES];
	struct pt_mmu mmu_info[MAX_CORES];
};

struct fuel_gauge_info{
	int percent_soc;
	unsigned int cable_type;
};

/*
  SMEM - SMEM_ID_VENDOR2
*/
struct panic_info {
	unsigned int magic;
	unsigned int version;
	struct painc_info_date date;

	char mmu_cmm_script[MMU_SCRIPT_BUF_SIZE];
	int mmu_cmm_size;

	unsigned int console_offset;
	unsigned int console_length;

	unsigned int threads_offset;
	unsigned int threads_length;

	unsigned int logcat_offset;
	unsigned int logcat_length;
	unsigned int total_size;
};

/*
  PANTECH Shared memory region
*/
struct pantech_panic_info {
	/* log info */
	void *kernellog_buf_adr;
	void *mainlogcat_buf_adr;
	void *systemlogcat_buf_adr;

	/* Dump mode setting info */
	int in_panic;
	int ramdump;
	int usbdump;

	/* Force crash */
	int errortest;

	/* Fuel gauge info */
	struct fuel_gauge_info fuel_gauge;
};

/* Multi core info */
struct cores_info multi_core_info;
static atomic_t waiting_for_crash_ipi;



static struct pantech_panic_info *p_panic_info;

extern int log_buf_copy(char *dest, int idx, int len);
extern void log_buf_clear(void);
extern int logcat_buf_copy(char *dest, int len);
extern void logcat_set_log(int index);
extern unsigned char* logcat_get_addr(int index);
extern void ram_console_enable_console(int);
#define QUAD_YEAR (366+(3*365))

/*
 * The year_tab table is used for determining the number of days which
 * have elapsed since the start of each year of a leap year set. It has
 * 1 extra entry which is used when trying to find a 'bracketing' year.
 * The search is for a day >= year_tab[i] and day < year_tab[i+1].
 */
static const unsigned short year_tab[] = {
	0,                              /* Year 0 (leap year) */
	366,                            /* Year 1             */
	366+365,                        /* Year 2             */
	366+365+365,                    /* Year 3             */
	366+365+365+365                 /* Bracket year       */
};

/*
 * The norm_month_tab table holds the number of cumulative days that have
 * elapsed as of the end of each month during a non-leap year.
 */
static const unsigned short norm_month_tab[] = {
	0,                                    /* --- */
	31,                                   /* Jan */
	31+28,                                /* Feb */
	31+28+31,                             /* Mar */
	31+28+31+30,                          /* Apr */
	31+28+31+30+31,                       /* May */
	31+28+31+30+31+30,                    /* Jun */
	31+28+31+30+31+30+31,                 /* Jul */
	31+28+31+30+31+30+31+31,              /* Aug */
	31+28+31+30+31+30+31+31+30,           /* Sep */
	31+28+31+30+31+30+31+31+30+31,        /* Oct */
	31+28+31+30+31+30+31+31+30+31+30,     /* Nov */
	31+28+31+30+31+30+31+31+30+31+30+31   /* Dec */
};

/*
 * The leap_month_tab table holds the number of cumulative days that have
 * elapsed as of the end of each month during a leap year.
 */
static const unsigned short leap_month_tab[] = {
	0,                                    /* --- */
	31,                                   /* Jan */
	31+29,                                /* Feb */
	31+29+31,                             /* Mar */
	31+29+31+30,                          /* Apr */
	31+29+31+30+31,                       /* May */
	31+29+31+30+31+30,                    /* Jun */
	31+29+31+30+31+30+31,                 /* Jul */
	31+29+31+30+31+30+31+31,              /* Aug */
	31+29+31+30+31+30+31+31+30,           /* Sep */
	31+29+31+30+31+30+31+31+30+31,        /* Oct */
	31+29+31+30+31+30+31+31+30+31+30,     /* Nov */
	31+29+31+30+31+30+31+31+30+31+30+31   /* Dec */
};

/*
 * The day_offset table holds the number of days to offset as of the end of
 * each year.
 */
static const unsigned short day_offset[] = {
	1,                                    /* Year 0 (leap year) */
	1+2,                                  /* Year 1             */
	1+2+1,                                /* Year 2             */
	1+2+1+1                               /* Year 3             */
};

/*****************************************************
 * ERROR LOG TIME  ROUTINE
 * **************************************************/
static unsigned int tmr_GetLocalTime(void)
{
	struct timeval tv;
	unsigned int seconds;

	do_gettimeofday(&tv);
	seconds = (unsigned int)tv.tv_sec;

	/* Offset to sync timestamps between Arm9 & Arm11
	Number of seconds between Jan 1, 1970 & Jan 6, 1980 */
	seconds = seconds - (10UL*365+5+2)*24*60*60 ;
	return seconds;
}

static unsigned int div4x2( unsigned int dividend, unsigned short divisor, unsigned short *rem_ptr)
{
	*rem_ptr = (unsigned short) (dividend % divisor);

	return (dividend /divisor);
}

static void clk_secs_to_julian(unsigned int secs, struct painc_info_date *julian_ptr)
{
	int i;
	unsigned short days;

	/* 5 days (duration between Jan 1 and Jan 6), expressed as seconds. */
	secs += 432000;
	/* GMT to Local time */
	secs += 32400;

	secs = div4x2( secs, 60, &julian_ptr->second );
	secs = div4x2( secs, 60, &julian_ptr->minute );
	secs = div4x2( secs, 24, &julian_ptr->hour );
	julian_ptr->year = 1980 + ( 4 * div4x2( secs, QUAD_YEAR, &days));

	for(i = 0; days >= year_tab[i + 1]; i++);
	days -= year_tab[i];
	julian_ptr->year += i;

	if(i == 0)
	{
		for(i = 0; days >= leap_month_tab[i + 1]; i++ );
		julian_ptr->day = days - leap_month_tab[i] + 1;
		julian_ptr->month = i + 1;
	} else {
		for(i = 0; days >= norm_month_tab[i + 1]; i++ );
		julian_ptr->day = days - norm_month_tab[i] + 1;
		julian_ptr->month = i + 1;
	}
}

static void tmr_GetDate( unsigned int dseconds, struct painc_info_date *pDate)
{
	if(pDate) {
		if(dseconds == 0) {
			memset(pDate, 0x00, sizeof(struct painc_info_date));
			pDate->year = 1980;
			pDate->month = 1;
			pDate->day = 6;
		} else {
			clk_secs_to_julian(dseconds, (struct painc_info_date *)pDate);
		}
	}
}
/***********************************************************************
 * END LOG TIME ROUTINE *****************************************
 * **********************************************************************/

static void apanic_get_mmu_info(struct panic_info *info)
{
	int  bufsize = MMU_SCRIPT_BUF_SIZE, n = 0,total=0;
	char *s;

	unsigned int mmu_transbase0,mmu_transbase1;
	unsigned int mmu_dac,mmu_control;
	unsigned int mmu_prrr,mmu_nmrr;

	asm("mrc p15,0,%0,c1,c0,0" : "=r" (mmu_control));
	asm("mrc p15,0,%0,c2,c0,0" : "=r" (mmu_transbase0));
	asm("mrc p15,0,%0,c3,c0,0" : "=r" (mmu_dac));
	asm("mrc p15,0,%0,c2,c0,1" : "=r" (mmu_transbase1));
	asm("mrc p15,0,%0,c10,c2,0" : "=r" (mmu_prrr));
	asm("mrc p15,0,%0,c10,c2,1" : "=r" (mmu_nmrr));

	s =(char *)info->mmu_cmm_script;	

	WRITE_LOG("PER.S C15:0x1 %%LONG 0x%X\n",mmu_control);
	WRITE_LOG("PER.S C15:0x2 %%LONG 0x%X\n",mmu_transbase0);
	WRITE_LOG("PER.S C15:0x3 %%LONG 0x%X\n",mmu_dac);
	WRITE_LOG("PER.S C15:0x102 %%LONG 0x%X\n",mmu_transbase1);
	WRITE_LOG("PER.S C15:0x2A %%LONG 0x%X\n",mmu_prrr);
	WRITE_LOG("PER.S C15:0x12A %%LONG 0x%X\n",mmu_nmrr);
	WRITE_LOG("MMU.SCAN\n");
	WRITE_LOG("MMU.ON\n");
	WRITE_LOG("\n\n\n"); /* 32bit boundary */
	info->mmu_cmm_size = total;
}

/*
 * Writes the contents of the console to the specified offset in flash.
 * Returns number of bytes written
 */
static int apanic_write_console_log(unsigned int off)
{
	struct apanic_config *ctx = &driver_context;
	int saved_oip;
	int idx = 0;
	int rc;
	unsigned int last_chunk = 0;
	unsigned char *cur_bounce;

	cur_bounce = (unsigned char *)((unsigned int)ctx->bounce + off);

	while (!last_chunk) {
		saved_oip = oops_in_progress;
		oops_in_progress = 1;
		rc = log_buf_copy(cur_bounce+idx, idx, ctx->writesize);
		if (rc < 0)
			break;

		if (rc != ctx->writesize)
			last_chunk = rc;

		oops_in_progress = saved_oip;
		if (rc <= 0)
			break;

		if (rc != ctx->writesize)
			memset(cur_bounce + idx + rc, 0, ctx->writesize - rc);

		if (!last_chunk)
			idx += rc;
		else
			idx += last_chunk;
	}
	return idx;
}

static int apanic_write_logcat_log(unsigned int off)
{
	struct apanic_config *ctx = &driver_context;
	int saved_oip;
	int idx = 0;
	unsigned char *cur_bounce;

	cur_bounce = (unsigned char *)((unsigned int)ctx->bounce + off);

	saved_oip = oops_in_progress;
	oops_in_progress = 1;
	idx = logcat_buf_copy(cur_bounce, ctx->buf_size - off);
	oops_in_progress = saved_oip;

	return idx;
}

typedef enum
{
	ERR_RESET_NONE = 0x0,    /* Indicates flags are cleared */
	ERR_RESET_DETECT_ENABLED,/* Indicates reset flags are set */
	ERR_RESET_SPLASH_RESET,  /* APPSBL should use reset splash screen */
	ERR_RESET_SPLASH_DLOAD,  /* APPSBL should use dload splash screen */
	ERR_RESET_DETECT_ENABLED_SDCARD,/* Indicates reset flags are set */
	ERR_RESET_DETECT_ENABLED_USB,/* Indicates reset flags are set */
	ERR_RESET_CRASH_LOG_DLOAD, /* Crash log use log dload */
	ERR_RESET_CRASH_SDCARD_DLOAD, /* Crash log, ramdump use sdcard dload */
	ERR_RESET_CRASH_USB_DLOAD, /* Crash log, ramdump use usb dload*/
	ERR_RESET_CRASH_NO_DLOAD,
} err_reset_type;

typedef struct
{
	unsigned long magic_1;  /**< Most-significant word of the predefined magic value. */
	unsigned long magic_2;  /**< Least-significant word of the predefined magic value. */
} smem_hw_reset_id_type;

typedef smem_hw_reset_id_type err_reset_id_type;
static err_reset_id_type* err_reset_id_ptr = NULL;

/* Magic numbers for notifying OEMSBL that a reset has occurred */
#define HW_RESET_DETECT_ENABLED_MAGIC1     0x12345678
#define HW_RESET_DETECT_ENABLED_MAGIC2     0x9ABCDEF0

#define HW_RESET_DETECT_ENABLED_SDCARD_MAGIC1     0x13572468
#define HW_RESET_DETECT_ENABLED_SDCARD_MAGIC2     0x9ABCDEF0

#define HW_RESET_DETECT_ENABLED_USB_MAGIC1     0x24681357
#define HW_RESET_DETECT_ENABLED_USB_MAGIC2     0x9ABCDEF0

#define HW_RESET_ERR_LOG_DLOAD_MAGIC1     0xDEAFDEAF
#define HW_RESET_ERR_LOG_DLOAD_MAGIC2     0xEFBEEFBE

#define HW_RESET_ERR_SDCARD_DLOAD_MAGIC1     0xDAEDDAED
#define HW_RESET_ERR_SDCARD_DLOAD_MAGIC2     0xEFBEEFBE

#define HW_RESET_ERR_USB_DLOAD_MAGIC1     0xDEADDEAD
#define HW_RESET_ERR_USB_DLOAD_MAGIC2     0xEFBEEFBE

#define HW_RESET_ERR_NO_DLOAD_MAGIC1     0xCDFCCDFC
#define HW_RESET_ERR_NO_DLOAD_MAGIC2     0x06155160

void err_reset_detect_set(err_reset_type state)
{
	switch(state)
	{
	case ERR_RESET_NONE:
		err_reset_id_ptr->magic_1 = 0;
		err_reset_id_ptr->magic_2 = 0;
		break;
	case ERR_RESET_DETECT_ENABLED:
		err_reset_id_ptr->magic_1 = HW_RESET_DETECT_ENABLED_MAGIC1;
		err_reset_id_ptr->magic_2 = HW_RESET_DETECT_ENABLED_MAGIC2;
		break;
	case ERR_RESET_DETECT_ENABLED_SDCARD:
		err_reset_id_ptr->magic_1 = HW_RESET_DETECT_ENABLED_SDCARD_MAGIC1;
		err_reset_id_ptr->magic_2 = HW_RESET_DETECT_ENABLED_SDCARD_MAGIC2;
		break;
	case ERR_RESET_DETECT_ENABLED_USB:
		err_reset_id_ptr->magic_1 = HW_RESET_DETECT_ENABLED_USB_MAGIC1;
		err_reset_id_ptr->magic_2 = HW_RESET_DETECT_ENABLED_USB_MAGIC2;
		break;
	case ERR_RESET_CRASH_LOG_DLOAD:
		err_reset_id_ptr->magic_1 = HW_RESET_ERR_LOG_DLOAD_MAGIC1;
		err_reset_id_ptr->magic_2 = HW_RESET_ERR_LOG_DLOAD_MAGIC2;
		break;
	case ERR_RESET_CRASH_SDCARD_DLOAD:
		err_reset_id_ptr->magic_1 = HW_RESET_ERR_SDCARD_DLOAD_MAGIC1;
		err_reset_id_ptr->magic_2 = HW_RESET_ERR_SDCARD_DLOAD_MAGIC2;
		break;
	case ERR_RESET_CRASH_USB_DLOAD:
		err_reset_id_ptr->magic_1 = HW_RESET_ERR_USB_DLOAD_MAGIC1;
		err_reset_id_ptr->magic_2 = HW_RESET_ERR_USB_DLOAD_MAGIC2;
		break;
	case ERR_RESET_CRASH_NO_DLOAD :
		err_reset_id_ptr->magic_1 = HW_RESET_ERR_NO_DLOAD_MAGIC1;
		err_reset_id_ptr->magic_2 = HW_RESET_ERR_NO_DLOAD_MAGIC2;
		break;
	default:
		break;
	}
}

void err_reset_detect_init(void)
{
	unsigned smem_size = sizeof(smem_hw_reset_id_type);

	err_reset_id_ptr 
		= (err_reset_id_type *)smem_get_entry(SMEM_HW_RESET_DETECT, &smem_size);

	err_reset_detect_set(ERR_RESET_DETECT_ENABLED);

}

static void patech_get_mmu_info( int cpu )
{
	asm("mrc p15,0,%0,c1,c0,0" : "=r" (multi_core_info.mmu_info[cpu].cp15_sctlr));
	asm("mrc p15,0,%0,c2,c0,0" : "=r" (multi_core_info.mmu_info[cpu].cp15_ttb0));
	asm("mrc p15,0,%0,c3,c0,0" : "=r" (multi_core_info.mmu_info[cpu].cp15_dacr));
	asm("mrc p15,0,%0,c2,c0,1" : "=r" (multi_core_info.mmu_info[cpu].cp15_ttb1));
	asm("mrc p15,0,%0,c10,c2,0" : "=r" (multi_core_info.mmu_info[cpu].cp15_prrr));
	asm("mrc p15,0,%0,c10,c2,1" : "=r" (multi_core_info.mmu_info[cpu].cp15_nmrr));
}

static DEFINE_SPINLOCK(save_lock);

static void pantech_crash_save_cpu(struct pt_regs *regs, int cpu)
{
	if ((cpu < 0) || (cpu >= nr_cpu_ids))
		return;

	spin_lock(&save_lock);
	if(cpu == 0)
	{
		memcpy((char *)&multi_core_info.core_info[cpu],
				(char *)regs, sizeof(struct pt_regs));
		patech_get_mmu_info(cpu);
	}
	else if(cpu == 1)
	{
		memcpy((char *)&multi_core_info.core_info[cpu],
				(char *)regs, sizeof(struct pt_regs));
		patech_get_mmu_info(cpu);		
	}
	spin_unlock(&save_lock);	
}

#ifndef __ASSEMBLY__
static inline void pantech_crash_setup_regs(struct pt_regs *newregs)
{
	__asm__ __volatile__ (
		"stmia	%[regs_base], {r0-r12}\n\t"
		"mov	%[_ARM_sp], sp\n\t"
		"str	lr, %[_ARM_lr]\n\t"
		"adr	%[_ARM_pc], 1f\n\t"
		"mrs	%[_ARM_cpsr], cpsr\n\t"
	"1:"
		: [_ARM_pc] "=r" (newregs->ARM_pc),
		  [_ARM_cpsr] "=r" (newregs->ARM_cpsr),
		  [_ARM_sp] "=r" (newregs->ARM_sp),
		  [_ARM_lr] "=o" (newregs->ARM_lr)
		: [regs_base] "r" (&newregs->ARM_r0)
		: "memory"
	);
}
#endif

void pantech_machine_crash_nonpanic_core(void *unused)
{
	struct pt_regs regs;

	pantech_crash_setup_regs(&regs);
	pantech_crash_save_cpu(&regs, smp_processor_id());
	flush_cache_all();

	atomic_dec(&waiting_for_crash_ipi);
	while (1)
		cpu_relax();
}

static int *panic_magic; 
static int backup_reg[18]={0};

void pantech_machine_crash_shutdown(struct pt_regs *regs)
{
	unsigned long msecs;

	panic_magic=(int*)__get_regs_crashed();
	if( (*(--panic_magic) != 0x0) && (regs == NULL) )
	{
		return;
	}
	
	if(regs == NULL)
	{
		unsigned int cpsr=0;
		
		asm volatile("\
		stmia	%0, {r0-r12,sp,lr,pc}"	
		:
		: "r" (&backup_reg));

		__asm__ volatile("mrs	%0, cpsr" : "=r" (cpsr));

		backup_reg[16]=cpsr;
		regs = (struct pt_regs*) backup_reg;
	}
	else
	{
		sky_reset_reason=SYS_RESET_REASON_LINUX;
	}

	printk(KERN_DEBUG "Devices has crashed in pantech_machine_crash_shutdown\n");
	local_irq_disable();

	atomic_set(&waiting_for_crash_ipi, num_online_cpus() - 1);
	smp_call_function(pantech_machine_crash_nonpanic_core, NULL, false);
	msecs = 1000; /* Wait at most a second for the other cpus to stop */
	while ((atomic_read(&waiting_for_crash_ipi) > 0) && msecs) {
		mdelay(1);
		msecs--;
	}
	if (atomic_read(&waiting_for_crash_ipi) > 0)
		printk(KERN_WARNING "Non-crashing CPUs did not react to IPI\n");

	pantech_crash_save_cpu(regs, smp_processor_id());

	printk(KERN_DEBUG "CPU %u has crashed, num online cpus : %u\n", 
		smp_processor_id(), num_online_cpus());
}
EXPORT_SYMBOL(pantech_machine_crash_shutdown);

void pantech_force_dump_key(unsigned int code, int value)
{
	static unsigned int step = 0;

	if( (p_panic_info->usbdump == 0) || (value != 1) )
	{
		step = 0;
		return;
	}

	switch(step)
	{
		case 0:
			if(code == KEY_VOLUMEDOWN)
				step = 1;
			else 
				step = 0;
			break;
		case 1:
			if(code == KEY_VOLUMEUP)
				step = 2;
			else 
				step = 0;
			break;
		case 2:
			if(code == KEY_POWER)
				panic("linux- Force dump key");
			else
				step = 0;
			break;
	}
}
EXPORT_SYMBOL(pantech_force_dump_key);

void pantech_pm_log(int percent_soc, unsigned int cable_type)
{
	p_panic_info->fuel_gauge.percent_soc = percent_soc;
	p_panic_info->fuel_gauge.cable_type = cable_type;
}
EXPORT_SYMBOL(pantech_pm_log);

static int apanic_logging(struct notifier_block *this, unsigned long event,
                  void *ptr)
{
	struct apanic_config *ctx = &driver_context;
	struct panic_info *hdr = (struct panic_info *) ctx->bounce;
	int console_offset = 0;
	int console_len = 0;
	int threads_offset = 0;
	int threads_len = 0;
	int logcat_offset = 0;
	int logcat_len = 0;
	static char diag_err_msg_buf[0xC8];
	static char buf[1024];

	if(!ctx->initialized)
		return -1;

	if (p_panic_info->in_panic)
		return NOTIFY_DONE;

	p_panic_info->in_panic = 1;

	if(p_panic_info->usbdump == 1){
		err_reset_detect_set(ERR_RESET_CRASH_USB_DLOAD);
	}
	else if( p_panic_info->ramdump == 1){
		err_reset_detect_set(ERR_RESET_CRASH_SDCARD_DLOAD);
	}
	else{
		err_reset_detect_set(ERR_RESET_CRASH_LOG_DLOAD);
	}

	if(!strncmp(ptr, "lpass", 5)){
		sky_reset_reason=SYS_RESET_REASON_LPASS;
	}
	else if(!strncmp(ptr, "modem", 5)){
		sky_reset_reason=SYS_RESET_REASON_EXCEPTION;
	}
	else if(!strncmp(ptr, "dsps", 4)){
		sky_reset_reason=SYS_RESET_REASON_DSPS;
	}
	else if( (!strncmp(ptr, "riva", 4)) || (!strncmp(ptr, "wcnss", 5)) ){
		sky_reset_reason=SYS_RESET_REASON_RIVA;
	}
	else if(!strncmp(ptr, "linux", 5)){
		sky_reset_reason=SYS_RESET_REASON_LINUX;
	}

	#ifdef CONFIG_PREEMPT
	/* Ensure that cond_resched() won't try to preempt anybody */
	add_preempt_count(PREEMPT_ACTIVE);
	#endif

	touch_softlockup_watchdog();

	/* this case is kernel panic message send to modem err */
	if(ptr){
		strcpy(buf,ptr);
		smem_diag_get_message(diag_err_msg_buf, 0xc8);
		strcat(buf,"\nPC Crash at ");
		strcat(buf,diag_err_msg_buf);
		printcrash((const char *)buf);
	}
	console_offset = 4096;

	/*
	* Write out the console
	*/
	console_len = apanic_write_console_log(console_offset);
	if (console_len < 0) {
		printk(KERN_EMERG "Error writing console to panic log! (%d)\n",
			console_len);
		console_len = 0;
	}

	/*
	* Write out all threads
	*/
	threads_offset = ALIGN(console_offset + console_len,ctx->writesize);
	if (!threads_offset)
		threads_offset = ctx->writesize;
	#ifdef CONFIG_THREAD_STATE_INFO
	/*
	logging time issue!!
	do use T32 simulator for thread info about it's schedule.
	*/
	ram_console_enable_console(0);

	log_buf_clear();
	show_state_filter(0);

	threads_len = apanic_write_console_log(threads_offset);
	if (threads_len < 0) {
		printk(KERN_EMERG "Error writing threads to panic log! (%d)\n",
			threads_len);
		threads_len = 0;
	}
	#endif
	logcat_offset = ALIGN(threads_offset + threads_len,ctx->writesize);
	if(!logcat_offset)
		logcat_offset = ctx->writesize;

	/* 1.log_main  2.log_event  3.log_radio  4.log_system */
	logcat_set_log(4); /* logcat log system */

	logcat_len = apanic_write_logcat_log(logcat_offset);
	if (logcat_len < 0) {
		printk(KERN_EMERG "Error writing logcat to panic log! (%d)\n",
			logcat_len);
		logcat_len = 0;
	}

	/*
	* Finally write the panic header
	*/
	memset(ctx->bounce, 0, PAGE_SIZE);
	hdr->magic = PANIC_MAGIC;
	hdr->version = PHDR_VERSION;
	tmr_GetDate( tmr_GetLocalTime(), &hdr->date );
	printk("===time is %4d %02d %02d %02d %02d %02d ===\n",
			hdr->date.year, hdr->date.month, hdr->date.day,
			hdr->date.hour, hdr->date.minute, hdr->date.second);
	
	apanic_get_mmu_info(hdr);
	
	hdr->console_offset    = console_offset;
	hdr->console_length  = console_len;
	
	hdr->threads_offset = threads_offset;
	hdr->threads_length = threads_len;
	
	hdr->logcat_offset   = logcat_offset;
	hdr->logcat_length  = logcat_len;
	
	hdr->total_size = logcat_offset + logcat_len;
	
	printk(KERN_EMERG 
		"pantech_apanic: Panic dump sucessfully written to smem\n");
	
	flush_cache_all();
	
	#ifdef CONFIG_PREEMPT
	sub_preempt_count(PREEMPT_ACTIVE);
	#endif
	p_panic_info->in_panic = 0;
	return NOTIFY_DONE;
}

static struct notifier_block panic_blk = {
	.notifier_call    = apanic_logging,
};

void set_err_reset_detect_mode( void )
{
	if(p_panic_info->usbdump == 1){
		err_reset_detect_set(ERR_RESET_DETECT_ENABLED_USB);
	}
	else if( p_panic_info->ramdump == 1){
		err_reset_detect_set(ERR_RESET_DETECT_ENABLED_SDCARD);
	}
	else{
		err_reset_detect_set(ERR_RESET_DETECT_ENABLED);
	}
}

typedef enum {
	LINUX_ERR_FATAL=20,
	LINUX_WATCHDOG=21,
	LINUX_EXCP_SWI=22,
	LINUX_EXCP_UNDEF=23,
	LINUX_EXCP_MIS_ALIGN=24,  
	LINUX_EXCP_PAGE_FAULT=25,
	LINUX_EXCP_EXE_FAULT=26,
	LINUX_EXCP_DIV_BY_Z=27,
	SUBSYSTEM_ERR_MAX_NUM
}subsystem_err_type;

#define WDT0_RST	(MSM_TMR0_BASE + 0x38)
typedef void (*func_ptr)(void);
static DEFINE_SPINLOCK(state_lock);

static void linux_crash_test(int select)
{

	static int div0_y=0;
	static const int _undef_inst = 0xFF0000FF;
	unsigned long irqflags;
	unsigned long  value = 0;
	int x = 1234567;
	int *y;
	func_ptr func;

	switch(select)
	{
		case LINUX_ERR_FATAL:
			BUG_ON(1);
			break;
		case LINUX_WATCHDOG:
			writel(1, WDT0_RST);
			spin_lock_irqsave(&state_lock, irqflags);
			while(1){
				value = value ? 0 : 1;
			}
			spin_unlock_irqrestore(&state_lock, irqflags);
			break;
		case LINUX_EXCP_SWI:
			break;
		case LINUX_EXCP_UNDEF:
			func = (func_ptr)(&_undef_inst);
			(*func)();
			break;
		case LINUX_EXCP_MIS_ALIGN:
			break;			
		case LINUX_EXCP_PAGE_FAULT:
			*(int *)0 = 0;
			break;
		case LINUX_EXCP_EXE_FAULT:
			func = (func_ptr)0xDEADDEAD;
			(*func)();
			break;
		case LINUX_EXCP_DIV_BY_Z:
			y = &div0_y;
			x = x / *y;
			break;
		default:
			break;
	}
}

static ssize_t ramdump_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%d\n", p_panic_info->ramdump);

}

static ssize_t ramdump_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	sscanf(buf, "%du", &p_panic_info->ramdump);
	set_err_reset_detect_mode();

	return count;
}

static struct kobj_attribute ramdump_attribute =
	__ATTR(ramdump, 0644, ramdump_show, ramdump_store);



static ssize_t usbdump_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%d\n", p_panic_info->usbdump);
}

static ssize_t usbdump_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	sscanf(buf, "%du", &p_panic_info->usbdump);
	set_err_reset_detect_mode();

	return count;
}

static struct kobj_attribute usbdump_attribute =
	__ATTR(usbdump, 0644, usbdump_show, usbdump_store);



static ssize_t errortest_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%d\n", p_panic_info->errortest);
}

static ssize_t errortest_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	sscanf(buf, "%du", &p_panic_info->errortest);
	linux_crash_test(p_panic_info->errortest);

	return count;
}

static struct kobj_attribute errortest_attribute =
			__ATTR(errortest, 0600, errortest_show, errortest_store);



/*
 * Create a group of attributes so that we can create and destroy them all
 * at once.
 */
static struct attribute *attrs[] = {
	&ramdump_attribute.attr,
	&usbdump_attribute.attr,
	&errortest_attribute.attr,	
	NULL,	/* need to NULL terminate the list of attributes */
};

/*
 * An unnamed attribute group will put all of the attributes directly in
 * the kobject directory.  If we specify a name, a subdirectory will be
 * created for the attributes with the directory being the name of the
 * attribute group.
 */
static struct attribute_group attr_group = {
	.attrs = attrs,
};

static struct kobject *pantech_apanic_kobj;

int __init apanic_pantech_init_sysfs(void)
{
	int retval;

	/*
	 * Create a simple kobject with the name of "kobject_example",
	 * located under /sys/kernel/
	 *
	 * As this is a simple directory, no uevent will be sent to
	 * userspace.  That is why this function should not be used for
	 * any type of dynamic kobjects, where the name and number are
	 * not known ahead of time.
	 */
	pantech_apanic_kobj = kobject_create_and_add("pantech_apanic", kernel_kobj);
	if (!pantech_apanic_kobj)
		return -ENOMEM;

	/* Create the files associated with this kobject */
	retval = sysfs_create_group(pantech_apanic_kobj, &attr_group);
	if (retval)
		kobject_put(pantech_apanic_kobj);

	return retval;
}

int __init apanic_pantech_init(void)
{
	unsigned size = MAX_CRASH_BUF_SIZE;
	unsigned char *crash_buf;

	/*
		using smem_get_entry function if already memory have allocated. 
		if not AMSS err_init function must be called alloc function for ID.
	*/
	memset(&driver_context,0,sizeof(struct apanic_config));
	
	crash_buf = (unsigned char *)smem_get_entry(SMEM_ID_VENDOR2, &size);
	if(!crash_buf){
		printk(KERN_ERR "pantech_apanic: no available crash buffer , initial failed\n");
		return 0;
	} else {
		atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);
		driver_context.buf_size = size;
		driver_context.writesize = 4096;
		driver_context.bounce    = (void *)crash_buf;
		driver_context.initialized = 1;
	}

	err_reset_detect_init();

	p_panic_info = ioremap_nocache(PANTECH_SHARED_RAM_BASE, SZ_4K);
	
	p_panic_info->in_panic = 0;
	p_panic_info->usbdump = 0;
	p_panic_info->ramdump = 0;
	__raw_writel(virt_to_phys(get_log_buf_addr()), &p_panic_info->kernellog_buf_adr);
	__raw_writel(virt_to_phys(logcat_get_addr(1)), &p_panic_info->mainlogcat_buf_adr);
	__raw_writel(virt_to_phys(logcat_get_addr(4)), &p_panic_info->systemlogcat_buf_adr);

	printk(KERN_INFO "Android kernel / Modem panic handler initialized \n");
	return 0;
}

module_init(apanic_pantech_init);
module_init(apanic_pantech_init_sysfs);


MODULE_AUTHOR("JungSik Jeong <chero@pantech.com>");
MODULE_DESCRIPTION("PANTECH Error Crash misc driver");
MODULE_LICENSE("GPL");
