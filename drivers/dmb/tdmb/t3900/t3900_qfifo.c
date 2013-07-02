//=============================================================================
// File       : T3900_qfifo.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2009/xx/xx       INC Tech       Draft
//  1.1.0       2009/04/28       yschoi         Android Porting
//  1.2.0       2010/07/30       wgyoo          T3900 porting
//=============================================================================

#include <linux/kernel.h>
#include <linux/module.h> // memset
#include <linux/slab.h>

#include "../tdmb_comdef.h"

#include "t3900_includes.h"
#include "t3900_bb.h"


#ifdef INC_FIFO_SOURCE_ENABLE
#if 1//def OLD_CH_SIZE
#define INC_GET_SUBCHANNEL_SIZE(X, Y) (((((X)<<8) | (Y)) & 0x3FF) * 2)
#else
#define INC_GET_SUBCHANNEL_SIZE(X, Y) ((((X)<<8) | (Y)) & 0x3FF)
#endif

#define INC_GET_SUBCHANNEL_ID(X) (((X) >> 2) & 0x3F)

ST_FIFO  g_astChFifo[MAX_CHANNEL_FIFO];

INC_UINT8 INC_QFIFO_INIT(PST_FIFO pFF, INC_UINT32 ulDepth)
{
  if(pFF == INC_NULL) return INC_ERROR;
  pFF->ulFront = pFF->ulRear = 0;
  pFF->unSubChID = INC_SUB_CHANNEL_ID_MASK;

  if(ulDepth == 0 || ulDepth >= INC_FIFO_DEPTH) pFF->ulDepth = INC_FIFO_DEPTH + 1;
  else pFF->ulDepth = ulDepth + 1;
  return INC_SUCCESS;
}

INC_UINT32 INC_QFIFO_FREE_SIZE(PST_FIFO pFF)
{
  if(pFF == INC_NULL) return INC_ERROR;

  return (pFF->ulFront >= pFF->ulRear) ?
    ((pFF->ulRear + pFF->ulDepth) - pFF->ulFront) - 1 : (pFF->ulRear - pFF->ulFront) - 1;
}



INC_UINT32 INC_QFIFO_GET_SIZE(PST_FIFO pFF)
{
  if(pFF == INC_NULL) return INC_ERROR;

  return (pFF->ulFront >= pFF->ulRear) ?
    (pFF->ulFront - pFF->ulRear) : (pFF->ulFront + pFF->ulDepth - pFF->ulRear);
}

INC_UINT8 INC_QFIFO_AT(PST_FIFO pFF, INC_UINT8* pData, INC_UINT32 ulSize)
{
  INC_UINT32 ulLoop, ulOldRear;

  if(pFF == INC_NULL || pData == INC_NULL || ulSize > INC_QFIFO_GET_SIZE(pFF)) 
    return INC_ERROR;

  ulOldRear = pFF->ulRear;
  for(ulLoop = 0 ; ulLoop < ulSize; ulLoop++)
  {
    pData[ulLoop] = pFF->acBuff[ulOldRear++];
    ulOldRear %= pFF->ulDepth;
  }
  return INC_SUCCESS;
}

INC_UINT8 INC_QFIFO_ADD(PST_FIFO pFF, INC_UINT8* pData, INC_UINT32 ulSize)
{
  INC_UINT32 ulLoop;

  if(pFF == INC_NULL || pData == INC_NULL || ulSize > INC_QFIFO_FREE_SIZE(pFF)) 
    return INC_ERROR;

  for(ulLoop = 0 ; ulLoop < ulSize; ulLoop++)
  {
    pFF->acBuff[pFF->ulFront++] = pData[ulLoop];
    pFF->ulFront %= pFF->ulDepth;
  }
  return INC_SUCCESS;
}

INC_UINT8 INC_QFIFO_BRING(PST_FIFO pFF, INC_UINT8* pData, INC_UINT32 ulSize)
{
  INC_UINT32 ulLoop;

  if(pFF == INC_NULL || pData == INC_NULL || ulSize > INC_QFIFO_GET_SIZE(pFF)) 
    return INC_ERROR;

  for(ulLoop = 0 ; ulLoop < ulSize; ulLoop++)
  {
    pData[ulLoop] = pFF->acBuff[pFF->ulRear++];
    pFF->ulRear %= pFF->ulDepth;
  }
  return INC_SUCCESS;
}

