# Makefile for LKM
obj-m := final.o
final-objs := ./src/main.o ./src/main_device.o ./src/group_manager.o ./src/message.o

KDIR=/lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(shell pwd)  modules 

clean:
	make -C $(KDIR) M=$(shell pwd) clean # from lkmpg
	rm -rvf *~
