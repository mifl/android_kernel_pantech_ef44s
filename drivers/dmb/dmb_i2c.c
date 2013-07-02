//=============================================================================
// File       : dmb_i2c.c
//
// Description: 
//
// Revision History:
//
// Version         Date           Author        Description of Changes
//-----------------------------------------------------------------------------
//  1.0.0       2009/05/06       yschoi         Create
//  1.1.0       2011/09/29       yschoi         tdmb_i2c.c => dmb_i2c.c
//=============================================================================

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include "dmb_comdef.h"
#include "dmb_i2c.h"
#include "dmb_type.h"



/*================================================================== */
/*================       DMB I2C Driver Definition        ================= */
/*================================================================== */

//#define FEATURE_DMB_I2C_DBG_MSG
//#define FEATURE_DMB_I2C_WRITE_CHECK


#ifdef FEATURE_DMB_I2C_DBG_MSG
#define DMB_MSG_I2C_DBG(fmt, arg...) \
  DMB_MSG_I2C(fmt, ##arg)
#else
#define DMB_MSG_I2C_DBG(fmt, arg...) \
  {}
#endif


#define MAX_REG_LEN 2
#define MAX_DATA_LEN 16
#define MAX_DATA_LEN_LONG (512 + 8)

#if 0
static struct i2c_client *dmb_i2c_client = NULL;
#else
struct i2c_client *dmb_i2c_client = NULL;
#endif

static int dmb_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);



/*================================================================== */
/*==============        DMB I2C Driver Function      =============== */
/*================================================================== */

/*====================================================================
FUNCTION       dmb_i2c_probe
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int dmb_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
  int rc = 0;

  if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
  {
    dmb_i2c_client = NULL;
    rc = -1;
    DMB_MSG_I2C("[%s] failed!!!\n", __func__);
  }
  else
  {
    dmb_i2c_client = client;
  }

  //DMB_MSG_I2C("[%s] succeed!!!\n", __func__);
  DMB_MSG_I2C("[%s] succeed!!! ID[0x%x]\n", __func__, dmb_i2c_client->addr);

  return rc;
}


/*====================================================================
FUNCTION       dmb_i2c_remove
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int dmb_i2c_remove(struct i2c_client *client)
{
#if 0 // 20101102 cys
  int rc;

  dmb_i2c_client = NULL;
#if 1
  rc = i2c_detach_client(client);

  return rc;
#endif
#endif // 0
  DMB_MSG_I2C("[%s] removed!!!\n", __func__);
  return 0;
}


static const struct i2c_device_id dmb_i2c_id[] = {
  {DMB_I2C_DEV_NAME, 0},
};

static struct i2c_driver dmb_i2c_driver = {
  .id_table = dmb_i2c_id,
  .probe  = dmb_i2c_probe,
  .remove = dmb_i2c_remove,
  .driver = {
    .name = DMB_I2C_DEV_NAME,
  },
};


/*====================================================================
FUNCTION       dmb_i2c_api_Init
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void dmb_i2c_api_Init(void)
{
  int result = 0;

  result = i2c_add_driver(&dmb_i2c_driver);
  DMB_MSG_I2C("[%s] i2c_add_driver!\n", __func__);

  if(result){
    DMB_MSG_I2C("[%s] error!!!\n", __func__);
  }

#ifdef FEATURE_DMB_I2C_HW
  DMB_MSG_I2C(" --> DMB I2C use QUP HW I2C \n");
#else
  DMB_MSG_I2C(" --> DMB I2C use GPIO SW I2C \n");
#endif
}


/*====================================================================
FUNCTION       dmb_i2c_api_DeInit
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void dmb_i2c_api_DeInit(void)
{
  i2c_del_driver(&dmb_i2c_driver);
}



/*================================================================== */
/*=================        DMB i2c Function       ================== */
/*================================================================== */