INC_UINT32 INC_GET_IDS_SIZE(INC_UINT16 unID)
{
  ST_FIFO*  pFifo;
  INC_UINT32  ulLoop;

  if(unID == INC_SUB_CHANNEL_ID_MASK) return 0;

  for(ulLoop = 0 ; ulLoop < MAX_CHANNEL_FIFO; ulLoop++)
  {
    pFifo = INC_GET_CHANNEL_FIFO((MULTI_CHANNEL_INFO)ulLoop);
    if(pFifo->unSubChID == unID)
    {
      return INC_QFIFO_GET_SIZE(pFifo);
    }
  }
  return 0;
}

INC_UINT8 INC_GET_ID_BRINGUP(INC_UINT16 unID, INC_UINT8* pData, INC_UINT32 ulSize)
{
  ST_FIFO*  pFifo;
  INC_UINT32  ulLoop;

  if(unID == INC_SUB_CHANNEL_ID_MASK) return 0;

  for(ulLoop = 0 ; ulLoop < MAX_CHANNEL_FIFO; ulLoop++)
  {
    pFifo = INC_GET_CHANNEL_FIFO((MULTI_CHANNEL_INFO)ulLoop);
    if(pFifo->unSubChID == unID)
    {
      return INC_QFIFO_BRING(pFifo, pData, ulSize);
    }
  }
  return 0; 
}

void INC_MULTI_SORT_INIT(void)
{
  INC_UINT32  ulLoop;
  ST_FIFO* pFifo;

  for(ulLoop = 0 ; ulLoop < MAX_CHANNEL_FIFO; ulLoop++)
  {
    pFifo = INC_GET_CHANNEL_FIFO((MULTI_CHANNEL_INFO)ulLoop);
    INC_QFIFO_INIT(pFifo, 0);
  }
}

ST_FIFO* INC_GET_CHANNEL_FIFO(MULTI_CHANNEL_INFO ucIndex)
{
  if(ucIndex > CHANNEL3_STREAM_DATA) return INC_NULL;
  return &g_astChFifo[ucIndex];
}

INC_UINT8 INC_GET_CHANNEL_DATA(MULTI_CHANNEL_INFO ucIndex, INC_UINT8* pData, INC_UINT32 ulSize)
{
  ST_FIFO* pFifo;
  pFifo = INC_GET_CHANNEL_FIFO(ucIndex);
  if(pFifo == INC_NULL) return INC_ERROR;
    INC_QFIFO_BRING(pFifo, pData, ulSize);
  return INC_SUCCESS;
}

INC_UINT32 INC_GET_CHANNEL_COUNT(MULTI_CHANNEL_INFO ucIndex)
{
  ST_FIFO* pFifo;
  pFifo = INC_GET_CHANNEL_FIFO(ucIndex);
  if(pFifo == INC_NULL) return INC_ERROR;
    return (INC_UINT32)INC_QFIFO_GET_SIZE(pFifo);
}

/*
#define  INC_CIF_MAX_SIZE  (188*10)
아래 코드의 변경은 처음에 헤더정보가 있고, 다음의 헤더까지 찾아내어서 정상적이면
처음의 헤더에 정보를 넣는 방식으로 변경을 하였습니다.
처음에 헤더는 정상이고, 다음의 헤더가 비정상적이면, 데이터를 전부다 버리는 코드입니다.
위의 코드는 EBI, TSIF나 모두가 적용이 되어야 합니다.
*/

#if 0
INC_UINT8 acBuff[INC_HEADER_CHECK_BUFF];

