/* -------------------------------------------------------------------- */
/* GPIO, VREG & resolution */
/* -------------------------------------------------------------------- */

#define MAX_NUM_FINGER	5

// Screen resolution
#define SCREEN_RESOLUTION_X		720
#define SCREEN_RESOLUTION_Y		1280

// Interrupt GPIO Pin
#define GPIO_TOUCH_CHG			11
#define GPIO_TOUCH_RST			50

#define GPIO_TOUCH_POWER 		51
// define ITO_TYPE_CHECK


#ifdef PROTECTION_MODE

/* -------------------------------------------------------------------- */
/* DEVICE   : mxT224 Lockscreen Mode CONFIGURATION */
/* -------------------------------------------------------------------- */
#define T8_TCHAUTOCAL_PROTECTION 		10  /* 10*(200ms) */
#define T8_ATCHCALST_PROTECTION 		0
#define T8_ATCHCALSTHR_PROTECTION		0
#define T8_ATCHFRCCALTHR_PROTECTION 	50        
#define T8_ATCHFRCCALRATIO_PROTECTION 	20     

#endif

/* -------------------------------------------------------------------- */
/* DEVICE   : mxT224 CONFIGURATION */
/* -------------------------------------------------------------------- */

/* [SPT_USERDATA_T38 INSTANCE 0] */
#define T38_USERDATA0           0
#define T38_USERDATA1           0 /* CAL_THR */
#define T38_USERDATA2           0 /* num_of_antitouch */
#define T38_USERDATA3           0 /* max touch for palm recovery  */
#define T38_USERDATA4           0 /* MXT_ADR_T8_ATCHFRCCALRATIO for normal */
#define T38_USERDATA5           0     
#define T38_USERDATA6           0 
#define T38_USERDATA7           0 /* max touch for check_auto_cal */

/* [GEN_POWERCONFIG_T7 INSTANCE 0] */
#define T7_IDLEACQINT			32
#define T7_IDLEACQINT_PLUG		255
#define T7_ACTVACQINT			255
#define T7_ACTV2IDLETO			30

/* _GEN_ACQUISITIONCONFIG_T8 INSTANCE 0 */
#define T8_CHRGTIME               	35	// 40
#define T8_ATCHDRIFT				0
#define T8_TCHDRIFT              	3	//5		//20
#define T8_DRIFTST              	1	
#define T8_TCHAUTOCAL            	0
#define T8_SYNC                  	0
#define T8_ATCHCALST             	255
#define T8_ATCHCALSTHR           	1
#define T8_ATCHFRCCALTHR         	255         
#define T8_ATCHFRCCALRATIO       	127      

/* _TOUCH_MULTITOUCHSCREEN_T9 INSTANCE 0 */

/*
	7: ScanEn
	6: disable Press
	5: disable Release
	4: disable Move
	3: disbale Vector
	2: Disable Amp
	1: Report En
	0: enable Multi-touch
*/
#define T9_CTRL                         143	// 1 0 0 0 1 1 1 1
//#define T9_CTRL                         131	// 1 0 0 0 0 0 1 1

#define T9_XORIGIN                      0
#define T9_YORIGIN                      0
#define T9_XSIZE                        19
#define T9_YSIZE                        11
#define T9_AKSCFG                       0
#define T9_BLEN                         16

#ifdef ITO_TYPE_CHECK
static uint8_t T9_TCHTHR[2]	= {50,45};
#else
#define T9_TCHTHR                       50
#endif

#define T9_TCHDI                      	2
#define T9_NEXTTCHDI                	2  // 3	// 2     
#define T9_ORIENT                       1
#define T9_MRGTIMEOUT                   0
#define T9_MOVHYSTI                     5 // 5    // 1
#define T9_MOVHYSTN                     2  // 2
#define T9_MOVFILTER                    0
#define T9_NUMTOUCH                     MAX_NUM_FINGER
#define T9_MRGHYST                      10
#define T9_MRGTHR                       50   // 0
#define T9_AMPHYST                      10
#define T9_XRANGE                       1279 // (874-1)    // 898 = (97.65/87.1) * 800 
#define T9_YRANGE                       719  //  (800-1)    

