//=============================================================================
// File       : T3700_test.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2009/04/28       yschoi         Create
//=============================================================================

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/fs.h>

#include "../dmb_hw.h"
#include "../dmb_tsif.h"

#include "tdmb_comdef.h"
#include "tdmb_chip.h"
#include "tdmb_bb.h"
#include "tdmb_test.h"
#include "tdmb_test_pn20_256k.h"


/*===================================================================
                     Pre Declalation  function
====================================================================*/

#define NETBER_CHECK_COUNT_MAX  20

nerber_info_type netber_info;

dword netber_frame;
dword err_frame, loop_cnt;
dword total_netber_frame;

extern tSignalQuality g_tSigQual;

#ifdef FEATURE_NETBER_TEST_ON_BOOT
extern tdmb_mode_type dmb_mode;
#endif



/*===========================================================================
FUNCTION       netber_init
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
void netber_init(void)
{
#ifdef FEATURE_DMB_TSIF_IF
  netber_info.frame_num = 37500;
#else
  netber_info.frame_num = 50;
#endif

  // cnt, err 초기화
  netber_clr_loop_count();

  // 읽어올 데이터 크기 구하기
  netber_frame = sizeof(PN20);

  return;
}

#if 1 // cys test
extern bool power_on_flag;
extern tdmb_mode_type dmb_mode;
#endif

/*===========================================================================
FUNCTION       netber_GetError
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
void netber_GetError(unsigned short buf_size, uint8 *buf)
{
  uint32 j;

#if 1 // cys test
  if((!power_on_flag) || ((dmb_mode != TDMB_MODE_NETBER)))
  {
    TDMB_MSG_TEST("[%s] TDMB is no running!!! PWR_flag[%d], dmb_mode[%d]\n", __func__, power_on_flag, dmb_mode);
    dump_stack();
    
    return;
  }
#endif

  ++loop_cnt;

  // 에러 체크
  for(j = 0; j < netber_frame && j < buf_size/*ebi2_buffer_len*/; j++)
  //for(j = 0; j < netber_frame; j++)
  {
    err_frame += netber_GetErrorBitCount(j, buf[j]);
  }
  
  if(loop_cnt >= NETBER_CHECK_COUNT_MAX)
  {
#ifdef FEATURE_DMB_TSIF_IF  
    total_netber_frame = netber_info.frame_num * 8;
#else
    total_netber_frame = netber_info.frame_num * netber_frame * 8;
#endif
    TDMB_MSG_TEST("[%s] error bit=[%d] total bit=[%d]\n", __func__, (int)err_frame, (int)total_netber_frame);
#if !defined(FEATURE_QTDMB_USE_TELECHIPS)
    g_tSigQual.SNR = err_frame;
    g_tSigQual.PCBER = total_netber_frame;
    g_tSigQual.RSBER = (err_frame * 10000) / total_netber_frame;
#endif /* !FEATURE_QTDMB_USE_TELECHIPS  */
    netber_clr_loop_count();
  }

  return;
}


/*===========================================================================
FUNCTION       netber_GetErrorBitCount
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
unsigned long netber_GetErrorBitCount(unsigned short src, unsigned char dest)
{
  unsigned char temp;
  int i, count;
  unsigned char ber_src;

  ber_src = PN20[src];

  temp = ber_src ^ dest;

  count = 0;
  for (i = 0; i < 8; i++)
  {
    if (temp & (0x01 << i))
    {
      count++;
    }
  }

  return count;
}


/*===========================================================================
FUNCTION       netber_clr_loop_count
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
void netber_clr_loop_count(void)
{
  err_frame = 0;
  loop_cnt = 0;  
  //total_netber_frame = 0;
}


/*===========================================================================
FUNCTION       tdmb_ch_test
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
void tdmb_ch_test(uint8 ch)
{
  service_t servicetype;
  servicetype = (service_t) ch;

  TDMB_MSG_TEST("[%s] !!\n", __func__);

  tdmb_bb_power_on();

  dmb_set_ant_path(DMB_ANT_EARJACK);

#if defined(FEATURE_TEST_ON_BOOT) && defined(FEATURE_DMB_TSIF_IF)
  dmb_tsif_test();
#endif /* FEATURE_DMB_TSIF_IF */

