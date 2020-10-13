all:tool

afl:
	CC=afl-gcc 
	$(CC) -static -g3 ../lib/thread_synch.c lib/ini.c benchmark.c main.c -o tool_afl -lpthread
tool:main.c
	CC=gcc
	$(CC) -g -DDEBUG -fno-omit-frame-pointer ../lib/thread_synch.c lib/ini.c benchmark.c main.c -o tool -lpthread
release:
	CC=gcc
	$(CC) -O3 ../lib/thread_synch.c lib/ini.c benchmark.c main.c -o tool -lpthread
clean:
	\rm -fr tool
