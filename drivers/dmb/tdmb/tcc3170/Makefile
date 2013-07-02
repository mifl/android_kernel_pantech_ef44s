#
# Makefile for the kernel TCC T3170 TDMB device drivers.
#

obj-y += src/ 
# obj-y += tcbd.o
obj-y += tcc3170_bb.o


TDMBDIR := dmb/tdmb

EXTRA_CFLAGS += -Idrivers/$(TDMBDIR)/tcc3170/inc
EXTRA_CFLAGS += -Idrivers/$(TDMBDIR)/tcc3170/inc/tcpal
EXTRA_CFLAGS += -Idrivers/$(TDMBDIR)/tcc3170/inc/tcbd_diagnosis
EXTRA_CFLAGS += -Idrivers/$(TDMBDIR)/tcc3170/inc/tcbd_stream_parser

