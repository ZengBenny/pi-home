PWD=$(shell pwd)
ARCH=arm
KERNEL=/projects/rpi3/linux
TOOLCHAIN=/projects/rpi3/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin
CROSS_COMPILE=${TOOLCHAIN}/arm-linux-gnueabihf-

export CC=${CROSS_COMPILE}gcc
export AR=${CROSS_COMPILE}ar
export LD=${CROSS_COMPILE}ld

obj-m += gpio.o

EXTRA_CFLAGS += -I$(PWD)

all:
	make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -C ${KERNEL} M=$(PWD) modules

clean:
	make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -C $(KERNEL) M=$(PWD) clean

