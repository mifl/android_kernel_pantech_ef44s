/******************************************************************************** 
* (c) COPYRIGHT 2010 RAONTECH, Inc. ALL RIGHTS RESERVED.
* 
* This software is the property of RAONTECH and is furnished under license by RAONTECH.                
* This software may be used only in accordance with the terms of said license.                         
* This copyright noitce may not be remoced, modified or obliterated without the prior                  
* written permission of RAONTECH, Inc.                                                                 
*                                                                                                      
* This software may not be copied, transmitted, provided to or otherwise made available                
* to any other person, company, corporation or other entity except as specified in the                 
* terms of said license.                                                                               
*                                                                                                      
* No right, title, ownership or other interest in the software is hereby granted or transferred.       
*                                                                                                      
* The information contained herein is subject to change without notice and should 
* not be construed as a commitment by RAONTECH, Inc.                                                                    
* 
* TITLE 	  : RAONTECH TV CIF Decoder source file for T-DMB and DAB. 
*
* FILENAME    : raontv_cif_dec.c
*
* DESCRIPTION : 
*		Library of routines to initialize, and operate on, the RAONTECH T-DMB demod.
*
********************************************************************************/

/******************************************************************************** 
* REVISION HISTORY
*
*    DATE	  	  NAME				REMARKS
* ----------  -------------    --------------------------------------------------
* 09/27/2010  Ko, Kevin        Created.
********************************************************************************/

#include "raontv_cif_dec.h"
#ifdef RTV_CIF_MODE_ENABLED
#include "mtv350_bb.h"
//#include "raontv.h"

#if 1
	#if defined(__KERNEL__) /* Linux kernel */
		#include <linux/kernel.h>
		//#include <linux/string.h>
	#else
		#include <stdio.h>
		#include <string.h>
	#endif

	#define CIF_DBGMSG0(fmt, arg...)  TDMB_MSG_RTV_BB(fmt, ##arg)
	#define CIF_DBGMSG1(fmt, arg...)  TDMB_MSG_RTV_BB(fmt, ##arg)
	#define CIF_DBGMSG2(fmt, arg...)  TDMB_MSG_RTV_BB(fmt, ##arg)
	#define CIF_DBGMSG3(fmt, arg...)  TDMB_MSG_RTV_BB(fmt, ##arg)
#else
	/* To eliminates the debug messages. */
	#define CIF_DBGMSG0(fmt)					((void)0)
	#define CIF_DBGMSG1(fmt, arg1)				((void)0)
	#define CIF_DBGMSG2(fmt, arg1, arg2)		((void)0)
	#define CIF_DBGMSG3(fmt, arg1, arg2, arg3)	((void)0)
#endif


#ifdef RTV_BUILD_CIFDEC_WITH_DRIVER /* Decode source place at kernel */
	#if defined(__KERNEL__)
		#include <linux/mutex.h>
		static struct mutex cif_mutex;
		#define CIF_MUTEX_INIT		mutex_init(&cif_mutex)
		#define CIF_MUTEX_LOCK		mutex_lock(&cif_mutex)
		#define CIF_MUTEX_UNLOCK	mutex_unlock(&cif_mutex)
		#define CIF_MUTEX_DEINIT 	((void)0)
	#elif defined(WINCE)
		static CRITICAL_SECTION cif_mutex;
		#define CIF_MUTEX_INIT		InitializeCriticalSection(&cif_mutex)
		#define CIF_MUTEX_LOCK		EnterCriticalSection(&cif_mutex)
		#define CIF_MUTEX_UNLOCK	LeaveCriticalSection(&cif_mutex)
		#define CIF_MUTEX_DEINIT	DeleteCriticalSection(&cif_mutex)
	#else
		#error "Code not present" // TODO
	#endif
#else
	#if defined(__linux__)
		#include <pthread.h>
		static pthread_mutex_t cif_mutex;
		#define CIF_MUTEX_INIT		pthread_mutex_init(&cif_mutex)
		#define CIF_MUTEX_LOCK		pthread_mutex_lock(&cif_mutex)
		#define CIF_MUTEX_UNLOCK	pthread_mutex_unlock(&cif_mutex)
		#define CIF_MUTEX_DEINIT 	((void)0)
	#elif defined(WIN32)
		#include <Windows.h>
		static CRITICAL_SECTION cif_mutex;
		#define CIF_MUTEX_INIT		InitializeCriticalSection(&cif_mutex)
		#define CIF_MUTEX_LOCK		EnterCriticalSection(&cif_mutex)
		#define CIF_MUTEX_UNLOCK	LeaveCriticalSection(&cif_mutex)
		#define CIF_MUTEX_DEINIT	DeleteCriticalSection(&cif_mutex)
	#else
		#error "Code not present"
		// temp: TODO
		#define CIF_MUTEX_INIT		((void)0)
		#define CIF_MUTEX_LOCK		((void)0)
		#define CIF_MUTEX_UNLOCK 	((void)0)
		#define CIF_MUTEX_DEINIT 	((void)0)
	#endif
#endif

#if defined(__KERNEL__) && defined(RTV_CIF_LINUX_USER_SPACE_COPY_USED)
	/* File dump enable */
	//#define _MSC_KERNEL_FILE_DUMP_ENABLE
#ifdef _MSC_KERNEL_FILE_DUMP_ENABLE
	#include <linux/fs.h>

	/* MSC filename: /data/dmb_msc_SUBCH_ID.ts */
	#define MSC_DEC_FNAME_PREFIX	"/data/dmb_msc"

	static struct file *g_ptDecMscFilp[64];
	static UINT g_nKfileDecSubchID;

	static void cif_kfile_write(const char __user *buf, size_t len)
	{
		mm_segment_t oldfs;
		struct file *fp_msc;

		if (g_nKfileDecSubchID != 0xFFFF) {
			fp_msc = g_ptDecMscFilp[g_nKfileDecSubchID];
			oldfs = get_fs();
			set_fs(KERNEL_DS);
			ret = fp_msc->f_op->write(fp_msc, buf, len, &fp_msc->f_pos);
			set_fs(oldfs);
		}
	}

	static INLINE void __cif_kfile_close(UINT subch_id)
	{
		if (g_ptDecMscFilp[subch_id]) {
			filp_close(g_ptDecMscFilp[subch_id], NULL);
			g_ptDecMscFilp[subch_id] = NULL;
		}
	}

	static void cif_kfile_close(UINT subch_id)
	{
		unsigned int i;

		if (subch_id != 0xFFFF)
			__cif_kfile_close(subch_id);
		else {
			for (i = 0; i < 64; i++)
				__cif_kfile_close(i);
		}
	}

	static int cif_kfile_open(UINT subch_id)
	{
		char fname[64];
		struct file *fp_msc;

		if(g_ptDecMscFilp[subch_id] == NULL) {
			 sprintf(fname, "%s_%u.ts", MSC_DEC_FNAME_PREFIX, subch_id);
			 fp_msc = filp_open(fname, O_WRONLY, S_IRUSR);
			 if(fp_msc == NULL) {
				 printk("[cif_kfile_open] File open error: %s!\n", fname);
				 return -1;
			 }
		
			 g_ptDecMscFilp[subch_id] = fp_msc;
			 printk("[cif_kfile_open] File opened: %s\n", fname);
		}
		else
			printk("[cif_kfile_open] Already opened!\n");

		return 0;
	}

	#define CIF_KFILE_CLOSE(subch_id)			cif_kfile_close(subch_id)
	#define CIF_KFILE_OPEN(subch_id)			cif_kfile_open(subch_id)
	#define CIF_KFILE_WRITE(buf, size)			cif_kfile_write(buf, size)
	#define CIF_MSC_KFILE_WRITE_DIS				g_nKfileDecSubchID = 0xFFFF;
	#define CIF_MSC_KFILE_WRITE_EN(subch_id)	g_nKfileDecSubchID = subch_id;	