ST_HEADER_INFO INC_HEADER_CHECK(ST_FIFO* pMainFifo)
{
  //INC_UINT8 acBuff[INC_HEADER_CHECK_BUFF];
  INC_UINT32 ulLoop,  ulFrame, ulIndex, ulTotalLength, ulSubChTSize;
  INC_UINT16 aunChSize[MAX_CHANNEL_FIFO-1];

  ulFrame = INC_QFIFO_GET_SIZE(pMainFifo) / MAX_HEADER_SIZE;
 
  for(ulIndex = 0; ulIndex < (ulFrame-1); ulIndex++)
  {
    INC_QFIFO_AT(pMainFifo, acBuff, MAX_HEADER_SIZE);
    for(ulLoop = 0 ; ulLoop < (MAX_HEADER_SIZE-1); ulLoop++)
    {
      if(acBuff[ulLoop] == HEADER_ID_0x33 && acBuff[ulLoop+1] == HEADER_ID_0x00)
      {
        if(ulLoop) INC_QFIFO_BRING(pMainFifo, acBuff, ulLoop);

        INC_QFIFO_AT(pMainFifo, acBuff, MAX_HEADER_SIZE);

        ulTotalLength = (INC_UINT16)(acBuff[4] << 8) | acBuff[5];
        ulTotalLength = (ulTotalLength & 0x8000) ? (ulTotalLength << 1) : ulTotalLength;
#if 1
        aunChSize[0] = INC_GET_SUBCHANNEL_SIZE(acBuff[0x8], acBuff[0x9]);
        aunChSize[1] = INC_GET_SUBCHANNEL_SIZE(acBuff[0xA], acBuff[0xB]);
        aunChSize[2] = INC_GET_SUBCHANNEL_SIZE(acBuff[0xC], acBuff[0xD]);
        aunChSize[3] = INC_GET_SUBCHANNEL_SIZE(acBuff[0xE], acBuff[0xF]);
#else				
        aunChSize[0] = INC_GET_SUBCHANNEL_SIZE(acBuff[0x8], acBuff[0x9]);
        aunChSize[1] = INC_GET_SUBCHANNEL_SIZE(acBuff[0xA], acBuff[0xB]) | (aunChSize[0] & 0xC00);
        aunChSize[2] = INC_GET_SUBCHANNEL_SIZE(acBuff[0xC], acBuff[0xD]) | ((aunChSize[0] >> 2) & 0xC00);
        aunChSize[3] = INC_GET_SUBCHANNEL_SIZE(acBuff[0xE], acBuff[0xF]);

        aunChSize[0] = aunChSize[0] << 1;
        aunChSize[1] = aunChSize[1] << 1;
        aunChSize[2] = aunChSize[2] << 1;
        aunChSize[3] = aunChSize[3] << 1;
#endif				
        ulSubChTSize = aunChSize[0] + aunChSize[1] + aunChSize[2] + aunChSize[3] + MAX_HEADER_SIZE;

        if(ulSubChTSize != ulTotalLength) {
          INC_QFIFO_INIT(pMainFifo, 0);
          return INC_HEADER_NOT_SEARACH;
        }

        if(INC_QFIFO_GET_SIZE(pMainFifo) < (ulTotalLength + MAX_HEADER_SIZE))
          return INC_HEADER_SIZE_ERROR;

        INC_QFIFO_AT(pMainFifo, acBuff, ulTotalLength + MAX_HEADER_SIZE);

        if(acBuff[ulTotalLength] == HEADER_ID_0x33 && acBuff[ulTotalLength+1] == HEADER_ID_0x00)
        {
          return INC_HEADER_GOOD;
        }
        else
        {
          INC_QFIFO_INIT(pMainFifo, 0);
          return INC_HEADER_NOT_SEARACH;
        }
      }
    }

    if(acBuff[ulLoop] == HEADER_ID_0x33) INC_QFIFO_BRING(pMainFifo, acBuff, MAX_HEADER_SIZE-1);
    else INC_QFIFO_BRING(pMainFifo, acBuff, MAX_HEADER_SIZE);
  }
 
 return INC_HEADER_NOT_SEARACH;
}