/*====================================================================
FUNCTION       dmb_i2c_check_id
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
static int dmb_i2c_check_id(uint8 chipid)
{
  if(dmb_i2c_client->addr != (chipid >> 1))
  {
    DMB_MSG_I2C("[%s] chipid error addr[0x%02x], chipid[0x%02x]!!!\n", __func__, dmb_i2c_client->addr, (chipid>>1));
    return FALSE;
  }
  return TRUE;
}


/*====================================================================
FUNCTION       dmb_i2c_write_data
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 dmb_i2c_write_data(uint16 reg, uint8 *data, uint16 data_len)
{
  static int ret = 0;
  struct i2c_msg msgs[] = 
  {
    {
      .addr = dmb_i2c_client->addr,  .flags = 0, .len = data_len,  .buf = data,
    },
  };

  if(!dmb_i2c_client)
  {
    DMB_MSG_I2C("[%s] dmb_i2c_client is null!!!\n", __func__);
    return -1;
  }

  DMB_MSG_I2C_DBG("[%s] ID[0x%02x] reg[0x%04x] data[0x%04x]\n", __func__, dmb_i2c_client->addr, reg, *data);

  ret = i2c_transfer(dmb_i2c_client->adapter, msgs, 1);
  if(ret < 0)
  {
    DMB_MSG_I2C_DBG("[%s] write error!!! reg[0x%x], ret[%d]\n", __func__, reg, ret);
    return FALSE;
  }
  else
  {
    DMB_MSG_I2C_DBG("[%s] write OK!!!\n", __func__);
    return TRUE;
  }
}


/*====================================================================
FUNCTION       dmb_i2c_write
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 dmb_i2c_write(uint8 chip_id, uint16 reg, uint8 reg_len, uint16 data, uint8 data_len)
{
  static int ret = 0;
  unsigned char wlen=0;
  unsigned char buf[MAX_DATA_LEN];
#ifdef FEATURE_DMB_I2C_WRITE_CHECK
  uint16 *rData;
#endif /* FEATURE_DMB_I2C_WRITE_CHECK */

  if(!dmb_i2c_client)
  {
    DMB_MSG_I2C("[%s] dmb_i2c_client is null!!!\n", __func__);
    return FALSE;
  }

  if(!dmb_i2c_check_id(chip_id))
  {
    DMB_MSG_I2C("[%s] dmb i2c chipID error !!!\n", __func__);
    return FALSE;
  }

  memset(buf, 0, sizeof(buf));

  wlen = reg_len + data_len;

  switch(reg_len) //현재는 byte, word write 만 사용 
  {
  case 2:
    buf[0] = (reg & 0xFF00) >> 8;
    buf[1] = (reg & 0x00FF);
    buf[2] = (data & 0xFF00) >> 8;
    buf[3] = (data & 0x00FF);
    break;

  case 1:
    buf[0] = (reg & 0x00FF);
    if(data_len > 1) //8bit addr, more than 8bit data For FCI
    {
      memcpy(buf + reg_len, &data, 2); //temp
    }
    else
    {
      buf[1] = (data & 0x00FF);
    }
    break;

  default:
    break;
  }

  DMB_MSG_I2C_DBG("[%s] ID[0x%02x] reg[0x%04x] data[0x%04x]\n", __func__, dmb_i2c_client->addr, reg, data);

  ret = dmb_i2c_write_data(reg, buf, wlen);

  if(ret > 0)
  {   
#ifdef FEATURE_DMB_I2C_WRITE_CHECK
    dmb_i2c_read(reg, rData, 2);
    if (rData != data)
    {
      DMB_MSG_I2C("[%s] r/w check error! reg[0x%04x], data[0x%04x]\n", __func__, reg, rData);
    }
#endif /* FEATURE_DMB_I2C_WRITE_CHECK */
  }
  else
  {
    DMB_MSG_I2C("[%s] Write Error!!![%d]\n", __func__, ret);	
    return FALSE;
  }
  return TRUE;
}