#else
	#define CIF_KFILE_CLOSE(subch_id)			((void)0)
	#define CIF_KFILE_OPEN(subch_id)			((void)0)
	#define CIF_KFILE_WRITE(buf, size)			((void)0)
	#define CIF_MSC_KFILE_WRITE_DIS 			((void)0)
	#define CIF_MSC_KFILE_WRITE_EN(subch_id)	((void)0)
#endif

	#include <asm/uaccess.h>

	#define CIF_DATA_COPY(ret, dst_ptr, src_ptr, size)	\
	do {	\
		CIF_KFILE_WRITE(src_ptr, size);\
		if(copy_to_user((void __user *)dst_ptr, src_ptr, size)) {\
			CIF_DBGMSG1("[CIF: %d] copy_to_user error\n", __LINE__);\
			ret = -EFAULT;\
		}	\
	} while(0)
#else
	#include <linux/string.h>
	#define CIF_KFILE_CLOSE(subch_id)			((void)0)
	#define CIF_KFILE_OPEN(subch_id)			((void)0)
	#define CIF_KFILE_WRITE(buf, size)			((void)0)
	#define CIF_MSC_KFILE_WRITE_DIS 			((void)0)
	#define CIF_MSC_KFILE_WRITE_EN(subch_id)	((void)0)

	#define CIF_DATA_COPY(ret, dst_ptr, src_ptr, size)	\
							memcpy(dst_ptr, src_ptr, size);
#endif

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)  
	#define RTV_CIF_HEADER_SIZE    4 /* bytes */ 
#else            
	#define RTV_CIF_HEADER_SIZE    16 /* bytes */ 
#endif 

#define DIV32(x)		((x) >> 5) // Divide by 32
#define MOD32(x)        ((x) & 31)
static U32 g_aAddedSubChIdBits[2]; /* Used sub channel ID bits. [0]: 0 ~ 31, [1]: 32 ~ 63 */

static UINT g_nOutDecBufIdxBits; /* Decoded out buffer index bits */
static UINT g_aOutDecBufIdx[64]; /* Decoded out buffer index for sub ch */

static E_RTV_SERVICE_TYPE g_eaSvcType[64];

static BOOL g_fCifInited = FALSE;
static UINT g_nFicIC;

#if defined(RTV_IF_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE)
typedef enum
{
	PARSER_STATUS__OK = 0,
	PARSER_STATUS__SHORT_PKT_0,
	PARSER_STATUS__SHORT_PKT_1,
	PARSER_STATUS__NONE_SC,
	PARSER_STATUS__INVALID_SC,
	PARSER_STATUS__INVALID_HDR,
	PARSER_STATUS__UNKNOWN_ERR
} PARSER_STATUS_TYPE;

typedef enum
{
	TSI_INVALID_HDR_0 = 0,
	TSI_INVALID_HDR_1,
	TSI_INVALID_HDR_2,
	TSI_INVALID_HDR_3,
	TSI_INVALID_HDR_4,
	TSI_INVALID_HDR_5,
	MAX_NUM_TSI_INVALID_HDR
} TSI_INVALID_HDR_TYPE;

typedef struct
{
	UINT tsi_cnt;
	UINT cif_size; // (FIC+MSC0+MSC1 SIZE)	
	UINT pkt_size; // (TSI_CIF_MODE_HEADER + cif_size + padding_bytes)
	
	UINT fic_size;

	UINT subch_id[RTV_NUM_DAB_AVD_SERVICE];
	UINT subch_size[RTV_NUM_DAB_AVD_SERVICE];
	UINT sum_subch_size;

	UINT padding_bytes;
} TSI_CIF_HEADER_INFO;

typedef struct
{
	UINT prev_len;
	const U8 *prev_data;

	UINT curr_len;
	const U8 *curr_data;
} TSI_CIF_BUF_INFO;

TSI_CIF_BUF_INFO g_tCifBuf;
static BOOL g_fUsePrevDecodedHeader;

typedef struct
{
	UINT len;
	U8 data[188];
} TSI_CIF_MSC_FRAG_INFO;
static TSI_CIF_MSC_FRAG_INFO g_atCifMscFrag[RTV_NUM_DAB_AVD_SERVICE];

#define TSIF_COLLECT_BUF_SIZE	(2 * (384 + RTV_NUM_DAB_AVD_SERVICE*6912/*1 CIF size*/))
static U8 temp_ts_buf_pool[TSIF_COLLECT_BUF_SIZE];
static U8 *g_pbPrevBufAllocPtr; // allocation pointer.(Fixed) Used to free.

static UINT g_nTsiCounter;
static ULONG g_nNumTsiDiscontinuity;
static ULONG g_nNumFicSizeNoData;

#if (RTV_NUM_DAB_AVD_SERVICE == 1)
static UINT g_nSingleSubchID;
#endif

#define _DEBUG_TSIF_CIFDEC_INVAL_HDR_STATIC /* Selection */
#ifdef _DEBUG_TSIF_CIFDEC_INVAL_HDR_STATIC
	/* TSIF CIF Invalid Header statistics */
	static unsigned long g_aTsiInvaHdrStat[MAX_NUM_TSI_INVALID_HDR];
	#define TSI_INVAL_HDR_STAT_INC(invalid_hdr_type)\
					g_aTsiInvaHdrStat[invalid_hdr_type]++;

	#define TSI_INVAL_HDR_STAT_RST(invalid_hdr_type)\
					g_aTsiInvaHdrStat[invalid_hdr_type] = 0
	#define TSI_INVAL_HDR_STAT_RST_ALL\
			memset(g_aTsiInvaHdrStat, 0, sizeof(g_aTsiInvaHdrStat))
	#define TSI_INVAL_HDR_STAT_SHOW\
	do {\
		CIF_DBGMSG0("|----------- TSIF CIF decode Statistics -----------\n");\
		CIF_DBGMSG3("| IH_0(%ld), IH_1(%ld), IH_2(%ld)\n",\
			g_aTsiInvaHdrStat[0], g_aTsiInvaHdrStat[1], g_aTsiInvaHdrStat[2]);\
		CIF_DBGMSG3("| IH_3(%ld), IH_4(%ld), IH_5(%ld)\n",\
			g_aTsiInvaHdrStat[3], g_aTsiInvaHdrStat[4], g_aTsiInvaHdrStat[5]);\
		CIF_DBGMSG3("| TSI_DC(%ld), FIC_ND(%ld), TOT_SCID(%ld) \n",\
			g_nNumTsiDiscontinuity, g_nNumFicSizeNoData, subch_id_pass_num);\
		CIF_DBGMSG0("|--------------------------------------------------\n");\
	} while(0)

	static unsigned long subch_id_pass_num;
