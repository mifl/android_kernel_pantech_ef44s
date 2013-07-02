/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : fc8050_demux.c
 
 Description : fc8050 TSIF demux
 
 History : 
 ----------------------------------------------------------------------
 2009/04/09   woost   initial
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
/*
** Include Header File
*/

#include "../../dmb_test.h"

#include "fci_types.h"
#include "fc8050_regs.h"
#include "fc8050_demux.h"

/*============================================================
**  1.   DEFINITIONS
*============================================================*/

// Sync byte
#define SYNC_MASK_FIC           0x80
#define SYNC_MASK_DM            0x90
#define SYNC_MASK_NVIDEO        0xC0
#define SYNC_MASK_VIDEO         0x47
#define SYNC_MASK_VIDEO1        0xB8
#define SYNC_MASK_APAD          0xA0

// packet indicator
#define PKT_IND_NONE            0x00
#define PKT_IND_END             0x20
#define PKT_IND_CONTINUE        0x40
#define PKT_IND_START           0x80
#define PKT_IND_MASK            0xE0

// data size
#define FIC_DATA_SIZE           (FIC_BUF_LENGTH / 2)
#define MSC_DATA_SIZE           (CH0_BUF_LENGTH / 2)
#define DM_DATA_SIZE            188
#define NV_DATA_SIZE            (CH3_BUF_LENGTH / 2)

#ifdef FEATURE_FIT_FRAME_SIZE
u32 audio_frame_size;
u32 remain_audio_frame;
u32 remain_audio_offset;
#endif

// TS service information
typedef struct _TS_FRAME_INFO
{
  u8  ind;
  u16 length;   // current receiving length
  u8  subch_id; // sub ch id
  u8* buffer;
  } TS_FRAME_INFO;

// TS frame header information
typedef struct _TS_FRAME_HDR_
{
  u8  sync;
  u8  ind;
  u16 length;
  u8* data;
  } TS_FRAME_HDR;

/*============================================================
**  2.   Variables
*============================================================*/
#if 0
static int (*pFicCallback)(u32 userdata, u8 *data, int length) = NULL;
static int (*pMscCallback)(u32 userdata, u8 subChId, u8 *data, int length) = NULL;
static u32 gFicUserData = 0, gMscUserData = 0;
#else
extern int (*pFicCallback)(u32 userdata, u8 *data, int length);
extern int (*pMscCallback)(u32 userdata, u8 subchid, u8 *data, int length);

extern u32 gFicUserData;
extern u32 gMscUserData;
#endif

extern ts_data_type ts_data;

extern void fc8050_get_demux_buffer(u8 **res_ptr, u32 *res_size);

static u8 sync_error_cnt = 0;

#ifdef FC8050_USE_TSIF
#define FEATURE_TDMB_DAB_BUF
#endif

#ifdef FEATURE_TDMB_DAB_BUF
#ifdef FC8050_USE_TSIF
extern tdmb_bb_service_type serviceType;

u8 dab_buf[BB_MAX_DATA_SIZE];
#endif
#endif

#if 1
u8 bTSVideo[2][188*80];
u8 bTSFic[188*64];
u8 bTSNVideo[2][188*80];
#else
u8 bTSVideo[2][188*64];
u8 bTSFic[188*64];
u8 bTSNVideo[2][188*64];
#endif

u8 g_vd_channel[2] = {
  0xff, 0xff
};
u8 g_nv_channel[] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff
};
TS_FRAME_INFO sTSVideo[2] = {
  {
    0, 0, 0xff, (u8*) bTSVideo[0]
  },
  {
    0, 0, 0xff, (u8*) bTSVideo[1]
  }
};
TS_FRAME_INFO sTSFic = {
  0, 0, 0xff, (u8*) &bTSFic[0]
};

TS_FRAME_INFO sTSNVideo[8] = {
  {0, 0, 0xff, NULL},
  {0, 0, 0xff, NULL},
  {0, 0, 0xff, (u8*) bTSNVideo[0]},
  {0, 0, 0xff, (u8*) bTSNVideo[1]},
  {0, 0, 0xff, NULL},
  {0, 0, 0xff, NULL},
  {0, 0, 0xff, NULL},
  {0, 0, 0xff, NULL}
};

