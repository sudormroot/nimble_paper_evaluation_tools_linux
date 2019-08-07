#!/bin/bash


CGROUP="test"

RESULT_DIR="test_result_dir-""`date '+%Y%m%d_%H-%M-%S_%s-%N'`"

LOG_FILE="$RESULT_DIR""/log.txt"
STATS_FILE="$RESULT_DIR""/stats.txt"
APPLOG_FILE="$RESULT_DIR""/applog.txt"
CONFIG_FILE="$RESULT_DIR""/config.txt"

MAX_OPEN_FILES=100000

DROP_CACHES_INTERVAL=3	   #drop caches for every 3 seconds	
STATS_COLLECT_INTERVAL=5   #collect system statistics every 5 seconds

MAX_MEM_SIZE="0"
FAST_MEM_SIZE="0"
MIGRATION_THREADS_NUM="0"
ENABLE_TRAFFIC_INJECTION="0"
THP_MIGRATION="x"

CHILD_PID="$$"


show_usage() {
	echo "$0 [--enable-traffic-injection] --thp_migration=<1|0> --max-mem-size=<Size-in-MB> --fast-mem-size=<Size-in-MB> --migration-threads-num=<Migration-Threads-Number> <Cmd> <Arg1> <Arg2> ..."
}

if [ $# = 0 ];then
	show_usage
	exit
fi


if [ ! -d "$RESULT_DIR" ];then
	mkdir $RESULT_DIR 2>/dev/zero
	echo "" >  $LOG_FILE
	echo "" >  $STATS_FILE
	echo "" >  $APPLOG_FILE
	echo "" >  $CONFIG_FILE
fi

log_time() {
	echo "Time [Year-Month-Day Hour:Minute:Second UnixTime Nanoseconds] `date '+%Y-%m-%d %H:%M:%S %s %N'`" >> $1
}


echo "CMD : $0 $*" | tee -a $LOG_FILE
log_time $LOG_FILE
echo "" >> $LOG_FILE




#############################################
#   Get parameters                          #
#############################################

while [ 1 = 1 ] ; do
	case "$1" in
		-M|--thp_migration=*) 
			THP_MIGRATION=`echo ${1#*=}`
			echo "THP_MIGRATION=$THP_MIGRATION" | tee -a $LOG_FILE
			shift 1;;
		-i|--enable-traffic-injection) 
			ENABLE_TRAFFIC_INJECTION=1
			echo "ENABLE_TRAFFIC_INJECTION=$ENABLE_TRAFFIC_INJECTION" | tee -a $LOG_FILE
			shift 1;;
		-M|--max-mem-size=*) 
			MAX_MEM_SIZE=`echo ${1#*=}`
			echo "MAX_MEM_SIZE=$MAX_MEM_SIZE MB" | tee -a $LOG_FILE
			shift 1;;
		-m|--fast-mem-size=*) 
			FAST_MEM_SIZE=`echo ${1#*=}`
			echo "FAST_MEM_SIZE=$FAST_MEM_SIZE MB" | tee -a $LOG_FILE
			shift 1;;
		-t|--migration-threads-num*) 
			MIGRATION_THREADS_NUM=`echo ${1#*=}`
			echo "MIGRATION_THREADS_NUM=$FAST_MEM_SIZE" | tee -a $LOG_FILE
			shift 1 ;;
		--) 
			shift 1
			break ;;
		*) 
			break ;;
	esac
done

if [ "$THP_MIGRATION" != "0" ] && [ "$THP_MIGRATION" != "1" ]; then
	echo "Required --thp_migration=<1|0>"
	show_usage
	exit
fi


if [ "$MAX_MEM_SIZE" = "0" ]; then
	echo "Required --max-mem-size=<Size-in-MB>"
	show_usage
	exit
fi


if [ "$FAST_MEM_SIZE" = "0" ]; then
	echo "Required --fast-mem-size=<Size-in-MB>"
	show_usage
	exit
fi

if [ "$MIGRATION_THREADS_NUM" = "0" ]; then
	echo "Required --migration-threads-num=<Migration-Threads-Number>"
	show_usage
	exit
fi

APP_CMD="numactl --cpunodebind=0 --preferred=0 $@" 

echo "APP_CMD=$APP_CMD"

echo "APP_CMD=$APP_CMD" >> $LOG_FILE
echo "APP_CMD=$APP_CMD" >> $APPLOG_FILE
echo "APP_CMD=$APP_CMD" >> $CONFIG_FILE

echo "DROP_CACHES_INTERVAL=$DROP_CACHES_INTERVAL" >> $CONFIG_FILE
echo "STATS_COLLECT_INTERVAL=$STATS_COLLECT_INTERVAL" >> $CONFIG_FILE
echo "MAX_MEM_SIZE=$MAX_MEM_SIZE" >> $CONFIG_FILE
echo "FAST_MEM_SIZE=$FAST_MEM_SIZE" >> $CONFIG_FILE
echo "MIGRATION_THREADS_NUM=$MIGRATION_THREADS_NUM" >> $CONFIG_FILE
echo "ENABLE_TRAFFIC_INJECTION=$ENABLE_TRAFFIC_INJECTION" >> $CONFIG_FILE
echo "THP_MIGRATION=$THP_MIGRATION" >> $CONFIG_FILE


