#
# Makefile for the MSM8960 ATMEL224E TOUCH drivers.
#


obj-$(CONFIG_TOUCHSCREEN_QT602240_MSM8960) += qt602240.o
#obj-m := qt602240.o

KDIR := ../../../../../out/target/product/msm8960/obj/KERNEL_OBJ/
PWD := $(shell pwd)

#EXTRA_CFLAGS += -include ../../../../../../../BOARD_REV.h

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -f -r *.o *.ko .*cmd *.mod.c .tmp* core *.i modules.order Module.symvers modules.builtin