INC_UINT8 INC_MULTI_FIFO_PROCESS(INC_UINT8* pData, INC_UINT32 ulSize)
{
  INC_UINT8 *acBuff;//[INC_CIF_MAX_SIZE], 
  INC_UINT8 cIndex, bIsData = INC_ERROR;
  INC_UINT16 aunChSize[MAX_CHANNEL_FIFO-1], unTotalSize;
  INC_UINT16 aunSubChID[MAX_CHANNEL_FIFO-1];

  ST_FIFO* pFifo;
  ST_FIFO* pMainFifo;

  acBuff = kmalloc(INC_CIF_MAX_SIZE, GFP_KERNEL);
  memset(acBuff, 0, INC_CIF_MAX_SIZE);

  pMainFifo = INC_GET_CHANNEL_FIFO(MAIN_INPUT_DATA);
  INC_QFIFO_ADD(pMainFifo, pData, ulSize);
 
  while(1)
  {
    if(INC_QFIFO_GET_SIZE(pMainFifo) < MAX_HEADER_SIZE)
    {
      //return bIsData;
      goto exit;
    }
    if(INC_HEADER_CHECK(pMainFifo) != INC_HEADER_GOOD)
    {
      //return bIsData;
      goto exit;
    }

    INC_QFIFO_AT(pMainFifo, acBuff, MAX_HEADER_SIZE);

    //헤더의 전체크기를 계산하고...
    unTotalSize = (INC_UINT16)(acBuff[4] << 8) | acBuff[5];
    unTotalSize = (unTotalSize & 0x8000) ? (unTotalSize << 1) : unTotalSize;

    if(unTotalSize > INC_QFIFO_GET_SIZE(pMainFifo))
    {
      //return bIsData;
      goto exit;
    }

    //각각의 채널별 크기를 구하고...
    memset(aunChSize, 0 , sizeof(aunChSize));
#if 1
      aunChSize[0] = INC_GET_SUBCHANNEL_SIZE(acBuff[0x8], acBuff[0x9]);
      aunChSize[1] = INC_GET_SUBCHANNEL_SIZE(acBuff[0xA], acBuff[0xB]);
      aunChSize[2] = INC_GET_SUBCHANNEL_SIZE(acBuff[0xC], acBuff[0xD]);
      aunChSize[3] = INC_GET_SUBCHANNEL_SIZE(acBuff[0xE], acBuff[0xF]);
#else
    aunChSize[0] = INC_GET_SUBCHANNEL_SIZE(acBuff[0x8], acBuff[0x9]);
    aunChSize[1] = INC_GET_SUBCHANNEL_SIZE(acBuff[0xA], acBuff[0xB]) | (aunChSize[0] & 0xC00);
    aunChSize[2] = INC_GET_SUBCHANNEL_SIZE(acBuff[0xC], acBuff[0xD]) | ((aunChSize[0] >> 2) & 0xC00);
    aunChSize[3] = INC_GET_SUBCHANNEL_SIZE(acBuff[0xE], acBuff[0xF]);

    aunChSize[0] = aunChSize[0] << 1;
    aunChSize[1] = aunChSize[1] << 1;
    aunChSize[2] = aunChSize[2] << 1;
    aunChSize[3] = aunChSize[3] << 1;
 #endif
    aunSubChID[0] = INC_GET_SUBCHANNEL_ID(acBuff[0x8]);
    aunSubChID[1] = INC_GET_SUBCHANNEL_ID(acBuff[0xA]);
    aunSubChID[2] = INC_GET_SUBCHANNEL_ID(acBuff[0xC]);
    aunSubChID[3] = INC_GET_SUBCHANNEL_ID(acBuff[0xE]);

    INC_QFIFO_BRING(pMainFifo, acBuff, MAX_HEADER_SIZE);

    for(cIndex = 0; cIndex < (MAX_CHANNEL_FIFO-1); cIndex++)
    {
      if(!aunChSize[cIndex]) continue;

      pFifo = INC_GET_CHANNEL_FIFO((MULTI_CHANNEL_INFO)(cIndex+1));

      if(pFifo->unSubChID == INC_SUB_CHANNEL_ID_MASK)
        pFifo->unSubChID = aunSubChID[cIndex];

      while(aunChSize[cIndex] > INC_CIF_MAX_SIZE)
      {
        INC_QFIFO_BRING(pMainFifo, acBuff, (INC_UINT32)INC_CIF_MAX_SIZE);
        INC_QFIFO_ADD(pFifo, acBuff, (INC_UINT32)INC_CIF_MAX_SIZE);
        aunChSize[cIndex] -= INC_CIF_MAX_SIZE;
      }

      INC_QFIFO_BRING(pMainFifo, acBuff, (INC_UINT32)aunChSize[cIndex]);
      INC_QFIFO_ADD(pFifo, acBuff, (INC_UINT32)aunChSize[cIndex]);
    }

    bIsData = INC_SUCCESS;
  }

  kfree(acBuff); 
  return INC_SUCCESS;


  exit:
  kfree(acBuff);
  return bIsData;
}
#else

INC_UINT8 g_acBuff[INC_HEADER_CHECK_BUFF];

