# Makefile for LKM
obj-m := aosv2020.o
aosv2020-objs := ./src/main.o ./src/main_device.o ./src/sysfs.o ./src/group_manager.o ./src/message.o ./src/sysfs.o

KDIR=/lib/modules/$(shell uname -r)/build

#Debug Flags
CFLAGS_main_device.o := -DDEBUG
CFLAGS_group_manager.o := -DDEBUG
CFLAGS_message.o := -DDEBUG
CFLAGS_main.o := -DDEBUG
CFLAGS_sysfs.o := -DDEBUG

all:
	make CFLAGS="-fanalyzer -Wextra -g3 -fno-omit-frame-pointer" -C $(KDIR) M=$(shell pwd) modules 
release:
	make CFLAGS="-O3" -C $(KDIR) M=$(shell pwd) modules
clean:
	make -C $(KDIR) M=$(shell pwd) clean # from lkmpg
	rm -rvf *~
