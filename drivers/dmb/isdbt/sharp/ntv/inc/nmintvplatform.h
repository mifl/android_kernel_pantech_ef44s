/*
* @file vtvl_platform.h VTVL (Virtual TV Layer)
*
* @copyright
* The contents of this document are ¨Ï copyright 2009 WRG. All rights are reserved. 
* A license is hereby granted to download and print a copy of this document for 
* personal use only. No other license to any other intellectual property rights is 
* granted herein. Unless expressly permitted herein, reproduction, transfer, 
* distribution of storage of part or all of the contents in any form without the 
* prior written permission of WRG is prohibited.
*
* @author Tony Cho (shun.cho@wrg.co.kr)
*/

#ifndef _NMINTVPLATFORM_H_
#define _NMINTVPLATFORM_H_

#define __ISDBT_ONLY_MODE__						1		// 325, 326
#define NMI_TV_MODE								__ISDBT_ONLY_MODE__
#define NMI_ISDBT_SIGNAL_INFO_POST_PROCESS		1
#define LOCKED_FREQ_LINKED_LIST_ENABLE			0

#define ___DEBUG_FULL__							0
#define __DEBUG_SIMPLE__						1
#define NMI_ISDBT_SIGNAL_INFO_SCALE				__DEBUG_SIMPLE__
#endif
