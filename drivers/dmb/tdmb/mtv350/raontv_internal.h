/******************************************************************************** 
* (c) COPYRIGHT 2010 RAONTECH, Inc. ALL RIGHTS RESERVED.
* 
* This software is the property of RAONTECH and is furnished under license by RAONTECH.                
* This software may be used only in accordance with the terms of said license.                         
* This copyright noitce may not be remoced, modified or obliterated without the prior                  
* written permission of RAONTECH, Inc.                                                                 
*                                                                                                      
* This software may not be copied, transmitted, provided to or otherwise made available                
* to any other person, company, corporation or other entity except as specified in the                 
* terms of said license.                                                                               
*                                                                                                      
* No right, title, ownership or other interest in the software is hereby granted or transferred.       
*                                                                                                      
* The information contained herein is subject to change without notice and should 
* not be construed as a commitment by RAONTECH, Inc.                                                                    
* 
* TITLE 	  : RAONTECH TV internal header file. 
*
* FILENAME    : raontv_internal.h
*
* DESCRIPTION : 
*		All the declarations and definitions necessary for the RAONTECH TV driver.
*
********************************************************************************/

/******************************************************************************** 
* REVISION HISTORY
*
*    DATE	  	  NAME				REMARKS
* ----------  -------------    --------------------------------------------------
* 10/01/2010  Ko, Kevin        Changed the order of E_RTV_TV_MODE_TYPE for HW spec.
* 09/29/2010  Ko, Kevin        Added the FM freq definition of brazil.
* 09/27/2010  Ko, Kevin        Creat for CS Realease
*             /Yang, Maverick  1.Reformating for CS API
*                              2.pll table, ADC clock switching, SCAN function, 
*								 FM function added..
* 04/09/2010  Yang, Maverick   REV1 SETTING 
* 01/25/2010  Yang, Maverick   Created.                                                              
********************************************************************************/

#ifndef __RAONTV_INTERNAL_H__
#define __RAONTV_INTERNAL_H__

