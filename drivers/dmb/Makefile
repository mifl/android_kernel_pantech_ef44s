#
# Makefile for the kernel DMB device drivers.
#
#########################################################

obj-$(CONFIG_SKY_DMB) += dmb_device.o

  dmb_device-y := dmb_hw.o
  dmb_device-y += dmb_test.o
  dmb_device-$(CONFIG_SKY_DMB_I2C_CMD) += dmb_i2c.o
  dmb_device-$(CONFIG_SKY_DMB_TSIF_IF) += dmb_tsif.o
  dmb_device-$(CONFIG_SKY_DMB_SPI_IF) += dmb_spi.o

ifeq ($(CONFIG_SKY_TDMB),y)
  obj-y += tdmb/
endif

ifeq ($(CONFIG_SKY_ISDBT),y)
  obj-y += isdbt/
endif

ifeq ($(CONFIG_SKY_ISDBTMM),y)
  obj-y += isdbtmm/
endif

#########################################################

ifeq ($(CONFIG_SKY_DMB_MODULE),m)
  KERNELDIR := /lib/modules/'uname -r'/build
  all:
    $(MAKE) -C $(KERNELDIR) -M='pwd'

  clean:
  rm -rf *.ko
  rm -rf *.mod.*
  rm -rf .*.cmd
  rm -rf *.o

  rm -rf $(TDMBBBDIR)*.ko
  rm -rf $(TDMBBBDIR)*.mod.*
  rm -rf $(TDMBBBDIR).*.cmd
  rm -rf $(TDMBBBDIR)*.o
endif