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
* TITLE 	  : RAONTECH TV OEM source file. 
*
* FILENAME    : raontv_port.c
*
* DESCRIPTION : 
*		User-supplied Routines for RAONTECH TV Services.
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
* 04/09/2010  Yang, Maverick   REV1 SETTING 
* 01/25/2010  Yang, Maverick   Created.                                                   
********************************************************************************/

//#include <linux/gpio.h>
//#include <linux/interrupt.h>
//#include <linux/spi/spi.h>
#include <linux/delay.h>
#include "raontv.h"
#include "raontv_internal.h"


/* Declares a variable of gurad object if neccessry. */
#if defined(RTV_IF_SPI) || defined(RTV_FIC_I2C_INTR_ENABLED)\
|| defined(RTV_DAB_RECONFIG_ENABLED)
	#if defined(__KERNEL__)	
		struct mutex raontv_guard;
    #elif defined(WINCE)
        CRITICAL_SECTION raontv_guard;

    #else
	/* non-OS and RTOS. */
	#endif
#endif


void rtvOEM_ConfigureInterrupt(void) 
{
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)\
|| defined(RTV_FIC_I2C_INTR_ENABLED) || defined(RTV_DAB_RECONFIG_ENABLED)
	RTV_REG_SET(0x09, 0x00); // [6]INT1 [5]INT0 - 1: Input mode, 0: Output mode
	RTV_REG_SET(0x0B, 0x00); // [2]INT1 PAD disable [1]INT0 PAD disable

	RTV_REG_MAP_SEL(HOST_PAGE);
	RTV_REG_SET(0x28, 0x01); // [5:3]INT1 out sel [2:0] INI0 out sel -  0:Toggle 1:Level,, 2:"0", 3:"1"

	RTV_REG_SET(0x2A, 0x13); // [5]INT1 pol [4]INT0 pol - 0:Active High, 1:Active Low [3:0] Period = (INT_TIME+1)/8MHz
#endif

	RTV_REG_MAP_SEL(HOST_PAGE);
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	#if defined(RTV_MSC_INTR_ISTAUS_ACC_CLR_MODE)
 	RTV_REG_SET(0x29, 0x08); // [3] Interrupt status register clear condition - 0:read data by memory access 1:status register access
	#else
	RTV_REG_SET(0x29, 0x00); // [3] Interrupt status register clear condition - 0:read data by memory access 1:status register access
	#endif
#else
	RTV_REG_SET(0x29, 0x00); // [3] auto&uclr, 1: user set only
#endif

}

//#include "./../mtv.h"
//#include "./../mtv_gpio.h"

void rtvOEM_PowerOn(int on)
{
	if( on )
	{
		/* Set the GPIO of MTV_EN pin to low. */
		//gpio_set_value(MTV_PWR_EN, 0);
		RTV_DELAY_MS(10);
		
		/* Set the GPIO of MTV_EN pin to high. */
		//gpio_set_value(MTV_PWR_EN, 1);
		RTV_DELAY_MS(20);	

		/* In case of SPI interface or FIC interrupt mode for T-DMB, we should lock the register page. */
		RTV_GUARD_INIT; 			
	}
	else
	{
		/* Set the GPIO of MTV_EN pin to low. */		
		//gpio_set_value(MTV_PWR_EN, 0);

		RTV_GUARD_DEINIT;
	}
}