#if (BOARD_VER >= TP20)
	#define T9_XLOCLIP                      9   // 13	//	28   // 0
	#define T9_XHICLIP                      7   // 13	//	28   // 0
	#define T9_YLOCLIP                      35  // 0
	#define T9_YHICLIP                      35  // 0
	#define T9_XEDGECTRL                    188 // 128  // 0
	#define T9_XEDGEDIST                    33  // 0
	#define T9_YEDGECTRL                    148 // 0
	#define T9_YEDGEDIST                    60  // 0
#else
	#define T9_XLOCLIP                      13	//	28   // 0
	#define T9_XHICLIP                      13	//	28   // 0
	#define T9_YLOCLIP                      0
	#define T9_YHICLIP                      0
	#define T9_XEDGECTRL                    128  // 0
	#define T9_XEDGEDIST                    0
	#define T9_YEDGECTRL                    0
	#define T9_YEDGEDIST                    0
#endif

#define T9_JUMPLIMIT					15	// 10
#ifdef ITO_TYPE_CHECK
static uint8_t T9_TCHHYST[2] = {15,14};
#else
#define T9_TCHHYST						(T9_TCHTHR/4)  /* V2.0 or MXT224E added */
#endif
#define T9_XPITCH                   	0  /* MXT224E added */
#define T9_YPITCH                   	0  /* MXT224E added */

/* [TOUCH_KEYARRAY_T15 INSTANCE 0]    */
#define T15_CTRL                        0
#define T15_XORIGIN                     0
#define T15_YORIGIN                     0
#define T15_XSIZE                       0
#define T15_YSIZE                       0
#define T15_AKSCFG                      0
#define T15_BLEN                        0
#define T15_TCHTHR                      0
#define T15_TCHDI                       0
#define T15_RESERVED_0                  0
#define T15_RESERVED_1                  0

/*  [SPT_COMMSCONFIG_T18 INSTANCE 0]        */
#define T18_CTRL                        0
#define T18_COMMAND                     0

/* _SPT_GPIOPWM_T19 INSTANCE 0 */
#define T19_CTRL                        0
#define T19_REPORTMASK                  0
#define T19_DIR                         0
#define T19_INTPULLUP                   0
#define T19_OUT                         0
#define T19_WAKE                        0
#define T19_PWM                         0
#define T19_PERIOD                      0
#define T19_DUTY_0                      0
#define T19_DUTY_1                      0
#define T19_DUTY_2                      0
#define T19_DUTY_3                      0
#define T19_TRIGGER_0                   0
#define T19_TRIGGER_1                   0
#define T19_TRIGGER_2                   0
#define T19_TRIGGER_3                   0

/* [TOUCH_PROXIMITY_T23 INSTANCE 0] */
#define T23_CTRL                    0
#define T23_XORIGIN               	0
#define T23_YORIGIN               	0
#define T23_XSIZE                 	0
#define T23_YSIZE                 	0
#define T23_RESERVED              	0
#define T23_BLEN                  	0
#define T23_FXDDTHR               	0
#define T23_FXDDI                 	0
#define T23_AVERAGE               	0
#define T23_MVNULLRATE            	0
#define T23_MVDTHR                	0

/* [SPT_SELFTEST_T25 INSTANCE 0] */
#define T25_CTRL                        0
#define T25_CMD                         0
#define T25_SIGLIM_0_UPSIGLIM        	0
#define T25_SIGLIM_0_LOSIGLIM        	0
#define T25_SIGLIM_1_UPSIGLIM        	0
#define T25_SIGLIM_1_LOSIGLIM        	0
#define T25_SIGLIM_2_UPSIGLIM        	0
#define T25_SIGLIM_2_LOSIGLIM        	0

/* [PROCI_GRIPSUPPRESSION_T40 INSTANCE 0] */
#define T40_CTRL                	0
#define T40_XLOGRIP             	0
#define T40_XHIGRIP             	0
#define T40_YLOGRIP             	0
#define T40_YHIGRIP					0

