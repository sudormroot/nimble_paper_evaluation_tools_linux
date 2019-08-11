#!/bin/bash



#
# Fast node is #1
# Slow node is #0
#

SLOW_NODE=0
FAST_NODE=1


CGROUP="test"


MAX_OPEN_FILES=100000

DROP_CACHES_INTERVAL=3	    #drop caches for every 3 seconds	
STATS_COLLECT_INTERVAL=5    #collect system statistics every 5 seconds

MIGRATION_BATCH_SIZE=8

MAX_MEM_SIZE="0"
FAST_MEM_SIZE="0"
MIGRATION_THREADS_NUM="0"
ENABLE_TRAFFIC_INJECTION="0"
THP_MIGRATION="x"

KILL_TIMEOUT="0"

START_UNIXTIME="0"
CURRENT_UNIXTIME="0"


PROG_HOME="`dirname $0`"

show_usage() {
	#echo "$0 [--enable-traffic-injection] [--kill-timeout=<Seconds-to-Kill>] --thp-migration=<1|0> [--max-mem-size=<Size-in-MB>] --fast-mem-size=<Size-in-MB> --migration-threads-num=<Migration-Threads-Number> <Cmd> <Arg1> <Arg2> ..."
	echo "$0 [--kill-timeout=<Seconds-to-Kill>] --thp-migration=<1|0> [--max-mem-size=<Size-in-MB>] --fast-mem-size=<Size-in-MB> --migration-threads-num=<Migration-Threads-Number> <Cmd> <Arg1> <Arg2> ..."
}

if [ $# = 0 ];then
	show_usage
	exit
fi



#############################################
#   Get parameters                          #
#############################################

while [ 1 = 1 ] ; do
	case "$1" in
		-M|--kill-timeout=*) 
			KILL_TIMEOUT=`echo ${1#*=}`
			shift 1;;
		-M|--thp-migration=*) 
			THP_MIGRATION=`echo ${1#*=}`
			shift 1;;
		-i|--enable-traffic-injection) 
			ENABLE_TRAFFIC_INJECTION=1
			shift 1;;
		-m|--max-mem-size=*) 
			MAX_MEM_SIZE=`echo ${1#*=}`
			shift 1;;
		-m|--fast-mem-size=*) 
			FAST_MEM_SIZE=`echo ${1#*=}`
			shift 1;;
		-t|--migration-threads-num*) 
			MIGRATION_THREADS_NUM=`echo ${1#*=}`
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

# Year-Month-Day_Hour-Minute-Second_unixtime-nanoseconds
RESULT_DIR="result_dir-""thp_$THP_MIGRATION""_fastmem_$FAST_MEM_SIZE""MB""_threads_$MIGRATION_THREADS_NUM""-""`date '+%Y-%m-%d_%H-%M-%S_%s-%N'`"

LOG_FILE="$RESULT_DIR""/log.txt"
STATS_FILE="$RESULT_DIR""/stats.txt"
APPLOG_FILE="$RESULT_DIR""/applog.txt"
CONFIG_FILE="$RESULT_DIR""/config.txt"
FINISH_FILE="$RESULT_DIR""/__finished__"

if [ -f "$FINISH_FILE" ];then
	echo "Just skip $RESULT_DIR ..." | tee -a $LOG_FILE
	echo "If you still want to test, please remove the file $FINISH_FILE manually ..." | tee -a $LOG_FILE
	#just skip
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
	#echo "Time [Year-Month-Day Hour:Minute:Second UnixTime Nanoseconds] `date '+%Y-%m-%d %H:%M:%S %s %N'`" >> $1
	echo "Time: `date '+%Y-%m-%d %H:%M:%S %s %N'`" >> $1
}

log_time $LOG_FILE

echo "THP_MIGRATION=$THP_MIGRATION" | tee -a $LOG_FILE
echo "ENABLE_TRAFFIC_INJECTION=$ENABLE_TRAFFIC_INJECTION" | tee -a $LOG_FILE
echo "FAST_MEM_SIZE=$FAST_MEM_SIZE MB" | tee -a $LOG_FILE
echo "MIGRATION_THREADS_NUM=$FAST_MEM_SIZE" | tee -a $LOG_FILE
echo "KILL_TIMEOUT=$KILL_TIMEOUT" | tee -a $LOG_FILE
echo "FAST_NODE=$FAST_NODE" | tee -a $LOG_FILE
echo "SLOW_NODE=$SLOW_NODE" | tee -a $LOG_FILE


PROG_HOME="`dirname $0`"

