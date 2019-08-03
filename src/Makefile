
CC	=	gcc
CFLAGS	=	-O3 -I.

numa_memory_access_time: numa_memory_access_time.c
	$(CC) $(CFLAGS) -o numa_memory_access_time numa_memory_access_time.c -lnuma
	sudo setcap "all=ep" $@


clean:
	rm -f *.o numa_memory_access_time
