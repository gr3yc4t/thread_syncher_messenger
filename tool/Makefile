all:tool

tool:main.c
	CC=gcc
	$(CC) -g -DDEBUG thread_synch.c main.c -o tool -lpthread
clean:
	\rm -fr tool
