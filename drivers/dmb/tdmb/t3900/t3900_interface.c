//=============================================================================
// File       : T3900_interface.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2009/02/17       wgyoo          Draft
//  1.1.0       2009/04/28       yschoi         Android Porting
//  1.2.0       2010/07/30       wgyoo          T3900 porting
//=============================================================================

#include <linux/delay.h>
#include <linux/module.h>

#include "../../dmb_interface.h"

#include "../tdmb_comdef.h"
#include "../tdmb_dev.h"

#ifdef FEATURE_DMB_SPI_IF
#include <linux/spi/spi.h>
#endif

#include "t3900_includes.h"
#include "t3900_bb.h"


/************************************************************************/
/* Operating Chip set : T3900                                           */
/* Software version   : version 1.00                                    */
/* Software Update    : 2011.02.15                                      */
/************************************************************************/

ST_SUBCH_INFO g_stDmbInfo;
ST_SUBCH_INFO g_stDabInfo;
ST_SUBCH_INFO g_stDataInfo;
ENSEMBLE_BAND m_ucRfBand = KOREA_BAND_ENABLE;

/*********************************************************************************/
/*  RF Band Select                                                               */
/*                                                                               */
/*  INC_UINT8 m_ucRfBand = KOREA_BAND_ENABLE,                                    */
/*                         BANDIII_ENABLE,                                       */
/*                         LBAND_ENABLE,                                         */
/*                         CHINA_ENABLE,                                         */
/*                         EXTERNAL_ENABLE,                                      */
/*********************************************************************************/
#if defined(FEATURE_DMB_SPI_CMD)
CTRL_MODE m_ucCommandMode = INC_SPI_CTRL;
#elif defined(FEATURE_DMB_EBI_CMD)
CTRL_MODE m_ucCommandMode = INC_EBI_CTRL;
#elif defined(FEATURE_DMB_I2C_CMD)
CTRL_MODE m_ucCommandMode = INC_I2C_CTRL;
#else
#error code "no INC TDMB command TYPE"
#endif

ST_TRANSMISSION m_ucTransMode = TRANSMISSION_MODE1;

#if defined(INC_T3700_USE_SPI)
UPLOAD_MODE_INFO m_ucUploadMode    = STREAM_UPLOAD_SPI;
INC_ACTIVE_MODE m_ucMPI_CS_Active  = INC_ACTIVE_LOW;
INC_ACTIVE_MODE m_ucMPI_CLK_Active = INC_ACTIVE_HIGH;
#elif defined(INC_T3700_USE_TSIF)
UPLOAD_MODE_INFO m_ucUploadMode    = STREAM_UPLOAD_TS;
INC_ACTIVE_MODE m_ucMPI_CS_Active  = INC_ACTIVE_HIGH;
INC_ACTIVE_MODE m_ucMPI_CLK_Active = INC_ACTIVE_HIGH;
#else //EBI2
UPLOAD_MODE_INFO m_ucUploadMode    = STREAM_UPLOAD_SLAVE_PARALLEL;
INC_ACTIVE_MODE m_ucMPI_CS_Active  = INC_ACTIVE_LOW;
INC_ACTIVE_MODE m_ucMPI_CLK_Active = INC_ACTIVE_LOW;
#endif /* INC_USE_TSIF */
CLOCK_SPEED m_ucClockSpeed = INC_OUTPUT_CLOCK_4096;

#ifdef INC_MANUAL_INTR_CLEAR
INC_UINT16 m_unIntCtrl = (INC_INTERRUPT_POLARITY_HIGH | \
                         INC_INTERRUPT_PULSE | \
                         (INC_INTERRUPT_PULSE_COUNT & INC_INTERRUPT_PULSE_COUNT_MASK));
#else
INC_UINT16 m_unIntCtrl = (INC_INTERRUPT_POLARITY_HIGH | \
                         INC_INTERRUPT_PULSE | \
                         INC_INTERRUPT_AUTOCLEAR_ENABLE | \
                         (INC_INTERRUPT_PULSE_COUNT & INC_INTERRUPT_PULSE_COUNT_MASK));
#endif

