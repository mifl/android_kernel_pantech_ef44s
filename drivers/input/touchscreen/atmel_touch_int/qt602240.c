/* file_name: qt602240.c
 *
 * description: Quantum TSP driver.
 *
 * Copyright (C) 2008-2010 Atmel & Pantech Co. Ltd.
 *
 */
 
#include <linux/module.h>
#include <linux/input.h>
#include <linux/input/mt.h>  //PROTOCOL_B 
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/earlysuspend.h>
#include <asm/io.h>
#include <linux/gpio.h>
#include <mach/vreg.h>
#include <linux/regulator/consumer.h>
#include <mach/gpio.h>
#include <linux/miscdevice.h>
#include <linux/hrtimer.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h> //TOUCH_REACTION_RATE
//#include "../../../../../pantech/include/BOARD_REV.h" // mirinae_20110920
#include "qt602240.h"
#ifdef ITO_TYPE_CHECK		//P13106 
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#endif

/* -------------------------------------------------------------------- */
/* debug option */
/* -------------------------------------------------------------------- */

static int DebugON=0, Debug_CR=0;
#define dbg(fmt, args...) if(DebugON)printk("[QT602240]" fmt, ##args)
#define dbg_CR(fmt, args...) if(Debug_CR)printk("[QT602240]" fmt, ##args)
#define dbg_func_in()		dbg("[+] %s\n", __func__)
#define dbg_func_out()		dbg("[-] %s\n", __func__)
#define dbg_line()		dbg("line : %d | func : %s\n", __LINE__, __func__)

/* -------------------------------------------------------------------- */
#define IRQ_TOUCH_INT		gpio_to_irq(GPIO_TOUCH_CHG)
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* qt602240 i2c slave address */
/* -------------------------------------------------------------------- */
#define QT602240_I2C_ADDR       0x4A
#define QT602240_I2C_BOOT_ADDR  0x24
/* -------------------------------------------------------------------- */

#ifdef QT_FIRMUP_ENABLE
/* -------------------------------------------------------------------- */
/* qt602240 chipset firmware data to update */
/* -------------------------------------------------------------------- */
unsigned char QT602240_E_firmware[] = {
#include "mXT224e__APP_v1_0_AA.h"
};

void	QT_reprogram(void);
uint8_t	QT_Boot(bool withReset);
/* -------------------------------------------------------------------- */
#endif // QT_FIRMUP_ENABLE

/* -------------------------------------------------------------------- */
/* function proto type & variable for driver							*/
/* -------------------------------------------------------------------- */
static int __devinit qt602240_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int __devexit qt602240_remove(struct i2c_client *client);
#if defined(CONFIG_PM) && defined(CONFIG_HAS_EARLYSUSPEND)
static int qt602240_late_resume(struct early_suspend *h);
static int qt602240_early_suspend(struct early_suspend *h);
#endif //CONFIG_PM && CONFIG_HAS_EARLYSUSPEND

static struct class *touch_atmel_class;
struct device *ts_dev;
static void report_input (int touch_status);

static const struct i2c_device_id qt602240_id[] = {
	{ "qt602240-i2c", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, qt602240_id);

static struct i2c_driver qt602240_driver = {
	.driver = {
		.name	= "qt602240-i2c",
		.owner	= THIS_MODULE,
	},
	.probe		= qt602240_probe,
	.remove		= __devexit_p(qt602240_remove),
	.id_table	= qt602240_id,
};

struct workqueue_struct *qt602240_wq;
struct qt602240_data_t
{
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct work;
#ifdef PROTECTION_MODE
	struct work_struct work_protection_mode_disable;
	struct work_struct work_tchautocal_disable;
#endif
	struct early_suspend es;
	struct mutex    lock;
};
struct qt602240_data_t *qt602240_data = NULL;
/* -------------------------------------------------------------------- */
unsigned char not_yet_count = 0;
static uint8_t cal_correction_limit = 0;
static bool f_resume=false;
static bool f_resume_cal=false;

/* -------------------------------------------------------------------- */
/* function proto type & variable for attribute				*/
/* -------------------------------------------------------------------- */
/* for attribute */
static ssize_t i2c_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t i2c_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t size);
static ssize_t gpio_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t gpio_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t size);
static ssize_t setup_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t setup_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t size);
static DEVICE_ATTR(gpio, S_IRUGO | S_IWUSR, gpio_show, gpio_store);
static DEVICE_ATTR(i2c, S_IRUGO | S_IWUSR, i2c_show, i2c_store);
static DEVICE_ATTR(setup, S_IRUGO | S_IWUSR, setup_show, setup_store);
/* -------------------------------------------------------------------- */


typedef struct
{
	info_id_t *info_id;          /*! Info ID struct. */
	object_t *objects;          /*! Pointer to an array of objects. */
	uint16_t CRC;               /*! CRC field, low bytes. */
	uint8_t CRC_hi;             /*! CRC field, high byte. */
} info_block_t;


typedef struct
{
	uint8_t object_type;     /*!< Object type. */
	uint8_t instance;        /*!< Instance number. */
} report_id_map_t;


typedef struct
{
	uint8_t id;                     /*!< id */
	int8_t status;          /*!< dn=1, up=0, none=-1 */
	int16_t x;                      /*!< X */
	int16_t y;                      /*!< Y */
	int8_t mode;
	int area;
} report_finger_info_t;

typedef struct
{
	uint16_t code;                      /*!< key code */
	uint8_t status;                      /*!< key status (press/release) */
	bool update;     
} report_key_info_t;

typedef struct 
{
	uint8_t tch_ch; /*number of touch channel*/
	uint8_t atch_ch; /*number of anti-touch channel*/	
	uint8_t calibration_cnt; /*number of anti-touch channel*/
	uint8_t autocal_flag; /*calibration flag 0 => normal, over 1 => auto calibration is enabled */
} debug_info_t;
static debug_info_t debugInfo;  //store tch_ch and atch_ch values
static report_finger_info_t fingerInfo[MAX_NUM_FINGER];
static report_key_info_t keyInfo[MAX_NUM_FINGER];
static info_block_t *info_block = NULL;
static report_id_map_t *report_id_map = NULL;
volatile uint8_t read_pending;
static int max_report_id = 0;
static uint8_t max_message_length;
static uint16_t message_processor_address;

/*! Command processor address. */
static uint16_t command_processor_address;

/*! Flag indicating if driver setup is OK. */
static enum driver_setup_t driver_setup = DRIVER_SETUP_INCOMPLETE;

/*! Message buffer holding the latest message received. */
static uint8_t *quantum_msg = NULL;

/*! \brief The current address pointer. */
static U16 address_pointer;

/* flag to indicate if last calibration has been confirmed good */
static uint8_t cal_check_flag = 0u;

#ifdef CHIP_NOINIT
static bool Chip_NoInit = false;
#endif

/* Calibration checking routine calling code - in interrupt handler */

static unsigned int mxt_time_point;
static unsigned int good_calibration_cnt;

#ifdef PROTECTION_MODE
static struct timer_list protection_mode_timer;
static struct timer_list tchautocal_timer;
bool protection_mode = false;

void disable_autocal_locked(struct work_struct * p);
void qt_Acquisition_Config_Init_locked(struct work_struct * p);

static int autocal_safecheck_flag = 0;
static int autocal_enable_time[2] = {6000, 3000};

#ifdef PROTECTION_MODE_SAFECHECK
static int autocal_safecheck(void);
#endif
#endif


static bool active_event = true;
static bool is_cal_success = false;
static bool touch_diagnostic_ret = true;
#ifndef min
#define min(a,b) ( a > b ? b : a)
#endif
#ifndef max
#define max(a,b) ( b > a ? b : a )
#endif

typedef enum
{
	TSC_EVENT_NONE,
	TSC_EVENT_WINDOW,	
} tsc_key_mode_type;

typedef enum
{
	TSC_CLEAR_ALL,
	TSC_CLEAR_EVENT,
} tsc_clear_type;

typedef enum
{
	TOUCH_EVENT_RELEASE = 0,
	TOUCH_EVENT_PRESS,
	TOUCH_EVENT_MOVE
} TOUCH_EVENT;

#ifdef ITO_TYPE_CHECK	
typedef enum
{
	TOUCH_WINDOW_WHITE = 0,
	TOUCH_WINDOW_BLACK
} TOUCH_WINDOW;

#endif
#ifdef TOUCH_IO
typedef enum 
{
	//p13106 add touch_app
	APPLY_TOUCH_CONFIG = 501,
	DIAG_DEBUG = 502,
	RESET_TOUCH_CONFIG = 503,
	GET_TOUCH_CONFIG = 504,
	SET_TOUCH_CONFIG = 505,
	//READ_ITO_TYPE = 506,
	TOUCH_IOCTL_DEBUG = 507,
	TOUCH_CHARGER_MODE = 701
}CONFIG_CMD;

typedef enum
{
	BATTERY_PLUGGED_NONE = 0,
	BATTERY_PLUGGED_CHARGER = 1	
}type_charger_mode;

unsigned int tsp_power_status=1;
unsigned long charger_mode;
unsigned long pre_charger_mode;
#endif //TOUCH_IO

static int diagnostic_min =0;
static int diagnostic_max =0;

/*------------------------------ functions prototype -----------------------------------*/
uint8_t init_touch_driver(void);
uint8_t reset_chip(void);
uint8_t calibrate_chip(void);
uint8_t diagnostic_chip(uint8_t mode);
uint8_t backup_config(void);
uint8_t write_power_config(gen_powerconfig_t7_config_t power_config);
uint8_t write_acquisition_config(gen_acquisitionconfig_t8_config_t acq_config);
uint8_t write_multitouchscreen_config(uint8_t screen_number, touch_multitouchscreen_t9_config_t cfg);
uint8_t write_keyarray_config(uint8_t key_array_number, touch_keyarray_t15_config_t cfg);
uint8_t write_comc_config(uint8_t instance, spt_comcconfig_t18_config_t cfg);
uint8_t write_gpio_config(uint8_t instance, spt_gpiopwm_t19_config_t cfg);
uint8_t write_proximity_config(uint8_t instance, touch_proximity_t23_config_t cfg);
uint8_t write_selftest_config(uint8_t instance, spt_selftest_t25_config_t cfg);
uint8_t write_simple_config(uint8_t object_type, uint8_t instance, void *cfg);
uint8_t get_object_size(uint8_t object_type);
uint8_t type_to_report_id(uint8_t object_type, uint8_t instance);
uint8_t report_id_to_type(uint8_t report_id, uint8_t *instance);
uint8_t read_id_block(info_id_t *id);
uint16_t get_object_address(uint8_t object_type, uint8_t instance);
uint32_t get_stored_infoblock_crc(void);
uint8_t calculate_infoblock_crc(uint32_t *crc_pointer);
uint32_t CRC_24(uint32_t crc, uint8_t byte1, uint8_t byte2);
void write_message_to_usart(uint8_t msg[], uint8_t length);
int init_hw_setting(void);							// in source of qt602240_cfg_project.h 
void TSP_Restart(void);								
int8_t check_chip_calibration(void);
int8_t get_touch_antitouch_info(void);
void quantum_touch_probe(void);
uint8_t write_gripsuppression_T40_config(proci_gripsuppression_t40_config_t cfg);
uint8_t write_touchsuppression_t42_config(proci_touchsuppression_t42_config_t cfg);
uint8_t write_CTE_T46_config(spt_cteconfig_t46_config_t cfg);
uint8_t  write_stylus_t47_config(proci_stylus_t47_config_t cfg);
uint8_t  write_noisesuppression_t48_config(procg_noisesuppression_t48_config_t cfg);
U8 read_mem(U16 start, U8 size, U8 *mem);
U8 read_U16(U16 start, U16 *mem);
U8 write_mem(U16 start, U8 size, U8 *mem);
void  clear_event(uint8_t clear);
static int* diag_debug(int command); 

#ifdef SKY_PROCESS_CMD_KEY
static long ts_fops_ioctl(struct file *filp,unsigned int cmd, unsigned long arg);
static int ts_fops_open(struct inode *inode, struct file *filp);
#endif

#ifdef TOUCH_IO
/* File IO */
static int open(struct inode *inode, struct file *file);
static int release(struct inode *inode, struct file *file);
static ssize_t read(struct file *file, char *buf, size_t count, loff_t *ppos);
static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *ppos);
static long ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int ioctl_debug(unsigned long arg);
static void apply_touch_config(void); 
static void reset_touch_config(void);
static uint8_t qt_charger_mode_config(unsigned long mode);
#endif // TOUCH_IO

#ifdef ITO_TYPE_CHECK
static int read_touch_id(void);
static int tsp_ito_type = -1;
#endif

void  qt602240_front_test_init(void);
void off_hw_setting(void);

/*------------------------------ for tunning ATmel - start ----------------------------*/
struct class *touch_class;
EXPORT_SYMBOL(touch_class);

struct device *switch_test;
EXPORT_SYMBOL(switch_test);

/*------------------------------ for tunning ATmel - end ----------------------------*/
#ifdef SKY_PROCESS_CMD_KEY
static struct file_operations ts_fops = {
	.owner = THIS_MODULE,
	.open = ts_fops_open,
	.unlocked_ioctl = ts_fops_ioctl, // mirinae
};

static struct miscdevice touch_event = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "touch_fops",
	.fops = &ts_fops,
};

static int ts_fops_open(struct inode *inode, struct file *filp)
{
	filp->private_data = qt602240_data;
	return 0;
}

typedef enum {	
	TOUCH_IOCTL_READ_LASTKEY=1001,	
	TOUCH_IOCTL_DO_KEY,	
	TOUCH_IOCTL_RELEASE_KEY, 
	TOUCH_IOCTL_CLEAN,
	TOUCH_IOCTL_DEBUG_,
	TOUCH_IOCTL_RESTART,
	TOUCH_IOCTL_PRESS_TOUCH,
	TOUCH_IOCTL_RELEASE_TOUCH,
	TOUCH_IOCTL_CHARGER_MODE,
	TOUCH_IOCTL_EARJACK_MODE,
	POWER_OFF,
	TOUCH_IOCTL_DELETE_ACTAREA = 2001,
	TOUCH_IOCTL_RECOVERY_ACTAREA,
	TOUCH_IOCTL_SENSOR_X = 2005,
	TOUCH_IOCTL_SENSOR_Y,
	TOUCH_IOCTL_CHECK_BASE,
	TOUCH_IOCTL_READ_IC_VERSION,
	TOUCH_IOCTL_READ_FW_VERSION,
	TOUCH_IOCTL_START_UPDATE,
	TOUCH_IOCTL_SELF_TEST,
	TOUCH_IOCTL_DIAGNOSTIC_MIN_DEBUG,
	TOUCH_IOCTL_DIAGNOSTIC_MAX_DEBUG,
	TOUCH_IOCTL_DIAG_DELTA = 2014,
	TOUCH_IOCTL_INIT = 3001,	
	TOUCH_IOCTL_OFF  = 3002,
	TOUCH_IOCTL_EVENT_TIME_GET  = 4001, //TOUCH_REACTION_RATE
	TOUCH_IOCTL_EVENT_AUTOCAL_DISABLE  = 5001,
} TOUCH_IOCTL_CMD;

#ifdef	QT602240_EARJACK_MODE
#if defined (CONFIG_MACH_MSM8960_EF45K) ||defined (CONFIG_MACH_MSM8960_EF46L) || defined (CONFIG_MACH_MSM8960_EF47S) ||	defined (CONFIG_MACH_MSM8960_STARQ)
#define EARJACK_TCHTHR	50
#elif defined (CONFIG_MACH_MSM8960_EF44S)
#define EARJACK_TCHTHR	55
#else
#define EARJACK_TCHTHR	45
#endif
#define EARJACK_TCHHYST	(EARJACK_TCHTHR/4)

void qt602240_touch_earjack(int flag)
{
	uint16_t object_address_t9 = 0;

	if(flag == true)	// earjack insert
	{
		if((touchscreen_config.tchthr == EARJACK_TCHTHR)
				&&(touchscreen_config.tchhyst == EARJACK_TCHHYST))

			return;

		touchscreen_config.tchthr = EARJACK_TCHTHR;	
		touchscreen_config.tchhyst = EARJACK_TCHHYST;
	}
	else		//earjack release
	{
#ifdef ITO_TYPE_CHECK	
		if((touchscreen_config.tchthr == T9_TCHTHR[tsp_ito_type]) 
				&& (touchscreen_config.tchhyst == T9_TCHHYST[tsp_ito_type]))
#else
			if((touchscreen_config.tchthr == T9_TCHTHR) ||(touchscreen_config.tchhyst == T9_TCHHYST))
#endif
				return;

#ifdef ITO_TYPE_CHECK
		touchscreen_config.tchthr = T9_TCHTHR[tsp_ito_type];
		touchscreen_config.tchhyst = T9_TCHHYST[tsp_ito_type];
#else
		touchscreen_config.tchthr = T9_TCHTHR;
		touchscreen_config.tchhyst = T9_TCHHYST;
#endif	

	}

	object_address_t9 = get_object_address(TOUCH_MULTITOUCHSCREEN_T9, 0);
	dbg("object_address_t9 = %d\n",object_address_t9);
	if (object_address_t9 == 0)
		return;	

	mutex_lock(&qt602240_data->lock);
	write_mem(object_address_t9+7, 1, &touchscreen_config.tchthr);
	write_mem(object_address_t9+31, 1, &touchscreen_config.tchhyst);
	mutex_unlock(&qt602240_data->lock);
	return;
}
#endif

static struct timespec ts; //TOUCH_REACTION_RATE

