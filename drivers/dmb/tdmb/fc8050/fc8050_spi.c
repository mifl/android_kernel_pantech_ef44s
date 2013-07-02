/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : fc8050_spi.c
 
 Description : fc8050 host interface
 
 History : 
 ----------------------------------------------------------------------
 2009/08/29 	jason		initial
*******************************************************************************/
#include <linux/input.h>
#include <linux/spi/spi.h>

#include "fci_types.h"
#include "fc8050_regs.h"

#define SPI_PLATFORM_DEV_NAME "tdmb_spi"

#define HPIC_READ     0x01 // read command
#define HPIC_WRITE    0x02 // write command
#define HPIC_AINC     0x04 // address increment
#define HPIC_BMODE    0x00 // byte mode
#define HPIC_WMODE    0x10 // word mode
#define HPIC_LMODE    0x20 // long mode
#define HPIC_ENDIAN   0x00 // little endian
#define HPIC_CLEAR    0x80 // currently not used

#define CHIPID 0
#if (CHIPID == 0)
#define SPI_CMD_WRITE           0x0
#define SPI_CMD_READ            0x1
#define SPI_CMD_BURST_WRITE    0x2
#define SPI_CMD_BURST_READ     0x3
#else
#define SPI_CMD_WRITE           0x4
#define SPI_CMD_READ            0x5
#define SPI_CMD_BURST_WRITE    0x6
#define SPI_CMD_BURST_READ     0x7
#endif 


struct spi_device *fc8050_spi = NULL;

static u8 tx_data[10];
static u8 data_buf[8136]={0};


static DEFINE_MUTEX(lock);



int fc8050_spi_write_then_read(struct spi_device *spi, u8 *txbuf, u16 tx_length, u8 *rxbuf, u16 rx_length)
{
  int res;

  struct spi_message	message;
  struct spi_transfer	x;

  //TDMB_MSG_FCI_BB("[%s]spi=%x\n", __func__, (unsigned int)spi);

  spi_message_init(&message);
  memset(&x, 0, sizeof x);

  spi_message_add_tail(&x, &message);

  x.tx_buf = txbuf;
  x.rx_buf = &data_buf;
  x.len = tx_length + rx_length;

  res = spi_sync(spi, &message);

  memcpy(rxbuf, x.rx_buf + tx_length, rx_length);

  return res;
}

int fc8050_spi_write_then_read_burst(struct spi_device *spi, u8 *txbuf, u16 tx_length, u8 *rxbuf, u16 rx_length)
{
  int res;

  struct spi_message	message;
  struct spi_transfer	x;

  //TDMB_MSG_FCI_BB("[%s]spi=%x\n", __func__, (unsigned int)spi);

  spi_message_init(&message);
  memset(&x, 0, sizeof x);

  spi_message_add_tail(&x, &message);

  x.tx_buf = txbuf;
  x.rx_buf = rxbuf;
  x.len = tx_length + rx_length;

  res = spi_sync(spi, &message);

  return res;
}


static int spi_bulkread(HANDLE hDevice, u8 addr, u8 *data, u16 length)
{
  int ret;

  tx_data[0] = SPI_CMD_BURST_READ;
  tx_data[1] = addr;

  ret = fc8050_spi_write_then_read(fc8050_spi, &tx_data[0], 2, &data[0], length);
  //ret = tdmb_spi_write_then_read(&tx_data[0], 2, &data[0], length);

  if(ret)
  {
    TDMB_MSG_FCI_BB("[%s] fail : %d\n", __func__, ret);
    return BBM_NOK;
  }

  return BBM_OK;
}