#else
	#define TSI_INVAL_HDR_STAT_INC(invalid_hdr_type)	((void)0)
	#define TSI_INVAL_HDR_STAT_RST(invalid_hdr_type)	((void)0)
	#define TSI_INVAL_HDR_STAT_RST_ALL					((void)0)
	#define TSI_INVAL_HDR_STAT_SHOW						((void)0)
#endif	

static PARSER_STATUS_TYPE proc_invalid_header(TSI_INVALID_HDR_TYPE inval_hdr_type)
{
	TSI_INVAL_HDR_STAT_INC(inval_hdr_type);
	return PARSER_STATUS__INVALID_HDR;
}

static PARSER_STATUS_TYPE verify_header(TSI_CIF_HEADER_INFO *hdr)
{
	/* Check if the size of CIF_SIZE is zero ? */
	if (!hdr->cif_size)
		return proc_invalid_header(TSI_INVALID_HDR_0);

	/* Check if the size of CIF_SIZE is over than max value ? */
	if (hdr->cif_size > (384/*FIC*/ + (RTV_NUM_DAB_AVD_SERVICE*4096)/*MSC*/))
		return proc_invalid_header(TSI_INVALID_HDR_1);

	/* Check if the size of FIC_SIZE is valid or not. */
	switch (hdr->fic_size) {
	case 0: /* Can be zero when FIC not exist in this CIF packet. */
		break;

	case 384: case 96: case 128: case 192:
		break;
	default:
		return proc_invalid_header(TSI_INVALID_HDR_2);
	}

#if (RTV_NUM_DAB_AVD_SERVICE == 1)
	if (!hdr->sum_subch_size) {
		if (hdr->cif_size != hdr->fic_size) {
			hdr->sum_subch_size = hdr->subch_size[0] = hdr->cif_size - hdr->fic_size;
			TSI_INVAL_HDR_STAT_INC(TSI_INVALID_HDR_5);
		}
	}
#else
#endif

	if (hdr->cif_size != (hdr->fic_size + hdr->sum_subch_size)) {
		if (hdr->sum_subch_size) {
			if (hdr->cif_size & 3)
				hdr->sum_subch_size = hdr->subch_size[0] = hdr->cif_size - hdr->fic_size;
			else
				return proc_invalid_header(TSI_INVALID_HDR_3);
		}
		else /* FIC */
			return proc_invalid_header(TSI_INVALID_HDR_3);
	}

	if (hdr->sum_subch_size) {
#if (RTV_NUM_DAB_AVD_SERVICE == 1)
		if ((g_aAddedSubChIdBits[DIV32(hdr->subch_id[0])]
				& (1<<MOD32(hdr->subch_id[0]))) == 0) {
			TSI_INVAL_HDR_STAT_INC(TSI_INVALID_HDR_4);
			hdr->subch_id[0] = g_nSingleSubchID;
			//return proc_invalid_header(TSI_INVALID_HDR_4);
		}
#else
		UINT i;
		for (i = 0; i < RTV_NUM_DAB_AVD_SERVICE; i++) {
			if (hdr->subch_size[i] != 0) {
				if ((g_aAddedSubChIdBits[DIV32(hdr->subch_id[i])]
					& (1<<MOD32(hdr->subch_id[i]))) == 0)
					return proc_invalid_header(TSI_INVALID_HDR_4);
			}
		}
#endif
	}

	return PARSER_STATUS__OK;
}


static PARSER_STATUS_TYPE parsing_header(TSI_CIF_HEADER_INFO *hdr,
										const U8 *hdr_buf_ptr)
{
	hdr->tsi_cnt = (hdr_buf_ptr[2]<<8) | hdr_buf_ptr[3];
	hdr->cif_size = ((hdr_buf_ptr[4]<<8) | hdr_buf_ptr[5]);
	hdr->fic_size = ((hdr_buf_ptr[6]&0x03)<<8) | hdr_buf_ptr[7];

	hdr->subch_id[0] = hdr_buf_ptr[8] >> 2;
	hdr->subch_size[0] = (((hdr_buf_ptr[8]&0x03)<<8) | hdr_buf_ptr[9]) * 4;
	hdr->sum_subch_size = hdr->subch_size[0];

#if (RTV_NUM_DAB_AVD_SERVICE >= 2)
	hdr->subch_id[1] = hdr_buf_ptr[10] >> 2;
	hdr->subch_size[1] = (((hdr_buf_ptr[10]&0x03)<<8) | hdr_buf_ptr[11]) * 4;
	hdr->sum_subch_size += hdr->subch_size[1];
#endif

#if (RTV_NUM_DAB_AVD_SERVICE >= 3)
	hdr->subch_id[2] = hdr_buf_ptr[12] >> 2;
	hdr->subch_size[2] = (((hdr_buf_ptr[12]&0x03)<<8) | hdr_buf_ptr[13]) * 4;	
	hdr->sum_subch_size += hdr->subch_size[2];
#endif

#if (RTV_NUM_DAB_AVD_SERVICE == 4)
	hdr->subch_id[3] = hdr_buf_ptr[14] >> 2;
	hdr->subch_size[3] = (((hdr_buf_ptr[14]&0x03)<<8) | hdr_buf_ptr[15]) * 4;
	hdr->sum_subch_size += hdr->subch_size[3];
#endif
	
	return verify_header(hdr);
}

static void align_cif_packet(TSI_CIF_HEADER_INFO *hdr)
{
	UINT mod;

	hdr->pkt_size = RTV_CIF_HEADER_SIZE + hdr->cif_size; 

	/* The number of 188 tsp is the transmitted size of TSIF. */
	mod = hdr->pkt_size % 188;
	if (mod) {
		hdr->padding_bytes = (188 - mod);
		hdr->pkt_size += hdr->padding_bytes; // next 188 align
	}
	else
		hdr->padding_bytes = 0;
}