/* PROCI_TOUCHSUPPRESSION_T42 */

#define T42_CTRL                	0
#define T42_APPRTHR             	0   /* 0 (TCHTHR/4), 1 to 255 */
#define T42_MAXAPPRAREA         	0   /* 0 (40ch), 1 to 255 */
#define T42_MAXTCHAREA          	0   /* 0 (35ch), 1 to 255 */
#define T42_SUPSTRENGTH         	0   /* 0 (128), 1 to 255 */
#define T42_SUPEXTTO            	0  /* 0 (never expires), 1 to 255 (timeout in cycles) */
#define T42_MAXNUMTCHS          	0  /* 0 to 9 (maximum number of touches minus 1) */
#define T42_SHAPESTRENGTH       	0  /* 0 (10), 1 to 31 */

/* SPT_CTECONFIG_T46  */
#define T46_CTRL                	4	//0     /*Reserved */
#define T46_MODE                	3	/*3*/     /*0: 16X14Y, 1: 17X13Y, 2: 18X12Y, 3: 19X11Y, 4: 20X10Y, 5: 21X15Y, 6: 22X8Y, */
#define T46_IDLESYNCSPERX       	32	//16/*40*/
#define T46_ACTVSYNCSPERX       	32	//24	// 24 //48/*40*/
#define T46_ADCSPERSYNC         	0 
#define T46_PULSESPERADC        	0  /*0:1  1:2   2:3   3:4 pulses */
#define T46_XSLEW               	1  /*0*/     /*0:500nsec,  1:350nsec */
#define T46_SYNCDELAY           	0 

/* PROCI_STYLUS_T47 */              
#define T47_CTRL                	0
#define T47_CONTMIN             	0
#define T47_CONTMAX             	0
#define T47_STABILITY           	0
#define T47_MAXTCHAREA          	0
#define T47_AMPLTHR             	0
#define T47_STYSHAPE            	0
#define T47_HOVERSUP            	0
#define T47_CONFTHR             	0
#define T47_SYNCSPERX           	0

/* PROCG_NOISESUPPRESSION_T48  */
#define T48_CTRL                	1  
#define T48_CFG                 	4
#define T48_CALCFG              	96
#define T48_CALCFG_PLUG             112
#define T48_BASEFREQ            	0  
#define T48_RESERVED0           	0  
#define T48_RESERVED1           	0  
#define T48_RESERVED2           	0  
#define T48_RESERVED3           	0  
#define T48_MFFREQ_2            	0	// 10 
#define T48_MFFREQ_3            	0	// 20  
#define T48_RESERVED4           	0  
#define T48_RESERVED5           	0  
#define T48_RESERVED6           	0  
#define T48_GCACTVINVLDADCS     	6  
#define T48_GCIDLEINVLDADCS     	6  
#define T48_RESERVED7           	0  
#define T48_RESERVED8           	0  
#define T48_GCMAXADCSPERX       	100
#define T48_GCLIMITMIN          	6//4  
#define T48_GCLIMITMAX          	64 
#define T48_GCCOUNTMINTGT       	10 
#define T48_MFINVLDDIFFTHR      	32//0 
#define T48_MFINCADCSPXTHR      	5  
#define T48_MFERRORTHR          	38 
#define T48_SELFREQMAX          	8//20 
#define T48_RESERVED9           	0  
#define T48_RESERVED10          	0  
#define T48_RESERVED11          	0  
#define T48_RESERVED12          	0  
#define T48_RESERVED13          	0  
#define T48_RESERVED14          	0  
#define T48_BLEN                	0  
#define T48_TCHTHR              	50  // 60
#define T48_TCHDI               	3  
#define T48_MOVHYSTI            	11  // 10  
#define T48_MOVHYSTN            	5   // 2  
#define T48_MOVFILTER           	0 
#define T48_NUMTOUCH            	5  
#define T48_MRGHYST             	20 
#define T48_MRGTHR              	25
#define T48_XLOCLIP             	-5  // 2   // 0
#define T48_XHICLIP             	-12 // 0 
#define T48_YLOCLIP             	45  // 0 
#define T48_YHICLIP             	45  // 0 
#define T48_XEDGECTRL           	151 // 148
#define T48_XEDGEDIST           	50  // 55
#define T48_YEDGECTRL           	128 // 0
#define T48_YEDGEDIST           	0
#define T48_JUMPLIMIT           	13
#define T48_TCHHYST             	15  // 12
#define T48_NEXTTCHDI           	0   //  2	// 0