#ifdef __cplusplus 
extern "C"{ 
#endif  

#include "raontv.h"


// Do not modify the order!
typedef enum
{	
	RTV_TV_MODE_TDMB   = 0,     // Band III  Korea
	RTV_TV_MODE_DAB_B3 = 1,      // Band III
	RTV_TV_MODE_DAB_L  = 2,      // L-Band		
	RTV_TV_MODE_1SEG   = 3, // UHF
	RTV_TV_MODE_FM     = 4,       // FM
	MAX_NUM_RTV_MODE
} E_RTV_TV_MODE_TYPE;


typedef struct
{
	U8	bReg;
	U8	bVal;
} RTV_REG_INIT_INFO;


typedef struct
{
	U8	bReg;
	U8  bMask;
	U8	bVal;
} RTV_REG_MASK_INFO;


#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
	#if defined(RTV_TSIF_CLK_SPEED_DIV_2) // Host Clk/2
		#define RTV_COMM_CON47_CLK_SEL	(0<<6)
	#elif defined(RTV_TSIF_CLK_SPEED_DIV_4) // Host Clk/4
		#define RTV_COMM_CON47_CLK_SEL	(1<<6)
	#elif defined(RTV_TSIF_CLK_SPEED_DIV_6) // Host Clk/6
		#define RTV_COMM_CON47_CLK_SEL	(2<<6)
	#elif defined(RTV_TSIF_CLK_SPEED_DIV_8) // Host Clk/8
		#define RTV_COMM_CON47_CLK_SEL	(3<<6)
	#else
		#error "Code not present"
	#endif
#endif


#define MSC1_E_OVER_FLOW       0x40
#define MSC1_E_UNDER_FLOW      0x20
#define MSC1_E_INT             0x10
#define MSC0_E_OVER_FLOW       0x08
#define MSC0_E_UNDER_FLOW      0x04
#define MSC0_E_INT             0x02
#define FIC_E_INT              0x01
#define RE_CONFIG_E_INT        0x04

#define MSC1_INTR_BITS	(MSC1_E_INT|MSC1_E_UNDER_FLOW|MSC1_E_OVER_FLOW)
#define MSC0_INTR_BITS	(MSC0_E_INT|MSC0_E_UNDER_FLOW|MSC0_E_OVER_FLOW)

#define INT_E_UCLRL         (0x35)  /// [2]MSC1 int clear [1]MSC0 int clear [0]FIC int clear
#define INT_E_UCLRH         (0x36)  /// [6]OFDM TII done clear

#define INT_E_STATL         (0x33)  /// [7]OFDM Lock status [6]MSC1 overrun [5]MSC1 underrun [4]MSC1 int [3]MSC0 overrun [2]MSC0 underrun [1]MSC0 int [0]FIC int
#define INT_E_STATH         (0x34)  /// [7]OFDM NIS [6]OFDM TII [5]OFDM scan [4]OFDM window position [3]OFDM unlock [2]FEC re-configuration [1]FEC CIF end [0]FEC soft reset

#define DD_E_TOPCON         (0x45)  /// [7]Buf_en [6]PKT_CRC_MODE:enable (stored) [5]MPEG_HEAD [4]MPEG-2TS_EN [3]EPKT_MODE [2]CAS_MODE [1]FIDC_MODE [0]FIC_INIT_EN == 1:enable
#define FIC_E_DDCON         (0x46)  /// [4]FIC_CRC stored- 1:enable [3]FIC uclear-1 [2]FIC_Update - 1:dependent by user  [1]FIC_EN [0]FIG_EN - 1:only FIG 6 dump
#define MSC0_E_CON          (0x47)  /// [3]MSC0 uclear-1 [2]MSC0_en [1]MSC0 read length -1:user length, 0:interrupt length [0]MSC0 interrupt sel-1:user th, 0:CIF end
#define MSC1_E_CON          (0x48)  /// [5]MSC1_length-0:subch+length [4]MSC1 header-0:disable [3]MSC1 uclear-1 [2]MSC1_en [1]MSC1 read length -1:user length, 0:interrupt length [0]MSC1 interrupt sel-1:user th, 0:CIF end
#define OFDM_E_DDCON        (0x49)  /// [3]NIS uclear-1 [2]NIS_Update - 1:dependent by user [1]TII status clear -1:user set only, 0:user set & internal event [0]TII_Update - 1:dependent by user

#define MSC0_E_RSIZE_H      (0x4E)  /// [11:8] setting in MSC0 read length for interrupt clear
#define MSC0_E_RSIZE_L      (0x4F)  /// [7:0]
#define MSC0_E_INTTH_H      (0x50)  /// [11:8] MSC0 interrupt threshold
#define MSC0_E_INTTH_L      (0x51)  /// [7:0]

#define MSC0_E_TSIZE_L      (0x52)  /// [11:8]
#define MSC0_E_TSIZE_H      (0x53)  /// [7:0] MSC0 total size which you can read 

#define MSC1_E_RSIZE_H      (0x54)  /// [11:8] setting in MSC1 read length for interrupt clear
#define MSC1_E_RSIZE_L      (0x55)  /// [7:0]
#define MSC1_E_INTTH_H      (0x56)  /// [11:8] MSC1 interrupt threshold
#define MSC1_E_INTTH_L      (0x57)  /// [7:0]

#define MSC1_E_TSIZE_L      (0x58)  /// [11:8]
#define MSC1_E_TSIZE_H      (0x59)  /// [7:0] MSC1 read length 


#define ACTIVE_CH_SET 0x30

#define RECONFIG_SUBCH0_CON (0x70)  /// [7] RE_SUBCH0_EN  [6] MPEG_EN0 [5:0] RE_SUBCH0_ID 
#define RECONFIG_SUBCH1_CON (0x71)  /// [7] RE_SUBCH1_EN  [6] MPEG_EN1 [5:0] RE_SUBCH1_ID 
#define RECONFIG_SUBCH2_CON (0x72)  /// [7] RE_SUBCH2_EN  [6] MPEG_EN2 [5:0] RE_SUBCH2_ID 
#define RECONFIG_SUBCH3_CON (0x73)  /// [7] RE_SUBCH3_EN  [5:0] RE_SUBCH3_ID
#define RECONFIG_SUBCH4_CON (0x74)  /// [7] RE_SUBCH4_EN  [5:0] RE_SUBCH4_ID
#define RECONFIG_SUBCH5_L   (0x75)  /// [7] RE_SUBCH5_EN  [6] RE_SUBCH5_PACK_EN  [5:0] RE_SUBCH5_ID
#define RECONFIG_SUBCH5_M   (0x76)  /// [7:0] RE_SUBCH5_PACK_ADDR
#define RECONFIG_SUBCH5_H   (0x77)  /// [9:8] RE_SUBCH5_PACK_ADDR
#define RECONFIG_SUBCH6_L   (0x78)  /// [7] RE_SUBCH6_EN  [6] RE_SUBCH6_PACK_EN  [5:0] RE_SUBCH6_ID
#define RECONFIG_SUBCH6_M   (0x79)  /// [7:0] RE_SUBCH6_PACK_ADDR
#define RECONFIG_SUBCH6_H   (0x7A)  /// [9:8] RE_SUBCH6_PACK_ADDR


#define MODE1 2 		
#define MODE2 1
#define MODE3 0


#define MAP_SEL_REG 	0x03

#define OFDM_PAGE       0x02 // for 1seg
#define FEC_PAGE        0x03 // for 1seg
#define COMM_PAGE       0x04
#define FM_PAGE         0x06 // T-DMB OFDM/FM
#define HOST_PAGE       0x07
#define CAS_PAGE        0x08
#define DD_PAGE         0x09 // FEC for TDMB, DAB, FM

#define FIC_PAGE        0x0A
#define MSC0_PAGE       0x0B
#define MSC1_PAGE       0x0C
#define RF_PAGE         0x0F
#define OFDM_E_CON      (0x10)


#define DEMOD_0SC_DIV2_ON  0x80
#define DEMOD_0SC_DIV2_OFF 0x00

#if (RTV_SRC_CLK_FREQ_KHz >= 32000)
	#define DEMOD_OSC_DIV2 	DEMOD_0SC_DIV2_ON
#else 
	#define DEMOD_OSC_DIV2 	DEMOD_0SC_DIV2_OFF
#endif 


#define MAP_SEL_VAL(page)		(DEMOD_OSC_DIV2|page)
#define RTV_REG_MAP_SEL(page)	\
	RTV_REG_SET(MAP_SEL_REG, MAP_SEL_VAL(page))

#define RTV_TS_STREAM_DISABLE_DELAY		20 // ms


// ISDB-T Channel 
#define ISDBT_CH_NUM_START__JAPAN			13
#define ISDBT_CH_NUM_END__JAPAN				62
#define ISDBT_CH_FREQ_START__JAPAN			473143
#define ISDBT_CH_FREQ_STEP__JAPAN			6000

#define ISDBT_CH_NUM_START__BRAZIL			14
#define ISDBT_CH_NUM_END__BRAZIL			69
#define ISDBT_CH_FREQ_START__BRAZIL			473143
#define ISDBT_CH_FREQ_STEP__BRAZIL			6000

#define ISDBT_CH_NUM_START__ARGENTINA		14
#define ISDBT_CH_NUM_END__ARGENTINA			69
#define ISDBT_CH_FREQ_START__ARGENTINA		473143
#define ISDBT_CH_FREQ_STEP__ARGENTINA		6000

// T-DMB Channel 
#define TDMB_CH_FREQ_START__KOREA		175280
#define TDMB_CH_FREQ_STEP__KOREA		1728 // about...

// DAB Channel
#define DAB_CH_BAND3_START_FREQ_KHz		174928
#define DAB_CH_BAND3_STEP_FREQ_KHz		1712 // in KHz

#define DAB_CH_LBAND_START_FREQ_KHz		1452960
#define DAB_CH_LBAND_STEP_STEP_FREQ_KHz	1712 // in KHz


extern volatile E_RTV_ADC_CLK_FREQ_TYPE g_eRtvAdcClkFreqType;
extern E_RTV_COUNTRY_BAND_TYPE g_eRtvCountryBandType;

#ifdef RTV_DAB_ENABLE
extern	volatile E_RTV_TV_MODE_TYPE g_curDabSetType;
extern     BOOL g_fDabEnableReconfigureIntr;
	#ifdef RTV_CIF_MODE_ENABLED
		extern INT g_nDabSpiDataSvcCifSize;
	#endif
#endif

#if defined(RTV_TDMB_ENABLE) || defined(RTV_DAB_ENABLE)
	typedef enum
	{
		FIC_NOT_OPENED = 0,
		FIC_OPENED_PATH_NOT_USE_IN_PLAY,
		FIC_OPENED_PATH_I2C_IN_SCAN,
		FIC_OPENED_PATH_TSIF_IN_SCAN,
		FIC_OPENED_PATH_I2C_IN_PLAY,
		FIC_OPENED_PATH_TSIF_IN_PLAY
	} E_RTV_FIC_OPENED_PATH_TYPE;

	extern BOOL g_fRtvFicOpened;
	#if defined(RTV_IF_SPI) && defined(RTV_CIF_MODE_ENABLED)
	extern U32 g_aOpenedCifSubChBits_MSC0[2];
	#endif

	#if defined(RTV_IF_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE)
	extern E_RTV_FIC_OPENED_PATH_TYPE g_nRtvFicOpenedStatePath;
	#endif
#endif


/* Use SPI/EBI2 interrupt handler to prevent the changing of register map. */
extern volatile BOOL g_fRtvChannelChange;
extern BOOL g_fRtvStreamEnabled;

extern UINT g_nRtvMscThresholdSize;	
extern U8 g_bRtvIntrMaskRegL;
#ifdef RTV_DAB_ENABLE
	extern U8 g_bRtvIntrMaskRegH;
#endif



#define FM_MIN_FREQ_KHz			75950 // See PLL table in raontv_rf_pll_data_fm.h
#define FM_MAX_FREQ_KHz			108050
#define FM_SCAN_STEP_FREQ_KHz		(RTV_FM_CH_STEP_FREQ_KHz/2)

/*==============================================================================
 *
 * Common inline functions.
 *
 *============================================================================*/ 
 
/* Forward prototype. */
static INLINE void rtv_SetupThreshold_MSC1(U16 wThresholdSize);
static INLINE void rtv_SetupMemory_MSC1(E_RTV_TV_MODE_TYPE eTvMode);

static INLINE E_RTV_TV_MODE_TYPE rtv_GetDabTvMode(U32 dwChFreqKHz)
{
#ifdef RTV_DAB_LBAND_ENABLED
	if( dwChFreqKHz >= DAB_CH_LBAND_START_FREQ_KHz)
	{
		//RTV_DBGMSG0("return RTV_TV_MODE_DAB_L;\n");
		return RTV_TV_MODE_DAB_L; // L-Band
	}
	else
#endif
	{
		switch(dwChFreqKHz)
		{
			case 175280/*7A*/: case 177008/*7B*/: case 178736/*7C*/: 
			case 181280/*8A*/: case 183008/*8B*/: case 184736/*8C*/: 
			case 187280/*9A*/: case 189008/*9B*/: case 190736/*9C*/: 
			case 193280/*10A*/: case 195008/*10B*/: case 196736/*10C*/: 
			case 199280/*11A*/: case 201008/*11B*/: case 202736/*11C*/: 
			case 205280/*12A*/: case 207008/*12B*/: case 208736/*12C*/: 
			case 211280/*13A*/: case 213008/*13B*/: case 214736/*13C*/:
				//RTV_DBGMSG0("return RTV_TV_MODE_TDMB;\n");
				return RTV_TV_MODE_TDMB; // Korea TDMB.

			default: 
				//RTV_DBGMSG0("return RTV_TV_MODE_DAB_B3;\n");
				return RTV_TV_MODE_DAB_B3;	// DAB Band-III	
		}
	}
}

// Pause straem
static INLINE void rtv_StreamDisable(E_RTV_TV_MODE_TYPE eTvMode)
{	
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF
	if(g_fRtvStreamEnabled == FALSE)
		return;

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x62, g_bRtvIntrMaskRegL|MSC0_INTR_BITS|MSC1_INTR_BITS);

#elif defined(RTV_IF_SERIAL_TSIF)
	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x29, 0x08);
	RTV_DELAY_MS(RTV_TS_STREAM_DISABLE_DELAY);