static PARSER_STATUS_TYPE decode_header(TSI_CIF_HEADER_INFO *hdr, 
										const U8 *prev_data, UINT prev_len,
										const U8 *curr_data, UINT curr_len)
{
	UINT i, k, diff;
	U8 header_data[RTV_CIF_HEADER_SIZE];
	PARSER_STATUS_TYPE status;

	//ASSERT((prev_len + curr_len) >= RTV_CIF_HEADER_SIZE);

	if (prev_len == 0) {
		if ((curr_data[0] == 0xE2) && (curr_data[1] == 0xE2))
			status = parsing_header(hdr, curr_data);
		else
			return PARSER_STATUS__INVALID_SC;
	}
	else {
		if (prev_len >= RTV_CIF_HEADER_SIZE) {
			if((prev_data[0] == 0xE2) && (prev_data[1] == 0xE2))
				status = parsing_header(hdr, prev_data);
			else
				return PARSER_STATUS__INVALID_SC;
		}
		else {
			header_data[0] = prev_data[0];
			if (prev_len >= 2)
				header_data[1] = prev_data[1];
			else
				header_data[1] = curr_data[0];

			if ((header_data[0] == 0xE2) && (header_data[1] == 0xE2)) {
				diff = RTV_CIF_HEADER_SIZE - prev_len;

				for (i = 2; i < prev_len; i++)
					header_data[i] = prev_data[i];
				
				for (k = 0; k < diff; i++, k++)
					header_data[i] = curr_data[k];

				status = parsing_header(hdr, header_data);
			}
			else
				return PARSER_STATUS__INVALID_SC;
		}
	}

	if (status == PARSER_STATUS__OK)
		align_cif_packet(hdr);

	return status;
}

static void pull_packet(TSI_CIF_BUF_INFO *cifb, UINT size)
{
	UINT diff;

	if (cifb->prev_len == 0) {
		cifb->curr_data += size;
		cifb->curr_len -= size;
	}
	else {
		if (cifb->prev_len >= size) {
			cifb->prev_data += size;
			cifb->prev_len -= size;
		}
		else {
			diff = size - cifb->prev_len;
			cifb->curr_data += diff;
			cifb->curr_len -= diff;
			cifb->prev_len = 0; /* All used */
		}
	}
}

static BOOL copy_data(U8 *dst, TSI_CIF_BUF_INFO *cifb, UINT size, BOOL to_user)
{
	UINT copy_bytes;
	int cp_ret = 0;

	//ASSERT((cifb->prev_len + cifb->curr_len) >= size);

	if (size == 0)
		return TRUE;

	if (cifb->prev_len) { /* prev first */
		copy_bytes = MIN(cifb->prev_len, size);
		if (to_user == TRUE) {
			CIF_DATA_COPY(cp_ret, dst, cifb->prev_data, copy_bytes);
			if(cp_ret)
				return FALSE;
		}
		else
			memcpy(dst, cifb->prev_data, copy_bytes);

		dst += copy_bytes;

		/* Update buffer pointer and size. */
		cifb->prev_data += copy_bytes;
		cifb->prev_len -= copy_bytes;
		size -= copy_bytes;
	}

	if (size) {
		copy_bytes = MIN(cifb->curr_len, size);
		if (to_user == TRUE) {
			CIF_DATA_COPY(cp_ret, dst, cifb->curr_data, copy_bytes);
			if(cp_ret)
				return FALSE;
		}
		else
			memcpy(dst, cifb->curr_data, copy_bytes);

		cifb->curr_data += copy_bytes;
		cifb->curr_len -= copy_bytes;
	}

	return TRUE;
}

static INLINE void reset_fragment(void)
{
#if (RTV_NUM_DAB_AVD_SERVICE == 1)
	g_atCifMscFrag[0].len = 0;
#else
	UINT i;

	for (i = 0; i < RTV_NUM_DAB_AVD_SERVICE; i++)
		g_atCifMscFrag[i].len = 0;
#endif
}

static void reset_processing_data(void)
{
	g_fUsePrevDecodedHeader = FALSE;
	g_tCifBuf.prev_len = 0;
	reset_fragment();
}

static void collect_ts_data(TSI_CIF_BUF_INFO *cifb)
{
	if (cifb->curr_len) {
		if ((cifb->prev_len + cifb->curr_len) <= TSIF_COLLECT_BUF_SIZE) {
			/* Save for next processing. */
			if (cifb->prev_len)
				memmove((void *)g_pbPrevBufAllocPtr,
						cifb->prev_data, cifb->prev_len);

			memcpy((void *)(g_pbPrevBufAllocPtr + cifb->prev_len),
					cifb->curr_data, cifb->curr_len);
			cifb->prev_len += cifb->curr_len;
			cifb->curr_len = 0;

			/* Set the pointer of previous ts buffer to 
			the base pointer of allocation buffer. */
			cifb->prev_data = g_pbPrevBufAllocPtr;
		}
		else {
			CIF_DBGMSG2("[collect_ts_data] Too small buffer size (%u / %u)\n",
				cifb->prev_len + cifb->curr_len, TSIF_COLLECT_BUF_SIZE);
			reset_processing_data();
		}
	}
}

static INLINE BOOL verity_tsi_counter(UINT prev_tsi_cnt, UINT next_tsi_cnt)
{
	if (((prev_tsi_cnt + 1) & 0xFFFF) == next_tsi_cnt) {
		g_nTsiCounter = next_tsi_cnt; /* Update for next TSI counter */
		return TRUE;
	}
	else {
		g_nNumTsiDiscontinuity++;
		return FALSE;
	}
}

/* Case for the truncated CIF packet. */
static PARSER_STATUS_TYPE proc_truncated_packet(TSI_CIF_HEADER_INFO *hdr,
										const TSI_CIF_BUF_INFO *cifb,
										UINT pkt_offset, UINT *invalid_offset)
{
	UINT hold_msc_size;
	U8 fic_buf[3];
	const U8 *fic_ptr;
#if (RTV_NUM_DAB_AVD_SERVICE >= 2)
	UINT i;
#endif

	if (hdr->fic_size) {
		if (g_tCifBuf.prev_len == 0)
				fic_ptr = &cifb->curr_data[RTV_CIF_HEADER_SIZE];
		else {
			if (cifb->prev_len >= (RTV_CIF_HEADER_SIZE + 3))
				fic_ptr = &cifb->prev_data[RTV_CIF_HEADER_SIZE];
			else {
				fic_ptr = (const U8 *)fic_buf;
				if (cifb->prev_len == (RTV_CIF_HEADER_SIZE + 1)) {
					fic_buf[0] = cifb->prev_data[RTV_CIF_HEADER_SIZE];
					fic_buf[1] = cifb->curr_data[0];
					fic_buf[2] = cifb->curr_data[1];
				}
				else if (g_tCifBuf.prev_len == (RTV_CIF_HEADER_SIZE + 2)) {
					fic_buf[0] = cifb->prev_data[RTV_CIF_HEADER_SIZE];
					fic_buf[1] = cifb->prev_data[RTV_CIF_HEADER_SIZE + 1];
					fic_buf[2] = cifb->curr_data[0];
				}
				else { /* g_tCifBuf.prev_len <= RTV_CIF_HEADER_SIZE */
					UINT diff = RTV_CIF_HEADER_SIZE - cifb->prev_len;
					fic_buf[0] = cifb->curr_data[diff];
					fic_buf[1] = cifb->curr_data[diff + 1];
					fic_buf[2] = cifb->curr_data[diff + 2];
				}
			}
		}

		if ((fic_ptr[0] != 0x05) || (fic_ptr[1] != 0x00) || (fic_ptr[2] != 0xE0)) {
			if (hdr->sum_subch_size) { /* Not fic only */
				if (g_nFicIC == 0) {
					g_nFicIC = 1;
					RTV_GUARD_LOCK;
					RTV_REG_SET(0x03, 0x09);
					RTV_REG_SET(0x46, 0x1E);
					RTV_REG_SET(0x35, 0x01);
					RTV_REG_SET(0x46, 0x16);
					RTV_GUARD_FREE;
				}

				hdr->fic_size = 0;
				g_nNumFicSizeNoData++;
			}
		}

		/* Check if we received the completed FIC ? */
		if (pkt_offset < (RTV_CIF_HEADER_SIZE + hdr->fic_size)) {
			*invalid_offset = pkt_offset;
			return PARSER_STATUS__INVALID_HDR;
		}
	}

	if (hdr->sum_subch_size) {
		hold_msc_size = pkt_offset - (RTV_CIF_HEADER_SIZE + hdr->fic_size);
#if (RTV_NUM_DAB_AVD_SERVICE == 1)
		hdr->subch_size[0] = MIN(hdr->subch_size[0], hold_msc_size);
		hdr->sum_subch_size = hdr->subch_size[0];
#else
		hdr->sum_subch_size = 0;
		for (i = 0; i < RTV_NUM_DAB_AVD_SERVICE; i++) {
			if (hold_msc_size) {
				hdr->subch_size[i] = MIN(hdr->subch_size[i], hold_msc_size);
				hdr->sum_subch_size += hdr->subch_size[i];
				hold_msc_size -= hdr->subch_size[i];
			}
		}
#endif
	}

	hdr->cif_size = hdr->fic_size + hdr->sum_subch_size;
	hdr->pkt_size = pkt_offset;
	hdr->padding_bytes = pkt_offset - (RTV_CIF_HEADER_SIZE + hdr->cif_size);

	return PARSER_STATUS__OK;
}