/*====================================================================
FUNCTION       dmb_i2c_write_len
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint8 dmb_i2c_write_len(uint8 chip_id, uint16 reg, uint8 reg_len, uint8 *data, uint16 data_len)
{
  static int ret = 0;
  uint16 buf_len = 0;
  unsigned char buf[MAX_DATA_LEN_LONG];

  if(!dmb_i2c_client)
  {
    DMB_MSG_I2C("[%s] dmb_i2c_client is null!!!\n", __func__);
    return FALSE;
  }

  //DMB_MSG_I2C_DBG("[%s] ID[0x%02x] reg[0x%04x]\n", __func__, dmb_i2c_client->addr, reg);

  if(!dmb_i2c_check_id(chip_id))
  {
    DMB_MSG_I2C("[%s] dmb i2c chipID error !!!\n", __func__);
    return FALSE;
  }
  memset(buf, 0, sizeof(buf));

  buf_len = reg_len + data_len;

  switch(reg_len)
  {
  case 2:
    buf[0] = (reg & 0xFF00) >> 8;
    buf[1] = (reg & 0x00FF);
    break;

  case 1:
    buf[0] = (reg & 0x00FF);
    break;

  default:
    break;
  }

  memcpy(&buf[reg_len], data, data_len); //temp

  DMB_MSG_I2C_DBG("[%s] ID[0x%02x] reg[0x%04x] cnt[%d]\n", __func__, dmb_i2c_client->addr, reg, data_len);

  ret = dmb_i2c_write_data(reg, buf, buf_len);

  if(ret == 0)
  {
    DMB_MSG_I2C_DBG("[%s] write len error!!!\n", __func__);
    return FALSE;
  }

  return TRUE;
}


/*====================================================================
FUNCTION       dmb_i2c_read_data
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint16 dmb_i2c_read_data(uint16 reg, uint8 reg_len, uint8 *data, uint8 *read_buf, uint16 data_len)
{
  static int ret = 0;
  struct i2c_msg msgs[] = 
  {
    {
      .addr = dmb_i2c_client->addr,  .flags = 0, .len = reg_len,  .buf = data,
    },
    {
      .addr = dmb_i2c_client->addr,  .flags = I2C_M_RD,  .len = data_len,  .buf = read_buf,
    },
  };

  DMB_MSG_I2C_DBG("[%s] ID[0x%02x] reg[0x%04x]\n", __func__, dmb_i2c_client->addr, reg);

//EF33S QUP I2C 사용시 두번 보내야함
#if 0 // android 3145 버전에서 msgs 2개 한번에 보내면 에러리턴됨
  ret = i2c_transfer(dmb_i2c_client->adapter, msgs, 1);
  if (ret < 0)
  {
    DMB_MSG_I2C_DBG("[%s] write error!!!\n", __func__);
    return FALSE;
  }
  ret = i2c_transfer(dmb_i2c_client->adapter, &msgs[1], 1);
#else
  ret = i2c_transfer(dmb_i2c_client->adapter, msgs, 2);
#endif

  if(ret < 0)
  {
    DMB_MSG_I2C("[%s] read Error!!![%d]\n", __func__, ret);
    return FALSE;
  }
  else
  {
    DMB_MSG_I2C_DBG("[%s] read OK!!!\n", __func__);
    return TRUE;
  }
}


/*====================================================================
FUNCTION       dmb_i2c_read_data_only
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint16 dmb_i2c_read_data_only(uint16 reg, uint8 *read_buf, uint16 data_len)
{
  static int ret = 0;
  struct i2c_msg msgs[] = 
  {
    {
      .addr = dmb_i2c_client->addr,  .flags = I2C_M_RD,  .len = data_len,  .buf = read_buf,
    },
  };

  DMB_MSG_I2C_DBG("[%s] ID[0x%02x] reg[0x%04x]\n", __func__, dmb_i2c_client->addr, reg);

  ret = i2c_transfer(dmb_i2c_client->adapter, msgs, 1);

  if(ret < 0)
  {
    DMB_MSG_I2C_DBG("[%s] read error!!! addr[0x%x], ret[%d]\n", __func__, dmb_i2c_client->addr, ret);
    return FALSE;
  }
  else
  {
    DMB_MSG_I2C_DBG("[%s] read OK!!!\n", __func__);
    return TRUE;
  }
}


/*====================================================================
FUNCTION       dmb_i2c_read
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint16 dmb_i2c_read(uint8 chip_id, uint16 reg, uint8 reg_len, uint16 *data, uint8 data_len)
{
  static int ret = 0;
  uint16 rlen = 0;
  unsigned char buf[2] = {0,0};

  if(!dmb_i2c_client)
  {
    DMB_MSG_I2C("[%s] dmb_i2c_client is null!!!\n", __func__);
    return FALSE;
  }

  DMB_MSG_I2C_DBG("[%s] ID[0x%02x] reg[0x%04x]\n", __func__, dmb_i2c_client->addr, reg);

  if(!dmb_i2c_check_id(chip_id))
  {
    DMB_MSG_I2C("[%s] dmb i2c chipID error !!!\n", __func__);
    return FALSE;
  }

  memset(buf, 0, sizeof(buf));

  rlen = data_len;

  switch(reg_len)
  {
  case 2:
    buf[0] = (reg & 0xFF00) >> 8;
    buf[1] = (reg & 0x00FF);
    ret = dmb_i2c_read_data(reg, reg_len, buf, buf, rlen);
    if(ret < 0)
    {
      return FALSE;
    }
    *data = (buf[0] << 8) | buf[1];
    break;

  case 1:
    buf[0] = (reg & 0x00FF);
    ret = dmb_i2c_read_data(reg, reg_len, buf, buf, rlen);
    if(ret < 0)
    {
      return FALSE;
    }
    *(uint8 *)data = buf[0];
    //(uint8)*data = buf[0];
    break;

  default:
    break;
  }

  if(ret > 0)
  {
    //DMB_MSG_I2C_DBG("[%s] read OK!!!\n", __func__);
    DMB_MSG_I2C_DBG("[%s] reg[0x%04x], data[0x%04x]\n", __func__, reg, *data);
  }
  else
  {
    DMB_MSG_I2C_DBG("[%s] read Error!!!\n", __func__);
  }

  return ret;
}


/*====================================================================
FUNCTION       dmb_i2c_read_len
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
void dmb_i2c_read_len(uint8 chip_id, uint16 reg, uint8 *read_buf, uint16 read_buf_len, uint8 reg_len)
{
  static int ret = 0;
  unsigned char buf_len = 0;
  unsigned char buf[2];

  if(!dmb_i2c_client)
  {
    DMB_MSG_I2C("[%s] dmb_i2c_client is null!!!\n", __func__);
    return;
  }

  //QUP I2C read시 data len 256byte 넘으면 에러.
#if 0 //def FEATURE_DMB_I2C_HW  
  if(ucCount > 255)
  {
    ucCount = 255;
    DMB_MSG_I2C("[%s] data len > 255!!!\n", __func__);
  }
#endif

  memset(buf, 0, sizeof(buf));

  buf_len = reg_len;

  switch(reg_len)
  {
  case 2:
    buf[0] = (reg & 0xFF00) >> 8;
    buf[1] = (reg & 0x00FF);
    break;

  case 1:
    buf[0] = (reg & 0x00FF);
    break;

  default:
    break;
  }

  DMB_MSG_I2C_DBG("[%s] ID[0x%02x] reg[0x%04x] cnt[%d]\n", __func__, dmb_i2c_client->addr, reg, read_buf_len);

  ret = dmb_i2c_read_data(reg, buf_len, buf, read_buf, read_buf_len);

  if(ret == 0)
  {
    DMB_MSG_I2C_DBG("[%s] read len error!!!\n", __func__);
  }
  else
  {    
#ifdef FEATURE_DMB_I2C_DBG_MSG
    if(read_buf_len == 2)
    {
      //uiData = (ucRecvBuf[0] << 8) | ucRecvBuf[1];
      DMB_MSG_I2C_DBG("R2 [%x] read [%x][%x]\n", buf[0],read_buf[0],read_buf[1]);
    }
    else if(read_buf_len ==4)
    {
      //uiData = (ucRecvBuf[0] << 8) | ucRecvBuf[1];
      DMB_MSG_I2C_DBG("R4 [%x]read [%x][%x][%x][%x]\n", buf[0],read_buf[0],read_buf[1],read_buf[2],read_buf[3]);
    }
    //DMB_MSG_I2C("[%s] reg[0x%04x], data[0x%04x]\n", __func__, uiAddr, uiData);
#endif /* DMB_MSG_I2C_DBG */
  }
  return;
}


/*====================================================================
FUNCTION       dmb_i2c_read_len_multi
DESCRIPTION 
DEPENDENCIES
RETURN VALUE
SIDE EFFECTS
======================================================================*/
uint16 dmb_i2c_read_len_multi(uint16 reg, uint8 *data, uint16 reg_len, uint8 *read_buf, uint16 data_len)
{
  static int ret = 0;

  ret = dmb_i2c_read_data(reg, reg_len, data, read_buf, data_len);

  if(ret == 0)
  {
    DMB_MSG_I2C("[%s] read len error!!!\n", __func__);
  }

  return ret;
}