//#define FEATURE_DMB_DATA_DUMP
#ifdef FEATURE_DMB_DATA_DUMP
#define FILENAME_PRE_DEMUX "/data/misc/dmb/pre_demux.txt"
#define FILENAME_POST_DEMUX "/data/misc/dmb/post_demux.txt"
#endif

/*============================================================
**  3.   Function Prototype
*============================================================*/
#ifdef FC8050_USE_TSIF
int ts_fic_gather(u8* data, u32 length, u8* fic_data, u32* fic_size)
#else
int ts_fic_gather(u8* data, u32 length)
#endif
{
  u16 len;
  TS_FRAME_HDR header;

  header.sync  = data[0];
  header.ind   = data[1];
  header.length = (data[2] << 8) | data[3];
  header.data  = &data[4];

  // current real length
  len = (header.length > 184) ? 184 : header.length;
  
  //TDMB_MSG_FCI_BB("[%s] header.length[%d], sTSFic.length[%d], header.ind[%d]\n", __func__, header.length, sTSFic.length, header.ind);

  if(header.ind == PKT_IND_START) // fic start frame
  {
    // discard already data if exist, receive new fic frame
    sTSFic.ind = header.ind;
    sTSFic.length = 0;
    memcpy((void*)&sTSFic.buffer[sTSFic.length], header.data, len);
    sTSFic.length = len;
    return BBM_OK;
  }
  else if(header.ind == PKT_IND_CONTINUE) // fic continue frame
  {
    if(sTSFic.ind != PKT_IND_START)
    {
      // discard already data & current receiving data
      sTSFic.ind = PKT_IND_NONE;
    }
    else
    {
      // store
      sTSFic.ind = header.ind;
      memcpy((void*)&sTSFic.buffer[sTSFic.length], header.data, len);
      sTSFic.length += len;
    }
    return BBM_OK;
  }
  else if(header.ind == PKT_IND_END) // fic end frame
  {
    if(sTSFic.ind != PKT_IND_CONTINUE)
    {
      // discard alread data & current receiving data
      sTSFic.ind = PKT_IND_NONE;
      return BBM_E_MUX_INDICATOR;
    }
    else
    {
      // store
      sTSFic.ind = header.ind;
      memcpy((void*)&sTSFic.buffer[sTSFic.length], header.data, len);
      sTSFic.length += len;
    }
  }

  // send host application
  if((sTSFic.length >= FIC_DATA_SIZE) && (sTSFic.ind == PKT_IND_END))
  {
#ifdef FC8050_USE_TSIF
    *fic_size = sTSFic.length;
#else
    if(pFicCallback)
      (*pFicCallback)(gFicUserData, sTSFic.buffer, sTSFic.length);
#endif
    sTSFic.length = 0;
    sTSFic.ind = PKT_IND_NONE;
  }

  return BBM_OK;
}