#endif

    g_fRtvStreamEnabled = FALSE; 
#endif /* #ifndef RTV_IF_MPEG2_PARALLEL_TSIF */
}

// Resume straem
static INLINE void rtv_StreamRestore(E_RTV_TV_MODE_TYPE eTvMode)
{
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF
	if(g_fRtvStreamEnabled == TRUE)
		return;

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(INT_E_UCLRL, MSC0_INTR_BITS|MSC1_INTR_BITS);

	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x62, g_bRtvIntrMaskRegL);

#else
	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x29, 0x00);
#endif

    g_fRtvStreamEnabled = TRUE;  
#endif /* #ifndef RTV_IF_MPEG2_PARALLEL_TSIF */
}


/* Enable the stream path forcely for ISDB-T and FM only! */
static INLINE void rtv_StreamEnable(void)
{
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF
	if(g_fRtvStreamEnabled == TRUE)
		return;
		
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)				
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x35, MSC1_INTR_BITS); // MSC1 Interrupt status clear.	

	RTV_REG_MAP_SEL(HOST_PAGE);
	g_bRtvIntrMaskRegL = 0x8F;
    RTV_REG_SET(0x62, g_bRtvIntrMaskRegL);
#else
	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x29, 0x00);
#endif

	rtv_SetupThreshold_MSC1(g_nRtvMscThresholdSize);
  	rtv_SetupMemory_MSC1(RTV_TV_MODE_1SEG);

	g_fRtvStreamEnabled = TRUE;