ST_HEADER_INFO INC_HEADER_CHECK(ST_FIFO* pMainFifo)
{
	INC_UINT32 ulSize, ulTotalLength, ulSubChTSize;
	INC_UINT16 aunChSize[MAX_CHANNEL_FIFO-1];
	INC_UINT16 aunSubChID[MAX_CHANNEL_FIFO-1];
	INC_UINT8 cIndex;
	ST_HEADER_INFO isData = INC_HEADER_NOT_SEARACH;
	ST_FIFO* pFifo;

    while(1){
  
   	  ulSize = INC_QFIFO_GET_SIZE(pMainFifo);
     if(ulSize < MAX_HEADER_SIZE) break;

  
      INC_QFIFO_AT(pMainFifo, g_acBuff, 2);
      if(g_acBuff[0] == HEADER_ID_0x33 && g_acBuff[1] == HEADER_ID_0x00)
      {
        INC_QFIFO_AT(pMainFifo, g_acBuff, MAX_HEADER_SIZE);
        
        ulTotalLength = (INC_UINT16)(g_acBuff[4] << 8) | g_acBuff[5];
        ulTotalLength = (ulTotalLength & 0x8000) ? (ulTotalLength << 1) : ulTotalLength;
        
        aunChSize[0] = INC_GET_SUBCHANNEL_SIZE(g_acBuff[0x8], g_acBuff[0x9]);
        aunChSize[1] = INC_GET_SUBCHANNEL_SIZE(g_acBuff[0xA], g_acBuff[0xB]) | (aunChSize[0] & 0xC00);
        aunChSize[2] = INC_GET_SUBCHANNEL_SIZE(g_acBuff[0xC], g_acBuff[0xD]) | ((aunChSize[0] >> 2) & 0xC00);
        aunChSize[3] = INC_GET_SUBCHANNEL_SIZE(g_acBuff[0xE], g_acBuff[0xF]);
        
        aunSubChID[0] = INC_GET_SUBCHANNEL_ID(g_acBuff[0x8]);
        aunSubChID[1] = INC_GET_SUBCHANNEL_ID(g_acBuff[0xA]);
        aunSubChID[2] = INC_GET_SUBCHANNEL_ID(g_acBuff[0xC]);
        aunSubChID[3] = INC_GET_SUBCHANNEL_ID(g_acBuff[0xE]);
        
        ulSubChTSize = aunChSize[0] + aunChSize[1] + aunChSize[2] + aunChSize[3] + MAX_HEADER_SIZE;
        
        if(ulSubChTSize != ulTotalLength) {
          INC_QFIFO_INIT(pMainFifo, 0);
          break;
        }
  
        if(INC_QFIFO_GET_SIZE(pMainFifo) < (ulTotalLength + MAX_HEADER_SIZE))
          break;
  
        memset(g_acBuff,0, sizeof(g_acBuff));
        INC_QFIFO_AT(pMainFifo, g_acBuff, ulTotalLength + MAX_HEADER_SIZE);
  
        if(g_acBuff[ulTotalLength] == HEADER_ID_0x33 && g_acBuff[ulTotalLength+1] == HEADER_ID_0x00)
        {
          INC_QFIFO_BRING(pMainFifo, g_acBuff, MAX_HEADER_SIZE);
  
          for(cIndex = 0; cIndex < (MAX_CHANNEL_FIFO-1); cIndex++){
            if(!aunChSize[cIndex]) continue;
  
            pFifo = INC_GET_CHANNEL_FIFO((MULTI_CHANNEL_INFO)(cIndex+1));
  
            if(pFifo->unSubChID == INC_SUB_CHANNEL_ID_MASK)
              pFifo->unSubChID = aunSubChID[cIndex];
  
            while(aunChSize[cIndex] > INC_CIF_MAX_SIZE)
            {
              INC_QFIFO_BRING(pMainFifo, g_acBuff, (INC_UINT32)INC_CIF_MAX_SIZE);
              INC_QFIFO_ADD(pFifo, g_acBuff, (INC_UINT32)INC_CIF_MAX_SIZE);
              aunChSize[cIndex] -= INC_CIF_MAX_SIZE;
            }
  
            INC_QFIFO_BRING(pMainFifo, g_acBuff, (INC_UINT32)aunChSize[cIndex]);
            INC_QFIFO_ADD(pFifo, g_acBuff, (INC_UINT32)aunChSize[cIndex]);
          }
          isData = INC_HEADER_GOOD;
        }
        else
        {
          INC_QFIFO_INIT(pMainFifo, 0);
        }
      }
      else
      {
        if(g_acBuff[1] == HEADER_ID_0x33) 
        {
          INC_QFIFO_BRING(pMainFifo, g_acBuff, 1);
        }
        else 
        {
          INC_QFIFO_BRING(pMainFifo, g_acBuff, 2);
        }
      }
    }
  
    return isData;
  }
  
  INC_UINT8 INC_MULTI_FIFO_PROCESS(INC_UINT8* pData, INC_UINT32 ulSize)
  {
    ST_FIFO* pMainFifo;
  
    pMainFifo = INC_GET_CHANNEL_FIFO(MAIN_INPUT_DATA);
    if(INC_QFIFO_ADD(pMainFifo, pData, ulSize) != INC_SUCCESS)
    {
      TDMB_MSG_INC_BB("INC_QFIFO_ADD Fail~~~  \n");
      INC_QFIFO_INIT(pMainFifo, 0);
      return INC_ERROR;
    }
  
    if(INC_HEADER_CHECK(pMainFifo) == INC_HEADER_GOOD){
      return INC_SUCCESS;
    }
  
    return INC_ERROR;
  }


#endif

#endif