static PARSER_STATUS_TYPE get_next_header(TSI_CIF_HEADER_INFO *hdr,
							TSI_CIF_HEADER_INFO *next_hdr,
							const TSI_CIF_BUF_INFO *cifb,
							UINT *invalid_offset)
{
	const U8 *prev_data, *curr_data;
	UINT prev_len, curr_len, pkt_offset, diff;
	PARSER_STATUS_TYPE status;
	BOOL tsi_ret;

	if ((cifb->prev_len + cifb->curr_len) >= (hdr->pkt_size + RTV_CIF_HEADER_SIZE)) {
		/* First, search the next header of packet with packet size. */
		if (cifb->prev_len <= hdr->pkt_size) {
			diff = hdr->pkt_size - cifb->prev_len;
			curr_len = cifb->curr_len - diff;
			curr_data = cifb->curr_data + diff;
			prev_len = 0;
			prev_data = NULL;
		}
		else {
			prev_len = cifb->prev_len - hdr->pkt_size;
			prev_data = cifb->prev_data + hdr->pkt_size;
			curr_len = cifb->curr_len;
			curr_data = cifb->curr_data;
		}

		status = decode_header(next_hdr, prev_data, prev_len, curr_data, curr_len);
		if (status == PARSER_STATUS__OK) {
			tsi_ret = verity_tsi_counter(hdr->tsi_cnt, next_hdr->tsi_cnt);
			if (tsi_ret == TRUE)
				return PARSER_STATUS__OK;
		}

		/* Second, search the next header of packet with byte size. */
		*invalid_offset = 0;
		pkt_offset = 188;
		if (cifb->prev_len <= pkt_offset) {
			diff = pkt_offset - cifb->prev_len;
			curr_len = cifb->curr_len - diff;
			curr_data = cifb->curr_data + diff;
			prev_len = 0;
			prev_data = NULL;
		}
		else {
			prev_len = cifb->prev_len - pkt_offset;
			prev_data = cifb->prev_data + pkt_offset;
			curr_len = cifb->curr_len;
			curr_data = cifb->curr_data;
		}

		while (prev_len) {
			if (*prev_data == 0xE2/*1st SC*/) { /* To fast detecting of SC */
				if (decode_header(next_hdr, prev_data, prev_len, curr_data, curr_len)
						== PARSER_STATUS__OK)
					goto byte_search_detected;
			}

			if (pkt_offset == hdr->pkt_size)
				goto byte_search_invalid_header;

			pkt_offset++;
			prev_data++;
			prev_len--;
		}
		
		while (curr_len) {
			if (*curr_data == 0xE2/*1st SC*/) { /* To fast detecting of SC */
				if (decode_header(next_hdr, NULL, 0, curr_data, curr_len)
						== PARSER_STATUS__OK)
					goto byte_search_detected;
			}

			if (pkt_offset == hdr->pkt_size)
				goto byte_search_invalid_header;

			pkt_offset++;
			curr_data++;
			curr_len--;
		}
	}
	else
		return PARSER_STATUS__SHORT_PKT_1;

byte_search_invalid_header:
	*invalid_offset = pkt_offset;
	return PARSER_STATUS__INVALID_HDR;

byte_search_detected:
	if (pkt_offset == hdr->pkt_size)
		return PARSER_STATUS__OK; /* for not null-pkt mode. */
	else /* pkt_offset < hdr->pkt_size */
		return proc_truncated_packet(hdr, cifb, pkt_offset, invalid_offset);
}