#endif /* #ifndef RTV_IF_MPEG2_PARALLEL_TSIF */
}



static INLINE void rtv_ConfigureTsifFormat(void)
{
	RTV_REG_MAP_SEL(COMM_PAGE);
	
#if defined(RTV_IF_MPEG2_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE) 
  #if defined(RTV_TSIF_FORMAT_1)
	RTV_REG_SET(0x45, 0x00);    
  #elif defined(RTV_TSIF_FORMAT_2)
	RTV_REG_SET(0x45, 0x02);
  #elif defined(RTV_TSIF_FORMAT_3)
	RTV_REG_SET(0x45, 0x21);
  #elif defined(RTV_TSIF_FORMAT_4)
	RTV_REG_SET(0x45, 0x23);
  #else
	#error "Code not present"
  #endif

#elif defined(RTV_IF_QUALCOMM_TSIF)
  #if defined(RTV_TSIF_FORMAT_1)
	RTV_REG_SET(0x45, 0x00);    
  #elif defined(RTV_TSIF_FORMAT_2)
	RTV_REG_SET(0x45, 0xE9);

  #elif defined(RTV_TSIF_FORMAT_3)
	RTV_REG_SET(0x45, 0xE1);
  #elif defined(RTV_TSIF_FORMAT_4)
	RTV_REG_SET(0x45, 0x40);
  #elif defined(RTV_TSIF_FORMAT_5)
	RTV_REG_SET(0x45, 0x21);    
  #else
	#error "Code not present"
  #endif
#endif	

#if defined(RTV_IF_MPEG2_SERIAL_TSIF)
	RTV_REG_SET(0x46, 0x84);
#elif defined(RTV_IF_QUALCOMM_TSIF)
	RTV_REG_SET(0x46, 0xA4);
#elif defined(RTV_IF_SPI_SLAVE)
	RTV_REG_SET(0x46, 0x86);
#endif
}


static INLINE void rtv_ResetMemory_MSC0(void)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x47, 0x00);  // MSC0 memory control register clear.
}

static INLINE void rtv_ResetMemory_MSC1(void)
{
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x48, 0x00);  // MSC1 memory control register clear.
#endif	
}

