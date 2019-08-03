
CC	=	gcc
CFLAGS	=	-O3 -I.

numa_memory_access_time numa_memory_traffic_injector: numa_memory_access_time.c numa_memory_traffic_injector.c cpu_util.h
	$(CC) $(CFLAGS) -o numa_memory_access_time numa_memory_access_time.c -lnuma
	$(CC) $(CFLAGS) -o numa_memory_traffic_injector numa_memory_traffic_injector.c -lnuma
	@sudo setcap "all=ep" numa_memory_access_time
	@sudo setcap "all=ep" numa_memory_traffic_injector


clean:
	rm -f *.o numa_memory_access_time numa_memory_traffic_injector