#ifdef ITO_TYPE_CHECK
typedef struct {
	int min;
	int max;
} ito_table_element;

static ito_table_element ito_table[] = {
	{0,	200000}, // 0 Ohm, 0V => WHITE
	{1000000,	1400000}, // 2M Ohm, 1.2V => BLACK
};
#define number_of_elements(ito_table) sizeof(ito_table)/sizeof(ito_table_element)
#define TOUCH_ID_MPP PM8XXX_AMUX_MPP_9
#endif
void TSP_Restart(void);		
int init_hw_setting(void)
{
	int rc; 
	unsigned gpioConfig;

#if (BOARD_VER == PT10 )
	struct regulator *vreg_touch_LVS4; // 1.8V	
	vreg_touch_LVS4 = regulator_get(NULL, "8921_lvs4");
	if(vreg_touch_LVS4 == NULL)
		printk(KERN_ERR "%s: vreg_touch_LVS4  \n", __func__);

	rc = regulator_enable(vreg_touch_LVS4);
	if (rc) {
		printk(KERN_ERR "%s: vreg_LVS4 enable failed (%d)\n", __func__, rc);
		return 0;
	}
#endif

	gpio_request(GPIO_TOUCH_POWER, "touch_power_n");
	gpioConfig = GPIO_CFG(GPIO_TOUCH_POWER, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA);
	rc = gpio_tlmm_config(gpioConfig, GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: GPIO_TOUCH_RST failed (%d)\n",__func__, rc);
		return -1;
	}
	gpio_set_value(GPIO_TOUCH_POWER, 1);


	/*========================================================================*/
	/*  T O U C H    C O N T R O L    P I N                                   */
	/*    Touch Reset, Touch change											  */	 
	/*========================================================================*/

	// GPIO Config: reset pin
	gpio_request(GPIO_TOUCH_RST, "touch_rst_n");
	gpioConfig = GPIO_CFG(GPIO_TOUCH_RST, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA);
	rc = gpio_tlmm_config(gpioConfig, GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: GPIO_TOUCH_RST failed (%d)\n",__func__, rc);
		return -1;
	}      

	// GPIO Config: interrupt pin
	gpio_request(GPIO_TOUCH_CHG, "touch_chg_int");
	gpioConfig = GPIO_CFG(GPIO_TOUCH_CHG, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA);
	rc = gpio_tlmm_config(gpioConfig, GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: GPIO_TOUCH_CHG failed (%d)\n",__func__, rc);
		return -1;
	}        

	TSP_Restart();
	msleep(100);

	return 0;

}

void off_hw_setting(void)
{

#if (BOARD_VER == PT10 )
	int rc; 
	struct regulator *vreg_touch_LVS4; // 1.8V

	vreg_touch_LVS4 = regulator_get(NULL, "8921_lvs4");

	if (IS_ERR(vreg_touch_LVS4)) {
		rc = PTR_ERR(vreg_touch_LVS4);
		printk(KERN_ERR "[QT602240]%s: regulator get of %s failed (%d)\n",
			__func__, "vreg_touch_1_8", rc);
	}

	rc = regulator_disable(vreg_touch_LVS4);
	if ( rc ) printk("[QT602240]8921_lvs4 regulator_disable return:  %d \n", rc);

	regulator_put(vreg_touch_LVS4);
#endif

	gpio_set_value(GPIO_TOUCH_POWER, 0);
	msleep(10);

	gpio_free(GPIO_TOUCH_CHG);

	msleep(100);
}