#APP_CMD="numactl --cpunodebind=$FAST_NODE --preferred=$FAST_NODE $@" 
APP_CMD="$PROG_HOME/numa_launch --cpu-node=$FAST_NODE --fast-mem-node=$FAST_NODE -- $@" 


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
	
	child_pids="`ps --ppid $$|awk '{print $1}'|sed '1d'`"

	echo "Child pids: $child_pids"

	for pid in $child_pids;do
		echo "Kill child pid $pid ..."

		kill $pid 2>/dev/zero
		sleep 1
		kill -9 $pid 2>/dev/zero
		sleep 1

		echo "Wait child pid $pid to exit ..."
		#wait $pid
		while [ 1 = 1 ];do
			status="`ps --pid $pid|sed '1d'`"
			if [ "$status" = "" ];then
				break;
			fi
			sleep 1
		done
		echo "Child pid $pid exited."
	done


	#echo "Kill numa_memory_traffic_injector ..."
	#kill -9 $! 2>/dev/zero
	#sudo killall numa_memory_traffic_injector 2>/dev/zero

	echo "Remove /sys/fs/cgroup/$CGROUP ..."
	sudo rmdir /sys/fs/cgroup/$CGROUP 2>/dev/zero

	echo "Cleanup finished."

	exit
}

handle_signal_INT() {
	echo "INT signal is captured, cleaning up ..." | tee -a $LOG_FILE
	log_time $LOG_FILE
	test_cleanup
}

collect_stats(){
	#echo "CMD=sysctl vm" >> $STATS_FILE
	#sudo sysctl vm >> $STATS_FILE
	#echo "" >> $STATS_FILE

	#echo "CMD=numastat" >> $STATS_FILE
	#numastat >> $STATS_FILE
	#echo "" >> $STATS_FILE
	
	#echo "CMD=numastat -m" >> $STATS_FILE
	#numastat -m >> $STATS_FILE
	#echo "" >> $STATS_FILE

	#echo "CMD=numastat -p $$" >> $STATS_FILE
	#numastat -p $$ >>  $STATS_FILE
	#echo "" >> $STATS_FILE

	#echo "CMD=cat /proc/meminfo" >> $STATS_FILE
	#cat /proc/meminfo >> $STATS_FILE
	#echo "" >> $STATS_FILE

	#echo "CMD=cat /proc/zoneinfo" >> $STATS_FILE
	#cat /proc/zoneinfo >> $STATS_FILE
	#echo "" >> $STATS_FILE

	#echo "CMD=cat /proc/vmstat" >> $STATS_FILE
	#cat /proc/vmstat >> $STATS_FILE
	#echo "" >> $STATS_FILE

	#echo "CMD=cat /proc/$$/page_migration_stats" >> $STATS_FILE
	#cat /proc/$$/page_migration_stats >> $STATS_FILE
	#echo "" >> $STATS_FILE

	numa_launch_pid="`ps --ppid $$|grep numa_launch|awk '{print $1}'`"
	appname="`echo $APP_CMD|cut -d' ' -f5`"

	if [ "$numa_launch_pid" != "" ];then
		app_pid="`ps --ppid $numa_launch_pid|grep $appname|awk '{print $1}'`"

		if [ "$app_pid" != "" ]; then
			cat /proc/$app_pid/page_migration_stats | sed 's/\( \)/=/g' >> $STATS_FILE
			#cat /proc/$app_pid/page_migration_stats | sed 's/\( \)/=/g'
		fi
	fi

}


handle_signal_ALRM() {
	collect_stats
	sleep $STATS_COLLECT_INTERVAL

	CURRENT_UNIXTIME="`date '+%s'`"
	diff="`expr $CURRENT_UNIXTIME - $START_UNIXTIME`"
	
	#child_pids="`jobs -p`"

	numa_launch_pid="`ps --ppid $$|grep numa_launch|awk '{print $1}'`"

	#echo "numa_launch_pid=$numa_launch_pid"

	appname="`echo $APP_CMD|cut -d' ' -f5`"

	if [ "$numa_launch_pid" = "" ];then
		app_pid=""
	else
		app_pid="`ps --ppid $numa_launch_pid|grep $appname|awk '{print $1}'`"
	fi

	#Trigger Nimble kernel part to do migration
	$PROG_HOME/nimble_control 	--pid=$app_pid \
								--fast-mem-node=$FAST_NODE --slow-mem-node=$SLOW_NODE \
								--move-hot-and-cold-pages --exchange-pages >> $LOG_FILE 

	#echo "appname=$appname app_pid=$app_pid"

	if [ "$app_pid" = "" ]; then
		echo "Child process exited, cleanup ..." | tee -a $LOG_FILE
		echo "START_UNIXTIME=$START_UNIXTIME" | tee -a $LOG_FILE
		echo "CURRENT_UNIXTIME=$CURRENT_UNIXTIME" | tee -a $LOG_FILE
		echo "Elapsed $diff seconds" | tee -a $LOG_FILE
		echo "call test_cleanup() to gracefully stop running ..." | tee -a $LOG_FILE
		touch $FINISH_FILE
		test_cleanup
	fi

	if [ $diff -gt $KILL_TIMEOUT ] && [ $KILL_TIMEOUT -gt 0 ];then
		echo "Running time is out, time to stop ..." | tee -a $LOG_FILE
		echo "START_UNIXTIME=$START_UNIXTIME" | tee -a $LOG_FILE
		echo "CURRENT_UNIXTIME=$CURRENT_UNIXTIME" | tee -a $LOG_FILE
		echo "Elapsed $diff seconds" | tee -a $LOG_FILE
		echo "call test_cleanup() to gracefully stop running ..." | tee -a $LOG_FILE
		test_cleanup
	else
		kill -ALRM $$
	fi
}

