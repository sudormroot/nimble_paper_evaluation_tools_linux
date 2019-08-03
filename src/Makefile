
CC=gcc

numa_memory_access_time: numa_memory_access_time.c
	$(CC) -o numa_memory_access_time numa_memory_access_time.c -lnuma
	sudo setcap "all=ep" $@

