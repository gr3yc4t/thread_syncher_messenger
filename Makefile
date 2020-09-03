# Makefile for LKM
obj-m := final.o
final-objs := ./src/main.o ./src/main_device.o ./src/group_manager.o ./src/message.o

KDIR=/lib/modules/$(shell uname -r)/build

#Debug Flags
CFLAGS_main_device.o := -DDEBUG -DEBUG
CFLAGS_group_manager.o := -DDEBUG -DEBUG
CFLAGS_message.o := -DDEBUG -DEBUG
CFLAGS_main.o := -DDEBUG -DEBUG

all:
	make -C $(KDIR) M=$(shell pwd)  modules 

clean:
	make -C $(KDIR) M=$(shell pwd) clean # from lkmpg
	rm -rvf *~
