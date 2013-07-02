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
* TITLE 	  : RAONTECH TV device driver API header file. 
*
* FILENAME    : raontv.c
*
* DESCRIPTION : 
*		Configuration for RAONTECH TV Services.
*
********************************************************************************/

/******************************************************************************** 
* REVISION HISTORY
*
*    DATE	  	  NAME				REMARKS
* ----------  -------------    --------------------------------------------------
* 09/27/2010  Ko, Kevin        Creat for CS Realease
*             /Yang, Maverick  1.Reformating for CS API
*                              2.pll table, ADC clock switching, SCAN function, 
*								 FM function added..
********************************************************************************/
#include <linux/kernel.h>
#include <linux/delay.h>
#include "mtv350_bb.h"

#include "raontv.h"
#include "raontv_rf.h"
//#include "raontv_internal.h"

volatile BOOL g_fRtvChannelChange;

volatile E_RTV_ADC_CLK_FREQ_TYPE g_eRtvAdcClkFreqType;
BOOL g_fRtvStreamEnabled;

#if defined(RTV_TDMB_ENABLE) || defined(RTV_ISDBT_ENABLE)
	E_RTV_COUNTRY_BAND_TYPE g_eRtvCountryBandType;
#endif

#if defined(RTV_TDMB_ENABLE) || defined(RTV_DAB_ENABLE)
	BOOL g_fRtvFicOpened;
	#if defined(RTV_IF_SPI) && defined(RTV_CIF_MODE_ENABLED)
	U32 g_aOpenedCifSubChBits_MSC0[2]; /* Used sub channel ID bits. [0]: 0 ~ 31, [1]: 32 ~ 63 */
	#endif

	#if defined(RTV_IF_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE)
	E_RTV_FIC_OPENED_PATH_TYPE g_nRtvFicOpenedStatePath;
	#endif
#endif


#ifdef RTV_DAB_ENABLE
	volatile E_RTV_TV_MODE_TYPE g_curDabSetType;
#endif

#ifdef RTV_IF_EBI2
	VU8 g_bRtvEbiMapSelData = 0x7;
#endif

UINT g_nRtvMscThresholdSize; 
U8 g_bRtvIntrMaskRegL;
#ifdef RTV_DAB_ENABLE
	U8 g_bRtvIntrMaskRegH;
#endif

void rtv_ConfigureHostIF(void)
{
#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
	RTV_REG_MAP_SEL(HOST_PAGE);
    RTV_REG_SET(0x77, 0x15);   // TSIF Enable
    RTV_REG_SET(0x22, 0x48);   

  #if defined(RTV_IF_MPEG2_PARALLEL_TSIF)
  	#ifdef RTV_FEC_SERIAL_ENABLE  /*Serial Out*/
        RTV_REG_SET(0x04, 0x29);   // I2C + TSIF Mode Enable
	#else
		RTV_REG_SET(0x04, 0x01);   // I2C + TSIF Mode Enable
	#endif
  #else
	RTV_REG_SET(0x04, 0x29);   // I2C + TSIF Mode Enable
  #endif
  
	RTV_REG_SET(0x0C, 0xF4);   // TSIF Enable

#elif defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x77, 0x14);   //SPI Mode Enable
    RTV_REG_SET(0x04, 0x28);   // SPI Mode Enable
	RTV_REG_SET(0x0C, 0xF5);
 
#else
	#error "Code not present"
#endif
}

INT rtv_InitSystem(E_RTV_TV_MODE_TYPE eTvMode, E_RTV_ADC_CLK_FREQ_TYPE eAdcClkFreqType)
{
	INT nRet;
	int i;
	
	g_fRtvChannelChange = FALSE;
	g_fRtvStreamEnabled = FALSE;

	g_bRtvIntrMaskRegL = 0xFF;
#ifdef RTV_DAB_ENABLE
	g_bRtvIntrMaskRegH = 0xFF;
#endif

#if defined(RTV_TDMB_ENABLE) || defined(RTV_DAB_ENABLE)
	g_fRtvFicOpened = FALSE;
	#if defined(RTV_IF_SPI) && defined(RTV_CIF_MODE_ENABLED)
	g_aOpenedCifSubChBits_MSC0[0] = 0x0;
	g_aOpenedCifSubChBits_MSC0[1] = 0x0;
	#endif
#endif

	for(i=1; i<=100; i++)
	{
		RTV_REG_MAP_SEL(HOST_PAGE);
		RTV_REG_SET(0x7D, 0x06);
		if(RTV_REG_GET(0x7D) == 0x06)
		{
			goto RTV_POWER_ON_SUCCESS;
		}

		RTV_DBGMSG1("[rtv_InitSystem] Power On wait: %d\n", i);

		RTV_DELAY_MS(5);
	}

	RTV_DBGMSG1("rtv_InitSystem: Power On Check error: %d\n", i);
	return RTV_POWER_ON_CHECK_ERROR;

RTV_POWER_ON_SUCCESS:
	
	rtvRF_ConfigurePowerType(eTvMode);

	if((nRet=rtvRF_ConfigureAdcClock(eTvMode, eAdcClkFreqType)) != RTV_SUCCESS)
		return nRet;
		
	return RTV_SUCCESS;
}

