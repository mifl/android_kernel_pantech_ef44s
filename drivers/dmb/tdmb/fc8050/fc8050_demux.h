/*****************************************************************************
 Copyright(c) 2009 FCI Inc. All Rights Reserved
 
 File name : fc8050_demux.h
 
 Description : baseband header file
 
 History : 
 ----------------------------------------------------------------------

*******************************************************************************/

#ifndef __FC8050_DEMUX_H__
#define __FC8050_DEMUX_H__

#include "fci_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void fc8050_demux_init(void);
#ifdef FC8050_USE_TSIF
int fc8050_demux(u8* data_buf, u32 length);
#else
int fc8050_demux(u8* data, u32 length, u8** res_ptr, u32* res_size);
#endif
int fc8050_demux_fic_callback_register(u32 userdata, int (*callback)(u32 userdata, u8 *data, int length));
int fc8050_demux_msc_callback_register(u32 userdata, int (*callback)(u32 userdata, u8 subChId, u8 *data, int length));
int fc8050_demux_select_video(u8 subChID, u8 cdiId);
int fc8050_demux_select_channel(u8 subChId, u8 svcChId);
int fc8050_demux_deselect_video(u8 subChId, u8 cdiId);
int fc8050_demux_deselect_channel(u8 subChId, u8 svcChId);

#ifdef __cplusplus
}
#endif

#endif // __FC8050_DEMUX_H__