static long ts_fops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	long event_time; //TOUCH_REACTION_RATE

	dbg_CR("[QT602240]ts_fops_ioctl(%d, %d) \n",(int)cmd,(int)arg);

	switch (cmd) 
	{
		case TOUCH_IOCTL_READ_LASTKEY:
			break;
		case TOUCH_IOCTL_DO_KEY:
			dbg("TOUCH_IOCTL_DO_KEY  = %d\n",(int)argp);			
			if ( (int)argp == KEY_NUMERIC_STAR )
				input_report_key(qt602240_data->input_dev, 0xe3, 1);
			else if ( (int)argp == KEY_NUMERIC_POUND )
				input_report_key(qt602240_data->input_dev, 0xe4, 1);
			else
				input_report_key(qt602240_data->input_dev, (int)argp, 1);
			input_sync(qt602240_data->input_dev); 
			break;
		case TOUCH_IOCTL_RELEASE_KEY:		
			dbg("TOUCH_IOCTL_RELEASE_KEY  = %d\n",(int)argp);
			if ( (int)argp == 0x20a )
				input_report_key(qt602240_data->input_dev, 0xe3, 0);
			else if ( (int)argp == 0x20b )
				input_report_key(qt602240_data->input_dev, 0xe4, 0);
			else
				input_report_key(qt602240_data->input_dev, (int)argp, 0);
			input_sync(qt602240_data->input_dev); 
			break;		
		case TOUCH_IOCTL_DEBUG_:
			dbg("Touch Screen Read Queue ~!!\n");	
			queue_work(qt602240_wq, &qt602240_data->work);
			break;
		case TOUCH_IOCTL_CLEAN:
			dbg("Touch Screen Previous Data Clean ~!!\n");
			clear_event(TSC_CLEAR_ALL);
			break;
		case TOUCH_IOCTL_RESTART:
			dbg("Touch Screen Calibration Restart ~!!\n");			
			calibrate_chip();
			break;
		case TOUCH_IOCTL_PRESS_TOUCH:
			input_report_abs(qt602240_data->input_dev, ABS_MT_POSITION_X, (int)(arg&0x0000FFFF));
			input_report_abs(qt602240_data->input_dev, ABS_MT_POSITION_Y, (int)((arg >> 16) & 0x0000FFFF));
			input_report_abs(qt602240_data->input_dev, ABS_MT_TOUCH_MAJOR, 255);
			input_report_abs(qt602240_data->input_dev, ABS_MT_WIDTH_MAJOR, 1);			
			input_sync(qt602240_data->input_dev);
			break;
		case TOUCH_IOCTL_RELEASE_TOUCH:		
			input_report_abs(qt602240_data->input_dev, ABS_MT_POSITION_X, (int)(arg&0x0000FFFF));
			input_report_abs(qt602240_data->input_dev, ABS_MT_POSITION_Y, (int)((arg >> 16) & 0x0000FFFF));
			input_report_abs(qt602240_data->input_dev, ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(qt602240_data->input_dev, ABS_MT_WIDTH_MAJOR, 1);			
			input_sync(qt602240_data->input_dev); 
			break;			
		case TOUCH_IOCTL_CHARGER_MODE:
			qt_charger_mode_config(arg);
			break;
#ifdef	QT602240_EARJACK_MODE
		case TOUCH_IOCTL_EARJACK_MODE:			
#if defined (CONFIG_MACH_MSM8960_EF45K) || defined (CONFIG_MACH_MSM8960_EF46L) || defined (CONFIG_MACH_MSM8960_EF47S) || \
	defined (CONFIG_MACH_MSM8960_EF44S) || defined (CONFIG_MACH_MSM8960_STARQ)
			dbg("[qt602240] Touch earjack mode change..%lu \n", arg);
			qt602240_touch_earjack(arg);
#endif
			break;            
#endif            
		case POWER_OFF:
			pm_power_off();
			break;
		case TOUCH_IOCTL_DELETE_ACTAREA:
			touchscreen_config.yloclip = 0;		// Change Active area
			touchscreen_config.yhiclip = 0;
			if (write_multitouchscreen_config(0, touchscreen_config) != CFG_WRITE_OK)
			{
				dbg("qt_Multitouchscreen_config Error!!!\n");
			}
			break;
		case TOUCH_IOCTL_RECOVERY_ACTAREA:
			touchscreen_config.yloclip = 15;	// Change Active area
			touchscreen_config.yhiclip = 15;
			if (write_multitouchscreen_config(0, touchscreen_config) != CFG_WRITE_OK)
				dbg("qt_Multitouchscreen_config Error!!!\n");

			break;
		case TOUCH_IOCTL_INIT:
			printk("Touch init \n");
			qt602240_front_test_init();
			break;

		case TOUCH_IOCTL_OFF:
			printk("Touch off \n");
			off_hw_setting();
			break;			

		case TOUCH_IOCTL_EVENT_TIME_GET:  //TOUCH_REACTION_RATE
			event_time = (ts.tv_sec * USEC_PER_MSEC) + (ts.tv_nsec / NSEC_PER_MSEC);	 
			if (copy_to_user(argp, &event_time, sizeof(event_time))){
				printk("Copy error!!  \n");
				//return -EFAULT;
			}
			dbg("Report Kernel event time: %ld \n",event_time);
			break;

		case TOUCH_IOCTL_SENSOR_X:
			{
				int send_data;
				send_data = touchscreen_config.xsize;

				if (copy_to_user(argp, &send_data, sizeof(send_data)))
					return false;
			}
			break;
		case TOUCH_IOCTL_SENSOR_Y:
			{
				int send_data;
				send_data = touchscreen_config.ysize;
				if (copy_to_user(argp, &send_data, sizeof(send_data)))
					return false;
			}
			break;

		case TOUCH_IOCTL_CHECK_BASE:
		case TOUCH_IOCTL_START_UPDATE:
			break;

		case TOUCH_IOCTL_SELF_TEST:
			{
				int* send_byte;
				send_byte = diag_debug(QT602240_REFERENCE_MODE);
				diagnostic_min = QT602240_REFERENCE_MIN;
				diagnostic_max = QT602240_REFERENCE_MAX;

				if (copy_to_user(argp, send_byte, sizeof(int) * QT602240_MAX_CHANNEL_NUM))
					return false;

				return touch_diagnostic_ret;
				break;
			}
		case TOUCH_IOCTL_DIAGNOSTIC_MIN_DEBUG:
			return diagnostic_min;
			break;
		case TOUCH_IOCTL_DIAGNOSTIC_MAX_DEBUG:
			return diagnostic_max;
			break;
		case TOUCH_IOCTL_DIAG_DELTA:
			{
				int* send_byte;
				send_byte = diag_debug(QT602240_DELTA_MODE);

				if (copy_to_user(argp, send_byte, sizeof(int) * QT602240_MAX_CHANNEL_NUM))
					return false;

				return touch_diagnostic_ret;
				break;
			}
		case TOUCH_IOCTL_EVENT_AUTOCAL_DISABLE:

			if ( acquisition_config.tchautocal == T8_TCHAUTOCAL_PROTECTION) {

				dbg_CR("[TOUCH] Camera In - Autocal & Finger & Palm recovery disable\n");

				cancel_work_sync(&qt602240_data->work_protection_mode_disable);	// If calibration is performed right after protection_mode_timer enable, 
				del_timer(&protection_mode_timer);								// Finger & Palm recal config will be disable
				cancel_work_sync(&qt602240_data->work_tchautocal_disable);
				del_timer(&tchautocal_timer);

				protection_mode = false;
				f_resume_cal = false;
				cal_correction_limit = 0;
				debugInfo.calibration_cnt=1;
				debugInfo.autocal_flag=0;

				acquisition_config.tchautocal = T8_TCHAUTOCAL;
				acquisition_config.atchcalst = T8_ATCHCALST;
				acquisition_config.atchcalsthr = T8_ATCHCALSTHR;
				acquisition_config.atchfrccalthr = T8_ATCHFRCCALTHR;
				acquisition_config.atchfrccalratio = T8_ATCHFRCCALRATIO;

				if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK)
				{
					dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);
				}
			}

			break;

		default:
			break;
	}
	return true;
}
#endif

#ifdef TOUCH_IO

int reference_data[224] = { 0 };
static int* diag_debug(int command) 
{
	/*command 0x10: Delta, 0x11: Reference*/
	uint8_t data_buffer[130] = { 0 };
	uint8_t try_ctr = 0;
	uint8_t data_byte = 0; /* dianostic command to get touch refs*/
	uint16_t diag_address;
	uint8_t diag_size;

	uint8_t page;
	uint8_t i;
	uint8_t j=0;
	uint16_t value;
	uint16_t max_page = 4; // max_page = ceil(224 / (128/2) );
	int16_t signed_value;

	dbg_func_in();
	disable_irq(qt602240_data->client->irq);
	mutex_lock(&qt602240_data->lock);

	if (driver_setup != DRIVER_SETUP_OK){
		mutex_unlock(&qt602240_data->lock);
		enable_irq(qt602240_data->client->irq);
		return NULL;
	}

	touch_diagnostic_ret = true;
	diagnostic_min = 0;
	diagnostic_max = 0;
	/* read touch flags from the diagnostic object - clear buffer so the while loop can run first time */
	diagnostic_chip(command);
	msleep(20); 

	/* read touch flags from the diagnostic object - clear buffer so the while loop can run first time */
	memset( data_buffer , 0xFF, sizeof( data_buffer ) );
	diag_address = get_object_address(DEBUG_DIAGNOSTIC_T37,0);

	/* data array is 20 x 16 bits for each set of flags, 2 byte header, 40 bytes for touch flags 40 bytes for antitouch flags*/
	/* count up the channels/bits if we recived the data properly */
	//printk("[TSP] DIAG! \n");
	// ceil (224 / (128/2) )
	for (page = 0; page < max_page; page++) {
#if 0
		/* wait for diagnostic object to update */
		while(!((data_buffer[0] == command) && (data_buffer[1] == page)))
		{
			/* wait for data to be valid  */
			if(try_ctr > 100)
			{
				/* Failed! */
				//dbg("[TSP] Diagnostic Data did not update!!\n");
				break;
			}
			msleep(5); 
			try_ctr++; /* timeout counter */
			read_mem(diag_address, 2,data_buffer);
			//printk("[TSP] Waiting for diagnostic data to update, try %d\n", try_ctr);
		}
#endif

		/* wait for diagnostic object to update */
		while(!((data_buffer[0] == command) && (data_buffer[1] == page)) && try_ctr++ < 100){
			read_mem(diag_address, 2,data_buffer);

			if(data_buffer[0] != command || data_buffer[1] != page)
				msleep(5);
			//printk("[TSP] Waiting for diagnostic data to update, try %d\n", try_ctr);
		}

		if(try_ctr >= 100){
			printk("[TSP] Error read diagnostic cmd: %d\n", command);
			enable_irq(qt602240_data->client->irq);
			mutex_unlock(&qt602240_data->lock);
			return NULL;
		}

		/* data is ready - read the detection flags */
		diag_size = 130;
		read_mem(diag_address, diag_size, data_buffer);
		//printk("[TSP] CMD: %x Page: %x \n", data_buffer[0],data_buffer[1]);
		if((data_buffer[0] == command) && (data_buffer[1] == page))
		{

			/* count the channels and print the flags to the log */
			for(i = 0; i < 128; i+=2) /* check X lines - data is in words so increment 2 at a time */
			{
				if(j>=QT602240_MAX_CHANNEL_NUM) break;  // p11223
				value =  (data_buffer[3+i]<<8) + data_buffer[2+i];
				if (command == QT602240_REFERENCE_MODE){
					value = value - 0x4000;
					reference_data[j] = value;
					if((reference_data[j] < QT602240_REFERENCE_MIN || reference_data[j] > QT602240_REFERENCE_MAX) && j < QT602240_MAX_CHANNEL_NUM){
						if(!(j == 201 || j == 205))	{
							touch_diagnostic_ret = false;
							printk("diag_debug >>> j %d , reference_data[j] %d \n", j, reference_data[j]);
						}
					}
				}
				else if (command == QT602240_DELTA_MODE){
					signed_value = value;
					reference_data[j] = signed_value;
				}
				else{
					reference_data[j] = value;
					diagnostic_min = min(diagnostic_min, reference_data[j] );
					diagnostic_max = max(diagnostic_max, reference_data[j] );
				}
				j++;
				//if(j>=QT602240_MAX_CHANNEL_NUM) break;

				/* print the flags to the log - only really needed for debugging */
				//printk("%05d " , value);
			}
		}
		else{
			dbg("[TSP]ERROR!!");
		}
		/* send page up command so we can detect when data updates next time,
		 * page byte will sit at 1 until we next send F3 command */
		data_byte = 0x01;
		write_mem(command_processor_address + DIAGNOSTIC_OFFSET, 1, &data_byte);
	}

	mutex_unlock(&qt602240_data->lock);
	enable_irq(qt602240_data->client->irq);
	return (int *)reference_data;;
}

static struct file_operations fops = 
{
	.owner =    THIS_MODULE,
	.unlocked_ioctl =    ioctl,  // mirinae
	.read =     read,
	.write =    write,
	.open =     open,
	.release =  release,
};

static struct miscdevice touch_io = 
{
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "qt602240",
	.fops =     &fops
};

static int open(struct inode *inode, struct file *file) 
{
	return 0; 
}

static int release(struct inode *inode, struct file *file) 
{
	return 0; 
}
static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	int i,j,nBufSize=0;
	if((size_t)(*ppos) > 0) return 0;	

	if(buf!=NULL)
	{
		nBufSize=strlen(buf);
		if(strncmp(buf, "queue", 5)==0)
		{
			queue_work(qt602240_wq, &qt602240_data->work);
		}
		if(strncmp(buf, "debug", 5)==0)
		{			
			DebugON=1;	 
		}
		if(strncmp(buf, "debugoff", 8)==0)
		{			
			DebugON=0;	    
		}
		if(strncmp(buf, "touchid", 7)==0)
		{	
			j=0;
			for (i = 0 ; i < MAX_NUM_FINGER; ++i) { 
				if (fingerInfo[i].status == -1)
					continue;
				j++;
				printk("[TOUCH] TOUCH ID => %d, fingerInfo[i].status=> %d, fingerInfo[i].mode=> %d\n",i,fingerInfo[i].status,fingerInfo[i].mode); 

			}
			printk("[TOUCH] TOUCH ID CNT => %d, cal_correction_limit=> %d\n",j,cal_correction_limit); 

		}
		if(strncmp(buf, "dbg_cr", 6)==0)
		{			
			if(Debug_CR){
				Debug_CR=0;
			}
			else{
				Debug_CR=1;
			}
		}
		if(strncmp(buf, "checkcal", 8)==0)
		{			
			check_chip_calibration();
		}
		if(strncmp(buf, "cal", 3)==0)
		{			
			calibrate_chip();
		}
		if(strncmp(buf, "save", 4)==0)
		{			
			backup_config();	    
		}
		if(strncmp(buf, "reference", 9)==0)
		{			
			diag_debug(QT602240_REFERENCE_MODE);	    
		}
		if(strncmp(buf, "delta", 5)==0)
		{			
			diag_debug(QT602240_DELTA_MODE);	    
		}
		if(strncmp(buf, "sleep", 5)==0)
		{			
			qt602240_early_suspend(NULL);   
		}
		if(strncmp(buf, "resume",6)==0)
		{			
			qt602240_late_resume(NULL);   
		}
		if(strncmp(buf, "init",4)==0)
		{			
			qt602240_front_test_init();   
		}
		if(strncmp(buf, "off",3)==0)
		{			
			off_hw_setting();   
		}
		if (strncmp(buf, "camerain", 8) == 0)
		{
			if ( acquisition_config.tchautocal == T8_TCHAUTOCAL_PROTECTION) {

				dbg_CR("[TOUCH] Camera In - Autocal & Finger & Palm recovery disable\n");

				cancel_work_sync(&qt602240_data->work_protection_mode_disable);	// If calibration is performed right after protection_mode_timer enable, 
				del_timer(&protection_mode_timer);								// Finger & Palm recal config will be disable
				cancel_work_sync(&qt602240_data->work_tchautocal_disable);
				del_timer(&tchautocal_timer);

				protection_mode = false;
				f_resume_cal = false;
				cal_correction_limit = 0;
				debugInfo.calibration_cnt=1;
				debugInfo.autocal_flag=0;

				acquisition_config.tchautocal = T8_TCHAUTOCAL;
				acquisition_config.atchcalst = T8_ATCHCALST;
				acquisition_config.atchcalsthr = T8_ATCHCALSTHR;
				acquisition_config.atchfrccalthr = T8_ATCHFRCCALTHR;
				acquisition_config.atchfrccalratio = T8_ATCHFRCCALRATIO;

				if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK)
				{
					dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);
				}
			}
		}
#ifdef ITO_TYPE_CHECK
		if(strncmp(buf, "id",2)==0)
		{			
			read_touch_id();   
		}
#endif
	}
	*ppos +=nBufSize;
	return nBufSize;
}

static ssize_t read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	return 0; 
}

/*
 * Touch Config Table
 */

static long ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	config_table_element config;
	int return_value = -1;
	int data, object_type, field_index;
	switch (cmd)
	{
		case SET_TOUCH_CONFIG:
			data		= (int)((arg & 0xFFFF0000) >> 16);
			object_type 	= (int)((arg & 0x0000FF00) >> 8);
			field_index 	= (int)((arg & 0x000000FF) >> 0);
			if (config_table[object_type] == 0) {
				printk("[TSP] Error! undefined object type! %d\n", object_type);
				break;
			}
			config = config_table[object_type][field_index];
			if (config.size == UINT8) {
				*((uint8_t*)config_table[object_type][field_index].value) = (data & 0xFF);
			}
			else if (config.size == UINT16) {
				*((uint16_t*)config_table[object_type][field_index].value) = (data & 0xFFFF);
			}
			else if (config.size == INT8) {
				*((int8_t*)config_table[object_type][field_index].value) = (data & 0xFF);
			}
			else if (config.size == INT16) {
				*((int16_t*)config_table[object_type][field_index].value) = (data & 0xFFFF);
			}
			else {
				// Error
			}
			printk("[TSP] set %d-%d with %d\n", object_type, field_index, data);
			break;
		case GET_TOUCH_CONFIG:
			object_type 	= (int)((arg & 0x0000FF00) >> 8);
			field_index 	= (int)((arg & 0x000000FF) >> 0);
			if (config_table[object_type] == 0) {
				//printk("[TSP] Error! undefined object type! %d\n", object_type);
				break;
			}
			config = config_table[object_type][field_index];
			if (config.size == UINT8 || config.size == INT8) {
				return_value = ((int16_t)*(config.value) & 0xFF) + (config.size << 16);
			}
			else if (config.size == UINT16 || config.size == INT16) {
				return_value = ((int16_t)*(config.value) & 0xFFFF) + (config.size << 16);
			}
			else {
				// Error
			}
			dbg("[TSP] GET config: %d-%d: %d (%d)\n", object_type, field_index, (return_value & 0xFFFF), (return_value & 0xF0000)>>16);
			return return_value;
			break;

		case APPLY_TOUCH_CONFIG:
			apply_touch_config();
			break;
		case RESET_TOUCH_CONFIG:
			reset_touch_config();
			break;
			/*
			   case READ_ITO_TYPE:
			   return tsp_ito_type;
			   break;
			 */
		case TOUCH_IOCTL_DEBUG:
			return ioctl_debug(arg);
			break;
		case TOUCH_CHARGER_MODE:
			dbg("Touch charger mode change \n");
			qt_charger_mode_config(arg);
			break;
			//Factory Cmd
		case TOUCH_IOCTL_READ_LASTKEY:
			break;
		case TOUCH_IOCTL_DO_KEY:
			input_report_key(qt602240_data->input_dev, (int)arg, 1);
			break;
		case TOUCH_IOCTL_RELEASE_KEY:		
			input_report_key(qt602240_data->input_dev, (int)arg, 0);
			break;
		case TOUCH_IOCTL_INIT:
			printk("Touch init \n");
			qt602240_front_test_init();
			break;
		case TOUCH_IOCTL_OFF:
			printk("Touch off \n");
			off_hw_setting();
			break;    
		case DIAG_DEBUG:
			/*
			 * Run Diag and save result into reference_data array when arg. is 5010 or 5011. 
			 * Returns diag result when the arg. is in range of 0~223. 
			 */
			if (arg == 5010) 
			{
				diag_debug(QT602240_DELTA_MODE);
				return 0;
			}
			if (arg == 5011) 
			{
				diag_debug(QT602240_REFERENCE_MODE);
				return 0;
			}
			else if (arg > 224-1)
			{
				printk("[TSP] ERROR!");
				return 0;
			}
			return reference_data[arg];

		default:
			break;
	}
	return 0;
}