#ifdef FC8050_USE_TSIF
int ts_nv_gather(u8* data, u32 length, u8* msc_data, u32* msc_size)
#else
int ts_nv_gather(u8* data, u32 length)
#endif
{
  TS_FRAME_HDR header;
  u32 len;
  u8  subch;
  u8  ch;

  header.sync   = data[0];
  header.ind    = data[1];
  header.length = (data[2] << 8) | data[3];
  header.data   = &data[4];

  len = (header.length >= 184) ? 184 : header.length;

  subch = header.sync & 0x3f;
  ch = g_nv_channel[subch];
  
  //TDMB_MSG_FCI_BB("[%s] ch[%d], header.length[%d], sTSNVideo[ch].length[%d], header.ind[%d]\n", __func__, ch, header.length, sTSNVideo[ch].length, header.ind);

  if(ch == 0xff)
    return BBM_E_MUX_SUBCHANNEL;

  if(header.ind == PKT_IND_START)
  {
#ifdef FEATURE_FIT_FRAME_SIZE  
    sTSNVideo[ch].length   = remain_audio_frame;

    if(remain_audio_frame>0)
      memcpy((void*)&sTSNVideo[ch].buffer[0], (void*)&sTSNVideo[ch].buffer[remain_audio_offset], remain_audio_frame);
	
    memcpy((void*)&sTSNVideo[ch].buffer[remain_audio_frame], header.data, len);

#else
    sTSNVideo[ch].length   = 0;
    memcpy((void*)&sTSNVideo[ch].buffer[0], header.data, len);
#endif

    sTSNVideo[ch].length += len;
    if(sTSNVideo[ch].length < header.length)
      sTSNVideo[ch].ind = header.ind;
    else
      sTSNVideo[ch].ind = PKT_IND_END;
  }
  else if(header.ind == PKT_IND_CONTINUE)
  {
    memcpy((void*)&sTSNVideo[ch].buffer[sTSNVideo[ch].length], header.data, len);
    sTSNVideo[ch].length += len;
    sTSNVideo[ch].ind     = header.ind;
    return BBM_OK;
  }
  else if(header.ind == PKT_IND_END)
  {
    memcpy((void*)&sTSNVideo[ch].buffer[sTSNVideo[ch].length], header.data, len);
    sTSNVideo[ch].length += len;
    sTSNVideo[ch].ind     = header.ind;
  }

  if((header.ind == PKT_IND_END) || (header.ind == PKT_IND_START && 184 >= header.length))
  {
#ifdef FC8050_USE_TSIF
#ifdef FEATURE_FIT_FRAME_SIZE
    *msc_size = remain_audio_offset = (sTSNVideo[ch].length/audio_frame_size)*audio_frame_size;
    remain_audio_frame = sTSNVideo[ch].length - *msc_size;
#else
    *msc_size = sTSNVideo[ch].length;
#endif
    
#else
    if(pMscCallback)
      (*pMscCallback)(gMscUserData, subch, sTSNVideo[ch].buffer, sTSNVideo[ch].length);
#endif

    sTSNVideo[ch].length = 0;
    sTSNVideo[ch].ind    = PKT_IND_NONE;
  }

  return BBM_OK;
}

int ts_dmb_gather(u8* data, u32 length) {
  u8 ch;
  
  //TDMB_MSG_FCI_BB("[%s] data0[0x%x] \n", __func__, data[0]);

  // trace sync
  if(data[0] == SYNC_MASK_VIDEO)
  {
    ch = 0;
  }
  else if(data[0] == SYNC_MASK_VIDEO1)
  {
    ch = 1;
    data[0] = 0x47;
  }
  else
  {
    return BBM_E_MUX_DATA_MASK;
  }

#ifdef FC8050_USE_TSIF
  return BBM_OK;
#else
  if(g_vd_channel[ch] == 0xff)
    return BBM_E_MUX_SUBCHANNEL;

#if 1
  if(pMscCallback)
    (*pMscCallback)(gMscUserData, g_vd_channel[ch], data, length);
#else
  memcpy((void*)(sTSVideo[ch].buffer + sTSVideo[ch].length), data, 188);
  sTSVideo[ch].length += length;

  if(sTSVideo[ch].length >= MSC_DATA_SIZE)
  {
    if(pMscCallback)
      (*pMscCallback)(gMscUserData, g_vd_channel[ch], sTSVideo[ch].buffer, MSC_DATA_SIZE);
    sTSVideo[ch].length = 0;
  }
#endif
  return BBM_OK;
#endif /* FC8050_USE_TSIF */
}

void fc8050_demux_init(void)
{
  sTSVideo->ind = 0;
  sTSVideo->length = 0;
  sTSVideo->subch_id = 0xff;

  sTSNVideo->ind = 0;
  sTSNVideo->length = 0;
  sTSNVideo->subch_id = 0xff;

  sTSFic.ind = 0;
  sTSFic.length = 0;
  sTSFic.subch_id = 0xff;
}

