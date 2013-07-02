/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved

 File name : fc8050_isr.c

 Description : fc8050 interrupt service routine

 History :
 ----------------------------------------------------------------------
 2009/08/29   jason   initial
*******************************************************************************/
#include <linux/input.h>
#include <linux/spi/spi.h>
#include "fci_types.h"
#include "fci_hal.h"
#include "fc8050_regs.h"
#include "fc8050_wrapper.h"


static u8 ficBuffer[512+4];
#if (defined(FC8050_USE_EBI2) || defined(FC8050_USE_SPI))
static u8 mscBuffer[8192+4];
#else
//static u8 mscBuffer[4];
#endif

int (*pFicCallback)(u32 userdata, u8 *data, int length) = NULL;
int (*pMscCallback)(u32 userdata, u8 subchid, u8 *data, int length) = NULL;

u32 gFicUserData;
u32 gMscUserData;

void fc8050_isr(HANDLE hDevice)
{
  u8 extIntStatus = 0;

  //bbm_write(hDevice, BBM_COM_INT_ENABLE, 0);
  bbm_read(hDevice, BBM_COM_INT_STATUS, &extIntStatus);
  //bbm_write(hDevice, BBM_COM_INT_STATUS, extIntStatus);  //INTR clear
  //bbm_write(hDevice, BBM_COM_INT_STATUS, 0x00);

  if(extIntStatus & BBM_MF_INT)
  {
    u16 mfIntStatus = 0;
    u16 size;

    bbm_word_read(hDevice, BBM_BUF_STATUS, &mfIntStatus);
    //bbm_word_write(hDevice, BBM_BUF_STATUS, mfIntStatus);
    //bbm_word_write(hDevice, BBM_BUF_STATUS, 0x0000);

    if(mfIntStatus & 0x0100)
    {
      bbm_word_read(hDevice, BBM_BUF_FIC_THR, &size);
      size += 1;
      if(size-1)
      {
        bbm_data(hDevice, BBM_COM_FIC_DATA, &ficBuffer[4], size);
#ifdef FC8050_USE_SPI
        if(pFicCallback)
          (*pFicCallback)(gFicUserData, &ficBuffer[6], size);
#else
        if(pFicCallback)
          (*pFicCallback)(gFicUserData, &ficBuffer[4], size);
#endif
      }
    }

#if (defined(FC8050_USE_EBI2) || defined(FC8050_USE_SPI))
    {
    int i;

    for(i=0; i<8; i++)
    {
      if(mfIntStatus & (1<<i))
      {
        bbm_word_read(hDevice, BBM_BUF_CH0_THR+i*2, &size);
        size += 1;

        if(size-1)
        {
          u8  subChId;

          bbm_read(hDevice, BBM_BUF_CH0_SUBCH+i, &subChId);
          subChId = subChId & 0x3f;
        
          bbm_data(hDevice, (BBM_COM_CH0_DATA+i), &mscBuffer[4], size);
    
          if(pMscCallback)
            (*pMscCallback)(gMscUserData, subChId, &mscBuffer[6], size);
        }
      }
    }
    }
#endif

    bbm_word_write(hDevice, BBM_BUF_STATUS, mfIntStatus);
    bbm_word_write(hDevice, BBM_BUF_STATUS, 0x0000);
  }

  bbm_write(hDevice, BBM_COM_INT_STATUS, extIntStatus);  //INTR clear
  bbm_write(hDevice, BBM_COM_INT_STATUS, 0x00);

#if 0
  if(extIntStatus & BBM_SCI_INT)
  {
    extern void PL131_IntHandler(void);
    PL131_IntHandler();
  }

  if(extIntStatus & BBM_WAGC_INT)
  {
  }

  if(extIntStatus & BBM_RECFG_INT)
  {
  }

  if(extIntStatus & BBM_TII_INT)
  {
  }

  if(extIntStatus & BBM_SYNC_INT)
  {
  }

  if(extIntStatus & BBM_I2C_INT)
  {
  }

  if(extIntStatus & BBM_MP2_INT)
  {
  }
#endif
  //bbm_write(hDevice, BBM_COM_INT_ENABLE, ENABLE_INT_MASK);
}

