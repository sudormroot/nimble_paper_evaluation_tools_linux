#!/bin/bash


CGROUP="test"

RESULT_DIR="test_result_dir-""`date '+%Y%m%d_%H-%M-%S_%s-%N'`"

LOG_FILE="$RESULT_DIR""/log.txt"
STATS_FILE="$RESULT_DIR""/stats.txt"
APPLOG_FILE="$RESULT_DIR""/applog.txt"


MAX_OPEN_FILES=100000

DROP_CACHES_INTERVAL=3	   #drop caches for every 3 seconds	
STATS_COLLECT_INTERVAL=5   #collect system statistics every 5 seconds

MAX_MEM_SIZE="0"
FAST_MEM_SIZE="0"
MIGRATION_THREADS_NUM="0"
ENABLE_TRAFFIC_INJECTION="0"
THP_MIGRATION="x"


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
fi

log_time() {
	echo "Time: `date '+%Y-%m-%d %H:%M:%S %s:%N'`" | tee $1
}


echo "CMD : $0 $*" | tee $LOG_FILE
log_time $LOG_FILE
echo "" >> $LOG_FILE




#############################################
#   Get parameters                          #
#############################################

while [ 1 = 1 ] ; do
	case "$1" in
		-M|--thp_migration=*) 
			THP_MIGRATION=`echo ${1#*=}`
			echo "THP_MIGRATION=$THP_MIGRATION" | tee $LOG_FILE
			shift 1;;
		-i|--enable-traffic-injection) 
			ENABLE_TRAFFIC_INJECTION=1
			echo "ENABLE_TRAFFIC_INJECTION=$ENABLE_TRAFFIC_INJECTION" | tee $LOG_FILE
			shift 1;;
		-M|--max-mem-size=*) 
			MAX_MEM_SIZE=`echo ${1#*=}`
			echo "MAX_MEM_SIZE=$MAX_MEM_SIZE MB" | tee $LOG_FILE
			shift 1;;
		-m|--fast-mem-size=*) 
			FAST_MEM_SIZE=`echo ${1#*=}`
			echo "FAST_MEM_SIZE=$FAST_MEM_SIZE MB" | tee $LOG_FILE
			shift 1;;
		-t|--migration-threads-num*) 
			MIGRATION_THREADS_NUM=`echo ${1#*=}`
			echo "MIGRATION_THREADS_NUM=$FAST_MEM_SIZE" | tee $LOG_FILE
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

echo "APP_CMD: $APP_CMD"

echo "APP_CMD: $APP_CMD" > $LOG_FILE
echo "APP_CMD: $APP_CMD" > $APPLOG_FILE


collect_stats(){
	echo "CMD=sysctl vm" | tee $STATS_FILE
	log_time $STATS_FILE
	sudo sysctl vm | tee $STATS_FILE
	echo "" | tee $STATS_FILE

	echo "Time: `date '+%Y%m%d_%H-%M-%S'`" | tee $LOG_FILE
	echo "=== NUMA stat for basic  ===" | tee $LOG_FILE

	echo "CMD=numastat" | tee $STATS_FILE
	log_time $STATS_FILE
	numastat | tee $STATS_FILE
	echo "" | tee $STATS_FILE
	
	echo "CMD=numastat -m" | tee $STATS_FILE
	log_time $STATS_FILE
	numastat -m | tee $STATS_FILE
	echo "" | tee $STATS_FILE

	echo "CMD=cat /proc/meminfo" | tee $STATS_FILE
	log_time $STATS_FILE
	cat /proc/meminfo | tee $STATS_FILE
	echo "" | tee $STATS_FILE

	echo "CMD=cat /proc/zoneinfo" | tee $STATS_FILE
	log_time $STATS_FILE
	cat /proc/zoneinfo | tee $STATS_FILE
	echo "" | tee $STATS_FILE

	echo "CMD=cat /proc/vmstat" | tee $STATS_FILE
	log_time $STATS_FILE
	cat /proc/vmstat | tee $STATS_FILE
	echo "" | tee $STATS_FILE
}

test_cleanup() {
	echo "Time: `date '+%Y%m%d_%H-%M-%S'`" | tee $LOG_FILE

	kill -9 $?
	killall numa_memory_traffic_injector 2>/dev/zero

	exit
}

handle_signal_INT() {
	echo "INT signal is captured, cleaning up ..." | tee $LOG_FILE
	test_cleanup
}

handle_signal_ALRM() {
	collect_stats
	sleep $STATS_COLLECT_INTERVAL
	kill -ALRM $$
}

trap "handle_signal_INT"  INT
trap "handle_signal_ALRM" ALRM


#############################################
#   Set parameters                          #
#############################################


echo "Set parameters ..." | tee $LOG_FILE

sudo ulimit -n $MAX_OPEN_FILES
echo "Set maximum open files to $MAX_OPEN_FILES"

# create customized control-group named two-level-memory
sudo mkdir /sys/fs/cgroup/$CGROUP 2>/dev/zero

# enable memory control
sudo echo "+memory" > /sys/fs/cgroup/cgroup.subtree_control

sudo echo "$MAX_MEM_SIZE""M" > /sys/fs/cgroup/$CGROUP/memory.max
echo "memory.max is set to $MAX_MEM_SIZE MB" | tee $LOG_FILE

sudo sysctl vm.sysctl_enable_thp_migration=$THP_MIGRATION
echo "Set vm.sysctl_enable_thp_migration=$THP_MIGRATION" | tee $LOG_FILE


sudo echo "$FAST_MEM_SIZE""M" > /sys/fs/cgroup/$CGROUP/memory.max_at_node:0
echo "Set /sys/fs/cgroup/$CGROUP/memory.max_at_node:0 to $FAST_MEM_SIZE MB" | tee $LOG_FILE

sudo sysctl vm/limit_mt_num=$MIGRATION_THREADS_NUM
echo "Set vm/limit_mt_num=$MIGRATION_THREADS_NUM" | tee $LOG_FILE

sync

sudo echo $DROP_CACHES_INTERVAL > /proc/sys/vm/drop_caches
echo "Set /proc/sys/vm/drop_caches to $DROP_CACHES_INTERVAL"

pid=$$
echo "$pid" > /sys/fs/cgroup/$CGROUP/cgroup.procs
echo "Set /sys/fs/cgroup/$CGROUP/cgroup.procs to $pid" | tee $LOG_FILE


#############################################
#   slow NUMA node #1                       #
#############################################

inject_traffic() {
	echo "Inject traffic to slow memory on NUMA node #1 ..." | tee $LOG_FILE

	if [ ! -f "numa_memory_traffic_injector" ]; then
		echo "numa_memory_traffic_injector not found, try make" | tee $LOG_FILE
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

collect_stats

echo "Time: `date '+%Y%m%d_%H-%M-%S'`" | tee $LOG_FILE

$APP_CMD | tee $APPLOG_FILE &

#echo "Child PID: $!"

# Begin to collect system statistics
kill -ALRM $$

test_cleanup
