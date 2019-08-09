
CC	=	gcc
INCS	=	-I.
CFLAGS	=	-O3 -I.
LIBS	=	-lnuma
RM	=	rm

TARGET_ACCESS_TIME		= numa_memory_access_time
TARGET_TRAFFIC_INJECTOR	= numa_memory_traffic_injector
TARGET_MEMORY_HOG		= numa_memory_hog
TARGET_NIMBLE_CONTROL	= nimble_control

TARGETS	= $(TARGET_ACCESS_TIME) $(TARGET_TRAFFIC_INJECTOR) $(TARGET_MEMORY_HOG) $(TARGET_NIMBLE_CONTROL)

OBJS_ACCESS_TIME		= numa_memory_access_time.o cpu_freq.o
OBJS_TRAFFIC_INJECTOR	= numa_memory_traffic_injector.o cpu_freq.o
OBJS_MEMORY_HOG			= numa_memory_hog.o
OBJS_NIMBLE_CONTROL		= nimble_control.o


OBJS			= $(OBJS_ACCESS_TIME) $(OBJS_TRAFFIC_INJECTOR) $(OBJS_MEMORY_HOG) $(OBJS_NIMBLE_CONTROL)

HEADERS	= cpu_util.h cpu_freq.h


$(TARGETS):$(OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET_ACCESS_TIME) $(OBJS_ACCESS_TIME) $(LIBS)
	$(CC) $(CFLAGS) -o $(TARGET_TRAFFIC_INJECTOR) $(OBJS_TRAFFIC_INJECTOR) $(LIBS)
	$(CC) $(CFLAGS) -o $(TARGET_MEMORY_HOG) $(OBJS_MEMORY_HOG) $(LIBS)
	$(CC) $(CFLAGS) -o $(TARGET_NIMBLE_CONTROL) $(OBJS_NIMBLE_CONTROL) $(LIBS)

#@sudo setcap "all=ep" numa_memory_access_time
#@sudo setcap "all=ep" numa_memory_traffic_injector

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -rf $(TARGETS) $(OBJS)
