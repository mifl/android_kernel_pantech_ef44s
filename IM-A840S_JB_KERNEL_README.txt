How to Build
    1. Get Toolchain (arm-eabi-4.6) and install
       (Visit android git server or codesourcery)   

    2. run build_kernel.sh
		
      $ export ARCH=arm
      $ export CROSS_COMPILE=~/your toolchain path/arm-eabi-4.6/bin/arm-eabi-
      $ ./build_kernel.sh

    3.Output Files
      -	kernel : kernel/obj/KERNEL_OBJ/arch/arm/boot/zImage
      -	module : kernel/obj/KERNEL_OBJ/drivers/*/*.ko
 
Kernel source:
    Source code for the kernel based in Qualcomm tag M8960AAAAANLYA1741J,
    inserted into the history for traceability:
    https://github.com/mifl/android_kernel_caf_msm/tree/M8960AAAAANLYA1741J