/*********************************************************************************/
/* PLL_MODE   m_ucPLL_Mode                                                       */
/*     T3700  Input Clock Setting                                                */
/*********************************************************************************/
#ifdef FEATURE_DMB_CLK_19200
PLL_MODE m_ucPLL_Mode = INPUT_CLOCK_19200KHZ;
#elif defined(FEATURE_DMB_CLK_24576)
PLL_MODE m_ucPLL_Mode = INPUT_CLOCK_24576KHZ;
#else
##error
#endif
/*********************************************************************************/
/* INC_DPD_MODE  m_ucDPD_Mode                                                    */
/*     T3700  Power Saving mode setting                                          */
/*********************************************************************************/
INC_DPD_MODE m_ucDPD_Mode = INC_DPD_ON;

#ifdef FEATURE_DMB_SPI_CMD
struct spi_device *inc_spi=NULL;
static INC_UINT8 rx_data[6];
INC_UINT8 auiBuff_burst[INC_INTERRUPT_SIZE+4];
INC_UINT8 auiBuff_burst2[INC_INTERRUPT_SIZE+4];
#endif
static DEFINE_MUTEX(spi_lock);
static DEFINE_MUTEX(i2c_lock);

/*********************************************************************************/
/*  MPI Chip Select and Clock Setup Part                                         */
/*                                                                               */
/*  INC_UINT8 m_ucCommandMode = INC_I2C_CTRL, INC_SPI_CTRL, INC_EBI_CTRL         */
/*                                                                               */
/*  INC_UINT8 m_ucUploadMode = STREAM_UPLOAD_MASTER_SERIAL,                      */
/*                             STREAM_UPLOAD_SLAVE_PARALLEL,                     */
/*                             STREAM_UPLOAD_TS,                                 */
/*                             STREAM_UPLOAD_SPI,                                */
/*                                                                               */
/*  INC_UINT8 m_ucClockSpeed = INC_OUTPUT_CLOCK_4096,                            */
/*                             INC_OUTPUT_CLOCK_2048,                            */
/*          I                  NC_OUTPUT_CLOCK_1024,                             */
/*********************************************************************************/

/*********************************************************************************/
/* 반드시 1ms Delay함수를 넣어야 한다.                                           */
/*********************************************************************************/
#ifdef FEATURE_COMMUNICATION_CHECK
void INC_RESET(void)
{
  t3900_reset();
}
#endif

//extern int rex_sleep(int);

void INC_DELAY(INC_UINT16 uiDelay)
{
   //rex_sleep(uiDelay);
   msleep(uiDelay);
}

INC_UINT16 INC_I2C_READ(INC_UINT8 ucI2CID, INC_UINT16 uiAddr)
{
  uint16 uiData = 0; 

  mutex_lock(&i2c_lock);

  if(t3700_i2c_read_word(ucI2CID, uiAddr, &uiData) == TRUE)
  {
    mutex_unlock(&i2c_lock);
    return uiData;
  }
  else
  {
    mutex_unlock(&i2c_lock);
    return INC_ERROR;
  }
}

INC_UINT8 INC_I2C_WRITE(INC_UINT8 ucI2CID, INC_UINT16 ulAddr, INC_UINT16 uiData)
{
  mutex_lock(&i2c_lock);

  if(t3700_i2c_write_word(ucI2CID,  ulAddr, uiData))
  {    
    mutex_unlock(&i2c_lock);
    return INC_SUCCESS;
  }
  else
  {
    mutex_unlock(&i2c_lock);
    return INC_ERROR;
  }
}

INC_UINT8 INC_I2C_READ_BURST(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT8* pData, INC_UINT16 nSize)
{
  mutex_lock(&i2c_lock);

  t3700_i2c_read_len(ucI2CID, uiAddr, pData, nSize);  

  mutex_unlock(&i2c_lock);
  return INC_SUCCESS;
}

INC_UINT8 INC_EBI_WRITE(INC_UINT8 ucI2CID, INC_UINT16 ulAddr, INC_UINT16 uiData)
{
  INC_UINT16 uiCMD = INC_REGISTER_CTRL(SPI_REGWRITE_CMD) | 1;
  INC_UINT16 uiNewAddr = (ucI2CID == T3700_I2C_ID82) ? (ulAddr | 0x8000) : ulAddr;
  unsigned long flags;

#ifdef FEATURE_EBI_WRITE_CHECK
  INC_UINT16 rData;
#endif

  //INTLOCK();
  local_irq_save(flags);
  *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiNewAddr >> 8;
  *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiNewAddr & 0xff;
  *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiCMD >> 8;
  *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiCMD & 0xff;

  *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = (uiData >> 8) & 0xff;
  *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS =  uiData & 0xff;
  //INTFREE();
  local_irq_restore(flags);

#ifdef FEATURE_EBI_WRITE_CHECK
  rData = INC_EBI_READ(ucI2CID, (INC_UINT16)ulAddr);
  if (uiData != rData)
  {
    TDMB_MSG_INC_BB("[%s] write error!!! w[0x%.4x] r[0x%.4x]\n", __func__, uiData, rData);
  }
  else
  {
    TDMB_MSG_INC_BB("[%s] write success!!!\n", __func__);
  }
#endif

  return INC_ERROR;
}