// Only for T-DMB and DAB.
static INLINE void rtv_ClearAndSetupMemory_MSC0(void)
{
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
   #ifdef RTV_MSC_INTR_ISTAUS_ACC_CLR_MODE
   	U8 msc0_int_usel = 0;  /// 0: auto&uclr, 1: user set only	
   #else
   	U8 msc0_int_usel = 1;  /// 0: auto&uclr, 1: user set only	
   #endif   	
#else
 	U8 msc0_int_usel = 0;  /// 0: auto&uclr, 1: user set only: TSIF
#endif

#ifndef RTV_CIF_MODE_ENABLED /* Individual Mode */
  	U8 int_type = 1; /// 0: CIF end,	 1: Threshold 
#else
	U8 int_type = 0; /// 0: CIF end,	 1: Threshold 
#endif

	RTV_REG_SET(0x47, 0x00);	
	RTV_REG_SET(0x47, (msc0_int_usel<<3) | (1/*msc0_en*/<<2) | int_type);
#endif /* #ifndef RTV_IF_MPEG2_PARALLEL_TSIF */	
}


// Only for T-DMB and DAB.
static INLINE void rtv_SetupThreshold_MSC0(U16 wThresholdSize)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x50,((wThresholdSize>>8) & 0x0F));
	RTV_REG_SET(0x51,(wThresholdSize & 0xFF));
}

static INLINE void rtv_SetupMemory_MSC0(U16 wThresholdSize)
{
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF
	RTV_REG_MAP_SEL(DD_PAGE);

	rtv_ClearAndSetupMemory_MSC0();

	RTV_REG_SET(0x35, 0x02); // MSC0 Interrupt clear.
	
#endif /* #ifndef RTV_IF_MPEG2_PARALLEL_TSIF */	
}

// For SPI interface ISR.
static INLINE void rtv_ClearAndSetupMemory_MSC1(E_RTV_TV_MODE_TYPE eTvMode)
{
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF
	//U8 msc1_length_off = 1;/// 0: subch+16-bit length, 1: subch
	//U8 msc1_header_on = 0; /// 0: disable for only one ts buffer, 1: enable for 2~3 ts buffer
	//U8 int_type = 1;       /// 0: CIF end,   1: Threshold

 #if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
   #ifdef RTV_MSC_INTR_ISTAUS_ACC_CLR_MODE
   	U8 msc1_int_usel = 0;  /// 0: auto&uclr, 1: user set only	
   #else
   	U8 msc1_int_usel = 1;  /// 0: auto&uclr, 1: user set only	
   #endif   	
 #else
 	/* TSIF */
 	U8 msc1_int_usel = 0;  /// 0: auto&uclr, 1: user set only: TSIF
 #endif

 #ifndef RTV_CIF_MODE_ENABLED /* Individual Mode */
 	U8 int_type = 1; /// 0: CIF end,	1: Threshold (for SPI, Header OFF)
 #else
    #if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	 U8 int_type = 1; /// 0: CIF end,	1: Threshold (for SPI, Always Threshold mode)
    #else
	 U8 int_type = 0; /// 0: CIF end,	1: Threshold 
    #endif
 #endif

#if defined(TDMBorDAB_FMor1SEG_ENABLED) && defined(RTV_CIF_MODE_ENABLED)
 	if((eTvMode==RTV_TV_MODE_1SEG) || (eTvMode==RTV_TV_MODE_FM))
		int_type = 1; // Threshold

#elif defined(FMor1SEG_ONLY_ENABLED)
	int_type = 1; // Threshold
#endif		

	RTV_REG_SET(0x48, 0x00);	
	RTV_REG_SET(0x48, (0/*msc1_length_off*/<<5) | (0/*msc1_header_on*/<<4) | (msc1_int_usel<<3) | (1/*msc1_en*/<<2) | int_type);

#endif /* #ifndef RTV_IF_MPEG2_PARALLEL_TSIF */	
}

static INLINE void rtv_SetupThreshold_MSC1(U16 wThresholdSize)
{
	RTV_REG_MAP_SEL(DD_PAGE);
    RTV_REG_SET(0x56, ((wThresholdSize>>8) & 0x0F));
	RTV_REG_SET(0x57, (wThresholdSize & 0xFF));
}

static INLINE void rtv_SetupMemory_MSC1(E_RTV_TV_MODE_TYPE eTvMode)
{
#ifndef RTV_IF_MPEG2_PARALLEL_TSIF
	RTV_REG_MAP_SEL(DD_PAGE);

	rtv_ClearAndSetupMemory_MSC1(eTvMode);
#endif /* #ifndef RTV_IF_MPEG2_PARALLEL_TSIF */	
}

/* Only this sub channel contains the RS decoder. */
static INLINE void rtv_Set_MSC1_SUBCH0(UINT nSubChID, BOOL fSubChEnable, BOOL fRsEnable)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	
#if !defined(RTV_IF_MPEG2_PARALLEL_TSIF)
	RTV_REG_SET(0x3A, (fSubChEnable << 7) | (fRsEnable << 6) | nSubChID);
#else
	if(fRsEnable ==TRUE)	
	{
		RTV_REG_SET(0xFF, 0x00);
		RTV_REG_SET(0x3A, (fSubChEnable << 7) | (fRsEnable << 6) | nSubChID);

	}
	else 
	{   RTV_REG_SET(0x3A, 0x00);
		RTV_REG_SET(0xFF, (fSubChEnable << 7) | nSubChID);
	}
