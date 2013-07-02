#
# Makefile for the kernel TDMB device drivers.
#
#########################################################

obj-y += tdmb_device.o

  tdmb_device-y := tdmb_dev.o
  tdmb_device-y += tdmb_bb.o
  tdmb_device-y += tdmb_test.o

ifeq ($(CONFIG_SKY_TDMB_INC_BB_T3700),y)
  obj-y += t3700/
endif

ifeq ($(CONFIG_SKY_TDMB_INC_BB_T3900),y)
  obj-y += t3900/
endif

ifeq ($(CONFIG_SKY_TDMB_FCI_BB_FC8050),y)
  obj-y += fc8050/
endif

ifeq ($(CONFIG_SKY_TDMB_RTV_BB_MTV350),y)
  obj-y += mtv350/
endif

ifeq ($(CONFIG_SKY_TDMB_TCC_BB_TCC3170),y)
  obj-y += tcc3170/
endif