trap "handle_signal_ALRM" ALRM
trap "handle_signal_INT"  INT


#############################################
#   Set parameters                          #
#############################################


echo "Set parameters ..." | tee -a $LOG_FILE

ulimit -n $MAX_OPEN_FILES
echo "Set maximum open files to $MAX_OPEN_FILES"


# remove old, if exists
sudo rmdir /sys/fs/cgroup/$CGROUP 2>/dev/zero

# create customized control-group 
sudo mkdir /sys/fs/cgroup/$CGROUP 2>/dev/zero

# enable memory control
echo "+memory" | sudo tee /sys/fs/cgroup/cgroup.subtree_control

if [ "$MAX_MEM_SIZE" != "0" ];then
	MAX_MEM_SIZE_BYTES="`expr $MAX_MEM_SIZE \\* 1024 \\* 1024`"
else
	MAX_MEM_SIZE_BYTES="max"
fi

echo "$MAX_MEM_SIZE_BYTES" | sudo tee /sys/fs/cgroup/$CGROUP/memory.max

sudo sysctl vm.sysctl_enable_thp_migration=$THP_MIGRATION
echo "Set vm.sysctl_enable_thp_migration=$THP_MIGRATION" | tee -a $LOG_FILE


#Disable linux auto-NUMA migration
echo "Disable Linux auto-NUMA migration" | tee -a $LOG_FILE
sudo sysctl kernel.numa_balancing=0

sudo sysctl vm/migration_batch_size=$MIGRATION_BATCH_SIZE
echo "Set vm/migration_batch_size=$MIGRATION_BATCH_SIZE"

FAST_MEM_SIZE_BYTES="`expr $FAST_MEM_SIZE \\* 1024 \\* 1024`"

echo "$FAST_MEM_SIZE_BYTES" | sudo tee /sys/fs/cgroup/$CGROUP/memory.max_at_node:$FAST_NODE
echo "Set /sys/fs/cgroup/$CGROUP/memory.max_at_node:$FAST_NODE to $FAST_MEM_SIZE MB" | tee -a $LOG_FILE

sudo sysctl vm/limit_mt_num=$MIGRATION_THREADS_NUM
echo "Set vm/limit_mt_num=$MIGRATION_THREADS_NUM" | tee -a $LOG_FILE

#sync
#sudo echo $DROP_CACHES_INTERVAL > /proc/sys/vm/drop_caches
#echo "Set /proc/sys/vm/drop_caches to $DROP_CACHES_INTERVAL"

pid=$$
echo "$pid" | sudo tee /sys/fs/cgroup/$CGROUP/cgroup.procs
echo "Set /sys/fs/cgroup/$CGROUP/cgroup.procs to $pid" | tee -a $LOG_FILE


#############################################
#   slow NUMA node #1                       #
#############################################

#inject_traffic() {
#	echo "Inject traffic to slow memory on NUMA node #1 ..." | tee -a $LOG_FILE
#
#	if [ ! -f "numa_memory_traffic_injector" ]; then
#		echo "numa_memory_traffic_injector not found, try make" | tee -a $LOG_FILE
#		make 
#	fi
#
#	$PROG_HOME/numa_memory_traffic_injector --cpu-node=$SLOW_NODE --mem-node=$SLOW_NODE --mem-size=256 --thread-num=16 &
#}
#
#if  [ "$ENABLE_TRAFFIC_INJECTION" = "1" ];then 
#	inject_traffic
#fi

#############################################
#   Start testing                           #
#############################################


log_time $LOG_FILE

#
# Programs such as python has its own output buffer, thus the pipe may not work.
# stdbuf is used to set the output buffer and let program immediately output information.
#
stdbuf -oL $APP_CMD | tee -a $APPLOG_FILE &

child_pids="`jobs -p`"

echo "Started child pids: $child_pids" | tee -a $LOG_FILE

START_UNIXTIME="`date '+%s'`"

#begin to collect statistics
kill -ALRM $$

test_cleanup