#endif
}

static INLINE void rtv_Set_MSC1_SUBCH1(UINT nSubChID, BOOL fSubChEnable)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x3B, (fSubChEnable << 7) | nSubChID);
}

static INLINE void rtv_Set_MSC1_SUBCH2(UINT nSubChID, BOOL fSubChEnable)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x3C, (fSubChEnable << 7) | nSubChID);
}

static INLINE void rtv_Set_MSC0_SUBCH3(UINT nSubChID, BOOL fSubChEnable)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x3D, (fSubChEnable << 7) | nSubChID);
}

static INLINE void rtv_Set_MSC0_SUBCH4(UINT nSubChID, BOOL fSubChEnable)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x3E, (fSubChEnable << 7) | nSubChID);
}

static INLINE void rtv_Set_MSC0_SUBCH5(UINT nSubChID, BOOL fSubChEnable)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x3F, (fSubChEnable << 7) | nSubChID);
}

static INLINE void rtv_Set_MSC0_SUBCH6(UINT nSubChID, BOOL fSubChEnable)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x42, (fSubChEnable << 7) | nSubChID);
}


/*==============================================================================
 *
 * T-DMB/DAB inline functions.
 *
 *============================================================================*/ 
#if defined(RTV_TDMB_ENABLE) || defined(RTV_DAB_ENABLE)
static INLINE void rtv_SetupMemory_FIC(void)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x46, 0x10); // auto user clr, get fic  CRC 2byte including[4]
	RTV_REG_SET(0x46, 0x16); // FIC enable
}

static INLINE void rtv_ResetMemory_FIC(void)
{
	RTV_REG_MAP_SEL(DD_PAGE);
	RTV_REG_SET(0x46, 0x00); 
}

static INLINE UINT rtv_GetFicSize(void)
{
	U8 tr_mode;
	UINT aFicSize[4] = {384/*TRMODE 1*/, 96, 128, 192};
	
   	RTV_REG_MAP_SEL(DD_PAGE);
	tr_mode = RTV_REG_GET(0x37);
	if( tr_mode & 0x01 )
	{	
		tr_mode = (tr_mode & 0x06) >> 1;
		return aFicSize[tr_mode];
	}
	else
		return 0;
}

static INLINE void rtv_DisableFicTsifPath(void)
{
#if defined(RTV_IF_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE)
	U8 bCon47;

	RTV_REG_MAP_SEL(COMM_PAGE);
	bCon47 = RTV_REG_GET(0x47);
	bCon47 &= ~(1<<2); /* FIC disable */ 	
	RTV_REG_SET(0x47, bCon47);
#endif
}

static INLINE void rtv_EnableFicTsifPath(void)
{
#if defined(RTV_IF_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE)
	U8 bCon47;

	RTV_REG_MAP_SEL(COMM_PAGE);
	bCon47 = RTV_REG_GET(0x47);
	bCon47 |= (1<<2); /* FIC enable */		
	RTV_REG_SET(0x47, bCon47);
#endif
}

static INLINE void rtv_DisableFicInterrupt(void)
{
#if defined(RTV_FIC_SPI_INTR_ENABLED) || defined(RTV_FIC_I2C_INTR_ENABLED)
	RTV_REG_MAP_SEL(HOST_PAGE);
	g_bRtvIntrMaskRegL |= FIC_E_INT;
	RTV_REG_SET(0x62, g_bRtvIntrMaskRegL);     
#endif
}

static INLINE void rtv_EnableFicInterrupt(void)
{
#if defined(RTV_FIC_SPI_INTR_ENABLED) || defined(RTV_FIC_I2C_INTR_ENABLED)
    RTV_REG_MAP_SEL(DD_PAGE);
    RTV_REG_SET(0x35, 0x01); // FIC Interrupt status clear.	
    
    /* Enable the FIC interrupt. */
    RTV_REG_MAP_SEL(HOST_PAGE);
    g_bRtvIntrMaskRegL &= ~(FIC_E_INT);
    RTV_REG_SET(0x62, g_bRtvIntrMaskRegL);
#endif
}

static INLINE void rtv_CloseFIC(UINT nOpenedSubChNum,
			E_RTV_FIC_OPENED_PATH_TYPE eOpenedPath)
{
	rtv_ResetMemory_FIC();

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	rtv_DisableFicInterrupt();

#elif defined(RTV_IF_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE)
	switch (eOpenedPath) {
	case FIC_OPENED_PATH_I2C_IN_SCAN: /* No sub channel */
	case FIC_OPENED_PATH_I2C_IN_PLAY:
		rtv_DisableFicInterrupt(); /* From I2C interrupt */
		break;

	case FIC_OPENED_PATH_TSIF_IN_SCAN: /* No sub channel */
	case FIC_OPENED_PATH_TSIF_IN_PLAY: /* Have sub channel */
		rtv_DisableFicTsifPath();
		break;

	case FIC_OPENED_PATH_NOT_USE_IN_PLAY:
	#if defined(RTV_FIC__SCAN_I2C__PLAY_NA)
		rtv_DisableFicInterrupt();
	#elif defined(RTV_FIC__SCAN_TSIF__PLAY_NA)
		rtv_DisableFicTsifPath();
	#endif
		break;

	default:
		break;
	}
#endif


}