#ifdef FC8050_USE_TSIF
int fc8050_demux(u8* data_buf, u32 length)
#else
int fc8050_demux(u8* data, u32 length, u8** res_ptr, u32* res_size)
#endif
{
  int res = BBM_OK;
  u32 i;//, pos;
  u8  sync_error = 0;
  int is_video_data = FALSE;
  //int fic_len = 0;
#ifdef FC8050_USE_TSIF
  int msc_idx = 0;
  int dab_len = 0;
#endif

#ifdef FEATURE_TDMB_DAB_BUF
  u8* data = NULL;

  if(serviceType == TDMB_BB_SVC_DAB)
  {
    memcpy(dab_buf, data_buf, length);
    data = dab_buf;
  }
  else
  {
    data = data_buf;
  }
#endif

#ifdef FEATURE_DMB_DATA_DUMP
  dmb_data_dump(data, length, FILENAME_PRE_DEMUX);
#endif

  for(i = 0; i < length; i += 188)
  {
    if(data[i] == SYNC_MASK_FIC)
    {
#ifdef FC8050_USE_TSIF
      ts_data.fic_size = 0;

      res = ts_fic_gather(&data[i], 188, ts_data.fic_buf, &ts_data.fic_size);

      if(ts_data.fic_size)
      {
        // FIC데이터는 384 만큼만 한다.
        memcpy((void*)ts_data.fic_buf, sTSFic.buffer, ts_data.fic_size);
      }
#else
      res = ts_fic_gather(&data[i], 188);
#endif
    }
    else if((data[i] == SYNC_MASK_VIDEO) || (data[i] == SYNC_MASK_VIDEO1))
    {
      is_video_data = TRUE;
      
      res = ts_dmb_gather(&data[i], 188);

#ifdef FC8050_USE_TSIF
      if(msc_idx != i)
      {
        memcpy(&data[msc_idx * 188], &data[i], 188);
      }
      msc_idx ++;
#endif
    }
    else if((data[i] & 0xC0) == 0xC0)
    {
#ifdef FC8050_USE_TSIF
      ts_data.msc_size = 0;

      res = ts_nv_gather(&data[i], 188, ts_data.msc_buf, &ts_data.msc_size);

      if(ts_data.msc_size)
      {
        memcpy((void*)&ts_data.msc_buf[dab_len], sTSNVideo[3].buffer, ts_data.msc_size);
        dab_len += ts_data.msc_size;
      }
#else
      res = ts_nv_gather(&data[i], 188);
#endif
    }
    else
    {
      //PRINTF(" %02X", data[i]);
      //TDMB_MSG_FCI_BB("[%s] sync_error \n", __func__);
      sync_error++;
    }
  }

#ifdef FC8050_USE_TSIF
  if(is_video_data)
  {
    ts_data.msc_size = msc_idx * 188;
  }
  else
  {
    ts_data.msc_size = dab_len;
  }

#ifdef FEATURE_DMB_DATA_DUMP
  dmb_data_dump(ts_data.msc_buf, ts_data.msc_size, FILENAME_POST_DEMUX);
#endif

  //TDMB_MSG_FCI_BB("[%s] msc_size[%d], fic_size[%d], length[%d]\n", __func__, ts_data.msc_size, ts_data.fic_size, length);

  if(sync_error)
  {
    TDMB_MSG_FCI_BB("[%s] sync_error cnt[%d]\n", __func__, sync_error);
  }

  if(ts_data.msc_size)
  {
    ts_data.type = MSC_DATA;

    if(ts_data.fic_size)
    {
      ts_data.type = FIC_MSC_DATA;
    }
  }
  else if(ts_data.fic_size)
  {
    ts_data.type = FIC_DATA;
  }
  sync_error_cnt = 0;

  return ts_data.msc_size;
#else
  if(sync_error > 0)
  {
    sync_error_cnt += sync_error;
    if(sync_error_cnt >= 5)
      return BBM_E_MUX_SYNC;
  }
  else
  {
    sync_error_cnt = 0;
  }

  fc8050_get_demux_buffer(res_ptr, res_size);

  return res;
#endif
}

int fc8050_demux_fic_callback_register(u32 userdata, int (*callback)(u32 userdata, u8 *data, int length)) {
  gFicUserData = userdata;
  pFicCallback = callback;
  return BBM_OK;
}

int fc8050_demux_msc_callback_register(u32 userdata, int (*callback)(u32 userdata, u8 subChId, u8 *data, int length)) {
  gMscUserData = userdata;
  pMscCallback = callback;
  return BBM_OK;
}

int fc8050_demux_select_video(u8 subChId, u8 cdiId) {
  g_vd_channel[cdiId] = subChId;
  return BBM_OK;
}

int fc8050_demux_select_channel(u8 subChId, u8 svcChId) {
  g_nv_channel[subChId] = svcChId;
  return BBM_OK;
}

int fc8050_demux_deselect_video(u8 subChId, u8 cdiId) {
  if(g_vd_channel[cdiId] == subChId)
    g_vd_channel[cdiId] = 0xff;
  return BBM_OK;
}

int fc8050_demux_deselect_channel(u8 subChId, u8 svcChId) {
  g_nv_channel[subChId] = 0xff;
  return BBM_OK;
}
