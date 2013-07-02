#
# Makefile for the kernel ISDB-T device drivers.
#
#########################################################

obj-y += sharp.o

  sharp-y := sharp_bb.o
  sharp-y += chipset/common/nmiisdbtcmn.o
  ##### nmiisdbtcmn.c includes nmiisdbt.c

  sharp-y += ntv/nmidrv.o
  #sharp-y += ntv/nmintvcommon.o
  #sharp-y += ntv/nmintvversion.o
  #sharp-y += ntv/nmi_test.o
  ##### nmintvcommon.c includes nmintv.c

  #sharp-y := ntv/nmintvversion.o
  