static INLINE E_RTV_FIC_OPENED_PATH_TYPE rtv_OpenFIC(UINT nOpenedSubChNum)
{
	E_RTV_FIC_OPENED_PATH_TYPE ePath = FIC_OPENED_PATH_NOT_USE_IN_PLAY;

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	rtv_EnableFicInterrupt();

#else
	if (nOpenedSubChNum == 0) /* Scan state */
	{
	#if defined(RTV_FIC__SCAN_I2C__PLAY_NA)\
	|| defined(RTV_FIC__SCAN_I2C__PLAY_I2C)\
	|| defined(RTV_FIC__SCAN_I2C__PLAY_TSIF)
		rtv_DisableFicTsifPath(); /* For I2C path */
		rtv_EnableFicInterrupt();
		ePath = FIC_OPENED_PATH_I2C_IN_SCAN;

	#elif defined(RTV_FIC__SCAN_TSIF__PLAY_NA)\
	|| defined(RTV_FIC__SCAN_TSIF__PLAY_I2C)\
	|| defined(RTV_FIC__SCAN_TSIF__PLAY_TSIF)
		rtv_EnableFicTsifPath();
		ePath = FIC_OPENED_PATH_TSIF_IN_SCAN;

	#else
		#error "Code not present"
	#endif
	}
	else /* Play state */
	{
	#if defined(RTV_FIC__SCAN_I2C__PLAY_I2C)\
	|| defined(RTV_FIC__SCAN_TSIF__PLAY_I2C)
		rtv_DisableFicTsifPath(); /* For I2C path */
		rtv_EnableFicInterrupt();
		ePath = FIC_OPENED_PATH_I2C_IN_PLAY;
	
	#elif defined(RTV_FIC__SCAN_I2C__PLAY_TSIF)\
	|| defined(RTV_FIC__SCAN_TSIF__PLAY_TSIF)
		rtv_DisableFicInterrupt(); /* To prevent I2C interrupt */
		rtv_EnableFicTsifPath();
		ePath = FIC_OPENED_PATH_TSIF_IN_PLAY;

	#elif defined(RTV_FIC__SCAN_I2C__PLAY_NA)
		rtv_DisableFicInterrupt();
		ePath = FIC_OPENED_PATH_NOT_USE_IN_PLAY;

	#elif defined(RTV_FIC__SCAN_TSIF__PLAY_NA)
		rtv_DisableFicTsifPath();
		ePath = FIC_OPENED_PATH_NOT_USE_IN_PLAY;

	#else
		#error "Code not present"
	#endif
	}
#endif

	rtv_SetupMemory_FIC(); 

	return ePath;
}

#if defined(RTV_IF_SPI) && defined(RTV_CIF_MODE_ENABLED)
static INLINE void rtv_DeleteSpiCifSubChannelID_MSC0(unsigned int nSubChID)
{
	UINT nArrayIdx = nSubChID >> 5; /* Divide by 32 */
	U32 dwBitVal = 1 << (nSubChID & 31); /* Modular and Shift */
	
	if((g_aOpenedCifSubChBits_MSC0[nArrayIdx] & dwBitVal) != 0)
		g_aOpenedCifSubChBits_MSC0[nArrayIdx] &= ~dwBitVal; 
}

/* This function add a sub channel ID to verfiy the SPI CIF header.*/
static INLINE void rtv_AddSpiCifSubChannelID_MSC0(unsigned int nSubChID)
{
	UINT nArrayIdx = nSubChID >> 5; /* Divide by 32 */
	U32 dwBitVal = 1 << (nSubChID & 31); /* Modular and Shift */

	if((g_aOpenedCifSubChBits_MSC0[nArrayIdx] & dwBitVal) == 0)
		g_aOpenedCifSubChBits_MSC0[nArrayIdx] |= dwBitVal;
	else {
		RTV_DBGMSG1("[rtv_AddSpiCifSubChannelID_MSC0] Error: %u\n", nSubChID);
	}
}

static INLINE BOOL rtv_IsOpenedCifSubChannelID_MSC0(unsigned int nSubChID)
{
	UINT nArrayIdx = nSubChID >> 5; /* Divide by 32 */
	U32 dwBitVal = 1 << (nSubChID & 31); /* Modular and Shift */

	if((g_aOpenedCifSubChBits_MSC0[nArrayIdx] & dwBitVal) != 0)
		return TRUE;
	else
		return FALSE;
}