INC_UINT16 INC_EBI_READ(INC_UINT8 ucI2CID, INC_UINT16 uiAddr)
{
  INC_UINT16 uiRcvData = 0;
  INC_UINT16 uiCMD = INC_REGISTER_CTRL(SPI_REGREAD_CMD) | 1;
  INC_UINT16 uiNewAddr = (ucI2CID == T3700_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;
  unsigned long flags;

  //INTLOCK();
  local_irq_save(flags);
  *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiNewAddr >> 8;
  *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiNewAddr & 0xff;
  *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiCMD >> 8;
  *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiCMD & 0xff;

  uiRcvData  = (*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS  & 0xff) << 8;
  uiRcvData |= (*(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS & 0xff);
  //INTFREE();
  local_irq_restore(flags);

  return uiRcvData;
}

INC_UINT8 INC_EBI_READ_BURST(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT8* pData, INC_UINT16 nSize)
{
  INC_UINT16 uiLoop, nIndex = 0, anLength[2], uiCMD, unDataCnt;
  INC_UINT16 uiNewAddr = (ucI2CID == T3700_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;
  unsigned long flags;

  if(nSize > INC_MPI_MAX_BUFF) return INC_ERROR;
  memset((INC_INT8*)anLength, 0, sizeof(anLength));

  if(nSize > INC_TDMB_LENGTH_MASK) {
  anLength[nIndex++] = INC_TDMB_LENGTH_MASK;
  anLength[nIndex++] = nSize - INC_TDMB_LENGTH_MASK;
  }
  else anLength[nIndex++] = nSize;

  //INTLOCK();
  local_irq_save(flags);
  for(uiLoop = 0; (uiLoop < nIndex) && (uiLoop < 2); uiLoop++)
  {
    uiCMD = INC_REGISTER_CTRL(SPI_MEMREAD_CMD) | (anLength[uiLoop] & INC_TDMB_LENGTH_MASK);

    *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiNewAddr >> 8;
    *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiNewAddr & 0xff;
    *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiCMD >> 8;
    *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS = uiCMD & 0xff;

    for(unDataCnt = 0 ; unDataCnt < anLength[uiLoop]; unDataCnt++){
      *pData++ = *(volatile INC_UINT8*)STREAM_PARALLEL_ADDRESS & 0xff;
    }
  }
  //INTFREE();
  local_irq_restore(flags);

  return INC_SUCCESS;
}

#ifdef FEATURE_DMB_SPI_CMD
int INC_SPI_INIT(void)
{
  inc_spi = dmb_spi_setup();
  
  TDMB_MSG_INC_BB("[%s] INC SPI mode[%d] speed[%d]\n", __func__, inc_spi->mode, inc_spi->max_speed_hz);
  return INC_SUCCESS;
}

int INC_SPI_WRITE_N_READ(struct spi_device *spi, INC_UINT8 *txbuf, INC_UINT16 tx_len, INC_UINT8 *rxbuf, INC_UINT16 rx_len)
{
  int res;

  struct spi_message	message;
  struct spi_transfer	x;

  spi_message_init(&message);
  memset(&x, 0, sizeof (x));

  x.tx_buf = txbuf;
  x.rx_buf = rx_data;
  x.len = tx_len + rx_len;  
  x.cs_change = 1; //8bit burst mode(CS)
  x.bits_per_word = 8;
  x.delay_usecs = 0;
  
  spi_message_add_tail(&x, &message);

  res = spi_sync(spi, &message);
  memcpy(rxbuf, x.rx_buf+tx_len, rx_len);
#if 0
  if(rx_len != 0)
  {
    TDMB_MSG_INC_BB("SPI read  %x %x %x %x %x %x \n", rxbuf[0], rxbuf[1],rxbuf[2],rxbuf[3],rxbuf[4],rxbuf[5]);
  }
#endif
  return res;
}

int INC_SPI_WRITE_N_READ_BURST(struct spi_device *spi, INC_UINT8 *txbuf, INC_UINT16 tx_len, INC_UINT8 *rxbuf, INC_UINT16 rx_len)
{
  int res;

  struct spi_message	message;
  struct spi_transfer	x;

  spi_message_init(&message);
  memset(&x, 0, sizeof (x));

  spi_message_add_tail(&x, &message);

  x.tx_buf = txbuf;
  x.rx_buf = auiBuff_burst2;
  x.len = tx_len + rx_len;
  x.cs_change = 1; //8bit burst mode(CS)
  x.bits_per_word = 8;
  x.delay_usecs = 0;

  res = spi_sync(spi, &message);
  memcpy(rxbuf, x.rx_buf+tx_len, rx_len);

  return res;
}
#endif

INC_UINT16 INC_SPI_REG_READ(INC_UINT8 ucI2CID, INC_UINT16 uiAddr)
{
  INC_UINT16 uiRcvData = 0;
  INC_UINT16 uiCMD = INC_REGISTER_CTRL(SPI_REGREAD_CMD) | 1;
  static INC_UINT8 auiBuff[6];
  INC_UINT8 cCnt = 0;
  INC_UINT16 uiNewAddr = (ucI2CID == T3700_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;
  static INC_UINT8 temp_buf[6];

  mutex_lock(&spi_lock);

  auiBuff[cCnt++] = uiNewAddr >> 8;
  auiBuff[cCnt++] = uiNewAddr & 0xff;
  auiBuff[cCnt++] = uiCMD >> 8;
  auiBuff[cCnt++] = uiCMD & 0xff;

  //TODO SPI Write code here...
#ifdef FEATURE_DMB_SPI_CMD
#if 1
  INC_SPI_WRITE_N_READ(inc_spi, auiBuff, cCnt, temp_buf, 2);
#else
  dmb_spi_write_then_read(auiBuff, cCnt, temp_buf, 2);
#endif
#endif

  uiRcvData = (INC_UINT16)(((temp_buf[0]<<8)&0xFF00) |(temp_buf[1]&0x00FF));

  mutex_unlock(&spi_lock);

  //TDMB_MSG_INC_BB("SPI Read Reg[%x]  Data [%x]  cnt[%x]  [%x][%x]\n",uiAddr, uiRcvData, cCnt,temp_buf[0],temp_buf[1]);

  //TODO SPI Read code here...
  return uiRcvData;
}

INC_UINT8 INC_SPI_REG_WRITE(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT16 uiData)
{
  INC_UINT16 uiCMD = INC_REGISTER_CTRL(SPI_REGWRITE_CMD) | 1;
  static INC_UINT8 auiBuff[6];
  INC_UINT8 cCnt = 0;
  INC_UINT16 uiNewAddr = (ucI2CID == T3700_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;

  mutex_lock(&spi_lock);

  auiBuff[cCnt++] = uiNewAddr >> 8;
  auiBuff[cCnt++] = uiNewAddr & 0xff;
  auiBuff[cCnt++] = uiCMD >> 8;
  auiBuff[cCnt++] = uiCMD & 0xff;
  auiBuff[cCnt++] = uiData >> 8;
  auiBuff[cCnt++] = uiData & 0xff;

  //TODO SPI SDO Send code here...
#ifdef FEATURE_DMB_SPI_CMD
#if 1
  INC_SPI_WRITE_N_READ(inc_spi, auiBuff, cCnt, NULL, 0);
#else
  dmb_spi_write_then_read(auiBuff, cCnt, NULL, 0);
#endif
#endif

  mutex_unlock(&spi_lock);

  //TDMB_MSG_INC_BB("SPI WRITE Reg[%x] Data[%x] NewAddr[%x] [%x][%x] %d\n",uiAddr, uiData, uiNewAddr,auiBuff[4], auiBuff[5],cCnt);

  return INC_SUCCESS;
}

INC_UINT8 INC_SPI_READ_BURST(INC_UINT8 ucI2CID, INC_UINT16 uiAddr, INC_UINT8* pBuff, INC_UINT16 wSize)
{
  INC_UINT16 uiLoop, nIndex = 0, anLength[2], uiCMD;
#ifndef FEATURE_DMB_SPI_CMD
  INC_UINT8 auiBuff[6];
#endif
  INC_UINT16 uiNewAddr = (ucI2CID == T3700_I2C_ID82) ? (uiAddr | 0x8000) : uiAddr;

  if(wSize > INC_MPI_MAX_BUFF) return INC_ERROR;
  memset((INC_INT8*)anLength, 0, sizeof(anLength));
#ifdef FEATURE_DMB_SPI_CMD
  memset((INC_INT8*)auiBuff_burst, 0, sizeof(auiBuff_burst));
#endif

  if(wSize > 0xfff) {
    anLength[nIndex++] = 0xfff;
    anLength[nIndex++] = wSize - 0xfff;
  }
  else anLength[nIndex++] = wSize;

  mutex_lock(&spi_lock);

  for(uiLoop = 0; uiLoop < nIndex; uiLoop++){

#ifdef FEATURE_DMB_SPI_CMD
    auiBuff_burst[0] = uiNewAddr >> 8;
    auiBuff_burst[1] = uiNewAddr & 0xff;
    uiCMD = INC_REGISTER_CTRL(SPI_MEMREAD_CMD) | (anLength[uiLoop] & 0xFFF);
    auiBuff_burst[2] = uiCMD >> 8;
    auiBuff_burst[3] = uiCMD & 0xff;

    //TODO SPI[SDO] command Write code here..
    INC_SPI_WRITE_N_READ_BURST(inc_spi, auiBuff_burst, 4, pBuff, anLength[uiLoop] );
    pBuff+=anLength[uiLoop] ;
#else //SDK
    auiBuff[0] = uiNewAddr >> 8;
    auiBuff[1] = uiNewAddr & 0xff;
    uiCMD = INC_REGISTER_CTRL(SPI_MEMREAD_CMD) | (anLength[uiLoop] & 0xFFF);
    auiBuff[2] = uiCMD >> 8;
    auiBuff[3] = uiCMD & 0xff;
#endif
    //TODO SPI[SDI] data Receive code here.. 
  }

  mutex_unlock(&spi_lock);

  return INC_SUCCESS;
}

INC_UINT8 INTERFACE_DBINIT(void)
{
  memset(&g_stDmbInfo, 0, sizeof(ST_SUBCH_INFO));
  memset(&g_stDabInfo, 0, sizeof(ST_SUBCH_INFO));
  memset(&g_stDataInfo, 0, sizeof(ST_SUBCH_INFO));
  return INC_SUCCESS;
}

// 초기 전원 입력시 호출
INC_UINT8 INTERFACE_INIT(INC_UINT8 ucI2CID)
{
  return INC_INIT(ucI2CID);
}

// 에러 발생시 에러코드 읽기
INC_ERROR_INFO INTERFACE_ERROR_STATUS(INC_UINT8 ucI2CID)
{
  ST_BBPINFO* pInfo;
  pInfo = INC_GET_STRINFO(ucI2CID);
  return pInfo->nBbpStatus;
}

/*********************************************************************************/
/* 단일 채널 선택하여 시작하기....                                               */  
/* pChInfo->ucServiceType, pChInfo->ucSubChID, pChInfo->ulRFFreq 는              */
/* 반드시 넘겨주어야 한다.                                                       */
/* DMB채널 선택시 pChInfo->ucServiceType = 0x18                                  */
/* DAB, DATA채널 선택시 pChInfo->ucServiceType = 0으로 설정을 해야함.            */
/*********************************************************************************/
INC_UINT8 INTERFACE_START(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pChInfo)
{
  return INC_CHANNEL_START(ucI2CID, pChInfo);
}

/*********************************************************************************/
/* 다중 채널 선택하여 시작하기....                                               */  
/* pChInfo->ucServiceType, pChInfo->ucSubChID, pChInfo->ulRFFreq 는              */
/* 반드시 넘겨주어야 한다.                                                       */
/* DMB채널 선택시 pChInfo->ucServiceType = 0x18                                  */
/* DAB, DATA채널 선택시 pChInfo->ucServiceType = 0으로 설정을 해야함.            */
/* pMultiInfo->nSetCnt은 선택한 서브채널 개수임.                                 */
/* FIC Data를 같이  선택시 INC_MULTI_CHANNEL_FIC_UPLOAD 매크로를 풀어야 한다.    */
/* DMB  채널은 최대 2개채널 선택이 가능하다.                                     */
/* DAB  채널은 최대 3개채널 선택이 가능하다.                                     */
/* DATA 채널은 최대 3개채널 선택이 가능하다.                                     */
/*********************************************************************************/
INC_UINT8 INTERFACE_MULTISUB_CHANNEL_START(INC_UINT8 ucI2CID, ST_SUBCH_INFO* pMultiInfo)
{
  return (INC_UINT8)INC_MULTI_START_CTRL(ucI2CID, pMultiInfo);
}

/*********************************************************************************/
/* 스캔시  호출한다.                                                             */
/* 주파수 값은 받드시넘겨주어야 한다.                                            */
/* Band를 변경하여 스캔시는 m_ucRfBand를 변경하여야 한다.                        */
/* 주파수 값은 받드시넘겨주어야 한다.                                            */
/*********************************************************************************/
#ifdef INC_FICDEC_USE
INC_UINT8 INTERFACE_SCAN(INC_UINT8 ucI2CID, INC_UINT32 ulFreq)
{
  INC_INT16 nIndex;
  ST_FIC_DB* pstFicDb;
  INC_CHANNEL_INFO* pChInfo;
  pstFicDb = INC_GETFIC_DB(ucI2CID);
  INTERFACE_DBINIT();

  if(!INC_ENSEMBLE_SCAN(ucI2CID, ulFreq)) return INC_ERROR;
  pstFicDb->ulRFFreq = ulFreq;

  //[09.06.23 zeros] SP26S K7 1 #7649,#7650,#7651,#7652,#7653,#7654,#7655
  for(nIndex = 0; (nIndex < pstFicDb->ucSubChCnt) && (nIndex < MAX_SUBCHANNEL); nIndex++){
    switch(pstFicDb->aucTmID[nIndex])
    {
      case 0x01 : pChInfo = &g_stDmbInfo.astSubChInfo[g_stDmbInfo.nSetCnt++]; break;
      case 0x00 : pChInfo = &g_stDabInfo.astSubChInfo[g_stDabInfo.nSetCnt++]; break;
      default : pChInfo = &g_stDataInfo.astSubChInfo[g_stDataInfo.nSetCnt++]; break;
    }
    INC_UPDATE(pChInfo, pstFicDb, nIndex);
  }
  return INC_SUCCESS;
}
#endif

/*********************************************************************************/
/*  After Full Scan and Single Scan, Channel Information read Part               */
/*********************************************************************************/
void INTERFACE_AUDIO_I2S_ENABLE(INC_UINT8 ucI2CID)
{
  INC_CMD_WRITE(ucI2CID, APB_I2S_BASE+ 0x05, 0x0003); // Audio Enable
}

/*********************************************************************************/
/* 단일채널 스캔이 완료되면 DMB채널 개수를 리턴한다.                             */
/*********************************************************************************/
INC_UINT16 INTERFACE_GETDMB_CNT(void)
{
  return (INC_UINT16)g_stDmbInfo.nSetCnt;
}

/*********************************************************************************/
/* 단일채널 스캔이 완료되면 DAB채널 개수를 리턴한다.                             */
/*********************************************************************************/
INC_UINT16 INTERFACE_GETDAB_CNT(void)
{
  return (INC_UINT16)g_stDabInfo.nSetCnt;
}

/*********************************************************************************/
/* 단일채널 스캔이 완료되면 DATA채널 개수를 리턴한다.                            */
/*********************************************************************************/
INC_UINT16 INTERFACE_GETDATA_CNT(void)
{
  return (INC_UINT16)g_stDataInfo.nSetCnt;
}

/*********************************************************************************/
/* 단일채널 스캔이 완료되면 Ensemble lable을 리턴한다.                           */
/*********************************************************************************/
#ifdef INC_FICDEC_USE
INC_UINT8* INTERFACE_GETENSEMBLE_LABEL(INC_UINT8 ucI2CID)
{
  ST_FIC_DB* pstFicDb;
  pstFicDb = INC_GETFIC_DB(ucI2CID);
  return pstFicDb->aucEnsembleLabel;
}
#endif

/*********************************************************************************/
/* DMB 채널 정보를 리턴한다.                                                     */
/*********************************************************************************/
INC_CHANNEL_INFO* INTERFACE_GETDB_DMB(INC_INT16 uiPos)
{
  if(uiPos >= MAX_SUBCH_SIZE) return INC_NULL;
  if(uiPos >= g_stDmbInfo.nSetCnt) return INC_NULL;
  return &g_stDmbInfo.astSubChInfo[uiPos];
}

/*********************************************************************************/
/* DAB 채널 정보를 리턴한다.                                                     */
/*********************************************************************************/
INC_CHANNEL_INFO* INTERFACE_GETDB_DAB(INC_INT16 uiPos)
{
  if(uiPos >= MAX_SUBCH_SIZE) return INC_NULL;
  if(uiPos >= g_stDabInfo.nSetCnt) return INC_NULL;
  return &g_stDabInfo.astSubChInfo[uiPos];
}

/*********************************************************************************/
/* DATA 채널 정보를 리턴한다.                                                    */
/*********************************************************************************/
INC_CHANNEL_INFO* INTERFACE_GETDB_DATA(INC_INT16 uiPos)
{
  if(uiPos >= MAX_SUBCH_SIZE) return INC_NULL;
  if(uiPos >= g_stDataInfo.nSetCnt) return INC_NULL;
  return &g_stDataInfo.astSubChInfo[uiPos];
}

// 시청 중 FIC 정보 변경되었는지를 체크
INC_UINT8 INTERFACE_RECONFIG(INC_UINT8 ucI2CID)
{
  return INC_FIC_RECONFIGURATION_HW_CHECK(ucI2CID);
}

INC_UINT8 INTERFACE_STATUS_CHECK(INC_UINT8 ucI2CID)
{
  return INC_STATUS_CHECK(ucI2CID);
}

INC_UINT16 INTERFACE_GET_CER(INC_UINT8 ucI2CID)
{
  return INC_GET_CER(ucI2CID);
}

INC_UINT8 INTERFACE_GET_SNR(INC_UINT8 ucI2CID)
{
  return INC_GET_SNR(ucI2CID);
}

INC_DOUBLE32 INTERFACE_GET_POSTBER(INC_UINT8 ucI2CID)
{
  return INC_GET_POSTBER(ucI2CID);
}

INC_DOUBLE32 INTERFACE_GET_PREBER(INC_UINT8 ucI2CID)
{
  return INC_GET_PREBER(ucI2CID);
}

/*********************************************************************************/
/* Scan, 채널 시작시에 강제로 중지시 호출한다.                                   */
/*********************************************************************************/
void INTERFACE_USER_STOP(INC_UINT8 ucI2CID)
{
  ST_BBPINFO* pInfo;
  pInfo = INC_GET_STRINFO(ucI2CID);
  pInfo->ucStop = 1;
}

// 인터럽트 인에이블...
void INTERFACE_INT_ENABLE(INC_UINT8 ucI2CID)
{
  INC_INTERRUPT_INIT(ucI2CID);
}

// 인터럽스 클리어
void INTERFACE_INT_CLEAR(INC_UINT8 ucI2CID)
{
  INC_INTERRUPT_CLEAR(ucI2CID);
}

// 인터럽트 서비스 루틴... // SPI Slave Mode or MPI Slave Mode
INC_UINT8 INTERFACE_ISR(INC_UINT8 ucI2CID, INC_UINT8* pBuff)
{
  INC_UINT16 unLoop;
  INC_UINT32 ulAddrSelect;

  if(m_ucUploadMode == STREAM_UPLOAD_SPI){
    INC_SPI_READ_BURST(ucI2CID, APB_STREAM_BASE, pBuff, INC_INTERRUPT_SIZE);
  }
  if(m_ucUploadMode == STREAM_UPLOAD_SLAVE_PARALLEL)
  {
    ulAddrSelect = (ucI2CID == T3700_I2C_ID80) ? (INC_UINT32)STREAM_PARALLEL_ADDRESS : (INC_UINT32)STREAM_PARALLEL_ADDRESS_CS;
    for(unLoop = 0; unLoop < INC_INTERRUPT_SIZE; unLoop++){
      pBuff[unLoop] = *(volatile INC_UINT8*)ulAddrSelect & 0xff;
    }
  }

  if((m_unIntCtrl & INC_INTERRUPT_LEVEL) && (!(m_unIntCtrl & INC_INTERRUPT_AUTOCLEAR_ENABLE)))
  INTERFACE_INT_CLEAR(ucI2CID);

  return INC_SUCCESS;
}