#ifdef FEATURE_TDMB_USE_INC  
  t3700_test(servicetype);
  //t3700_test(T3700_MYTN); // 8B (183008)
  //t3700_test(T3700_TEST); // 10B (195008)
  //t3700_test(T3700_MBC);  // 12A (205280)
  //t3700_test(T3700_KBS_STAR); // 12B (207008)
  //t3700_i2c_test2();
  //t3700_i2c_test3();
#elif defined(FEATURE_TDMB_USE_FCI)
  fc8050_test(servicetype);
#elif defined(FEATURE_TDMB_USE_RTV)
  mtv350_test(servicetype);
#elif defined(FEATURE_TDMB_USE_TCC)
  tcc3170_test(servicetype);
#else
  #error
#endif
}

/*===========================================================================
FUNCTION       tdmb_get_fixed_chan_info
DESCRIPTION
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
===========================================================================*/
int tdmb_get_fixed_chan_info(service_t servicetype, chan_info* pChInfo) 
{
// 2009.08.12 cys updated
  chan_info stInfo;

  memset(&stInfo, 0, sizeof(chan_info));
  
  switch(servicetype) {
  case TDMB_U1: // 8A : OK
    stInfo.uiServiceType     = 0x18;
    stInfo.ulRFNum           = 181280;
    stInfo.uiSubChID         = 0x0;
    stInfo.uiStarAddr        = 0x0;
    stInfo.uiBitRate         = 384;
    stInfo.uiTmID            = 128;
    stInfo.uiSlFlag          = 1;
    stInfo.ucTableIndex      = 0;
    stInfo.ucOption          = 0;
    stInfo.uiProtectionLevel = 2;
    stInfo.uiDifferentRate   = 0x3e;
    stInfo.uiSchSize         = 0x0120;
    break;

  case TDMB_mYTN: // 8B : OK
    stInfo.uiServiceType     = 0x18;
    stInfo.ulRFNum           = 183008;
    stInfo.uiSubChID         = 0x1;
    stInfo.uiStarAddr        = 0x0;
    stInfo.uiBitRate         = 432;
    stInfo.uiTmID            = 128;
    stInfo.uiSlFlag          = 1;
    stInfo.ucTableIndex      = 0;
    stInfo.ucOption          = 0;
    stInfo.uiProtectionLevel = 2;
    stInfo.uiDifferentRate   = 0x40;
    stInfo.uiSchSize         = 0x144;
    break;

  case TDMB_QBS: // 8C (1 to 1) : OK
    stInfo.uiServiceType     = 0x18;
    stInfo.ulRFNum           = 184736;
    stInfo.uiSubChID         = 0x1;
    stInfo.uiStarAddr        = 0x0;
    stInfo.uiBitRate         = 352;
    stInfo.uiTmID            = 128;
    stInfo.uiSlFlag          = 1;
    stInfo.ucTableIndex      = 0;
    stInfo.ucOption          = 0;
    stInfo.uiProtectionLevel = 2;
    stInfo.uiDifferentRate   = 0x3e;
    stInfo.uiSchSize         = 0x0108;
    break;
    
  case TDMB_myMBC: // 12A : OK
    stInfo.uiServiceType     = 0x18;
    stInfo.ulRFNum           = 205280;
    stInfo.uiSubChID         = 0x1;
    stInfo.uiStarAddr        = 0x0;
    stInfo.uiBitRate         = 544;
    stInfo.uiTmID            = 128;
    stInfo.uiSlFlag          = 1;
    stInfo.ucTableIndex      = 0;
    stInfo.ucOption          = 0;
    stInfo.uiProtectionLevel = 2;
    stInfo.uiDifferentRate   = 0x44;
    stInfo.uiSchSize         = 0x198;
    break;

  case TDMB_KBS_STAR: // 12B : OK
    stInfo.uiServiceType     = 0x18;
    stInfo.ulRFNum           = 207008;
    stInfo.uiSubChID         = 0x0;
    stInfo.uiStarAddr        = 0x0;
    stInfo.uiBitRate         = 424;
    stInfo.uiTmID            = 128;
    stInfo.uiSlFlag          = 1;
    stInfo.ucTableIndex      = 0;
    stInfo.ucOption          = 0;
    stInfo.uiProtectionLevel = 2;
    stInfo.uiDifferentRate   = 0x35;
    stInfo.uiSchSize         = 0x013e;
    break;

  case TDMB_KBS_HEART: // 12B : OK
    stInfo.uiServiceType     = 0x18;
    stInfo.ulRFNum           = 207008;
    stInfo.uiSubChID         = 0x0b;
    stInfo.uiStarAddr        = 0x0222;
    stInfo.uiBitRate         = 424;
    stInfo.uiTmID            = 128;
    stInfo.uiSlFlag          = 1;
    stInfo.ucTableIndex      = 0;
    stInfo.ucOption          = 0;
    stInfo.uiProtectionLevel = 2;
    stInfo.uiDifferentRate   = 0x35;
    stInfo.uiSchSize         = 0x013e;
    break;
    
  case TDMB_SBS_u_TV: // 12C : OK
    stInfo.uiServiceType     = 0x18;
    stInfo.ulRFNum           = 208736;
    stInfo.uiSubChID         = 0x0;
    stInfo.uiStarAddr        = 0xa8;
    stInfo.uiBitRate         = 544;
    stInfo.uiTmID            = 1;
    stInfo.uiSlFlag          = 1;
    stInfo.ucTableIndex      = 0;
    stInfo.ucOption          = 0;
    stInfo.uiProtectionLevel = 2;
    stInfo.uiDifferentRate   = 0x44;
    stInfo.uiSchSize         = 0x0198;
    break;

  case TDMB_RADIO: // 8B TBN
    stInfo.uiServiceType     = 0x1;
    stInfo.ulRFNum           = 183008;
    stInfo.uiSubChID         = 0x0B;
    stInfo.uiStarAddr        = 0x180;
    stInfo.uiBitRate         = 160;
    stInfo.uiTmID            = 28;
    stInfo.uiSlFlag          = 0;
    stInfo.ucTableIndex      = 35;
    stInfo.ucOption          = 0;
    stInfo.uiProtectionLevel = 0;
    stInfo.uiDifferentRate   = 0x00;
    stInfo.uiSchSize         = 0x74;
    break;

  case TDMB_TEST: // 10B (HW TDMB Input Matching test)
    stInfo.uiServiceType     = 0x18;
    stInfo.ulRFNum           = 195008;
    stInfo.uiSubChID         = 0x1;
    stInfo.uiStarAddr        = 0xf00;
    stInfo.uiBitRate         = 0x220;
    stInfo.uiTmID            = 1;
    stInfo.uiSlFlag          = 1;
    stInfo.ucTableIndex      = 0;
    stInfo.ucOption          = 0;
    stInfo.uiProtectionLevel = 2;
    stInfo.uiDifferentRate   = 0x44;
    stInfo.uiSchSize         = 0x198;
    break;
    
  case TDMB_HW_DTV_TEST:
    stInfo.uiServiceType     = 0x18;
    stInfo.ulRFNum           = 184736;
    stInfo.uiSubChID         = 0x0;
    stInfo.uiStarAddr        = 0x0;
    stInfo.uiBitRate         = 544;
    stInfo.uiTmID            = 128;
    stInfo.uiSlFlag          = 1;
    stInfo.ucTableIndex      = 0;
    stInfo.ucOption          = 0;
    stInfo.uiProtectionLevel = 2;
    stInfo.uiDifferentRate   = 0x3e;
    stInfo.uiSchSize         = 0x0198;
    break;

  case TDMB_NETBER:
    stInfo.uiServiceType     = 0x1;
    stInfo.ulRFNum           = 181280;
    stInfo.uiSubChID         = 0x0;
    stInfo.uiStarAddr        = 0x0;
    stInfo.uiBitRate         = 256;
    stInfo.uiTmID            = 1;
    stInfo.uiSlFlag          = 1;
    stInfo.ucTableIndex      = 0;
    stInfo.ucOption          = 0;
    stInfo.uiProtectionLevel = 2;
    stInfo.uiDifferentRate   = 0x20;
    stInfo.uiSchSize         = 0x00c0;
    break;

  case TDMB_MFLO_IU:
    stInfo.uiServiceType     = 0x18;
    stInfo.ulRFNum           = 178736;
    stInfo.uiSubChID         = 0x0;
    stInfo.uiStarAddr        = 0x0;
    stInfo.uiBitRate         = 544;
    stInfo.uiTmID            = 128;
    stInfo.uiSlFlag          = 1;
    stInfo.ucTableIndex      = 0;
    stInfo.ucOption          = 0;
    stInfo.uiProtectionLevel = 2;
    stInfo.uiDifferentRate   = 0x44;
    stInfo.uiSchSize         = 0x198;
    break;

  default:
    return FALSE;
    break;
  }

  memcpy(pChInfo, &stInfo, sizeof(chan_info));
  return TRUE;
}