static PARSER_STATUS_TYPE get_header(TSI_CIF_HEADER_INFO *hdr,
									TSI_CIF_HEADER_INFO *next_hdr,
								TSI_CIF_BUF_INFO *cifb)
{
	UINT invalid_offset;
	PARSER_STATUS_TYPE status = PARSER_STATUS__NONE_SC;

	/* First, search and decode a header with previous and current data. */
	while (cifb->prev_len) {
		if (g_fUsePrevDecodedHeader == FALSE) {
			if (*cifb->prev_data == 0xE2/*1st SC*/) { /* To fast detecting of SC */
				if ((cifb->prev_len + cifb->curr_len) >= RTV_CIF_HEADER_SIZE)
					status = decode_header(hdr, cifb->prev_data, cifb->prev_len,
											cifb->curr_data, cifb->curr_len);
				else
					return PARSER_STATUS__SHORT_PKT_0;
			}
		}
		else {
			g_fUsePrevDecodedHeader = FALSE; /* reset */
			status = PARSER_STATUS__OK;
		}

		if (status == PARSER_STATUS__OK) {
			status = get_next_header(hdr, next_hdr, cifb, &invalid_offset);
			if (status == PARSER_STATUS__OK)
				return PARSER_STATUS__OK;
			else {
				if (status == PARSER_STATUS__SHORT_PKT_1) {
					/* The next header was NOT exist in this chunk data. */
					/* Use current header in the next parsing time. */
					g_fUsePrevDecodedHeader = TRUE;
					return PARSER_STATUS__SHORT_PKT_1;
				}
				else {
					if (invalid_offset) {
						if (cifb->prev_len > invalid_offset) {
							cifb->prev_len -= invalid_offset;
							cifb->prev_data += invalid_offset;
							continue;
						}
						else {
							invalid_offset -= cifb->prev_len;
							cifb->curr_len -= invalid_offset;
							cifb->curr_data += invalid_offset;
							cifb->prev_len = 0; /* Stop */
							break;
						}
					}
				}
			}
		}

		/* Advacne to search the start-command. */
		cifb->prev_data++;;
		cifb->prev_len--;
	}
	cifb->prev_data = NULL;

	/* Search and decode a header with current data only. */
	while (cifb->curr_len) {
		if (g_fUsePrevDecodedHeader == FALSE) {
			if (*cifb->curr_data == 0xE2/*1st SC*/) { /* To fast detecting of SC */
				if (cifb->curr_len >= RTV_CIF_HEADER_SIZE)
					status = decode_header(hdr, NULL, 0,
										cifb->curr_data, cifb->curr_len);
				else
					return PARSER_STATUS__SHORT_PKT_0;
			}
		}
		else {
			g_fUsePrevDecodedHeader = FALSE; /* reset */
			status = PARSER_STATUS__OK;
		}

		if (status == PARSER_STATUS__OK) {
			status = get_next_header(hdr, next_hdr, cifb, &invalid_offset);
			if (status == PARSER_STATUS__OK)
				return PARSER_STATUS__OK;
			else {
				if (status == PARSER_STATUS__SHORT_PKT_1) {
					/* The next header was NOT exist in this chunk data. */
					/* Use current header in the next parsing time. */
					g_fUsePrevDecodedHeader = TRUE;
					return PARSER_STATUS__SHORT_PKT_1;
				}
				else {
					if (invalid_offset) {
						if (cifb->curr_len > invalid_offset) {
							cifb->curr_len -= invalid_offset;
							cifb->curr_data += invalid_offset;
							continue;
						}
						else {
							cifb->curr_len = 0; /* Stop */
							break;
						}
					}
				}
			}
		}

		/* Advance to search the start-command. */
		cifb->curr_data++;
		cifb->curr_len--;
	}
	cifb->curr_data = NULL;

	return status;
}

/* Copy the MSC data into user-buffer with 188-bytes align. */
static int msc_copy_188(RTV_CIF_DEC_INFO *cifdec, UINT idx,
						TSI_CIF_BUF_INFO *cifb, UINT subch_size,
						E_RTV_SERVICE_TYPE svc_type)
{
	UINT mod;
	int cp_ret = 0;
	UINT total_bytes, copy_bytes;
	UINT msc_size = cifdec->msc_size[idx];
	U8 *msc_buf_ptr = &cifdec->msc_buf_ptr[idx][msc_size];
	TSI_CIF_MSC_FRAG_INFO *frag = &g_atCifMscFrag[idx];
	UINT _2nd_sb_offset;
	const U8 *frag_data_ptr;
	BOOL _2nd_sync_byte_detected = FALSE;

	total_bytes = frag->len + subch_size;

	if (svc_type == RTV_SERVICE_VIDEO) {
		frag_data_ptr = (const U8 *)frag->data;

		while (frag->len) {
			if (frag_data_ptr[0] == 0x47) { /* 1st sb */
				_2nd_sb_offset = 188 - frag->len;
				if (subch_size <= _2nd_sb_offset)
					goto frag_subch_data; /* Stop */

				if (cifb->prev_len > _2nd_sb_offset) {
					if (cifb->prev_data[_2nd_sb_offset] == 0x47) /* 2nd sb */
						_2nd_sync_byte_detected = TRUE;
				}
				else { /* 2nd sb */
					if (cifb->curr_data[_2nd_sb_offset - cifb->prev_len] == 0x47)
						_2nd_sync_byte_detected = TRUE;
				}

				if (_2nd_sync_byte_detected == TRUE) {
					CIF_DATA_COPY(cp_ret, msc_buf_ptr, frag_data_ptr, frag->len);
					msc_buf_ptr += frag->len;

					copy_data(msc_buf_ptr, cifb, _2nd_sb_offset, TRUE);
					msc_buf_ptr += _2nd_sb_offset;
					subch_size -= _2nd_sb_offset;
					cifdec->msc_size[idx] += 188;
					frag->len = 0;
					break; /* Stop */
				}
			}
			frag_data_ptr++;
			frag->len--;
		}

		_2nd_sync_byte_detected = FALSE;
		while (cifb->prev_len && subch_size) {
			if (cifb->prev_data[0] == 0x47) { /* 1st sb */
				if (subch_size > 188) {
					if (cifb->prev_len > 188) {
						if (cifb->prev_data[188] == 0x47) /* 2nd sb */
							_2nd_sync_byte_detected = TRUE;
					}
					else { /* 2nd sb */
						if (cifb->curr_data[188 - cifb->prev_len] == 0x47)
							_2nd_sync_byte_detected = TRUE;
					}
				}
				else if (subch_size == 188) /* The last 188 data was copied. */
					_2nd_sync_byte_detected = TRUE;
				else /* subch_size < 188 */
					goto frag_subch_data; /* Stop */
			}

			if (_2nd_sync_byte_detected == TRUE) {
				_2nd_sync_byte_detected = FALSE;
				copy_data(msc_buf_ptr, cifb, 188, TRUE);
				msc_buf_ptr += 188;
				cifdec->msc_size[idx] += 188;
				subch_size -= 188;
			}
			else {
				subch_size--;
				cifb->prev_data++;
				cifb->prev_len--;
			}
		}

		while (cifb->curr_len && subch_size) {
			if (cifb->curr_data[0] == 0x47) { /* 1st sb */
				if (subch_size > 188) {
					if (cifb->curr_data[188] == 0x47) /* 2nd sb */
						_2nd_sync_byte_detected = TRUE;
				}
				else if (subch_size == 188) /* The last 188 data was copied. */
					_2nd_sync_byte_detected = TRUE;
				else /* subch_size < 188 */
					goto frag_subch_data; /* Stop */
			}

			if (_2nd_sync_byte_detected == TRUE) {
				_2nd_sync_byte_detected = FALSE;
				copy_data(msc_buf_ptr, cifb, 188, TRUE);
				msc_buf_ptr += 188;
				cifdec->msc_size[idx] += 188;
				subch_size -= 188;
			}
			else {
				subch_size--;
				cifb->curr_data++;
				cifb->curr_len--;
			}
		}	
	}
	else { /* Audio or Data */
		if (total_bytes >= 188) {
			mod = total_bytes % 188;
			copy_bytes = total_bytes - mod;

			if (frag->len) {
				/* Copy the frag-data into user buffer. */
				CIF_DATA_COPY(cp_ret, msc_buf_ptr, frag->data, frag->len);
				if (cp_ret == 0) {
					msc_buf_ptr += frag->len;
					cifdec->msc_size[idx] += frag->len;
					copy_bytes -= frag->len; /* Update */
					frag->len = 0;
				}
				else
					return -1;
			}

			if (copy_data(msc_buf_ptr, cifb, copy_bytes, TRUE)) {
				cifdec->msc_size[idx] += copy_bytes;
				subch_size -= copy_bytes;
			}
			else
				return -2;
		}
	}

frag_subch_data:
	if (subch_size) {
		copy_data(frag->data + frag->len, cifb, subch_size, FALSE);
		frag->len += subch_size;
	}

	return (cifdec->msc_size[idx] - msc_size);
}

