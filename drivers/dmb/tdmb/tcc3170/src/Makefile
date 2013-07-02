
obj-y   += tcbd_hal.o
obj-y   += tcbd_drv_io.o
obj-y   += tcbd_drv_rf.o
obj-y   += tcbd_drv_peri.o
obj-y   += tcbd_drv_dual.o
obj-y   += tcbd_drv_ip.o
obj-y   += tcbd_api_common.o

obj-y   += tcbd_diagnosis/ 
obj-y   += tcc_fic_decoder/ 
obj-y   += tcbd_stream_parser/
obj-y   += tcbd_test/ 
obj-y   += tcpal_linux/


TDMBDIR := dmb/tdmb

EXTRA_CFLAGS += -Idrivers/$(TDMBDIR)/tcc3170/inc
EXTRA_CFLAGS += -Idrivers/$(TDMBDIR)/tcc3170/inc/tcbd_diagnosis
EXTRA_CFLAGS += -Idrivers/$(TDMBDIR)/tcc3170/inc/tcbd_stream_parser
EXTRA_CFLAGS += -Idrivers/$(TDMBDIR)/tcc3170/inc/tcpal