static int spi_bulkwrite(HANDLE hDevice, u8 addr, u8* data, u16 length)
{
  int ret;
  int i;

  tx_data[0] = SPI_CMD_BURST_WRITE;
  tx_data[1] = addr;

  for(i=0;i<length;i++)
  {
    tx_data[2+i] = data[i];
  }

  ret = fc8050_spi_write_then_read(fc8050_spi, &tx_data[0], length+2, NULL, 0);
  //ret = tdmb_spi_write_then_read(&tx_data[0], length+2, NULL, 0);

  if(ret)
  {
    TDMB_MSG_FCI_BB("[%s] fail : %d\n", __func__, ret);
    return BBM_NOK;
  }

  return BBM_OK;
}

static int spi_dataread(HANDLE hDevice, u8 addr, u8* data, u16 length)
{
  int ret=0;

  tx_data[0] = SPI_CMD_BURST_READ;
  tx_data[1] = addr;

  ret = fc8050_spi_write_then_read_burst(fc8050_spi, &tx_data[0], 2, &data[0], length);
  //ret = tdmb_spi_write_then_read(&tx_data[0], 2, &data[0], length);

  if(ret)
  {
    TDMB_MSG_FCI_BB("[%s] fail : %d\n", __func__, ret);
    return BBM_NOK;
  }

  return BBM_OK;
}


#if 0 // not used
static int __devinit fc8050_spi_probe(struct spi_device *spi)
{
  int ret;

  TDMB_MSG_FCI_BB("[%s]\n", __func__);

  //spi->max_speed_hz =  4000000;
  spi->max_speed_hz =  8000000;

  ret = spi_setup(spi);
  if (ret < 0)
    return ret;

  fc8050_spi = spi;

  return ret;
}

static int fc8050_spi_remove(struct spi_device *spi)
{
  return 0;
}

static struct spi_driver fc8050_spi_driver = {
  .driver = {
    .name = SPI_PLATFORM_DEV_NAME,
    .bus = &spi_bus_type,
    .owner = THIS_MODULE,
  },
  .probe = fc8050_spi_probe,
  .remove = __devexit_p(fc8050_spi_remove),
};
#endif // not used


int fc8050_spi_init(HANDLE hDevice, u16 param1, u16 param2)
{
#if 1
  TDMB_MSG_FCI_BB("[%s] do nothing!!!\n", __func__);

#ifdef FEATURE_DMB_SPI_IF
  fc8050_spi = tdmb_spi_setup();
#endif

#else
  int res;

  TDMB_MSG_FCI_BB("[%s]start\n", __func__);

  res = spi_register_driver(&fc8050_spi_driver);

  if(res)
  {
    TDMB_MSG_FCI_BB("[%s] register fail : %d\n", __func__, res);
    return BBM_NOK;
  }

#if 0
  data_buf = kmalloc(8192, GFP_DMA|GFP_KERNEL);
  if (!data_buf)
  {
    TDMB_MSG_FCI_BB("[%s] kmalloc fail ", __func__);
    return BBM_NOK;
  }
#endif

  if(res)
  {
    TDMB_MSG_FCI_BB("[%s]spi_probe fail : %d", __func__,res);
    return BBM_NOK;
  }

#if 1
  if(fc8050_spi == NULL)
  {
    TDMB_MSG_FCI_BB("[%s] SPI device is null\n", __func__);
    return 0;
  }
  else 
  {
    TDMB_MSG_FCI_BB("[%s] SPI  Max speed [%d] CS[%d]  Mode[%d] \n", __func__,
    fc8050_spi->max_speed_hz, fc8050_spi->chip_select, fc8050_spi->mode);
  }
#endif
#endif

  TDMB_MSG_FCI_BB("[%s]end\n", __func__);  

  return BBM_OK;
}

int fc8050_spi_byteread(HANDLE hDevice, u16 addr, u8 *data)
{
  int res;
  u8 command = HPIC_READ | HPIC_BMODE | HPIC_ENDIAN;

  mutex_lock(&lock);
  res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
  res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8*)&addr, 2);
  res |= spi_bulkread(hDevice, BBM_DATA_REG, data, 1);
  mutex_unlock(&lock);

  return res;
}