static INLINE unsigned int rtv_VerifySpiCif_MSC0(const unsigned char *buf,
										unsigned int size)
{
	unsigned int ch_length, subch_id, remaining_size = size;
	const unsigned char *header_ptr = buf;

	do {
		ch_length = (header_ptr[2]<<8) | header_ptr[3];
		if((ch_length < 4) || (ch_length > (3*1024)))
			break;

		if((header_ptr[1] & 0x7F) != 0)
			break;

		subch_id = header_ptr[0] >> 2;
		if(rtv_IsOpenedCifSubChannelID_MSC0(subch_id) == FALSE)
				break;

		header_ptr += ch_length;                   
		remaining_size -= ch_length;
	} while(remaining_size > 0);

	return size - remaining_size; /* Returns the valid size. */
}
#endif /* #if defined(RTV_IF_SPI) && defined(RTV_CIF_MODE_ENABLED) */
#endif /* #if defined(RTV_TDMB_ENABLE) || defined(RTV_DAB_ENABLE) */

#ifdef RTV_DAB_ENABLE
static INLINE void rtv_DisableReconfigInterrupt(void)
{
	if(g_fDabEnableReconfigureIntr == FALSE)
		return;
	
	RTV_REG_MAP_SEL(HOST_PAGE);
	g_bRtvIntrMaskRegH |= (RE_CONFIG_E_INT);
	RTV_REG_SET(0x63, g_bRtvIntrMaskRegH);	   

	g_fDabEnableReconfigureIntr = FALSE;
}

static INLINE void rtv_EnableReconfigInterrupt(void)
{
	if(g_fDabEnableReconfigureIntr == TRUE)
		return;
		
	g_fDabEnableReconfigureIntr = TRUE;

	RTV_REG_MAP_SEL(HOST_PAGE);
	g_bRtvIntrMaskRegH &= ~(RE_CONFIG_E_INT);
	RTV_REG_SET(0x63, g_bRtvIntrMaskRegH);	   

}
#endif /* #ifdef RTV_DAB_ENABLE */


/*==============================================================================
 *
 * Parallel TSIF inline functions.
 *
 *============================================================================*/ 
#ifdef RTV_IF_MPEG2_PARALLEL_TSIF
#if defined(RTV_ISDBT_ENABLE)
static INLINE void rtv_SetParallelTsif_1SEG_Only(void)
{

	RTV_REG_MAP_SEL(FEC_PAGE);
	
#ifdef RTV_FEC_SERIAL_ENABLE  /*Serial Out*/

#if defined(RTV_TSIF_FORMAT_1)
    RTV_REG_SET(0x6E, 0x91);    
#elif defined(RTV_TSIF_FORMAT_2)
    #error "Code not present"    
#elif defined(RTV_TSIF_FORMAT_3)
    RTV_REG_SET(0x6E, 0x90);    
#elif defined(RTV_TSIF_FORMAT_4)
    #error "Code not present" 
#elif defined(RTV_TSIF_FORMAT_5)
   RTV_REG_SET(0x6E, 0x90); 
#else
    #error "Code not present"
#endif
	RTV_REG_SET(0x6F, 0x81);
    RTV_REG_SET(0x70, 0x88);   

#else /* Parallel Out */
	
#if defined(RTV_TSIF_FORMAT_1)
    RTV_REG_SET(0x6E, 0x11);    
#elif defined(RTV_TSIF_FORMAT_2)
    RTV_REG_SET(0x6E, 0x13);  
#elif defined(RTV_TSIF_FORMAT_3)
    RTV_REG_SET(0x6E, 0x10);    
#elif defined(RTV_TSIF_FORMAT_4)
    RTV_REG_SET(0x6E, 0x12); 
#else
    #error "Code not present"
#endif
	RTV_REG_SET(0x6F, 0x02);
    RTV_REG_SET(0x70, 0x88);   
	
#endif

}

#elif defined(RTV_TDMB_ENABLE)
static INLINE void rtv_SetParallelTsif_TDMB_Only(void)
{
	RTV_REG_MAP_SEL(0x09);
	
#if defined(RTV_TSIF_FORMAT_1)
    RTV_REG_SET(0xDD, 0xD0);    
#elif defined(RTV_TSIF_FORMAT_2)
    RTV_REG_SET(0xDD, 0xD2);    
#elif defined(RTV_TSIF_FORMAT_3)
    RTV_REG_SET(0xDD, 0xD1);    
#elif defined(RTV_TSIF_FORMAT_4)
    RTV_REG_SET(0xDD, 0xD3);    
#else
    #error "Code not present"
#endif
	RTV_REG_SET(0xDE, 0x12);
	RTV_REG_SET(0x45, 0xB0);
}
#else
	#error "Code not present"
#endif
#endif


/*==============================================================================
 * External functions for RAONTV driver core.
 *============================================================================*/ 
void rtv_ConfigureHostIF(void);
INT  rtv_InitSystem(E_RTV_TV_MODE_TYPE eTvMode, E_RTV_ADC_CLK_FREQ_TYPE eAdcClkFreqType);

#ifdef __cplusplus 
} 
#endif 

#endif /* __RAONTV_INTERNAL_H__ */

/*
WINDOWS
	#define INLINE __inline
	#define FORCE_INLINE __forceinline
	#define NAKED __declspec(naked)

__GNUC__
	#define INLINE __inline__
	#define FORCE_INLINE __attribute__((always_inline)) __inline__
	#define NAKED __attribute__((naked))

ARM RVDS compiler
	#define INLINE __inline
	#define FORCE_INLINE __forceinline
	#define NAKED __asm
*/