typedef enum  {
	IOCTL_DEBUG_SUSPEND = 0,	
	IOCTL_DEBUG_RESUME = 1,	
	IOCTL_DEBUG_GET_TOUCH_ANTITOUCH_INFO = 2,
	IOCTL_DEBUG_TCH_CH= 3,	
	IOCTL_DEBUG_ATCH_CH= 4,	
	IOCTL_GET_CALIBRATION_CNT = 5,	
} ioctl_debug_cmd;

static int ioctl_debug(unsigned long arg) 
{
	switch (arg)
	{
		case IOCTL_DEBUG_SUSPEND:
			qt602240_early_suspend(NULL);
			break;
		case IOCTL_DEBUG_RESUME:
			qt602240_late_resume(NULL);
			break;
		case IOCTL_DEBUG_GET_TOUCH_ANTITOUCH_INFO:
			check_chip_calibration();
			return get_touch_antitouch_info();
			break;
		case IOCTL_DEBUG_TCH_CH:
			return debugInfo.tch_ch;
			break;
		case IOCTL_DEBUG_ATCH_CH:
			return debugInfo.atch_ch;
			break;
		case IOCTL_GET_CALIBRATION_CNT:
			return debugInfo.calibration_cnt;
			break;
		default:
			break;
	}
	return 0;
}
static void apply_touch_config(void)
{
	dbg_func_in();

	if (driver_setup != DRIVER_SETUP_OK)
		return;

	if (write_power_config(power_config) != CFG_WRITE_OK)	{
		dbg("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}

	dbg("xxxx acquisition config\n");
	if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK)	{
		dbg("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}

	dbg("xxxx tchthr : %d",  touchscreen_config.tchthr);
	dbg("xxxx multitouchscreen config\n");
	if (write_multitouchscreen_config(0, touchscreen_config) != CFG_WRITE_OK)	{
		dbg("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}

	dbg("xxxx keyarray config \n");
	if (write_keyarray_config(0, keyarray_config) != CFG_WRITE_OK)	{
		dbg("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}

	dbg("xxxx comc config \n");
	if (get_object_address(SPT_COMCONFIG_T18, 0) != OBJECT_NOT_FOUND)	{
		if (write_comc_config(0, comc_config) != CFG_WRITE_OK){
			dbg("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	}

	dbg("xxxx gpio pwm config \n");
	if (write_gpio_config(0, gpiopwm_config) != CFG_WRITE_OK)	{
		dbg("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}	

	dbg("xxxx proximity config \n");
	if (get_object_address(TOUCH_PROXIMITY_T23, 0) != OBJECT_NOT_FOUND)	{
		if (write_proximity_config(0, proximity_config) != CFG_WRITE_OK)		{
			dbg("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	}

	dbg("xxxx self test config \n");
	if (get_object_address(SPT_SELFTEST_T25, 0) != OBJECT_NOT_FOUND)	{
		if (write_selftest_config(0,selftest_config) != CFG_WRITE_OK)		{
			dbg("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	}

	if (get_object_address(PROCI_GRIPSUPPRESSION_T40, 0) != OBJECT_NOT_FOUND)	{
		if (write_gripsuppression_T40_config(gripsuppression_t40_config) !=	CFG_WRITE_OK)		{
			dbg("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	}

	if (get_object_address(PROCI_TOUCHSUPPRESSION_T42, 0) != OBJECT_NOT_FOUND)	{
		if (write_touchsuppression_t42_config(touchsuppression_t42_config) != CFG_WRITE_OK)		{
			dbg("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	}

	if (get_object_address(SPT_CTECONFIG_T46, 0) != OBJECT_NOT_FOUND)	{
		if (write_CTE_T46_config(cte_t46_config) != CFG_WRITE_OK)		{
			dbg("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	}

	if (get_object_address(PROCI_STYLUS_T47, 0) != OBJECT_NOT_FOUND){
		if (write_stylus_t47_config(stylus_t47_config) != CFG_WRITE_OK)	{
			dbg("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	}

	if (get_object_address(PROCG_NOISESUPPRESSION_T48, 0) != OBJECT_NOT_FOUND)	{
		if (write_noisesuppression_t48_config(noisesuppression_t48_config) != CFG_WRITE_OK)	{
			dbg("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
		}
	}

	dbg_func_out();
}


static uint8_t qt_charger_mode_config(unsigned long mode)
{
	uint8_t status = 0;
	uint16_t object_address_t7, object_address_t48= 0;

	dbg_func_in();

	charger_mode=mode;

	if (driver_setup != DRIVER_SETUP_OK)
		return 0;

	if(pre_charger_mode == mode)
		return status;

	printk("[TSP] qt_charger_mode_config [charger_mode=%d, pre_charger_mode=%d] \n", (int)mode, (int)pre_charger_mode);	
	pre_charger_mode = mode;

	switch (mode)
	{
		case BATTERY_PLUGGED_NONE:
			dbg("Touch charger mode BATTERY_PLUGGED_NONE \n");
			noisesuppression_t48_config.calcfg = T48_CALCFG;
			power_config.idleacqint = T7_IDLEACQINT;
			break;

		case BATTERY_PLUGGED_CHARGER:
			dbg("Touch charger mode BATTERY_PLUGGED \n");
			noisesuppression_t48_config.calcfg = T48_CALCFG_PLUG;
			dbg("noisesuppression_t48_config.calcfg = %d\n",noisesuppression_t48_config.calcfg);
			power_config.idleacqint = T7_IDLEACQINT_PLUG;
			break;
		default:
			break;
	}

	object_address_t7 = get_object_address(GEN_POWERCONFIG_T7, 0);
	dbg("object_address_t7 = %d\n",object_address_t7);
	if (object_address_t7 == 0)
		return(CFG_WRITE_FAILED);

	object_address_t48 = get_object_address(PROCG_NOISESUPPRESSION_T48, 0);
	dbg("object_address_t48 = %d\n",object_address_t48);
	if (object_address_t48 == 0)
		return(CFG_WRITE_FAILED);
	mutex_lock(&qt602240_data->lock);
	status |= write_mem(object_address_t7, 1, &power_config.idleacqint);

	if (write_noisesuppression_t48_config(noisesuppression_t48_config) !=	CFG_WRITE_OK)
		dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);

	dbg("noisesuppression_t48_config.calcfg = %d\n",noisesuppression_t48_config.calcfg);
	mutex_unlock(&qt602240_data->lock);
	dbg_func_out();

	return status;
}


#endif //TOUCH_IO

#ifdef ITO_TYPE_CHECK
static int read_touch_id(void)
{
	struct pm8xxx_adc_chan_result result;
	int rc,i;
	tsp_ito_type=0; // p13106 default white

	rc = pm8xxx_adc_mpp_config_read(TOUCH_ID_MPP, ADC_MPP_1_AMUX6, &result);
	if(rc == -EINVAL){
		printk("[Touch_id] read_touch_id is failed~!! \n");
		return -EINVAL;
	}
	printk("[Touch_id] ADC MPP9 value raw:%x physical:%lld\n", result.adc_code, result.physical);

	for(i = 0; i < number_of_elements(ito_table); i++) {
		if(result.physical >= ito_table[i].min && result.physical <= ito_table[i].max) {
			tsp_ito_type = i;
			break;
		}
	}
	if (tsp_ito_type >= 0) {
		printk("[Touch_id] Color: %d (adc value:%lld)\n", tsp_ito_type, result.physical);
	}
	else {
		printk("[Touch_id] ERROR! undefined ito type.\n");
	}

	return 0;
}
#endif


/*****************************************************************************
 *
 *       QT602240  Configuration Block
 *
 * ***************************************************************************/

void qt_Power_Sleep(void)
{
	dbg_func_in();

	if (driver_setup != DRIVER_SETUP_OK)
		return;

	/* Set Idle Acquisition Interval to 32 ms. */
	power_config.idleacqint = 0;

	/* Set Active Acquisition Interval to 16 ms. */
	power_config.actvacqint = 0;

	/* Set Active to Idle Timeout to 4 s (one unit = 200ms). */
	power_config.actv2idleto = 0;

	/* Write power config to chip. */
	if (write_power_config(power_config) != CFG_WRITE_OK)	{
		dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);
	}

	dbg_func_out();
}

/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

void qt_Power_Config_Init(void)
{
	dbg_func_in();

	if (driver_setup != DRIVER_SETUP_OK)
		return;
	if(charger_mode==BATTERY_PLUGGED_CHARGER){
		power_config.idleacqint = T7_IDLEACQINT_PLUG;
	}else{
		power_config.idleacqint = T7_IDLEACQINT;
	}
	/* Set Active Acquisition Interval to 16 ms. */
	power_config.actvacqint = T7_ACTVACQINT;
	/* Set Active to Idle Timeout to 4 s (one unit = 200ms). */
	power_config.actv2idleto = T7_ACTV2IDLETO;

	/* Write power config to chip. */
	if (write_power_config(power_config) != CFG_WRITE_OK)
	{
		dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);
	}

	dbg_func_out();
}

/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

void qt_Acquisition_Config_Init(void)
{
	dbg_func_in();

	if (driver_setup != DRIVER_SETUP_OK)
		return;

	acquisition_config.chrgtime = T8_CHRGTIME; // 2us
	acquisition_config.reserved = 0;	//5;
	acquisition_config.tchdrift = T8_TCHDRIFT;	 // 4s
	acquisition_config.driftst = T8_DRIFTST;	 // 4s
	acquisition_config.tchautocal = T8_TCHAUTOCAL; // infinite
	acquisition_config.sync = T8_SYNC; // disabled
	acquisition_config.atchcalst = T8_ATCHCALST;
	acquisition_config.atchcalsthr = T8_ATCHCALSTHR;
	acquisition_config.atchfrccalthr = T8_ATCHFRCCALTHR;     /*!< Anti-touch force calibration threshold */
	acquisition_config.atchfrccalratio = T8_ATCHFRCCALRATIO;  /*!< Anti-touch force calibration ratio */  

	if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK)	{
		dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);
	}

	dbg_func_out();
}

#ifdef PROTECTION_MODE
void disable_autocal_locked(struct work_struct * p) {
	//mutex_lock(&qt602240_data->lock);
	dbg_func_in();

	dbg_CR("[touch] touch auto calibration disable\n");
	acquisition_config.tchautocal = T8_TCHAUTOCAL;
	debugInfo.calibration_cnt=0;
	debugInfo.autocal_flag=0;

	if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK){
		dbg("[TOUCH] Configuration Fail!!! , Line %d \n", __LINE__);
	}

#ifdef PROTECTION_MODE_SAFECHECK
	autocal_safecheck_flag = 0;
#endif

	dbg_func_out();
	//mutex_unlock(&qt602240_data->lock);
}

void qt_Acquisition_Config_Init_locked(struct work_struct * p) {
	//mutex_lock(&qt602240_data->lock);
	dbg_CR("[TOUCH] Finger & Palm recalibration disable\n");
	protection_mode = false;
	acquisition_config.atchcalst = T8_ATCHCALST;
	acquisition_config.atchcalsthr = T8_ATCHCALSTHR;
	acquisition_config.atchfrccalthr = T8_ATCHFRCCALTHR;
	acquisition_config.atchfrccalratio = T8_ATCHFRCCALRATIO;

	if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK)
	{
		dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);
	}

	//mutex_unlock(&qt602240_data->lock);
}

#endif

/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

void qt_Multitouchscreen_Init(void)
{
	dbg_func_in();

	if (driver_setup != DRIVER_SETUP_OK)
		return;

	touchscreen_config.ctrl = T9_CTRL; //131; // enable + message-enable
	touchscreen_config.movhysti = T9_MOVHYSTI/*10*/;	// Relate Touch Move and Click
	touchscreen_config.movhystn = T9_MOVHYSTN/*2*/;
	touchscreen_config.mrghyst = T9_MRGHYST/*10*/;
	touchscreen_config.mrgthr = T9_MRGTHR/*30*/;
	touchscreen_config.amphyst = T9_AMPHYST/*10*/;
	touchscreen_config.yloclip = T9_YLOCLIP/*15*/;	// Change Active area
	touchscreen_config.yhiclip = T9_YHICLIP/*15*/;
	touchscreen_config.jumplimit = T9_JUMPLIMIT/*30*/;
	touchscreen_config.xpitch = T9_XPITCH;
	touchscreen_config.ypitch =  T9_YPITCH;
	touchscreen_config.nexttchdi =  T9_NEXTTCHDI;
	touchscreen_config.blen = T9_BLEN;
#ifdef ITO_TYPE_CHECK
	touchscreen_config.tchthr = T9_TCHTHR[tsp_ito_type];
#else
	touchscreen_config.tchthr = T9_TCHTHR;
#endif
	touchscreen_config.tchdi = T9_TCHDI;
#ifdef ITO_TYPE_CHECK
	touchscreen_config.tchhyst = T9_TCHHYST[tsp_ito_type];   // 25% of tchthr
#else
	touchscreen_config.tchhyst = T9_TCHHYST;   // 25% of tchthr
#endif
	touchscreen_config.xorigin = T9_XORIGIN;
	touchscreen_config.yorigin = T9_YORIGIN;
	touchscreen_config.xsize = T9_XSIZE;
	touchscreen_config.ysize = T9_YSIZE;
	touchscreen_config.akscfg = T9_AKSCFG;
	touchscreen_config.orient = T9_ORIENT;
	touchscreen_config.mrgtimeout = T9_MRGTIMEOUT;
	touchscreen_config.movfilter = T9_MOVFILTER;
	touchscreen_config.numtouch= T9_NUMTOUCH;
	touchscreen_config.xrange = T9_XRANGE;    // 902 = (98.2/87.1) * 800
	touchscreen_config.yrange = T9_YRANGE;
	touchscreen_config.xloclip = T9_XLOCLIP;
	touchscreen_config.xhiclip = T9_XHICLIP;
	touchscreen_config.xedgectrl = T9_XEDGECTRL;
	touchscreen_config.xedgedist = T9_XEDGEDIST;
	touchscreen_config.yedgectrl = T9_YEDGECTRL;
	touchscreen_config.yedgedist = T9_YEDGEDIST;

	if (write_multitouchscreen_config(0, touchscreen_config) != CFG_WRITE_OK)
		dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);

	dbg_func_out();
}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

void qt_KeyArray_Init(void)
{
	dbg_func_in();

	if (driver_setup != DRIVER_SETUP_OK)
		return;

	keyarray_config.ctrl = T15_CTRL;
	keyarray_config.xorigin = T15_XORIGIN;
	keyarray_config.yorigin = T15_YORIGIN;
	keyarray_config.xsize = T15_XSIZE;
	keyarray_config.ysize = T15_YSIZE;
	keyarray_config.akscfg = T15_AKSCFG;
	keyarray_config.blen = T15_BLEN;
	keyarray_config.tchthr = T15_TCHTHR;
	keyarray_config.tchdi = T15_TCHDI;
	keyarray_config.reserved[0] = T15_RESERVED_0;
	keyarray_config.reserved[1] = T15_RESERVED_1;

	if (write_keyarray_config(0, keyarray_config) != CFG_WRITE_OK)
		dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);

	dbg_func_out();
}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

void qt_ComcConfig_Init(void)
{
	dbg_func_in();

	if (driver_setup != DRIVER_SETUP_OK)
		return;

	comc_config.ctrl = T18_CTRL;
	comc_config.cmd = NO_COMMAND;

	if (get_object_address(SPT_COMCONFIG_T18, 0) != OBJECT_NOT_FOUND){
		if (write_comc_config(0, comc_config) != CFG_WRITE_OK)
			dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);
	}

	dbg_func_out();
}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

void qt_Gpio_Pwm_Init(void)
{
	dbg_func_in();

	if (driver_setup != DRIVER_SETUP_OK)
		return;

	gpiopwm_config.ctrl = T19_CTRL;
	gpiopwm_config.reportmask = T19_REPORTMASK;
	gpiopwm_config.dir = T19_DIR;
	gpiopwm_config.intpullup = T19_INTPULLUP;
	gpiopwm_config.out = T19_OUT;
	gpiopwm_config.wake = T19_WAKE;
	gpiopwm_config.pwm = T19_PWM;
	gpiopwm_config.period = T19_PERIOD;
	gpiopwm_config.duty[0] = T19_DUTY_0;
	gpiopwm_config.duty[1] = T19_DUTY_1;
	gpiopwm_config.duty[2] = T19_DUTY_2;
	gpiopwm_config.duty[3] = T19_DUTY_3;
	gpiopwm_config.trigger[0] = T19_TRIGGER_0;
	gpiopwm_config.trigger[1] = T19_TRIGGER_1;
	gpiopwm_config.trigger[2] = T19_TRIGGER_2;
	gpiopwm_config.trigger[3] = T19_TRIGGER_3;

	if (write_gpio_config(0, gpiopwm_config) != CFG_WRITE_OK)
		dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);

	dbg_func_out();
}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

void qt_Proximity_Config_Init(void)
{
	dbg_func_in();

	if (driver_setup != DRIVER_SETUP_OK)
		return;

	proximity_config.ctrl = T23_CTRL;
	proximity_config.xorigin = T23_XORIGIN;
	proximity_config.yorigin = T23_YORIGIN;    
	proximity_config.xsize = T23_XSIZE;
	proximity_config.ysize = T23_YSIZE;
	proximity_config.reserved_for_future_aks_usage = T23_RESERVED;
	proximity_config.blen = T23_BLEN;
	proximity_config.fxddthr = T23_FXDDTHR; 
	proximity_config.fxddi = T23_FXDDI; 
	proximity_config.average = T23_AVERAGE;     
	proximity_config.mvnullrate = T23_MVNULLRATE; 
	proximity_config.mvdthr = T23_MVDTHR; 

	if (get_object_address(TOUCH_PROXIMITY_T23, 0) != OBJECT_NOT_FOUND)	{
		if (write_proximity_config(0, proximity_config) != CFG_WRITE_OK)
			dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);
	}

	dbg_func_out();
}



/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

void qt_Selftest_Init(void)
{
	dbg_func_in();

	if (driver_setup != DRIVER_SETUP_OK)
		return;

	selftest_config.ctrl = T25_CTRL;
	selftest_config.cmd = T25_CMD;

	if (get_object_address(SPT_SELFTEST_T25, 0) != OBJECT_NOT_FOUND) {
		if (write_selftest_config(0,selftest_config) != CFG_WRITE_OK)
			dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);
	}

	dbg_func_out();
}

/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * 2011-01-19 10:29:00
 * MXT224E 
 *
 * ***************************************************************************/

void qt_Grip_Suppression_T40_Config_Init(void)
{
	gripsuppression_t40_config.ctrl     = T40_CTRL;
	gripsuppression_t40_config.xlogrip  = T40_XLOGRIP;
	gripsuppression_t40_config.xhigrip  = T40_XHIGRIP;
	gripsuppression_t40_config.ylogrip  = T40_YLOGRIP;
	gripsuppression_t40_config.yhigrip  = T40_YHIGRIP;

	/* Write grip suppression config to chip. */
	if (get_object_address(PROCI_GRIPSUPPRESSION_T40, 0) != OBJECT_NOT_FOUND) {
		if (write_gripsuppression_T40_config(gripsuppression_t40_config) != CFG_WRITE_OK)
			dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);

	}
}

void qt_Touch_Suppression_T42_Config_Init(void)
{
	touchsuppression_t42_config.ctrl             = T42_CTRL;         
	touchsuppression_t42_config.apprthr          = T42_APPRTHR;      
	touchsuppression_t42_config.maxapprarea      = T42_MAXAPPRAREA;  
	touchsuppression_t42_config.maxtcharea       = T42_MAXTCHAREA;   
	touchsuppression_t42_config.supstrength      = T42_SUPSTRENGTH;  
	touchsuppression_t42_config.supextto         = T42_SUPEXTTO;     
	touchsuppression_t42_config.maxnumtchs       = T42_MAXNUMTCHS;   
	touchsuppression_t42_config.shapestrength    = T42_SHAPESTRENGTH;

	/* Write grip suppression config to chip. */
	if (get_object_address(PROCI_TOUCHSUPPRESSION_T42, 0) != OBJECT_NOT_FOUND)
	{
		if (write_touchsuppression_t42_config(touchsuppression_t42_config) != CFG_WRITE_OK)
			dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);		
	}
}

void qt_CTE_T46_Config_Init(void)
{
	/* Set CTE config */
	cte_t46_config.ctrl                 = T46_CTRL;
	cte_t46_config.mode                 = T46_MODE;
	cte_t46_config.idlesyncsperx        = T46_IDLESYNCSPERX;
	cte_t46_config.actvsyncsperx        = T46_ACTVSYNCSPERX;
	cte_t46_config.adcspersync          = T46_ADCSPERSYNC;
	cte_t46_config.pulsesperadc         = T46_PULSESPERADC;
	cte_t46_config.xslew                = T46_XSLEW;
	cte_t46_config.syncdelay            = T46_SYNCDELAY;

	/* Write CTE config to chip. */
	if (get_object_address(SPT_CTECONFIG_T46, 0) != OBJECT_NOT_FOUND)
	{
		if (write_CTE_T46_config(cte_t46_config) != CFG_WRITE_OK)
			dbg("[TSP] Configuration Fail!!! , Line %d \n", __LINE__);
	}
}

void qt_Stylus_T47_Config_Init(void)
{
	stylus_t47_config.ctrl          = T47_CTRL; 
	stylus_t47_config.contmin       = T47_CONTMIN; 
	stylus_t47_config.contmax       = T47_CONTMAX; 
	stylus_t47_config.stability     = T47_STABILITY; 
	stylus_t47_config.maxtcharea    = T47_MAXTCHAREA;  
	stylus_t47_config.amplthr       = T47_AMPLTHR; 
	stylus_t47_config.styshape      = T47_STYSHAPE; 
	stylus_t47_config.hoversup      = T47_HOVERSUP; 
	stylus_t47_config.confthr       = T47_CONFTHR; 
	stylus_t47_config.syncsperx     = T47_SYNCSPERX;

	/* Write grip suppression config to chip. */
	if (get_object_address(PROCI_STYLUS_T47, 0) != OBJECT_NOT_FOUND) {
		if (write_stylus_t47_config(stylus_t47_config) != CFG_WRITE_OK)
			dbg("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}
}


void qt_Noisesuppression_T48_config_Init(void)
{
	noisesuppression_t48_config.ctrl  			= T48_CTRL ;	
	noisesuppression_t48_config.cfg  			= T48_CFG ;
	noisesuppression_t48_config.calcfg  			= T48_CALCFG ;
	noisesuppression_t48_config.basefreq  			= T48_BASEFREQ ;
	noisesuppression_t48_config.freq_0  			= T48_RESERVED0 ;
	noisesuppression_t48_config.freq_1  			= T48_RESERVED1 ;
	noisesuppression_t48_config.freq_2  			= T48_RESERVED2 ;
	noisesuppression_t48_config.freq_3  			= T48_RESERVED3 ;
	noisesuppression_t48_config.mffreq_2  			= T48_MFFREQ_2 ;
	noisesuppression_t48_config.mffreq_3  			= T48_MFFREQ_3 ;
	noisesuppression_t48_config.nlgain  			= T48_RESERVED4 ;
	noisesuppression_t48_config.nlthr  			= T48_RESERVED5 ;
	noisesuppression_t48_config.gclimit  			= T48_RESERVED6 ;
	noisesuppression_t48_config.gcactvinvldadcs  		= T48_GCACTVINVLDADCS ;
	noisesuppression_t48_config.gcidleinvldadcs  		= T48_GCIDLEINVLDADCS ;
	noisesuppression_t48_config.gcinvalidthr  		= T48_RESERVED7 ;
	noisesuppression_t48_config.gcmaxadcsperx  		= T48_GCMAXADCSPERX ;
	noisesuppression_t48_config.gclimitmin  		= T48_GCLIMITMIN ;
	noisesuppression_t48_config.gclimitmax  		= T48_GCLIMITMAX ;
	noisesuppression_t48_config.gccountmintgt  		= T48_GCCOUNTMINTGT ;
	noisesuppression_t48_config.mfinvlddiffthr  		= T48_MFINVLDDIFFTHR ;
	noisesuppression_t48_config.mfincadcspxthr  		= T48_MFINCADCSPXTHR ;
	noisesuppression_t48_config.mferrorthr  		= T48_MFERRORTHR ;
	noisesuppression_t48_config.selfreqmax  		= T48_SELFREQMAX ;
	noisesuppression_t48_config.reserved9  			= T48_RESERVED9 ;
	noisesuppression_t48_config.reserved10  		= T48_RESERVED10 ;
	noisesuppression_t48_config.reserved11  		= T48_RESERVED11 ;
	noisesuppression_t48_config.reserved12  		= T48_RESERVED12 ;
	noisesuppression_t48_config.reserved13  		= T48_RESERVED13 ;
	noisesuppression_t48_config.reserved14 		 	= T48_RESERVED14 ;
	noisesuppression_t48_config.blen  			= T48_BLEN ;
	noisesuppression_t48_config.tchthr  			= T48_TCHTHR ;
	noisesuppression_t48_config.tchdi  			= T48_TCHDI ;
	noisesuppression_t48_config.movhysti  			= T48_MOVHYSTI ;
	noisesuppression_t48_config.movhystn  			= T48_MOVHYSTN ;
	noisesuppression_t48_config.movfilter  			= T48_MOVFILTER ;
	noisesuppression_t48_config.numtouch  			= T48_NUMTOUCH ;
	noisesuppression_t48_config.mrghyst  			= T48_MRGHYST ;
	noisesuppression_t48_config.mrgthr  			= T48_MRGTHR ;
	noisesuppression_t48_config.xloclip  			= T48_XLOCLIP ;
	noisesuppression_t48_config.xhiclip  			= T48_XHICLIP ;
	noisesuppression_t48_config.yloclip  			= T48_YLOCLIP ;
	noisesuppression_t48_config.yhiclip  			= T48_YHICLIP ;
	noisesuppression_t48_config.xedgectrl  			= T48_XEDGECTRL ;
	noisesuppression_t48_config.xedgedist  			= T48_XEDGEDIST ;
	noisesuppression_t48_config.yedgectrl  			= T48_YEDGECTRL ;
	noisesuppression_t48_config.yedgedist  			= T48_YEDGEDIST ;
	noisesuppression_t48_config.jumplimit  			= T48_JUMPLIMIT ;
	noisesuppression_t48_config.tchhyst  			= T48_TCHHYST ;
	noisesuppression_t48_config.nexttchdi  			= T48_NEXTTCHDI ;

	/* Write grip suppression config to chip. */
	if (get_object_address(PROCG_NOISESUPPRESSION_T48, 0) != OBJECT_NOT_FOUND){
		if (write_noisesuppression_t48_config(noisesuppression_t48_config) != CFG_WRITE_OK)
			dbg("[TSP] Configuration Fail!!! , Line %d \n\r", __LINE__);
	}
}

static void reset_touch_config(void)
{
	qt_Power_Config_Init();
	qt_Acquisition_Config_Init();
	qt_Multitouchscreen_Init();
	qt_KeyArray_Init();
	qt_ComcConfig_Init();
	qt_Gpio_Pwm_Init();
	qt_Proximity_Config_Init();
	qt_Selftest_Init();
	qt_Grip_Suppression_T40_Config_Init();
	qt_Touch_Suppression_T42_Config_Init();
	qt_CTE_T46_Config_Init();
	qt_Stylus_T47_Config_Init();
	qt_Noisesuppression_T48_config_Init();
}

/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint8_t reset_chip(void)
{
	uint8_t data = 1u;
	uint8_t rc;

	dbg_func_in();

	if (driver_setup != DRIVER_SETUP_OK)
		return WRITE_MEM_FAILED;

	rc = write_mem(command_processor_address + RESET_OFFSET, 1, &data);

	dbg_func_out();

	return rc;
}

/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 * \brief Calibrates the chip.
 * 
 * This function will send a calibrate command to touch chip.
 * Whilst calibration has not been confirmed as good, this function will set
 * the ATCHCALST and ATCHCALSTHR to zero to allow a bad cal to always recover
 * 
 * @return WRITE_MEM_OK if writing the command to touch chip was successful.
 * 
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint8_t calibrate_chip(void)
{
	uint8_t data = 1u;
	int ret = WRITE_MEM_OK;
	uint8_t atchcalst, atchcalsthr;
	uint8_t atchfrccalthr, atchfrccalratio;

	dbg_func_in();

	if (driver_setup != DRIVER_SETUP_OK)
		return WRITE_MEM_FAILED;

	not_yet_count = 0;
	mxt_time_point = 0;
	good_calibration_cnt = 0;

	/* change calibration suspend settings to zero until calibration confirmed good */
	/* store normal settings */
	atchcalst = acquisition_config.atchcalst;
	atchcalsthr = acquisition_config.atchcalsthr;
	atchfrccalthr= acquisition_config.atchfrccalthr;
	atchfrccalratio= acquisition_config.atchfrccalratio;

	/* resume calibration must be performed with zero settings */
	acquisition_config.atchcalst = T8_ATCHCALST;
	acquisition_config.atchcalsthr = T8_ATCHCALSTHR; 
	acquisition_config.atchfrccalthr = T8_ATCHFRCCALTHR;
	acquisition_config.atchfrccalratio = T8_ATCHFRCCALRATIO; 

	dbg("[TSP] reset acq atchcalst=%d, atchcalsthr=%d\n", acquisition_config.atchcalst, acquisition_config.atchcalsthr );

	/* Write temporary acquisition config to chip. */
	if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK)
	{
		/* "Acquisition config write failed!\n" */
		printk("[TOUCH][Inform calibration] line : %d\n", __LINE__);
		ret = WRITE_MEM_FAILED; /* calling function should retry calibration call */
	}

	/* send calibration command to the chip */
	if(ret == WRITE_MEM_OK)
	{
		/* change calibration suspend settings to zero until calibration confirmed good */
		ret = write_mem(command_processor_address + CALIBRATE_OFFSET, 1, &data);

		/* set flag for calibration lockup recovery if cal command was successful */
		if(ret == WRITE_MEM_OK)
		{ 
			/* set flag to show we must still confirm if calibration was good or bad */
			cal_check_flag = 1u;
		}
		else{
			printk("[TOUCH][Inform calibration] line : %d\n", __LINE__);
		}
	}

	msleep(120);


	dbg_func_out();
	return ret;
}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint8_t diagnostic_chip(uint8_t mode)
{
	uint8_t status;
	dbg_func_in();

	if (driver_setup != DRIVER_SETUP_OK)
		return WRITE_MEM_FAILED;

	status = write_mem(command_processor_address + DIAGNOSTIC_OFFSET, 1, &mode);

	dbg_func_out();
	return(status);
}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint8_t backup_config(void)
{
	/* Write 0x55 to BACKUPNV register to initiate the backup. */
	uint8_t data = 0x55u;
	uint8_t rc;

	dbg_func_in();

	if (driver_setup != DRIVER_SETUP_OK)
		return WRITE_MEM_FAILED;

	rc = write_mem(command_processor_address + BACKUP_OFFSET, 1, &data);

	dbg_func_out();

	return rc;
}

uint8_t write_power_config(gen_powerconfig_t7_config_t cfg)
{
	uint8_t rc;

	dbg_func_in();

	rc = write_simple_config(GEN_POWERCONFIG_T7, 0, (void *) &cfg);

	dbg_func_out();
	return rc;
}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint8_t write_acquisition_config(gen_acquisitionconfig_t8_config_t cfg)
{
	uint8_t rc;

	dbg_func_in();

	rc = write_simple_config(GEN_ACQUISITIONCONFIG_T8, 0, (void *) &cfg);

	dbg_func_out();
	return rc;
}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint8_t write_multitouchscreen_config(uint8_t instance, touch_multitouchscreen_t9_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;

	dbg_func_in();

	object_size = get_object_size(TOUCH_MULTITOUCHSCREEN_T9);

	if (object_size == 0)
		return(CFG_WRITE_FAILED);

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);

	if (tmp == NULL)
		return(CFG_WRITE_FAILED);

	memset(tmp,0,object_size);

	/* 18 elements at beginning are 1 byte. */
	memcpy(tmp, &cfg, 18);

	/* Next two are 2 bytes. */

	*(tmp + 18) = (uint8_t) (cfg.xrange &  0xFF);
	*(tmp + 19) = (uint8_t) (cfg.xrange >> 8);

	*(tmp + 20) = (uint8_t) (cfg.yrange &  0xFF);
	*(tmp + 21) = (uint8_t) (cfg.yrange >> 8);

	/* And the last 4(8) 1 bytes each again. */

	*(tmp + 22) = cfg.xloclip;
	*(tmp + 23) = cfg.xhiclip;
	*(tmp + 24) = cfg.yloclip;
	*(tmp + 25) = cfg.yhiclip;
	*(tmp + 26) = cfg.xedgectrl;
	*(tmp + 27) = cfg.xedgedist;
	*(tmp + 28) = cfg.yedgectrl;
	*(tmp + 29) = cfg.yedgedist;
	*(tmp + 30) = cfg.jumplimit;
	*(tmp + 31) = cfg.tchhyst;
	*(tmp + 32) = cfg.xpitch;
	*(tmp + 33) = cfg.ypitch;
	*(tmp + 34) = cfg.nexttchdi;

	object_address = get_object_address(TOUCH_MULTITOUCHSCREEN_T9,instance);

	if (object_address == 0)
		return(CFG_WRITE_FAILED);

	status = write_mem(object_address, object_size, tmp);

	kfree(tmp);

	dbg_func_out();
	return(status);

}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint8_t write_keyarray_config(uint8_t instance, touch_keyarray_t15_config_t cfg)
{
	uint8_t rc;
	dbg_func_in();

	rc = write_simple_config(TOUCH_KEYARRAY_T15, instance, (void *) &cfg);

	dbg_func_out();
	return rc;
}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint8_t write_comc_config(uint8_t instance, spt_comcconfig_t18_config_t cfg)
{
	uint8_t rc;
	dbg_func_in();

	rc = write_simple_config(SPT_COMCONFIG_T18, instance, (void *) &cfg);

	dbg_func_out();
	return rc;
}

uint8_t write_gpio_config(uint8_t instance, spt_gpiopwm_t19_config_t cfg)
{
	uint8_t rc;
	dbg_func_in();
	rc = write_simple_config(SPT_GPIOPWM_T19, instance, (void *) &cfg);
	dbg_func_out();

	return rc;
}

uint8_t write_proximity_config(uint8_t instance, touch_proximity_t23_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;

	dbg_func_in();

	object_size = get_object_size(TOUCH_PROXIMITY_T23);
	if (object_size == 0)
		return(CFG_WRITE_FAILED);

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);
	if (tmp == NULL)
		return(CFG_WRITE_FAILED);

	memset(tmp,0,object_size);

	*(tmp + 0) = cfg.ctrl;
	*(tmp + 1) = cfg.xorigin;
	*(tmp + 2) = cfg.yorigin;
	*(tmp + 3) = cfg.xsize;
	*(tmp + 4) = cfg.ysize;
	*(tmp + 5) = cfg.reserved_for_future_aks_usage;
	*(tmp + 6) = cfg.blen;
	*(tmp + 7) = (uint8_t) (cfg.fxddthr & 0x00FF);
	*(tmp + 8) = (uint8_t) (cfg.fxddthr >> 8);
	*(tmp + 9) = cfg.fxddi;
	*(tmp + 10) = cfg.average;
	*(tmp + 11) = (uint8_t) (cfg.mvnullrate & 0x00FF);
	*(tmp + 12) = (uint8_t) (cfg.mvnullrate >> 8);
	*(tmp + 13) = (uint8_t) (cfg.mvdthr & 0x00FF);
	*(tmp + 14) = (uint8_t) (cfg.mvdthr >> 8);

	object_address = get_object_address(TOUCH_PROXIMITY_T23,instance);

	if (object_address == 0)
		return(CFG_WRITE_FAILED);

	status = write_mem(object_address, object_size, tmp);

	kfree(tmp);
	dbg_func_out();
	return(status);
}

uint8_t write_selftest_config(uint8_t instance, spt_selftest_t25_config_t cfg)
{

	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;

	dbg_func_in();

	object_size = get_object_size(SPT_SELFTEST_T25);
	if (object_size == 0)
		return(CFG_WRITE_FAILED);

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);

	if (tmp == NULL)
		return(CFG_WRITE_FAILED);

	memset(tmp,0,object_size);

	*(tmp + 0) = cfg.ctrl;
	*(tmp + 1) = cfg.cmd;
	object_address = get_object_address(SPT_SELFTEST_T25,instance);

	if (object_address == 0)
		return(CFG_WRITE_FAILED);

	status = write_mem(object_address, object_size, tmp);

	kfree(tmp);
	dbg_func_out();
	return(status);
}

uint8_t write_gripsuppression_T40_config(proci_gripsuppression_t40_config_t cfg)
{
	return(write_simple_config(PROCI_GRIPSUPPRESSION_T40, 0, (void *) &cfg));
}

uint8_t write_touchsuppression_t42_config(proci_touchsuppression_t42_config_t cfg)
{
	return(write_simple_config(PROCI_TOUCHSUPPRESSION_T42, 0, (void *) &cfg));
}

uint8_t write_CTE_T46_config(spt_cteconfig_t46_config_t cfg)
{
	return(write_simple_config(SPT_CTECONFIG_T46, 0, (void *) &cfg));
}

uint8_t  write_stylus_t47_config(proci_stylus_t47_config_t cfg)
{
	return(write_simple_config(PROCI_STYLUS_T47, 0, (void *) &cfg));
}

uint8_t  write_noisesuppression_t48_config(procg_noisesuppression_t48_config_t cfg)
{
	uint16_t object_address;
	uint8_t *tmp;
	uint8_t status;
	uint8_t object_size;

	object_size = get_object_size(PROCG_NOISESUPPRESSION_T48);
	if (object_size == 0)
		return(CFG_WRITE_FAILED);

	tmp = (uint8_t *) kmalloc(object_size, GFP_KERNEL | GFP_ATOMIC);
	if (tmp == NULL)
		return(CFG_WRITE_FAILED);

	memset(tmp,0,object_size);
	/* 18 elements at beginning are 1 byte. */
	memcpy(tmp, &cfg, 15);

	/* Next two are 2 bytes. */
	*(tmp + 15) = (uint8_t) (cfg.gcinvalidthr &  0xFF);
	*(tmp + 16) = (uint8_t) (cfg.gcinvalidthr >> 8);
	*(tmp + 17) = cfg.gcmaxadcsperx;
	*(tmp + 18) = cfg.gclimitmin;
	*(tmp + 19) = cfg.gclimitmax;
	*(tmp + 20) = (uint8_t) (cfg.gccountmintgt &  0xFF);
	*(tmp + 21) = (uint8_t) (cfg.gccountmintgt >> 8);
	*(tmp + 22) = (uint8_t) cfg.mfinvlddiffthr;
	*(tmp + 23) = (uint8_t) (cfg.mfincadcspxthr &  0xFF);
	*(tmp + 24) = (uint8_t) (cfg.mfincadcspxthr >> 8);
	*(tmp + 25) = (uint8_t) (cfg.mferrorthr &  0xFF);
	*(tmp + 26) = (uint8_t) (cfg.mferrorthr >> 8);
	*(tmp + 27) = cfg.selfreqmax;
	*(tmp + 28) = cfg.reserved9;
	*(tmp + 29) = cfg.reserved10;
	*(tmp + 30) = cfg.reserved11;
	*(tmp + 31) = cfg.reserved12;
	*(tmp + 32) = cfg.reserved13;
	*(tmp + 33) = cfg.reserved14;
	*(tmp + 34) = cfg.blen;
	*(tmp + 35) = cfg.tchthr; 
	*(tmp + 36) = cfg.tchdi;
	*(tmp + 37) = cfg.movhysti;
	*(tmp + 38) = cfg.movhystn;
	*(tmp + 39) = cfg.movfilter;
	*(tmp + 40) = cfg.numtouch;
	*(tmp + 41) = cfg.mrghyst;
	*(tmp + 42) = cfg.mrgthr;
	*(tmp + 43) = cfg.xloclip;
	*(tmp + 44) = cfg.xhiclip;
	*(tmp + 45) = cfg.yloclip;
	*(tmp + 46) = cfg.yhiclip;
	*(tmp + 47) = cfg.xedgectrl;
	*(tmp + 48) = cfg.xedgedist;
	*(tmp + 49) = cfg.yedgectrl;
	*(tmp + 50) = cfg.yedgedist;
	*(tmp + 51) = cfg.jumplimit;
	*(tmp + 52) = cfg.tchhyst; 
	*(tmp + 53) = cfg.nexttchdi;

	object_address = get_object_address(PROCG_NOISESUPPRESSION_T48, 0);

	if (object_address == 0)
		return(CFG_WRITE_FAILED);

	status = write_mem(object_address, object_size, tmp);
	kfree(tmp);
	return(status);
}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint8_t write_simple_config(uint8_t object_type, uint8_t instance, void *cfg)
{
	uint16_t object_address;
	uint8_t object_size;
	uint8_t rc;

	dbg_func_in();

	object_address = get_object_address(object_type, instance);
	object_size = get_object_size(object_type);

	if ((object_size == 0) || (object_address == 0))
	{
		rc = CFG_WRITE_FAILED;
	}
	else
	{
		rc = write_mem(object_address, object_size, cfg);
	}

	dbg_func_out();
	return rc; 
}



/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint8_t get_object_size(uint8_t object_type)
{
	uint8_t object_table_index = 0;
	uint8_t object_found = 0;
	uint16_t size = OBJECT_NOT_FOUND;
	object_t *object_table;
	object_t obj;

	dbg_func_in();

	if(info_block == NULL)		
		return 0;

	object_table = info_block->objects;
	while ((object_table_index < info_block->info_id->num_declared_objects) &&
			!object_found)
	{
		obj = object_table[object_table_index];
		/* Does object type match? */
		if (obj.object_type == object_type){
			object_found = 1;
			size = obj.size + 1;
		}
		object_table_index++;
	}

	dbg_func_out();
	return(size);
}

/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint8_t type_to_report_id(uint8_t object_type, uint8_t instance)
{

	uint8_t report_id = 1;
	int8_t report_id_found = 0;
	uint8_t rc;

	dbg_func_in();

	while((report_id <= max_report_id) && (report_id_found == 0))
	{
		if((report_id_map[report_id].object_type == object_type) &&
				(report_id_map[report_id].instance == instance))
		{
			report_id_found = 1;
		}
		else
		{
			report_id++;
		}
	}
	if (report_id_found){
		rc = report_id;
	}
	else{
		rc = ID_MAPPING_FAILED;
	}

	dbg_func_out();

	return rc;
}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint8_t report_id_to_type(uint8_t report_id, uint8_t *instance)
{
	uint8_t rc;

	dbg_func_in();

	if (report_id <= max_report_id){
		*instance = report_id_map[report_id].instance;
		rc = report_id_map[report_id].object_type;
	}
	else{
		rc = ID_MAPPING_FAILED;
	}

	dbg_func_out();

	return rc;
}



/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint8_t read_id_block(info_id_t *id)
{
	uint8_t status;

	dbg_func_in();

	status = read_mem(0, 1, (void *) &id->family_id);
	if (status != READ_MEM_OK) goto read_id_block_exit;
	dbg("family_id = %d\n",id->family_id);

	status = read_mem(1, 1, (void *) &id->variant_id);
	if (status != READ_MEM_OK) goto read_id_block_exit;
	dbg("variant_id = %d\n",id->variant_id);

	status = read_mem(2, 1, (void *) &id->version);
	if (status != READ_MEM_OK) goto read_id_block_exit;
	dbg("version = %d\n",id->version);

	status = read_mem(3, 1, (void *) &id->build);
	if (status != READ_MEM_OK) goto read_id_block_exit;
	dbg("familybuild_id = %d\n",id->build);

	status = read_mem(4, 1, (void *) &id->matrix_x_size);
	if (status != READ_MEM_OK) goto read_id_block_exit;
	dbg("matrix_x_size = %d\n",id->matrix_x_size);

	status = read_mem(5, 1, (void *) &id->matrix_y_size);
	if (status != READ_MEM_OK) goto read_id_block_exit;
	dbg("matrix_y_size = %d\n",id->matrix_y_size);

	status = read_mem(6, 1, (void *) &id->num_declared_objects);

	dbg_func_out();

	return status;

read_id_block_exit:

	dbg("error : read_id_block_exit\n");
	dbg_func_out();
	return status;
}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint16_t get_object_address(uint8_t object_type, uint8_t instance)
{
	uint8_t object_table_index = 0;
	uint8_t address_found = 0;
	uint16_t address = OBJECT_NOT_FOUND;
	object_t *object_table;
	object_t obj;

	dbg_func_in();

	if(info_block == NULL)		
		return 0;

	object_table = info_block->objects;
	while ((object_table_index < info_block->info_id->num_declared_objects) &&
			!address_found)
	{
		obj = object_table[object_table_index];
		/* Does object type match? */
		if (obj.object_type == object_type){

			address_found = 1;

			/* Are there enough instances defined in the FW? */
			if (obj.instances >= instance){
				address = obj.i2c_address + (obj.size + 1) * instance;
			}
		}
		object_table_index++;
	}

	dbg_func_out();
	return(address);
}



/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint32_t get_stored_infoblock_crc()
{
	uint32_t crc;

	dbg_func_in();

	crc = (uint32_t) (((uint32_t) info_block->CRC_hi) << 16);
	crc = crc | info_block->CRC;

	dbg_func_out();
	return(crc);
}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint8_t calculate_infoblock_crc(uint32_t *crc_pointer)
{

	uint32_t crc = 0;
	uint16_t crc_area_size;
	uint8_t *mem;
	uint8_t i;
	uint8_t rc;

	uint8_t status;

	dbg_func_in();

	rc = CRC_CALCULATION_OK;

	/* 7 bytes of version data, 6 * NUM_OF_OBJECTS bytes of object table. */
	crc_area_size = 7 + info_block->info_id->num_declared_objects * 6;

	mem = (uint8_t *) kmalloc(crc_area_size, GFP_KERNEL | GFP_ATOMIC);
	if (mem == NULL){
		rc = CRC_CALCULATION_FAILED;
		goto calculate_infoblock_crc_exit;
	}

	status = read_mem(0, crc_area_size, mem);
	if (status != READ_MEM_OK){
		rc = CRC_CALCULATION_FAILED;
		goto calculate_infoblock_crc_exit;
	}

	i = 0;
	while (i < (crc_area_size - 1))
	{
		crc = CRC_24(crc, *(mem + i), *(mem + i + 1));
		i += 2;
	}

	crc = CRC_24(crc, *(mem + i), 0);

	kfree(mem);

	/* Return only 24 bit CRC. */
	*crc_pointer = (crc & 0x00FFFFFF);

	dbg_func_out();
	return rc;

calculate_infoblock_crc_exit:
	dbg("error : calculate_infoblock_crc\n");
	dbg_func_out();
	return rc;
}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

uint32_t CRC_24(uint32_t crc, uint8_t byte1, uint8_t byte2)
{
	static const uint32_t crcpoly = 0x80001B;
	uint32_t result;
	uint16_t data_word;

	dbg_func_in();

	data_word = (uint16_t) ((uint16_t) (byte2 << 8u) | byte1);
	result = ((crc << 1u) ^ (uint32_t) data_word);

	if (result & 0x1000000)
	{
		result ^= crcpoly;
	}

	dbg_func_out();

	return(result);
}

void touch_data_init(void)
{
	int i = 0;
	dbg_func_in();

	for (i = 0; i<MAX_NUM_FINGER; i++ )
	{
		fingerInfo[i].mode = TSC_EVENT_NONE;
		fingerInfo[i].status = -1;
		fingerInfo[i].area = 0;
		keyInfo[i].update = false;
		input_mt_slot(qt602240_data->input_dev, i);						// TOUCH_ID_SLOT
		input_report_abs(qt602240_data->input_dev, ABS_MT_TRACKING_ID, -1);		// RELEASE TOUCH_ID					
	}
	input_report_key(qt602240_data->input_dev, BTN_TOUCH, 0);  // mirinae_ICS
	input_sync(qt602240_data->input_dev);
	active_event = true;
	is_cal_success = false;

	dbg_func_out();
}

/*------------------------------ main block -----------------------------------*/
void quantum_touch_probe(void)
{
	U8 report_id;
	U8 object_type, instance;
	uint32_t crc, stored_crc;
#ifdef CHIP_NOINIT
	uint16_t object_address;
	uint16_t xres, yres;
	U8 xysize[3],  status;
#endif

	dbg_func_in();

	if (init_touch_driver() == DRIVER_SETUP_OK) {
		dbg("\n[TSP] Touch device found\n");
	}
	else {
		dbg("\n[TSP][ERROR] Touch device NOT found\n");
		return ;
	}

	/* Get and show the version information. */
	if(calculate_infoblock_crc(&crc) != CRC_CALCULATION_OK) {
		dbg("Calculating CRC failed, skipping check!\n\r");
	}
	else {
		dbg("Calculated CRC:\t\n");
		write_message_to_usart((uint8_t *) &crc, 4);
		dbg("\n\r");
	}

	stored_crc = get_stored_infoblock_crc();
	dbg("Stored CRC:\t");
	write_message_to_usart((uint8_t *) &stored_crc, 4);
	dbg("\n\r");

	if (stored_crc != crc) {
		dbg("Warning: info block CRC value doesn't match the calculated!\n\r");
	}
	else {
		dbg("Info block CRC value OK.\n\r");
	}

	/* Test the report id to object type / instance mapping: get the maximum
	 * report id and print the report id map. */

	dbg("Report ID to Object type / instance mapping:\n\r");

	for (report_id = 0; report_id <= max_report_id; report_id++) {
		object_type = report_id_to_type(report_id, &instance);
		dbg("Report ID : %d, Object Type : T%d, Instance : %d\n",report_id, object_type, instance);
	}

#ifdef CHIP_NOINIT
	object_address = get_object_address(TOUCH_MULTITOUCHSCREEN_T9,0);

	status = read_U16(object_address+18, &xres);
	status = read_U16(object_address+20, &yres);
	status = read_mem(object_address+3, 2, (U8 *) xysize);
	dbg("[TSP] Read Chip xyInfo : xres(%d), yres(%d), xsize(%d), ysize(%d)\n\r",xres , yres, xysize[0], xysize[1]);

	if(Chip_NoInit == false){
		if(!((yres == (SCREEN_RESOLUTION_X-1) || yres == SCREEN_RESOLUTION_X) && xysize[0] == T9_XSIZE && xysize[1] == T9_YSIZE))
			Chip_NoInit = true;
	}    
#endif

	reset_touch_config();

	/* Backup settings to NVM. */
	if (backup_config() != WRITE_MEM_OK){
		dbg("Failed to backup, exiting...\n");
		return;
	}
	else{
		dbg("Backed up the config to non-volatile memory!\n");
	}

	/* Calibrate the touch IC. */
	if (calibrate_chip() != WRITE_MEM_OK){
		dbg("Failed to calibrate, exiting...\n");
		return;
	}
	else{
		dbg("Chip calibrated!\n");
	}

	touch_data_init();

	charger_mode = -1;
	pre_charger_mode = -1;

	dbg_func_out();
	dbg("Waiting for touch chip messages...\n");
}

/*------------------------------ Sub functions -----------------------------------*/
/*!
  \brief Initializes touch driver.

  This function initializes the touch driver: tries to connect to given 
  address, sets the message handler pointer, reads the info block and object
  table, sets the message processor address etc.

  @param I2C_address is the address where to connect.
  @param (*handler) is a pointer to message handler function.
  @return DRIVER_SETUP_OK if init successful, DRIVER_SETUP_FAILED 
  otherwise.
 */
uint8_t init_touch_driver(void)
{
	int i;
	int current_report_id = 0;
	uint8_t tmp;
	uint16_t current_address;
	uint16_t crc_address;
	object_t *object_table;
	info_id_t *id;
	uint32_t *CRC;
	uint8_t status;

	dbg_func_in();

	// P13106 max_report_id values is changed because memory leaks 120130
	max_report_id = 0;

	// To avoid memory leaks	P13106 120130
	if(info_block){
		dbg("[TSP] info_block is not NULL\n");
		if(info_block->info_id){
			dbg("[TSP] info_block->info_id is not NULL\n");
			kfree(info_block->info_id);
		}
		if(info_block->objects){
			dbg("[TSP] info_block->objects is not NULL\n");
			kfree(info_block->objects);
		}		
		kfree(info_block);
	}
	if(quantum_msg){
		dbg("[TSP] quantum_msg is not NULL\n");
		kfree(quantum_msg);
	}
	if(report_id_map){
		dbg("[TSP] report_id_map is not NULL\n");
		kfree(report_id_map);
	}
	// To avoid memory leaks	P13106 120130

	/* Read the info block data. */
	id = (info_id_t *) kmalloc(sizeof(info_id_t), GFP_KERNEL | GFP_ATOMIC);
	if (id == NULL)
		return(DRIVER_SETUP_INCOMPLETE);

	if (read_id_block(id) != 1){
		dbg("[TSP][ERROR] can't read info block data.\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}  

	/* Read object table. */
	object_table = (object_t *) kmalloc(id->num_declared_objects * sizeof(object_t), GFP_KERNEL | GFP_ATOMIC);
	if (object_table == NULL){
		dbg("[TSP][ERROR] 3\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Reading the whole object table block to memory directly doesn't work cause sizeof object_t isn't necessarily the same on every compiler/platform due to alignment issues. Endianness can also cause trouble. */
	current_address = OBJECT_TABLE_START_ADDRESS;

	for (i = 0; i < id->num_declared_objects; i++)
	{
		status = read_mem(current_address, 1, &(object_table[i]).object_type);
		if (status != READ_MEM_OK){
			dbg("[TSP][ERROR] 4\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		status = read_U16(current_address, &object_table[i].i2c_address);
		if (status != READ_MEM_OK){
			dbg("[TSP][ERROR] 5\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address += 2;

		status = read_mem(current_address, 1, (U8*)&object_table[i].size);
		if (status != READ_MEM_OK){
			dbg("[TSP][ERROR] 6\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		status = read_mem(current_address, 1, &object_table[i].instances);
		if (status != READ_MEM_OK){
			dbg("[TSP][ERROR] 7\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		status = read_mem(current_address, 1, &object_table[i].num_report_ids);
		if (status != READ_MEM_OK){
			dbg("[TSP][ERROR] 8\n");
			return(DRIVER_SETUP_INCOMPLETE);
		}
		current_address++;

		max_report_id += object_table[i].num_report_ids;

		/* Find out the maximum message length. */
		if (object_table[i].object_type == GEN_MESSAGEPROCESSOR_T5){
			max_message_length = object_table[i].size + 1;
		}
	}

	/* Check that message processor was found. */
	if (max_message_length == 0){
		dbg("[TSP][ERROR] 9\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Read CRC. */
	CRC = (uint32_t *) kmalloc(sizeof(info_id_t), GFP_KERNEL | GFP_ATOMIC);
	if (CRC == NULL){
		dbg("[TSP][ERROR] 10\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	info_block = kmalloc(sizeof(info_block_t), GFP_KERNEL | GFP_ATOMIC);
	if (info_block == NULL){
		dbg("err\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	info_block->info_id = id;
	info_block->objects = object_table;
	crc_address = OBJECT_TABLE_START_ADDRESS + id->num_declared_objects * OBJECT_TABLE_ELEMENT_SIZE;

	status = read_mem(crc_address, 1u, &tmp);
	if (status != READ_MEM_OK){
		dbg("[TSP][ERROR] 11\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}
	info_block->CRC = tmp;

	status = read_mem(crc_address + 1u, 1u, &tmp);
	if (status != READ_MEM_OK){
		dbg("[TSP][ERROR] 12\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}
	info_block->CRC |= (tmp << 8u);

	status = read_mem(crc_address + 2, 1, &info_block->CRC_hi);
	if (status != READ_MEM_OK){
		dbg("[TSP][ERROR] 13\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Store message processor address, it is needed often on message reads. */
	message_processor_address = get_object_address(GEN_MESSAGEPROCESSOR_T5, 0);
	if (message_processor_address == 0){
		dbg("[TSP][ERROR] 14 !!\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Store command processor address. */
	command_processor_address = get_object_address(GEN_COMMANDPROCESSOR_T6, 0);
	if (command_processor_address == 0){
		dbg("[TSP][ERROR] 15\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	quantum_msg = kmalloc(max_message_length, GFP_KERNEL | GFP_ATOMIC);
	if (quantum_msg == NULL){
		dbg("[TSP][ERROR] 16\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Allocate memory for report id map now that the number of report id's 
	 * is known. */

	report_id_map = kmalloc(sizeof(report_id_map_t) * max_report_id, GFP_KERNEL | GFP_ATOMIC);

	if (report_id_map == NULL){
		dbg("[TSP][ERROR] 17\n");
		return(DRIVER_SETUP_INCOMPLETE);
	}

	/* Report ID 0 is reserved, so start from 1. */

	report_id_map[0].instance = 0;
	report_id_map[0].object_type = 0;
	current_report_id = 1;

	for (i = 0; i < id->num_declared_objects; i++)
	{
		if (object_table[i].num_report_ids != 0)
		{
			int instance;
			for (instance = 0; instance <= object_table[i].instances; instance++)
			{
				int start_report_id = current_report_id;
				for (; current_report_id < (start_report_id + object_table[i].num_report_ids); current_report_id++)
				{
					report_id_map[current_report_id].instance = instance;
					report_id_map[current_report_id].object_type = object_table[i].object_type;
				}
			}
		}
	}
	driver_setup = DRIVER_SETUP_OK;

	/* Initialize the pin connected to touch ic pin CHANGELINE to catch the
	 * falling edge of signal on that line. */

	// To avoid memory leaks	P13106 120130
	kfree(CRC);
	// To avoid memory leaks	P13106 120130	

	dbg_func_out();
	return(DRIVER_SETUP_OK);
}


void  clear_event(uint8_t clear)
{
	uint8_t valid_input_count=0;
	int i;   

	dbg_func_in();
	for ( i= 0; i<MAX_NUM_FINGER; i++ )
	{
		if(fingerInfo[i].mode == TSC_EVENT_WINDOW)
		{
			dbg_CR("[QT602240] clear_event U:(%d, %d) (id:%d)\n", fingerInfo[i].x, fingerInfo[i].y, fingerInfo[i].id);
			input_mt_slot(qt602240_data->input_dev, fingerInfo[i].id);						// TOUCH_ID_SLOT
			input_report_abs(qt602240_data->input_dev, ABS_MT_TRACKING_ID, -1);		// RELEASE TOUCH_ID
			fingerInfo[i].mode = TSC_EVENT_NONE;
			fingerInfo[i].status= -1;
		}
		else{
			valid_input_count++;
		}
	}
	input_report_key(qt602240_data->input_dev, BTN_TOUCH, 0);  // mirinae_ICS
	dbg("[TOUCH] touch event num => %d\n",valid_input_count);
	input_sync(qt602240_data->input_dev);

	if(clear == TSC_CLEAR_ALL)
	{
		for ( i= 0; i<MAX_NUM_FINGER; i++ )
		{
			fingerInfo[i].mode = TSC_EVENT_NONE;
			fingerInfo[i].status = -1;
			fingerInfo[i].area = 0;
			keyInfo[i].update = false; 
		}     
	}
	dbg_func_out();
}

void cal_maybe_good(void)
{    
	//	int ret;
	dbg_func_in();

	/* Check if the timer is enabled */
	if (good_calibration_cnt >= 1) {
		dbg_CR("[TSP] good_calibration_cnt = %d \n", good_calibration_cnt);
		/* Check if the timer timedout of 0.3seconds */
		if ((jiffies_to_msecs(jiffies) - mxt_time_point) >= 300) {
			dbg_CR("[TSP] time from touch press after calibration started = %d\n", (jiffies_to_msecs(jiffies) - mxt_time_point));
			/* cal was good - don't need to check any more */
			mxt_time_point = 0;
			cal_check_flag = 0;

			/* Disable the timer */
			cal_correction_limit = 5;

			/* Write back the normal acquisition config to chip. */
			acquisition_config.atchcalst = T8_ATCHCALST/*2*/;
			acquisition_config.atchcalsthr = T8_ATCHCALSTHR/*50*/;
			acquisition_config.atchfrccalthr = T8_ATCHFRCCALTHR;     /*!< Anti-touch force calibration threshold */
			acquisition_config.atchfrccalratio = T8_ATCHFRCCALRATIO;  /*!< Anti-touch force calibration ratio */  
			dbg("[TSP] write acq atchcalst=%d, atchcalsthr=%d\n", acquisition_config.atchcalst, acquisition_config.atchcalsthr );
			if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK)
			{
				/* "Acquisition config write failed!\n" */
				dbg("\n[TSP][ERROR] line : %d\n", __LINE__);
				//ret = WRITE_MEM_FAILED; /* calling function should retry calibration call */
			}

			dbg_CR("[TSP] T6 Calibration success!! \n");
		}
		else { 
			cal_check_flag = 1u;
			good_calibration_cnt++;
		}
	}
	else { 
		/* Timer not enabled, so enable it */
		cal_check_flag = 1u;
		good_calibration_cnt++;
	}   

	dbg_func_out();

}


/* Calibration Checking routine - called from interrupt handler */

/*!
 * \brief Used to ensure that calibration was good
 *
 * This function will check if the last calibration was good.
 * 
 * It should be called on every touch message whilst 'cal_check_flag' is set,
 * it will recalibrate the chip if the calibration is bad. If the calibration
 * is good it will restore the ATCHCALST and ATCHCALSTHR settings in the chip 
 * so that we do not get water issues.
 *
 *
 */
int8_t check_chip_calibration(void)
{
	uint8_t tch_ch = 0, atch_ch = 0;
	uint8_t i;
	uint8_t CAL_THR = 10;
	uint8_t num_of_antitouch = 1;
	uint8_t finger_cnt = 0;

	dbg_func_in();
	if(get_touch_antitouch_info()) {
		dbg_CR("[TSP] tch_ch:%d, atch_ch:%d\n",debugInfo.tch_ch, debugInfo.atch_ch);
		tch_ch = debugInfo.tch_ch;
		atch_ch = debugInfo.atch_ch;
		/* process counters and decide if we must re-calibrate or if cal was good */      

		// Count fingers
		for (i = 0 ; i < MAX_NUM_FINGER; ++i) { 
			if (fingerInfo[i].status == -1)
				continue;
			finger_cnt++;
		}

		if (cal_check_flag != 1) {
			dbg("[TSP] check_chip_calibration just return!! finger_cnt = %d\n", finger_cnt);
			return 0;
		}

		/* process counters and decide if we must re-calibrate or if cal was good */ 

		// Palm 
		if ((tch_ch > 18) || (atch_ch > 15) || ((tch_ch + atch_ch) > 20)) {  
			dbg_CR("[TSP] maybe palm, re-calibrate!! \n");
			calibrate_chip();
		} 
		// Good 
		else if((tch_ch > 0) && (tch_ch <= 18) && (atch_ch == 0)) {
			dbg_CR("[TSP] calibration maybe good\n");
			if ((finger_cnt >= 2) && (tch_ch <= 3)) {
				dbg_CR("[TSP]finger_cnt = %d, re-calibrate!! \n", finger_cnt);	
				calibrate_chip();
			} else {
				cal_maybe_good();
				not_yet_count = 0;
			}
		}
		// When # of antitouch is large related to finger cnt	
		else if (atch_ch > ((finger_cnt * num_of_antitouch) + 5)) { //2 
			dbg_CR("[TSP] calibration was bad (finger : %d, max_antitouch_num : %d)\n", finger_cnt, finger_cnt*num_of_antitouch);
			calibrate_chip();
		} 
		// When atch > tch 
		else if((tch_ch + CAL_THR) <= atch_ch) { 
			dbg_CR("[TSP] calibration was bad (CAL_THR : %d)\n",CAL_THR);
			/* cal was bad - must recalibrate and check afterwards */
			calibrate_chip();
		} 
		// No touch, only antitouch
		else if((tch_ch == 0 ) && (atch_ch >= 2)) { 
			dbg_CR("[TSP] calibration was bad, tch_ch = %d, atch_ch = %d)\n", tch_ch, atch_ch);
			/* cal was bad - must recalibrate and check afterwards */
			calibrate_chip();
		} 
		else if((tch_ch == 0 ) && (atch_ch ==0)) { 
			dbg_CR("[TSP] calibration was good, tch_ch = %d, atch_ch = %d)\n", tch_ch, atch_ch);
			cal_maybe_good();
			not_yet_count = 0;
		}
		// Else. 
		else { 
			not_yet_count++;
			dbg_CR("[TSP] calibration was not decided yet, not_yet_count = %d\n", not_yet_count);
			if(not_yet_count >= 2) {		// retry count 30 -> 2
				dbg_CR("[TSP] not_yet_count over 2, re-calibrate!! \n");
				not_yet_count =0;
				calibrate_chip();
			}
		}
	}
	dbg_func_out();
	return 0;
}

int8_t get_touch_antitouch_info(void) {
	uint8_t data_buffer[100] = { 0 };
	uint8_t try_ctr = 0;
	uint8_t data_byte = 0xF3; /* dianostic command to get touch flags */
	uint16_t diag_address;
	uint8_t tch_ch = 0, atch_ch = 0;
	uint8_t check_mask;
	uint8_t i;
	uint8_t j;
	uint8_t x_line_limit;

	dbg_func_in();

	if (driver_setup != DRIVER_SETUP_OK)
		return -1;

	/* we have had the first touchscreen or face suppression message 
	 * after a calibration - check the sensor state and try to confirm if
	 * cal was good or bad */

	/* get touch flags from the chip using the diagnostic object */
	/* write command to command processor to get touch flags - 0xF3 Command required to do this */
	write_mem(command_processor_address + DIAGNOSTIC_OFFSET, 1, &data_byte);
	/* get the address of the diagnostic object so we can get the data we need */
	diag_address = get_object_address(DEBUG_DIAGNOSTIC_T37,0);

	msleep(10); 

	/* read touch flags from the diagnostic object - clear buffer so the while loop can run first time */
	memset( data_buffer , 0xFF, sizeof( data_buffer ) );

	/* wait for diagnostic object to update */
	while(!((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00)))
	{
		/* wait for data to be valid  */
		if(try_ctr > 10) {
			/* Failed! */
			printk("[TSP][ERROR] Diagnostic Data did not update!!\n");
			break;
		}
		msleep(10); 
		try_ctr++; /* timeout counter */
		read_mem(diag_address, 2,data_buffer);
		//dbg("[TSP] Waiting for diagnostic data to update, try %d\n", try_ctr);
	}

	/* data is ready - read the detection flags */
	read_mem(diag_address, 100, data_buffer);

	/* data array is 20 x 16 bits for each set of flags, 2 byte header, 40 bytes for touch flags 40 bytes for antitouch flags*/

	/* count up the channels/bits if we recived the data properly */
	if((data_buffer[0] == 0xF3) && (data_buffer[1] == 0x00))
	{

		/* mode 0 : 16 x line, mode 1 : 17 etc etc upto mode 4.*/
		x_line_limit = 16 + cte_t46_config.mode;

		if(x_line_limit > 20)
		{
			/* hard limit at 20 so we don't over-index the array */
			x_line_limit = 20;
		}

		/* double the limit as the array is in bytes not words */
		x_line_limit = x_line_limit << 1;

		/* count the channels and print the flags to the log */
		for(i = 0; i < x_line_limit; i+=2) /* check X lines - data is in words so increment 2 at a time */
		{
			/* print the flags to the log - only really needed for debugging */
			dbg("[TSP] Detect Flags X%d, %x%x, %x%x \n", i>>1,data_buffer[3+i],data_buffer[2+i],data_buffer[47+i],data_buffer[46+i]);

			/* count how many bits set for this row */
			for(j = 0; j < 8; j++)
			{
				/* create a bit mask to check against */
				check_mask = 1 << j;

				/* check detect flags */
				if(data_buffer[2+i] & check_mask){
					tch_ch++;
				}
				if(data_buffer[3+i] & check_mask){
					tch_ch++;
				}

				/* check anti-detect flags */
				if(data_buffer[46+i] & check_mask){
					atch_ch++;
				}
				if(data_buffer[47+i] & check_mask){
					atch_ch++;
				}
			}
		}

		/* print how many channels we counted */
		dbg("[TSP] Flags Counted channels: t:%d a:%d \n", tch_ch, atch_ch);

		/* send page up command so we can detect when data updates next time,
		 * page byte will sit at 1 until we next send F3 command */
		data_byte = 0x01;
		write_mem(command_processor_address + DIAGNOSTIC_OFFSET, 1, &data_byte);

		debugInfo.tch_ch = tch_ch;
		debugInfo.atch_ch = atch_ch;

		return 1;
	}

	dbg_func_out();

	return 0;	
}

#ifdef PROTECTION_MODE
static void protection_mode_timer_func(unsigned long data)
{
	queue_work(qt602240_wq, &qt602240_data->work_protection_mode_disable);
}
static void tchautocal_disable_timer_func(unsigned long data)
{
	queue_work(qt602240_wq, &qt602240_data->work_tchautocal_disable);
}
#endif 


void  get_message(struct work_struct * p)
{
	unsigned long x, y;
	unsigned int touch_status = 0;
	uint8_t ret_val = MESSAGE_READ_FAILED;
	int size=0;
	uint8_t touch_message_flag = 0;
	uint8_t object_type, instance;
	uint8_t id = 0;
#ifdef READ_ERROR_RETRY	
	uint8_t read_fail_cnt = 0;
#endif
	dbg_func_in();

	/* Get the lock */
	mutex_lock(&qt602240_data->lock);

	if (driver_setup != DRIVER_SETUP_OK)
		goto interrupt_return;

#ifdef READ_ERROR_RETRY
	if(read_mem(message_processor_address, max_message_length, quantum_msg) != READ_MEM_OK) 
	{
		do{
			read_fail_cnt++;
			printk("[TSP] read_mem failed cnt=%d.\n", read_fail_cnt);	

			if(read_fail_cnt>3)
			{
				printk("[TSP] Hard Reset \n");

				read_fail_cnt=0;		  
				clear_event(TSC_CLEAR_ALL);	
				gpio_set_value(GPIO_TOUCH_POWER, 0);
				msleep(20);	  
				gpio_set_value(GPIO_TOUCH_POWER, 1);
				msleep(100); 
				goto interrupt_return;
			}
			msleep(20);
		} while(read_mem(message_processor_address, max_message_length, quantum_msg) != READ_MEM_OK);
	}
#else
	if(read_mem(message_processor_address, max_message_length, quantum_msg) != READ_MEM_OK) {
		printk("[TSP] read_mem failed.\n");
		goto interrupt_return;
	}
#endif

	/* Call the main application to handle the message. */
	dbg("[TSP] msg id =  %d, msg = %x\n", quantum_msg[0], quantum_msg[1]);
	object_type = report_id_to_type(quantum_msg[0], &instance);

	if (object_type == TOUCH_MULTITOUCHSCREEN_T9)
	{
		id= quantum_msg[0] - 2;
		/* Detect & Press */
		if( (( quantum_msg[1] & (TOUCH_DETECT|TOUCH_PRESS)) == (TOUCH_DETECT|TOUCH_PRESS))) {
			//touch_message_flag = 1;					
			ktime_get_ts(&ts); //TOUCH_REACTION_RATE
			dbg("[TSP] Down Interrupt : (msg id =  %d, msg = %x)\n", quantum_msg[0], quantum_msg[1]);
		}

		x = (quantum_msg[2] << 2) | (quantum_msg[4] >> 6);

#if defined (CONFIG_MACH_MSM8960_EF45K) ||defined (CONFIG_MACH_MSM8960_EF46L) || defined (CONFIG_MACH_MSM8960_EF47S) || \
		defined (CONFIG_MACH_MSM8960_EF44S)
		//if y resolution is bigger than 1024
		y = (quantum_msg[3] << 4) | (quantum_msg[4] & 0xf);
#else
		y = (quantum_msg[3] << 2) | ((quantum_msg[4] & 0x6) >> 2);
#endif
		size = quantum_msg[5];
		if ( (quantum_msg[1] & TOUCH_RELEASE) )    
		{
			if(f_resume){
				dbg_CR("[TOUCH] TOUCH Release event but,debugInfo.autocal_flag is true id => %d \n",id);
				goto interrupt_return;
			}
			fingerInfo[id].status= TOUCH_EVENT_RELEASE;
			touch_status=TOUCH_EVENT_RELEASE;
			fingerInfo[id].area= 0;
			touch_message_flag = 1;
			dbg_CR("##### Finger[%d] Up (%d,%d)\n", id, fingerInfo[id].x, fingerInfo[id].y );
		}
		else if ( (quantum_msg[1] & TOUCH_DETECT) && (quantum_msg[1] & TOUCH_MOVE) )  
		{
			if(f_resume){
				dbg_CR("[TOUCH] TOUCH Move event but,debugInfo.autocal_flag is true id => %d \n",id);
				goto interrupt_return;
			}
			fingerInfo[id].id= id;
			fingerInfo[id].status= TOUCH_EVENT_MOVE;
			touch_status=TOUCH_EVENT_MOVE;
			fingerInfo[id].x= (int16_t)x;
			fingerInfo[id].y= (int16_t)y;
			fingerInfo[id].area= size;
			dbg("##### Finger[%d] Move (%d,%d)!\n", id, fingerInfo[id].x, fingerInfo[id].y );
		}
		else if ( (quantum_msg[1] & TOUCH_DETECT) && (quantum_msg[1] & TOUCH_PRESS) )       
		{                               
			if(f_resume) f_resume=false;
			fingerInfo[id].id= id;
			fingerInfo[id].status= TOUCH_EVENT_PRESS;
			touch_status=TOUCH_EVENT_PRESS;
			fingerInfo[id].x= (int16_t)x;
			fingerInfo[id].y= (int16_t)y;
			fingerInfo[id].area= size;
			dbg_CR("##### Finger[%d] Down (%d,%d)\n", id, fingerInfo[id].x, fingerInfo[id].y ); // p11223
#ifdef PROTECTION_MODE
			if(debugInfo.autocal_flag==1){																			// After touch calibration, first touch press event.
				dbg_CR("[TOUCH] First touch event after touch calibration\n");
				debugInfo.autocal_flag++;
				if(f_resume_cal){
					//mod_timer(&tchautocal_timer, jiffies + msecs_to_jiffies(6000) );				// after 6 sec, touch auto calibration will be disable.
					dbg_CR("[++++ PROTECTION MODE Timer - Press] %d\n", autocal_enable_time[autocal_safecheck_flag]);
					mod_timer(&tchautocal_timer, jiffies + msecs_to_jiffies( autocal_enable_time[autocal_safecheck_flag]));				// after 3 or 6 sec, touch auto calibration will be disable.
				}
				acquisition_config.atchcalst = T8_ATCHCALST/*2*/;
				acquisition_config.atchcalsthr = T8_ATCHCALSTHR/*50*/;
				acquisition_config.atchfrccalthr = T8_ATCHFRCCALTHR;     /*!< Anti-touch force calibration threshold */
				acquisition_config.atchfrccalratio = T8_ATCHFRCCALRATIO;  /*!< Anti-touch force calibration ratio */  
				if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK) {
					dbg("\n[TSP][ERROR] line : %d\n", __LINE__);
				}
			}
#endif 
		}                 
		else{
			touch_status = 3;
			dbg("[TOUCH] Unknown state ! status = %d \n", quantum_msg[1]);
			goto interrupt_return;
		}

		ret_val = MESSAGE_READ_OK;
	}                     
	else if (object_type == TOUCH_KEYARRAY_T15){
		dbg("[TSP] TOUCH_KEYARRAY_T15 (quantum_msg[1]: %d, quantum_msg[3]: %d, quantum_msg[4] %d)!\n\n", quantum_msg[1], quantum_msg[3], quantum_msg[4]);
	}               
	else if (object_type == GEN_COMMANDPROCESSOR_T6){
		//Reset, Overflow, SigErr, Calibration, CFG Err ...
		dbg("[TSP] msg id =  %d, status = 0x%x \n", quantum_msg[0],quantum_msg[1]);
		if((quantum_msg [1] != 0x10) && (quantum_msg [1] != 0x00))
			goto interrupt_return;

#ifdef PROTECTION_MODE 			
		if(quantum_msg[1] == 0x10) {
			//cal_check_flag=1u;
			dbg_CR("[TOUCH] T6 Calibrating!\n");
			cal_check_flag=1u;
			mxt_time_point = 0;

			debugInfo.calibration_cnt++;
			cal_correction_limit = 5;
			
			cancel_work_sync(&qt602240_data->work_protection_mode_disable);	// If calibration is performed right after protection_mode_timer enable, 
			del_timer(&protection_mode_timer);								// Finger & Palm recal config will be disable

			if(debugInfo.autocal_flag){	// If touch auto calibration is enabled.
				dbg_CR("[TOUCH] autocal timer deleted...Touch calibration is executed in auto calibration\n");
				cancel_work_sync(&qt602240_data->work_tchautocal_disable);
				del_timer(&tchautocal_timer);
			}
			else{
				dbg_CR("[TOUCH] First Touch & Auto calibration is executed \n");
				acquisition_config.tchautocal= T8_TCHAUTOCAL_PROTECTION; // 10
			}
			acquisition_config.atchcalst = T8_ATCHCALST_PROTECTION;
			acquisition_config.atchcalsthr = T8_ATCHCALSTHR_PROTECTION; // Never suspend
			acquisition_config.atchfrccalthr = T8_ATCHFRCCALTHR_PROTECTION;
			acquisition_config.atchfrccalratio = T8_ATCHFRCCALRATIO_PROTECTION;
			if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK) {
				dbg_CR("\n[TSP][ERROR] line : %d\n", __LINE__);
			}

#ifdef PROTECTION_MODE_SAFECHECK		
			if ( autocal_safecheck_flag == 1 && acquisition_config.tchautocal == T8_TCHAUTOCAL_PROTECTION) {
				dbg_CR("[++++ PROTECTION MODE Timer - T6 Cmd] %d\n", autocal_enable_time[autocal_safecheck_flag]);
				mod_timer(&tchautocal_timer, jiffies + msecs_to_jiffies( autocal_enable_time[autocal_safecheck_flag]));				// after 3 or 6 sec, touch auto calibration will be disable.
			}
			else {
				if(!f_resume_cal){
					//mod_timer(&tchautocal_timer, jiffies + msecs_to_jiffies(6000));				// after 6 sec, touch auto calibration will be disable.
					dbg_CR("[++++ PROTECTION MODE Timer - T6 Cmd] %d\n", autocal_enable_time[autocal_safecheck_flag]);
					mod_timer(&tchautocal_timer, jiffies + msecs_to_jiffies( autocal_enable_time[autocal_safecheck_flag]));				// after 3 or 6 sec, touch auto calibration will be disable.
				}
			}
#else
			if(!f_resume_cal){
				//mod_timer(&tchautocal_timer, jiffies + msecs_to_jiffies(6000));				// after 6 sec, touch auto calibration will be disable.
				dbg_CR("[++++ PROTECTION MODE Timer - T6 Cmd] %d\n", autocal_enable_time[autocal_safecheck_flag]);
				mod_timer(&tchautocal_timer, jiffies + msecs_to_jiffies( autocal_enable_time[autocal_safecheck_flag]));				// after 3 or 6 sec, touch auto calibration will be disable.
			}
#endif
			debugInfo.autocal_flag=1;
		}
#endif
	}
	else{
		dbg("[TSP] msg id =  %d, status = 0x%x \n", quantum_msg[0],quantum_msg[1]);
		goto interrupt_return;
	}

	/* touch_message_flag - set when tehre is a touch message
	   facesup_message_flag - set when there is a face suppression message */
	// check chip's calibration status when touch down or up
	if(cal_check_flag == 1 && touch_message_flag) { 
		if (mxt_time_point == 0) 
			mxt_time_point = jiffies_to_msecs(jiffies);
		cal_correction_limit = 0;
		check_chip_calibration();
	}
	if (object_type == TOUCH_MULTITOUCHSCREEN_T9) {
		report_input(touch_status);
	}

interrupt_return:
	enable_irq(qt602240_data->client->irq);
	mutex_unlock(&qt602240_data->lock);
	dbg_func_out();
	return ;
}

void report_input (int touch_status) {
	int i;
	int valid_input_count=0;

	for ( i= 0; i<MAX_NUM_FINGER; i++ )	{
		//printk("X : %d, Y: %d\n", fingerInfo[i].x, fingerInfo[i].y);

		if ( fingerInfo[i].status == -1 || (fingerInfo[i].mode == TSC_EVENT_NONE && fingerInfo[i].status == 0)) 
			continue;

		if(fingerInfo[i].mode == TSC_EVENT_NONE )			// TOUCH_EVENT_PRESS (DOWN)
		{
			//printk("[TSP] Finger[%d] Down(TSC_EVENT_WINDOW) XY(%d, %d)\n", i, fingerInfo[i].x, fingerInfo[i].y);  //p11223
			/* Touch Window ?????? Touch ?? ???? */
			if(fingerInfo[i].y < SCREEN_RESOLUTION_Y)	{
				input_mt_slot(qt602240_data->input_dev, fingerInfo[i].id);														// TOUCH_ID_SLOT
				input_report_abs(qt602240_data->input_dev, ABS_MT_TRACKING_ID, fingerInfo[i].id);			// TOUCH_ID
				input_report_abs(qt602240_data->input_dev, ABS_MT_POSITION_X, fingerInfo[i].x);				// TOUCH_X
				input_report_abs(qt602240_data->input_dev, ABS_MT_POSITION_Y, fingerInfo[i].y);				// TOUCH_Y
				input_report_abs(qt602240_data->input_dev, ABS_MT_WIDTH_MAJOR, fingerInfo[i].area);   // TOUCH_SIZE
				fingerInfo[i].mode = TSC_EVENT_WINDOW;
			}

			valid_input_count++;
		}
		else
		{
			// TOUCH_EVENT_RELEASE (UP)
			if (fingerInfo[i].status == TOUCH_EVENT_RELEASE) { 		 
				if(fingerInfo[i].mode == TSC_EVENT_WINDOW) {
					dbg_CR("[QT602240] U:(%d, %d) (id:%d)\n", fingerInfo[i].x, fingerInfo[i].y, fingerInfo[i].id);
					input_mt_slot(qt602240_data->input_dev, fingerInfo[i].id);						// TOUCH_ID_SLOT
					input_report_abs(qt602240_data->input_dev, ABS_MT_TRACKING_ID, -1);		// RELEASE TOUCH_ID
					fingerInfo[i].mode = TSC_EVENT_NONE;
					fingerInfo[i].status= -1;
				}
			}
			// TOUCH_EVENT_MOVE
			else if(fingerInfo[i].status == TOUCH_EVENT_MOVE && fingerInfo[i].mode == TSC_EVENT_WINDOW)
			{
				if(fingerInfo[i].y>(SCREEN_RESOLUTION_Y-1)) fingerInfo[i].y=SCREEN_RESOLUTION_Y-1;	
				dbg("[TSP] Finger[%d] Move(TSC_EVENT_WINDOW) XY(%d, %d)\n", i, fingerInfo[i].x, fingerInfo[i].y);
				input_mt_slot(qt602240_data->input_dev, fingerInfo[i].id);														// TOUCH_ID_SLOT
				input_report_abs(qt602240_data->input_dev, ABS_MT_POSITION_X, fingerInfo[i].x);				// TOUCH_X
				input_report_abs(qt602240_data->input_dev, ABS_MT_POSITION_Y, fingerInfo[i].y);				// TOUCH_Y
				input_report_abs(qt602240_data->input_dev, ABS_MT_WIDTH_MAJOR, fingerInfo[i].area);   // TOUCH_SIZE

				fingerInfo[i].status= TOUCH_EVENT_PRESS;
				valid_input_count++;
			}
			// TOUCH_EVENT_PRESS 
			else if (fingerInfo[i].status == TOUCH_EVENT_PRESS)	
			{
				valid_input_count++;
			}
			else {
			}
		}
	}

	input_report_key(qt602240_data->input_dev, BTN_TOUCH, !!valid_input_count);  // mirinae_ICS
	dbg("[mt slot TOUCH] touch event num => %d\n",valid_input_count);
	input_sync(qt602240_data->input_dev);


#ifdef PROTECTION_MODE
	if(!touch_status){
		if(f_resume_cal && !cal_correction_limit && !debugInfo.autocal_flag){
			dbg_CR("[TOUCH] f_resume_cal false \n");
			f_resume_cal = false;
		}
		valid_input_count=0;
		// Run protection_mode when no finger 	
		for (i = 0 ; i < MAX_NUM_FINGER; ++i) { 
			if ( fingerInfo[i].status == -1 || (fingerInfo[i].mode == TSC_EVENT_NONE && fingerInfo[i].status == 0))
				continue;
			valid_input_count++;
		}
		// When all finger released (finger_cnt == 0)
		if (valid_input_count == 0 && cal_correction_limit > 0) {
			cal_correction_limit--;
			dbg_CR("[TOUCH] Finger & Palm recalibration start 300ms \n");
			acquisition_config.atchcalst = T8_ATCHCALST_PROTECTION;
			acquisition_config.atchcalsthr = T8_ATCHCALSTHR_PROTECTION; // Never suspend
			acquisition_config.atchfrccalthr = T8_ATCHFRCCALTHR_PROTECTION;
			acquisition_config.atchfrccalratio = T8_ATCHFRCCALRATIO_PROTECTION;
			if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK) {
				dbg("\n[TSP][ERROR] line : %d\n", __LINE__);
			}
			mod_timer(&protection_mode_timer, jiffies + msecs_to_jiffies(300));
		}
		
	}

#endif
}

#ifdef PROTECTION_MODE_SAFECHECK

static int autocal_safecheck(void)
{
	int i=0;	
	int nTouchCnt=0, nAntiCnt=0;
	int tch_atch_check_threshold = (touchscreen_config.tchthr << 3 ) * 30 / 100;	

	diag_debug(QT602240_DELTA_MODE);	

	for (i=0; i< 224; i++ ) {
		//	Touch
		if ( reference_data[i] > tch_atch_check_threshold ) {						
			nTouchCnt++;
		}
		//	Anti-Touch
		if ( reference_data[i] < -tch_atch_check_threshold ) {						
			nAntiCnt++;
		}	
	}

	dbg_CR("[++++ Protection Mode Safecheck] tch=%d, atch=%d\n", nTouchCnt, nAntiCnt);
	
	if ( (nTouchCnt > 0) || (nAntiCnt > 0) ) return 0;
	return 1;
}

#endif

/*------------------------------ I2C Driver block -----------------------------------*/
#define I2C_M_WR 0 /* for i2c */
#define I2C_MAX_SEND_LENGTH     300
int qt602240_i2c_write(u16 reg, u8 *read_val, unsigned int len)
{
	struct i2c_msg wmsg;
	unsigned char data[I2C_MAX_SEND_LENGTH];
	int ret,i;

	//dbg_func_in();
	dbg("[+] %s\n", __func__);

	address_pointer = reg;

	if(len+2 > I2C_MAX_SEND_LENGTH)
	{
		dbg("[TSP][ERROR] %s() data length error\n", __FUNCTION__);
		return -ENODEV;
	}

	wmsg.addr = qt602240_data->client->addr;
	wmsg.flags = I2C_M_WR;
	wmsg.len = len + 2;
	wmsg.buf = data;
	data[0] = reg & 0x00ff;
	data[1] = reg >> 8;

	for (i = 0; i < len; i++){
		data[i+2] = *(read_val+i);
	}

	ret = i2c_transfer(qt602240_data->client->adapter, &wmsg, 1);

	//dbg_func_out();
	dbg("[-] %s\n", __func__);

	return ret;
}

int boot_qt602240_i2c_write(u16 reg, u8 *read_val, unsigned int len)
{
	struct i2c_msg wmsg;
	unsigned char data[I2C_MAX_SEND_LENGTH];
	int ret,i;

	if(len+2 > I2C_MAX_SEND_LENGTH) {
		dbg("[TSP][ERROR] %s() data length error\n", __FUNCTION__);
		return -ENODEV;
	}

	wmsg.addr = QT602240_I2C_BOOT_ADDR;
	wmsg.flags = I2C_M_WR;
	wmsg.len = len;
	wmsg.buf = data;


	for (i = 0; i < len; i++) {
		data[i] = *(read_val+i);
	}

	ret = i2c_transfer(qt602240_data->client->adapter, &wmsg, 1);

	return ret;
}


int qt602240_i2c_read(u16 reg,unsigned char *rbuf, int buf_size)
{
	static unsigned char first_read=1;
	struct i2c_msg rmsg;
	int ret;
	unsigned char data[2];

	rmsg.addr = qt602240_data->client->addr;

	if(first_read == 1)	{
		first_read = 0;
		address_pointer = reg+1;
	}

	if((address_pointer != reg) || (reg != message_processor_address))
	{
		address_pointer = reg;
		rmsg.flags = I2C_M_WR;
		rmsg.len = 2;
		rmsg.buf = data;
		data[0] = reg & 0x00ff;
		data[1] = reg >> 8;
		ret = i2c_transfer(qt602240_data->client->adapter, &rmsg, 1);
	}

	rmsg.flags = I2C_M_RD;
	rmsg.len = buf_size;
	rmsg.buf = rbuf;
	ret = i2c_transfer(qt602240_data->client->adapter, &rmsg, 1);

	return ret;
}

/*! \brief Maxtouch Memory read by I2C bus */
U8 read_mem(U16 start, U8 size, U8 *mem)
{
	int ret;
	U8 rc;

	memset(mem,0xFF,size);
	ret = qt602240_i2c_read(start,mem,size);
	if(ret < 0) {
		dbg("%s : i2c read failed\n",__func__);
		rc = READ_MEM_FAILED;
	}
	else {
		rc = READ_MEM_OK;
	}

	return rc;
}

U8 boot_read_mem(U16 start, U8 size, U8 *mem)
{
	struct i2c_msg rmsg;
	int ret;

	dbg_func_in();

	rmsg.addr = QT602240_I2C_BOOT_ADDR;
	rmsg.flags = I2C_M_RD;
	rmsg.len = size;
	rmsg.buf = mem;
	ret = i2c_transfer(qt602240_data->client->adapter, &rmsg, 1);

	dbg_func_out();

	return ret;
}

U8 read_U16(U16 start, U16 *mem)
{
	U8 status;

	status = read_mem(start, 2, (U8 *) mem);

	return status;
}

U8 write_mem(U16 start, U8 size, U8 *mem)
{
	int ret;
	U8 rc;

	ret = qt602240_i2c_write(start,mem,size);
	if(ret < 0) 
		rc = WRITE_MEM_FAILED;
	else
		rc = WRITE_MEM_OK;

	return rc;
}

U8 boot_write_mem(U16 start, U16 size, U8 *mem)
{
	int ret;
	U8 rc;

	dbg_func_in();

	ret = boot_qt602240_i2c_write(start,mem,size);
	if(ret < 0){
		dbg("boot write mem fail: %d \n",ret);
		rc = WRITE_MEM_FAILED;
	}
	else {
		rc = WRITE_MEM_OK;
	}

	dbg_func_out();

	return rc;
}


/*****************************************************************************
 *
 *  FUNCTION
 *  PURPOSE
 *  INPUT
 *  OUTPUT
 *
 * ***************************************************************************/

void write_message_to_usart(uint8_t msg[], uint8_t length)
{
	int i;
	dbg_func_in();

	for (i=0; i < length; i++){
		dbg("0x%02x ", msg[i]);
	}
	dbg("\n\r");

	dbg_func_out();
}


irqreturn_t qt602240_irq_handler(int irq, void *dev_id)
{	
	disable_irq_nosync(qt602240_data->client->irq);
	if(!queue_work(qt602240_wq, &qt602240_data->work)){			// p13106 
		enable_irq(qt602240_data->client->irq);
		printk("[TOUCH] qt602240_irq_handler pueue_work failed\n");
	}
	return IRQ_HANDLED;
}


static int qt602240_remove(struct i2c_client *client)
{
	dbg_func_in();

	input_mt_destroy_slots(qt602240_data->input_dev); // PROTOCOL TYPE B
	if(qt602240_data->client->irq)
	{
		free_irq(qt602240_data->client->irq, qt602240_data);
	}
	mutex_destroy(&qt602240_data->lock);	
#ifdef SKY_PROCESS_CMD_KEY
	misc_deregister(&touch_event);
#endif	//SKY_PROCESS_CMD_KEY
#ifdef TOUCH_IO
	misc_deregister(&touch_io);
#endif //TOUCH_IO
#if defined(CONFIG_PM) && defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&qt602240_data->es);
#endif
	input_unregister_device(qt602240_data->input_dev);
	input_free_device(qt602240_data->input_dev);

	device_destroy(touch_atmel_class, 0);
	class_destroy(touch_atmel_class);

	dbg_line();
	if (qt602240_wq)
		destroy_workqueue(qt602240_wq);
	kfree(qt602240_data);        

	off_hw_setting();
	dbg_func_out();
	return 0;
}

#if defined(CONFIG_PM) && defined(CONFIG_HAS_EARLYSUSPEND)
static int qt602240_early_suspend(struct early_suspend *h)
{
	dbg_func_in();
	disable_irq(qt602240_data->client->irq);

#ifdef PROTECTION_MODE 
	cancel_work_sync(&qt602240_data->work_protection_mode_disable);
	cancel_work_sync(&qt602240_data->work_tchautocal_disable);
	del_timer(&protection_mode_timer);
	del_timer(&tchautocal_timer);
	protection_mode = false;
#endif

	qt_Power_Sleep();
	clear_event(TSC_CLEAR_ALL);

//++ p11309 2012.11.02 for remove mt slots for ghost touch
	input_mt_destroy_slots(qt602240_data->input_dev); // PROTOCOL_B
//-- p11309

	dbg_func_out();
	return 0;
}

static int  qt602240_late_resume(struct early_suspend *h)
{
	dbg_func_in();
//++ p11309 2012.11.02 for remove mt slots for ghost touch
	input_mt_init_slots(qt602240_data->input_dev, MAX_NUM_FINGER); // PROTOCOL_B
//-- p11309

	touch_data_init();
	qt_Power_Config_Init();
	
	autocal_safecheck_flag=0;
#ifdef PROTECTION_MODE_SAFECHECK
	autocal_safecheck_flag=autocal_safecheck();
#endif	
	// touch ic calibration.
	calibrate_chip();
	acquisition_config.tchautocal= T8_TCHAUTOCAL_PROTECTION; // 10
	acquisition_config.atchcalst = T8_ATCHCALST_PROTECTION;
	acquisition_config.atchcalsthr = T8_ATCHCALSTHR_PROTECTION; // Never suspend
	acquisition_config.atchfrccalthr = T8_ATCHFRCCALTHR_PROTECTION;
	acquisition_config.atchfrccalratio = T8_ATCHFRCCALRATIO_PROTECTION;
	if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK) {
		dbg_CR("\n[TSP][ERROR] line : %d\n", __LINE__);
	}
	cal_correction_limit = 5;
	debugInfo.calibration_cnt=0;
	debugInfo.autocal_flag=1;
	f_resume=true;
	f_resume_cal=true;
	enable_irq(qt602240_data->client->irq);

	dbg_func_out();
	return 0;
}
#endif // CONFIG_PM && CONFIG_HAS_EARLYSUSPEND


/* I2C driver probe function */
static int __devinit qt602240_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int rc;
	dbg_func_in();
	qt602240_data = kzalloc(sizeof(struct qt602240_data_t), GFP_KERNEL);
	if (!qt602240_data){
		pr_err("qt602240_data is not NULL.\n");
		return -ENOMEM;
	}
	qt602240_data->client = client;

	qt602240_wq = create_singlethread_workqueue("qt602240_wq");
	if (!qt602240_wq){
		pr_err("create_singlethread_workqueue(qt602240_wq) error.\n");
		return -ENOMEM;
	}

	if(!touch_atmel_class)
		touch_atmel_class=class_create(THIS_MODULE, "touch_ateml");

	ts_dev = device_create(touch_atmel_class, NULL, 0, NULL, "ts");
	if (IS_ERR(ts_dev))
		pr_err("Failed to create device(ts)!\n");
	if (device_create_file(ts_dev, &dev_attr_gpio) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_gpio.attr.name);
	if (device_create_file(ts_dev, &dev_attr_i2c) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_i2c.attr.name);
	if (device_create_file(ts_dev, &dev_attr_setup) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_setup.attr.name);

	dbg("ts_dev creation : success.\n");

	dbg("+-----------------------------------------+\n");
	dbg("|  Quantum Touch Driver Probe!            |\n");
	dbg("+-----------------------------------------+\n");

	INIT_WORK(&qt602240_data->work, get_message );

#ifdef PROTECTION_MODE
	INIT_WORK(&qt602240_data->work_protection_mode_disable, qt_Acquisition_Config_Init_locked);
	INIT_WORK(&qt602240_data->work_tchautocal_disable, disable_autocal_locked);

	init_timer(&protection_mode_timer);
	protection_mode_timer.function = protection_mode_timer_func;
	protection_mode_timer.data = 0;
	init_timer(&tchautocal_timer);
	tchautocal_timer.function = tchautocal_disable_timer_func;
	tchautocal_timer.data = 0;

#endif
	debugInfo.calibration_cnt = 0;
	debugInfo.autocal_flag = 0; 
	qt602240_data->input_dev = input_allocate_device();
	if (qt602240_data->input_dev == NULL)
	{
		rc = -ENOMEM;
		pr_err("qt602240_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
#if defined (CONFIG_MACH_MSM8960_EF45K)
	qt602240_data->input_dev->name = "qt602240_ts_input_ef45k";
#elif defined (CONFIG_MACH_MSM8960_EF46L)
	qt602240_data->input_dev->name = "qt602240_ts_input_ef46l";
#elif defined (CONFIG_MACH_MSM8960_EF47S)
	qt602240_data->input_dev->name = "qt602240_ts_input_ef47s";
#else
	qt602240_data->input_dev->name = "qt602240_ts_input";
#endif
	set_bit(EV_SYN, qt602240_data->input_dev->evbit);
	set_bit(EV_KEY, qt602240_data->input_dev->evbit);
	set_bit(EV_ABS, qt602240_data->input_dev->evbit);
	set_bit(INPUT_PROP_DIRECT, qt602240_data->input_dev->propbit);

	set_bit(BTN_TOUCH, qt602240_data->input_dev->keybit);
	set_bit(KEY_MENU, qt602240_data->input_dev->keybit);
	set_bit(KEY_HOMEPAGE, qt602240_data->input_dev->keybit);
	set_bit(KEY_BACK, qt602240_data->input_dev->keybit);	
	
#ifdef SKY_PROCESS_CMD_KEY
	set_bit(KEY_SEARCH, qt602240_data->input_dev->keybit);
	set_bit(KEY_RECENTAPPS, qt602240_data->input_dev->keybit);  // p11223_Touch_key
	set_bit(KEY_HOMEPAGE, qt602240_data->input_dev->keybit);
	set_bit(KEY_0, qt602240_data->input_dev->keybit);
	set_bit(KEY_1, qt602240_data->input_dev->keybit);
	set_bit(KEY_2, qt602240_data->input_dev->keybit);
	set_bit(KEY_3, qt602240_data->input_dev->keybit);
	set_bit(KEY_4, qt602240_data->input_dev->keybit);
	set_bit(KEY_5, qt602240_data->input_dev->keybit);
	set_bit(KEY_6, qt602240_data->input_dev->keybit);
	set_bit(KEY_7, qt602240_data->input_dev->keybit);
	set_bit(KEY_8, qt602240_data->input_dev->keybit);
	set_bit(KEY_9, qt602240_data->input_dev->keybit);
	set_bit(0xe3, qt602240_data->input_dev->keybit); /* '*' */
	set_bit(0xe4, qt602240_data->input_dev->keybit); /* '#' */
	set_bit(0xe5, qt602240_data->input_dev->keybit); /* 'KEY_END' p13106 120105 */
	set_bit(KEY_POWER, qt602240_data->input_dev->keybit);
	set_bit(KEY_LEFTSHIFT, qt602240_data->input_dev->keybit);
	set_bit(KEY_RIGHTSHIFT, qt602240_data->input_dev->keybit);
	set_bit(KEY_LEFT, qt602240_data->input_dev->keybit);
	set_bit(KEY_RIGHT, qt602240_data->input_dev->keybit);
	set_bit(KEY_UP, qt602240_data->input_dev->keybit);
	set_bit(KEY_DOWN, qt602240_data->input_dev->keybit);
	set_bit(KEY_ENTER, qt602240_data->input_dev->keybit);
	set_bit(KEY_SEND, qt602240_data->input_dev->keybit);
	set_bit(KEY_END, qt602240_data->input_dev->keybit);
	set_bit(KEY_F1, qt602240_data->input_dev->keybit);
	set_bit(KEY_F2, qt602240_data->input_dev->keybit);
	set_bit(KEY_F3, qt602240_data->input_dev->keybit);				// P13106 VT_CALL for VT TEST 121019				
	set_bit(KEY_F4, qt602240_data->input_dev->keybit);
	set_bit(KEY_VOLUMEUP, qt602240_data->input_dev->keybit);
	set_bit(KEY_VOLUMEDOWN, qt602240_data->input_dev->keybit);
	set_bit(KEY_CLEAR, qt602240_data->input_dev->keybit);
	set_bit(KEY_CAMERA, qt602240_data->input_dev->keybit);
	//    set_bit(KEY_HOLD, qt602240_data->input_dev->keybit);
#endif // SKY_PROCESS_CMD_KEY

	input_mt_init_slots(qt602240_data->input_dev, MAX_NUM_FINGER); // PROTOCOL_B
	input_set_abs_params(qt602240_data->input_dev, ABS_X, 0, SCREEN_RESOLUTION_X, 0, 0);
	input_set_abs_params(qt602240_data->input_dev, ABS_Y, 0, SCREEN_RESOLUTION_Y, 0, 0);
	input_set_abs_params(qt602240_data->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(qt602240_data->input_dev, ABS_TOOL_WIDTH, 0, 15, 0, 0);
	input_set_abs_params(qt602240_data->input_dev, ABS_MT_POSITION_X, 0, SCREEN_RESOLUTION_X-1, 0, 0);
	input_set_abs_params(qt602240_data->input_dev, ABS_MT_POSITION_Y, 0, SCREEN_RESOLUTION_Y-1, 0, 0);
	input_set_abs_params(qt602240_data->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(qt602240_data->input_dev, ABS_MT_WIDTH_MAJOR, 0, QT602240_MAX_CHANNEL_NUM, 0, 0);

	rc = input_register_device(qt602240_data->input_dev);
	if (rc){
		pr_err("qt602240_probe: Unable to register %s input device\n", qt602240_data->input_dev->name);
		goto err_input_register_device_failed;
	}
	dbg("input_register_device : success.\n");

#ifdef TOUCH_IO  
	rc = misc_register(&touch_io);
	if (rc){
		pr_err("::::::::: can''t register qt602240 misc\n");
	}
#endif //TOUCH_IO

#ifdef ITO_TYPE_CHECK		//p13106 TOUCH_ID
	read_touch_id();
#endif //TOUCH_ID

	mutex_init(&qt602240_data->lock);

#ifdef QT_FIRMUP_ENABLE
	QT_reprogram();
#else
	quantum_touch_probe();
#endif

#ifdef CHIP_NOINIT
	if(Chip_NoInit)
	{
		TSP_Restart();
		quantum_touch_probe();
		Chip_NoInit = false;
	}
#endif

	qt602240_data->client->irq = IRQ_TOUCH_INT;
#ifdef INT_LEVEL
	rc = request_irq(qt602240_data->client->irq, qt602240_irq_handler, IRQF_TRIGGER_LOW, "qt602240-irq", qt602240_data);
#else
	rc = request_irq(qt602240_data->client->irq, qt602240_irq_handler, IRQF_TRIGGER_FALLING, "qt602240-irq", qt602240_data);
#endif //INT_LEVEL
	if (!rc){
		dbg("request_irq : success.\n");
		dbg("qt602240_probe: Start touchscreen %s\n", qt602240_data->input_dev->name);
	}
	else{
		dbg_CR("[QT602240]request_irq failed : %d\n", rc);
	}

#if defined(CONFIG_PM) && defined(CONFIG_HAS_EARLYSUSPEND)
	qt602240_data->es.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	qt602240_data->es.suspend = (void*)qt602240_early_suspend;
	qt602240_data->es.resume = (void*)qt602240_late_resume;
	register_early_suspend(&qt602240_data->es);
#endif

#ifdef SKY_PROCESS_CMD_KEY
	rc = misc_register(&touch_event);
	if (rc) {
		pr_err("::::::::: can''t register touch_fops\n");
	}
#endif //SKY_PROCESS_CMD_KEY    


	dbg_func_out();
	return 0;

err_input_register_device_failed:
	input_free_device(qt602240_data->input_dev);

err_input_dev_alloc_failed:
	kfree(qt602240_data);
	pr_err("qt602240 probe failed: rc=%d\n", rc);

	return rc;
}

void  qt602240_front_test_init(void)
{
	disable_irq(qt602240_data->client->irq);
#ifdef PROTECTION_MODE 
	cancel_work_sync(&qt602240_data->work_protection_mode_disable);
	cancel_work_sync(&qt602240_data->work_tchautocal_disable);
	del_timer(&protection_mode_timer);
	del_timer(&tchautocal_timer);
	protection_mode = false;
#endif

	off_hw_setting();
	init_hw_setting();
	quantum_touch_probe();

	enable_irq(qt602240_data->client->irq);
	return ;
}

static ssize_t i2c_show(struct device *dev, struct device_attribute *attr, char *buf)
{       
	int ret;
	unsigned char read_buf[5];

	dbg_func_in();

	ret = qt602240_i2c_read(0,read_buf, 5);
	if (ret < 0){
		dbg("qt602240 i2c read failed.\n");
	}

	dbg_func_out();
	return sprintf(buf, "%s\n", buf);
}

static ssize_t i2c_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	dbg_func_in();
	dbg_func_out();
	return size;
}

static ssize_t gpio_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	dbg_func_in();
	dbg_func_out();
	return sprintf(buf, "%s\n", buf);
}

static ssize_t gpio_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	dbg_func_in();

	if(strncmp(buf, "RSTHIGH", 7) == 0 || strncmp(buf, "rsthigh", 7) == 0) {
		gpio_set_value(GPIO_TOUCH_RST, 1);
		dbg("(touch)set TOUCH_RST High.\n");
		msleep(100);
	}
	if(strncmp(buf, "RSTLOW", 6) == 0 || strncmp(buf, "rstlow", 6) == 0) {
		gpio_set_value(GPIO_TOUCH_RST, 0);
		dbg("(touch) TOUCH_RST Low.\n");
		msleep(100);
	}

	dbg_func_out();
	return size;
}

void TSP_Restart(void)
{
	dbg_func_in();

#if defined (CONFIG_MACH_MSM8960_OSCAR)	
#else
	msleep(10);
	gpio_set_value(GPIO_TOUCH_RST, 0);
	dbg("(skytouch)set TOUCH_RST Low.\n");
	msleep(100);

	gpio_set_value(GPIO_TOUCH_RST, 1);
	dbg("(skytouch)set TOUCH_RST High.\n");

	msleep(10);
#endif


}

#ifdef QT_FIRMUP_ENABLE

uint8_t boot_unlock(void)
{
	int ret;
	unsigned char data[2];
	uint8_t rc;

	dbg_func_in();

	//   read_buf = (char *)kmalloc(size, GFP_KERNEL | GFP_ATOMIC);
	data[0] = 0xDC;
	data[1] = 0xAA;

	ret = boot_qt602240_i2c_write(0,data,2);
	if(ret < 0) {
		dbg("%s : i2c write failed\n",__func__);
		rc = WRITE_MEM_FAILED;
	}
	else
	{
		rc = WRITE_MEM_OK;
	}

	dbg_func_out();

	return rc;
}

uint8_t QT_Boot(bool withReset)
{
	unsigned char	boot_status;
	unsigned char	retry_cnt, retry_cnt_max;
	unsigned long int	character_position = 0;
	unsigned int	frame_size = 0;
	unsigned int	next_frame = 0;
	unsigned int	crc_error_count = 0;
	unsigned int	size1,size2;
	uint8_t			data = 0xA5;
	uint8_t			reset_result = 0;
	unsigned char	*firmware_data;

	firmware_data = QT602240_E_firmware;

	dbg_func_in();

	if(withReset){
		retry_cnt_max = 10;
		reset_result = write_mem(command_processor_address + RESET_OFFSET, 1, &data);

		if(reset_result != WRITE_MEM_OK){
			for(retry_cnt =0; retry_cnt < 3; retry_cnt++){
				msleep(100);
				reset_result = write_mem(command_processor_address + RESET_OFFSET, 1, &data);
				if(reset_result == WRITE_MEM_OK){
					dbg("write_mem(RESET_OFFSET) : fail.\n");
					break;
				}
			}
			dbg("write_mem(RESET_OFFSET) : fail.\n");
		}
		else{
			dbg("write_mem(RESET_OFFSET) : fail.\n");
		}

		msleep(100);
	}
	else{
		retry_cnt_max = 30;
	}

	for(retry_cnt = 0; retry_cnt < retry_cnt_max; retry_cnt++){
		if(boot_read_mem(0,1,&boot_status) == READ_MEM_OK){
			retry_cnt = 0;
			dbg("TSP boot status is %x stage 2 \n", boot_status);

			if((boot_status & QT_WAITING_BOOTLOAD_COMMAND) == QT_WAITING_BOOTLOAD_COMMAND){
				if(boot_unlock() == WRITE_MEM_OK){
					msleep(10);
					dbg("Unlock OK\n");
				}
				else{
					dbg("Unlock fail\n");
				}
			}
			else if((boot_status & 0xC0) == QT_WAITING_FRAME_DATA){
				/* Add 2 to frame size, as the CRC bytes are not included */
				size1 =  *(firmware_data+character_position);
				size2 =  *(firmware_data+character_position+1)+2;
				frame_size = (size1<<8) + size2;

				dbg("Frame size:%d\n", frame_size);
				dbg("Firmware pos:%d\n", (int)character_position);
				/* Exit if frame data size is zero */
				if( 0 == (int)frame_size ){
					printk("[QT602240]0 == frame_size\n");
					return 1;
				}
				next_frame = 1;
				boot_write_mem(0,frame_size, (firmware_data +character_position));
				msleep(10);
				dbg(".");

			}
			else if(boot_status == QT_FRAME_CRC_CHECK){
				dbg("CRC Check\n");
			}
			else if(boot_status == QT_FRAME_CRC_PASS){
				if( next_frame == 1){
					dbg("CRC Ok\n");
					character_position += frame_size;
					next_frame = 0;
				}
				else{
					dbg("next_frame != 1\n");
				}
			}
			else if(boot_status  == QT_FRAME_CRC_FAIL){
				dbg("CRC Fail\n");
				crc_error_count++;
			}
			if(crc_error_count > 10){
				return QT_FRAME_CRC_FAIL;
			}
		}
	}
	dbg_func_out();
	return (0);
}

/* qt602240 chipset version check */
void QT_reprogram(void)
{
	uint8_t version=0, build=0;
	uint8_t status = 0;
	unsigned char rxdata=0;

	dbg_func_in();

	if(boot_read_mem(0,1,&rxdata) == READ_MEM_OK)
	{
		dbg("Enter to new firmware : boot mode\n");
		if(QT_Boot(0)) {
			TSP_Restart();  //cte mode == 0
			quantum_touch_probe(); //cte mode ==3 
			TSP_Restart();
		}
		quantum_touch_probe(); //cte mode ==3 
		TSP_Restart();	
		dbg("Reprogram done : boot mode\n");       
	}

	/* find and initialise QT device */
	quantum_touch_probe();

	if (driver_setup != DRIVER_SETUP_OK)
		return;

	status = read_mem(3, 1, (void *) &build);
	if (status != READ_MEM_OK) printk("[TSP]ERROR!");
	dbg("familybuild_id = 	0x%x\n",build);

	status = read_mem(2, 1, (void *) &version);
	if (status != READ_MEM_OK) printk("[TSP]ERROR!");
	dbg("version = 		0x%x\n",version);

	if((version < 0x10) || ((version == 0x10)&&(build != 0xAA))){
		printk("Enter to new firmware : ADDR = Other Version\n");
		if(QT_Boot(1)) {
			TSP_Restart();
			quantum_touch_probe();
			TSP_Restart(); 
		}
		quantum_touch_probe();
		TSP_Restart(); 
		dbg("Reprogram done : ADDR = Other Version\n");
	}

	dbg_func_out();
}
#endif

static ssize_t setup_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	dbg_func_in();
	dbg_func_out();
	return 0;
}

static ssize_t setup_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	dbg_func_in();
	dbg_func_out();
	return size;
}

int __init qt602240_init(void)
{
	int rc;

	dbg_func_in();
	rc = init_hw_setting();
	if(rc<0){
		printk("[QT602240]qt602240 : init_hw_setting failed. (rc=%d)\n", rc);
		return rc;
	}
	dbg("i2c_add_driver\n");
	rc = i2c_add_driver(&qt602240_driver);
	dbg("rc=%d\n", rc);
	if(rc)
		printk("[QT602240]qt602240 : i2c_add_driver failed. (rc=%d)\n", rc);

	dbg_func_out();
	return rc;
}

void __exit qt602240_exit(void)
{
	dbg_func_in();

	i2c_del_driver(&qt602240_driver);
	dbg_func_out();
	return;
}

late_initcall(qt602240_init);
module_exit(qt602240_exit);

MODULE_DESCRIPTION("ATMEL qt602240 Touchscreen Driver");
MODULE_LICENSE("GPL");