void rtvCIFDEC_Decode(RTV_CIF_DEC_INFO *ptDecInfo, UINT nMscBufSize,
						const U8 *pbTsBuf, UINT nTsLen)
{
	UINT i, nOutDecBufIdx, msc_copy_size;
	PARSER_STATUS_TYPE status = PARSER_STATUS__UNKNOWN_ERR; 
	/* To use a previous decoded header. "static" */
	static TSI_CIF_HEADER_INFO hdr;
	TSI_CIF_HEADER_INFO next_hdr;
	UINT msc_buf_size[RTV_NUM_DAB_AVD_SERVICE];

	ptDecInfo->fic_size = 0;
#if (RTV_NUM_DAB_AVD_SERVICE == 1)
	ptDecInfo->msc_size[0] = 0;
	ptDecInfo->msc_subch_id[0] = 0xFF; /* Default invalid */
	msc_buf_size[0] = nMscBufSize;
#else
	for (i = 0; i < RTV_NUM_DAB_AVD_SERVICE; i++) {
		ptDecInfo->msc_size[i] = 0;
		ptDecInfo->msc_subch_id[i] = 0xFF; /* Default invalid */
		msc_buf_size[i] = nMscBufSize;
	}
#endif

	if (nTsLen < (8 * 188)) {
		CIF_DBGMSG1("[rtvCIFDEC_Decode] To small source TS size: %u\n", nTsLen);
		return;
	}

	if (g_fCifInited == FALSE) {
		CIF_DBGMSG0("[rtvCIFDEC_Decode] Not yet init\n");
		return;
	}

	CIF_MUTEX_LOCK;
	g_nFicIC = 0;
	g_tCifBuf.curr_len = nTsLen;
	g_tCifBuf.curr_data = pbTsBuf;

	/* Parsing the TSI CIF data by processing a TSI CIF packet unit. */
	do {
		status = get_header(&hdr, &next_hdr, &g_tCifBuf);
		if (status != PARSER_STATUS__OK) {
			//CIF_DBGMSG1("[rtvCIFDEC_Decode] get_header error: %d\n", status);
			collect_ts_data(&g_tCifBuf);
			goto out;
		}

		/* Remove the header. */
		pull_packet(&g_tCifBuf, RTV_CIF_HEADER_SIZE);

		/* Get and process FIC data. */
		if (hdr.fic_size != 0) {
			if (copy_data(ptDecInfo->fic_buf_ptr, &g_tCifBuf, hdr.fic_size, TRUE))
				ptDecInfo->fic_size = hdr.fic_size;
			else {
				reset_processing_data();
				goto out;
			}
		}

#if (RTV_NUM_DAB_AVD_SERVICE == 1)
		i = 0;
		nOutDecBufIdx = 0;
#else
		for (i = 0; i < RTV_NUM_DAB_AVD_SERVICE; i++)
#endif
		{
			if (hdr.subch_size[i] != 0) {
				/* Get the index of output buffer determined when add sub channel. */
		#if (RTV_NUM_DAB_AVD_SERVICE >= 2)
				nOutDecBufIdx = g_aOutDecBufIdx[hdr.subch_id[i]];
		#endif
				if (ptDecInfo->msc_subch_id[nOutDecBufIdx] == 0xFF) /* The first */
					ptDecInfo->msc_subch_id[nOutDecBufIdx] = hdr.subch_id[i];

				/* Check if subch ID was changed in the same out buf index ? */
				if (ptDecInfo->msc_subch_id[nOutDecBufIdx] == hdr.subch_id[i]) {
					/* Check if the size of current MSC is greater than 
					the size of output buffer. */
					if (hdr.subch_size[i] <= msc_buf_size[nOutDecBufIdx]) {
						CIF_MSC_KFILE_WRITE_EN(hdr.subch_id[i]);
						msc_copy_size = msc_copy_188(ptDecInfo, nOutDecBufIdx,
											&g_tCifBuf, hdr.subch_size[i],
											g_eaSvcType[hdr.subch_id[i]]);
						CIF_MSC_KFILE_WRITE_DIS;
						if (msc_copy_size >= 0)
							msc_buf_size[nOutDecBufIdx] -= msc_copy_size;
						else {
							reset_processing_data();
							goto out;
						}

						subch_id_pass_num++;
					}
					else {
						CIF_DBGMSG0("[rtvCIFDEC_Decode] Too small Out buffer size\n");
						reset_processing_data();
						goto out;
					}
				}
				else {
					CIF_DBGMSG0("[rtvCIFDEC_Decode] Subch ID changed! Discarded\n");
					pull_packet(&g_tCifBuf, hdr.subch_size[i]);
				}
			}
		}

		/* Remove the padding. */
		pull_packet(&g_tCifBuf, hdr.padding_bytes);

		/* Copy the next header into prev header to use in the next pasrsing time. */
		hdr = next_hdr;

		/* Use the decoded header in the next parsing time. */
		g_fUsePrevDecodedHeader = TRUE;

		/* To collect the remaind data, we check 0 bytes. */
	} while((g_tCifBuf.prev_len + g_tCifBuf.curr_len) != 0);

out:
	CIF_MUTEX_UNLOCK;

	return;
}