test_cleanup() {
	kill -9 $?
	killall numa_memory_traffic_injector 2>/dev/zero
	killall collect_stats.sh 2>/dev/zero
	exit
}

handle_signal_INT() {
	echo "INT signal is captured, cleaning up ..." | tee -a $LOG_FILE
	log_time $LOG_FILE
	test_cleanup
}

collect_stats(){
	echo "CMD=sysctl vm" >> $STATS_FILE
	sudo sysctl vm >> $STATS_FILE
	echo "" >> $STATS_FILE

	echo "CMD=numastat" >> $STATS_FILE
	numastat >> $STATS_FILE
	echo "" >> $STATS_FILE
	
	echo "CMD=numastat -m" >> $STATS_FILE
	numastat -m >> $STATS_FILE
	echo "" >> $STATS_FILE

	echo "CMD=numastat -p $CHILD_PID" >> $STATS_FILE
	numastat -p $CHILD_PID >>  -a $STATS_FILE
	echo "" >> $STATS_FILE

	echo "CMD=cat /proc/meminfo" >> $STATS_FILE
	cat /proc/meminfo >> $STATS_FILE
	echo "" >> $STATS_FILE

	echo "CMD=cat /proc/zoneinfo" >> $STATS_FILE
	cat /proc/zoneinfo >> $STATS_FILE
	echo "" >> $STATS_FILE

	echo "CMD=cat /proc/vmstat" >> $STATS_FILE
	cat /proc/vmstat >> $STATS_FILE
	echo "" >> $STATS_FILE
}


handle_signal_ALRM() {
	collect_stats
	sleep $STATS_COLLECT_INTERVAL
	kill -ALRM $$
}

trap "handle_signal_ALRM" ALRM
trap "handle_signal_INT"  INT


#############################################
#   Set parameters                          #
#############################################


echo "Set parameters ..." | tee -a $LOG_FILE

ulimit -n $MAX_OPEN_FILES
echo "Set maximum open files to $MAX_OPEN_FILES"

# create customized control-group named two-level-memory
sudo mkdir /sys/fs/cgroup/$CGROUP 2>/dev/zero

# enable memory control
sudo echo "+memory" > /sys/fs/cgroup/cgroup.subtree_control

sudo echo "$MAX_MEM_SIZE""M" > /sys/fs/cgroup/$CGROUP/memory.max
echo "memory.max is set to $MAX_MEM_SIZE MB" | tee -a $LOG_FILE

sudo sysctl vm.sysctl_enable_thp_migration=$THP_MIGRATION
echo "Set vm.sysctl_enable_thp_migration=$THP_MIGRATION" | tee -a $LOG_FILE


sudo echo "$FAST_MEM_SIZE""M" > /sys/fs/cgroup/$CGROUP/memory.max_at_node:0
echo "Set /sys/fs/cgroup/$CGROUP/memory.max_at_node:0 to $FAST_MEM_SIZE MB" | tee -a $LOG_FILE

sudo sysctl vm/limit_mt_num=$MIGRATION_THREADS_NUM
echo "Set vm/limit_mt_num=$MIGRATION_THREADS_NUM" | tee -a $LOG_FILE

#sync
#sudo echo $DROP_CACHES_INTERVAL > /proc/sys/vm/drop_caches
#echo "Set /proc/sys/vm/drop_caches to $DROP_CACHES_INTERVAL"

pid=$$
echo "$pid" > /sys/fs/cgroup/$CGROUP/cgroup.procs
echo "Set /sys/fs/cgroup/$CGROUP/cgroup.procs to $pid" | tee -a $LOG_FILE


#############################################
#   slow NUMA node #1                       #
#############################################

inject_traffic() {
	echo "Inject traffic to slow memory on NUMA node #1 ..." | tee -a $LOG_FILE

	if [ ! -f "numa_memory_traffic_injector" ]; then
		echo "numa_memory_traffic_injector not found, try make" | tee -a $LOG_FILE
		make 
	fi

	./numa_memory_traffic_injector --cpu-node=1 --mem-node=1 --mem-size=128 --thread-num=8 &
}

if  [ "$ENABLE_TRAFFIC_INJECTION" = "1" ];then 
	inject_traffic
fi

#############################################
#   Start testing                           #
#############################################

#############################################
#   Start testing                           #
#############################################


log_time $LOG_FILE

#
# Programs such as python has its own output buffer, thus the pipe may not work.
# stdbuf is used to set the output buffer and let program immediately output information.
#
stdbuf -oL $APP_CMD | tee -a $APPLOG_FILE &

CHILD_PID="$?"

#begin to collect statistics
kill -ALRM $$

test_cleanup
