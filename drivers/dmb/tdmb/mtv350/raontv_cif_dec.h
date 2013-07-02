
#ifndef __RAONTV_CIF_DEC_H__
#define __RAONTV_CIF_DEC_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "raontv.h"

typedef struct
{
#if defined(RTV_IF_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE)
	unsigned int fic_size; /* Result size. */
	unsigned char *fic_buf_ptr; /* Destination buffer address. */
#endif	

	unsigned int msc_size[RTV_NUM_DAB_AVD_SERVICE];  /* Result size. */
	unsigned int msc_subch_id[RTV_NUM_DAB_AVD_SERVICE]; /* Result sub channel ID. */	
	unsigned char *msc_buf_ptr[RTV_NUM_DAB_AVD_SERVICE]; /* Destination buffer address. */
} RTV_CIF_DEC_INFO;

void rtvCIFDEC_Decode(RTV_CIF_DEC_INFO *ptDecInfo, UINT nMscBufSize,
						const unsigned char *pbTsBuf, unsigned int nTsLen);
void rtvCIFDEC_DeleteSubChannelID(unsigned int nSubChID);
BOOL rtvCIFDEC_AddSubChannelID(unsigned int nSubChID, E_RTV_SERVICE_TYPE eServiceType);
void rtvCIFDEC_Deinit(void);
void rtvCIFDEC_Init(void);



#ifdef __cplusplus
} 
#endif 

#endif /* __RAONTV_CIF_DEC_H__ */


