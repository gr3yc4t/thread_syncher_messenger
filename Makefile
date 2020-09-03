# Makefile for LKM
obj-m := final.o
final-objs := ./src/main.o ./src/main_device.o ./src/group_manager.o ./src/message.o

KDIR=/lib/modules/$(shell uname -r)/build

#Debug Flags
CFLAGS_main_device.o := -DDEBUG
CFLAGS_group_manager.o := -DDEBUG
CFLAGS_message.o := -DDEBUG
CFLAGS_main.o := -DDEBUG
all:
	make -C $(KDIR) M=$(shell pwd)  modules 

clean:
	make -C $(KDIR) M=$(shell pwd) clean # from lkmpg
	rm -rvf *~
