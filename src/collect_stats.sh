#!/bin/sh

statsfile="$1"
interval="$2"
pid="$3"

if [ "$#" != 3 ];then
	echo "$0 <statsfile> <interval> <pid>"
	exit
fi

collect_stats(){
	echo "CMD=sysctl vm" >> $statsfile
	sudo sysctl vm >> $statsfile
	echo "" >> $statsfile

	echo "CMD=numastat" >> $statsfile
	numastat >> $statsfile
	echo "" >> $statsfile
	
	echo "CMD=numastat -m" >> $statsfile
	numastat -m >> $statsfile
	echo "" >> $statsfile

	echo "CMD=numastat -p $pid" >> $statsfile
	numastat -p $pid | tee -a $statsfile
	echo "" >> $statsfile

	echo "CMD=cat /proc/meminfo" >> $statsfile
	cat /proc/meminfo >> $statsfile
	echo "" >> $statsfile

	echo "CMD=cat /proc/zoneinfo" >> $statsfile
	cat /proc/zoneinfo >> $statsfile
	echo "" >> $statsfile

	echo "CMD=cat /proc/vmstat" >> $statsfile
	cat /proc/vmstat >> $statsfile
	echo "" >> $statsfile
}

handle_signal_ALRM() {
	collect_stats
	sleep $STATS_COLLECT_INTERVAL
	kill -ALRM $$
}

trap "handle_signal_ALRM" ALRM

# begin to collect stats
kill -ALRM $$