int fc8050_spi_wordread(HANDLE hDevice, u16 addr, u16 *data)
{
  int res;
  u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

  if(BBM_SCI_DATA <= addr && BBM_SCI_SYNCRX >= addr)
    command = HPIC_READ | HPIC_WMODE | HPIC_ENDIAN;

  mutex_lock(&lock);
  res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
  res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8*)&addr, 2);
  res |= spi_bulkread(hDevice, BBM_DATA_REG, (u8*)data, 2);
  mutex_unlock(&lock);

  return res;
}

int fc8050_spi_longread(HANDLE hDevice, u16 addr, u32 *data)
{
  int res;
  u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

  mutex_lock(&lock);
  res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
  res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8*)&addr, 2);
  res |= spi_bulkread(hDevice, BBM_DATA_REG, (u8*)data, 4);
  mutex_unlock(&lock);

  return res;
}

int fc8050_spi_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
  int res;
  u8 command = HPIC_READ | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

  mutex_lock(&lock);
  res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
  res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8*)&addr, 2);
  res |= spi_bulkread(hDevice, BBM_DATA_REG, data, length);
  mutex_unlock(&lock);

  return res;
}

int fc8050_spi_bytewrite(HANDLE hDevice, u16 addr, u8 data)
{
  int res;
  u8 command = HPIC_WRITE | HPIC_BMODE | HPIC_ENDIAN;

  mutex_lock(&lock);
  res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
  res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8*)&addr, 2);
  res |= spi_bulkwrite(hDevice, BBM_DATA_REG, (u8*)&data, 1);
  mutex_unlock(&lock);

  return res;
}

int fc8050_spi_wordwrite(HANDLE hDevice, u16 addr, u16 data)
{
  int res;
  u8 command = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

  if(BBM_SCI_DATA <= addr && BBM_SCI_SYNCRX >= addr)
    command = HPIC_WRITE | HPIC_WMODE | HPIC_ENDIAN;

  mutex_lock(&lock);
  res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
  res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8*)&addr, 2);
  res |= spi_bulkwrite(hDevice, BBM_DATA_REG, (u8*)&data, 2);
  mutex_unlock(&lock);

  return res;
}

int fc8050_spi_longwrite(HANDLE hDevice, u16 addr, u32 data)
{
  int res;
  u8 command = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

  mutex_lock(&lock);
  res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
  res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8*)&addr, 2);
  res |= spi_bulkwrite(hDevice, BBM_DATA_REG, (u8*)&data, 4);
  mutex_unlock(&lock);

  return res;
}

int fc8050_spi_bulkwrite(HANDLE hDevice, u16 addr, u8* data, u16 length)
{
  int res;
  u8 command = HPIC_WRITE | HPIC_AINC | HPIC_BMODE | HPIC_ENDIAN;

  mutex_lock(&lock);
  res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
  res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8*)&addr, 2);
  res |= spi_bulkwrite(hDevice, BBM_DATA_REG, data, length);
  mutex_unlock(&lock);

  return res;
}

int fc8050_spi_dataread(HANDLE hDevice, u16 addr, u8* data, u16 length)
{
  int res;
  u8 command = HPIC_READ | HPIC_BMODE | HPIC_ENDIAN;

  mutex_lock(&lock);
  res  = spi_bulkwrite(hDevice, BBM_COMMAND_REG, &command, 1);
  res |= spi_bulkwrite(hDevice, BBM_ADDRESS_REG, (u8*)&addr, 2);
  res |= spi_dataread(hDevice, BBM_DATA_REG, data, length);
  mutex_unlock(&lock);
  
  return res;
}

int fc8050_spi_deinit(HANDLE hDevice)
{
  TDMB_MSG_FCI_BB("[%s]\n", __func__);

#if 0 //not used
  spi_unregister_driver(&fc8050_spi_driver);

  kfree(data_buf);
  data_buf = NULL;
#endif

  return BBM_OK;
}