#else
/* SPI */
/* Only MSC0 data. */
void rtvCIFDEC_Decode(RTV_CIF_DEC_INFO *ptDecInfo, UINT nMscBufSize,
						const U8 *pbTsBuf, UINT nTsLen)
{     
	UINT i, ch_len, subch_id, subch_size, remaining_size = nTsLen;
	const U8 *data_ptr;
	const U8 *cif_header_ptr = pbTsBuf;
	UINT nOutDecBufIdx = 0;
	UINT msc_buf_size[RTV_MAX_NUM_DAB_DATA_SVC];
	U8 *dst_msc_buf_ptr[RTV_MAX_NUM_DAB_DATA_SVC];
	int cp_ret = 0;

	for (i = 0; i < RTV_MAX_NUM_DAB_DATA_SVC; i++) { /* Only MSC0. */
		ptDecInfo->msc_size[i] = 0;
		ptDecInfo->msc_subch_id[i] = 0xFF; /* Default invalid */
		msc_buf_size[i] = nMscBufSize;
		dst_msc_buf_ptr[i] = ptDecInfo->msc_buf_ptr[i];
	}

	if (g_fCifInited == FALSE) {
		CIF_DBGMSG0("[rtvCIFDEC_Decode] Not yet init\n");
		return;
	}

	CIF_MUTEX_LOCK;

	do {
		subch_id = cif_header_ptr[0] >> 2;
		ch_len = (cif_header_ptr[2]<<8) | cif_header_ptr[3];

		subch_size = ch_len - RTV_CIF_HEADER_SIZE;
		data_ptr = cif_header_ptr + RTV_CIF_HEADER_SIZE;

		/* Check for this sub channel ID was registerd ? */
		if ((g_aAddedSubChIdBits[DIV32(subch_id)] & (1<<MOD32(subch_id))) != 0) {
			/* Get the index of output buffer determined when add sub channel. */
			nOutDecBufIdx = g_aOutDecBufIdx[subch_id];

			if (ptDecInfo->msc_subch_id[nOutDecBufIdx] == 0xFF) /* The first */
				ptDecInfo->msc_subch_id[nOutDecBufIdx] = subch_id;

			/* Check if subch ID was changed in the same out buf index ? */
			if (ptDecInfo->msc_subch_id[nOutDecBufIdx] == subch_id) {
				if (subch_size <= msc_buf_size[nOutDecBufIdx]) {
					CIF_DATA_COPY(cp_ret, dst_msc_buf_ptr[nOutDecBufIdx],
								data_ptr, subch_size);
					if(cp_ret)
						goto out;
					
					dst_msc_buf_ptr[nOutDecBufIdx] += subch_size;
					ptDecInfo->msc_size[nOutDecBufIdx] += subch_size;
					msc_buf_size[nOutDecBufIdx] -= subch_size;
				}
				else {
					CIF_DBGMSG0("[rtvCIFDEC_Decode] Too small Out buffer size\n");
					goto out;
				}
			}
			else {
				CIF_DBGMSG0("[rtvCIFDEC_Decode] Subch ID changed! Discarded\n");
			}
		}
		else {
			CIF_DBGMSG2("[rtvCIFDEC_Decode] Not registerd: ID(%u), ch_len(%u)\n",
				subch_id, ch_len);
			goto out;
		}

		cif_header_ptr += ch_len;                   
		remaining_size -= ch_len;
	} while(remaining_size != 0);

out:
	CIF_MUTEX_UNLOCK;

	return;
}
#endif


/* 
This function delete a sub channel ID from the CIF decoder. 
This function should called after Sub Channel Close. */
void rtvCIFDEC_DeleteSubChannelID(UINT nSubChID)
{
	UINT nOutDecBufIdx;

	CIF_MUTEX_LOCK;

	if ((g_aAddedSubChIdBits[DIV32(nSubChID)] & (1<<MOD32(nSubChID))) != 0) {
		// Delete a sub channel ID.
		g_aAddedSubChIdBits[DIV32(nSubChID)] &= ~(1 << MOD32(nSubChID)); 

		nOutDecBufIdx = g_aOutDecBufIdx[nSubChID];
		g_nOutDecBufIdxBits &= ~(1 << nOutDecBufIdx);
		CIF_KFILE_CLOSE(nSubChID);
	}

	CIF_MUTEX_UNLOCK;
}

/*
This function add a sub channel ID to the CIF decoder to verify CIF header.
This function should called before Sub Channel Open. */
BOOL rtvCIFDEC_AddSubChannelID(UINT nSubChID, E_RTV_SERVICE_TYPE eServiceType)
{
	BOOL fRet = TRUE;
	UINT nOutDecBufIdx;
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
	UINT nMaxNumMscSvc = RTV_MAX_NUM_DAB_DATA_SVC; /* Only MSC0. */
#else
	UINT nMaxNumMscSvc = RTV_NUM_DAB_AVD_SERVICE;
#endif

	CIF_MUTEX_LOCK;

	// Check if already registerd ?
	if ((g_aAddedSubChIdBits[DIV32(nSubChID)] & (1<<MOD32(nSubChID))) == 0) {
		/* Adds a sub channel ID. */
		g_aAddedSubChIdBits[DIV32(nSubChID)] |= (1 << MOD32(nSubChID)); 

		for (nOutDecBufIdx = 0; nOutDecBufIdx < nMaxNumMscSvc; nOutDecBufIdx++) {
			if ((g_nOutDecBufIdxBits & (1<<nOutDecBufIdx)) == 0) 		
				break;
		}

		if (nOutDecBufIdx != nMaxNumMscSvc) {
			g_nOutDecBufIdxBits |= (1<<nOutDecBufIdx);
			g_aOutDecBufIdx[nSubChID] = nOutDecBufIdx;
			g_eaSvcType[nSubChID] = eServiceType;
#if (RTV_NUM_DAB_AVD_SERVICE == 1)
			g_nSingleSubchID = nSubChID; /* for low */
#endif

#if defined(RTV_IF_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE)
			g_atCifMscFrag[nOutDecBufIdx].len = 0; /* Reset frag len */
#endif
			CIF_KFILE_OPEN(nSubChID);
		}
		else {
			CIF_DBGMSG1("[rtvCIFDEC_AddSubChannelID] Error: %u\n", nSubChID);
			fRet = FALSE;
		}
	}

	CIF_MUTEX_UNLOCK;

	return fRet;
}

/* This function deinitialize the CIF decoder.*/
void rtvCIFDEC_Deinit(void)
{
	if (g_fCifInited == FALSE)
		return;

	g_fCifInited = FALSE;

	CIF_DBGMSG0("[rtvCIFDEC_Deinit] CIF decode Exit\n");

	CIF_MUTEX_LOCK;
#if defined(RTV_IF_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE)
	// Free a pool
	g_pbPrevBufAllocPtr = NULL;
	TSI_INVAL_HDR_STAT_SHOW;
#endif

	CIF_KFILE_CLOSE(0xFFFF);

	CIF_MUTEX_UNLOCK;

	CIF_MUTEX_DEINIT;
}


/* This function initialize the CIF decoder. */
void rtvCIFDEC_Init(void)
{
	if (g_fCifInited == TRUE)
		return;

#if defined(RTV_IF_SERIAL_TSIF) || defined(RTV_IF_SPI_SLAVE)
	CIF_DBGMSG1("[rtvCIFDEC_Init] TSIF collect buffer size: %u\n",
			TSIF_COLLECT_BUF_SIZE);

	/* Allocate a new buffer from memory pool. */
	g_pbPrevBufAllocPtr = &temp_ts_buf_pool[0];		

	reset_processing_data();

	TSI_INVAL_HDR_STAT_RST_ALL;
	subch_id_pass_num = 0;
	g_nNumTsiDiscontinuity = 0;
	g_nNumFicSizeNoData = 0;
#else
	CIF_DBGMSG0("[rtvCIFDEC_Init] SPI\n");
#endif

	g_nOutDecBufIdxBits = 0x00; 

	g_aAddedSubChIdBits[0] = 0;
	g_aAddedSubChIdBits[1] = 0;

#ifdef _MSC_KERNEL_FILE_DUMP_ENABLE
	memset(g_ptDecMscFilp, 0, sizeof(g_ptDecMscFilp));
	CIF_MSC_KFILE_WRITE_DIS;
#endif

	CIF_MUTEX_INIT;

	g_fCifInited = TRUE;
}

#endif /* #ifdef RTV_CIF_MODE_ENABLED */

