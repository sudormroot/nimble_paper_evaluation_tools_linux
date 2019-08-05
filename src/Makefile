
CC	=	gcc
INCS	=	-I.
CFLAGS	=	-O3 -I.
LIBS	=	-lnuma
RM	=	rm

TARGET_ACCESS_TIME	= numa_memory_access_time
TARGET_TRAFFIC_INJECTOR	= numa_memory_traffic_injector

TARGETS	= $(TARGET_ACCESS_TIME) $(TARGET_TRAFFIC_INJECTOR)

OBJS_ACCESS_TIME	= numa_memory_access_time.o cpu_freq.o
OBJS_TRAFFIC_INJECTOR	= numa_memory_traffic_injector.o cpu_freq.o
#OBJS			= numa_memory_access_time.o numa_memory_traffic_injector.o cpu_freq.o
OBJS			= $(OBJS_ACCESS_TIME) $(OBJS_TRAFFIC_INJECTOR)

HEADERS	= cpu_util.h cpu_freq.h


$(TARGETS):$(OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET_ACCESS_TIME) $(OBJS_ACCESS_TIME) $(LIBS)
	$(CC) $(CFLAGS) -o $(TARGET_TRAFFIC_INJECTOR) $(OBJS_TRAFFIC_INJECTOR) $(LIBS)

	#@sudo setcap "all=ep" numa_memory_access_time
	#@sudo setcap "all=ep" numa_memory_traffic_injector

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -rf $(TARGETS) $(OBJS)
