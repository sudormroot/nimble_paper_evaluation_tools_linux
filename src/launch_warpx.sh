#!/bin/bash



if [ "$#" -ne 7 ];then
    echo "$0 <mpi-ranks> <omp-threads> <fast-mem-size-in-mb> <num-of-migration-threads> <migration-interval-seconds> <warpx_cmd> <problem>"
    exit
fi

MPI_RANKS="$1"
OMP_THREADS="$2"

SLOW_NODE=2
FAST_NODE=0


CGROUP="test_optane"

MAX_MEM_SIZE_BYTES=0

MAX_OPEN_FILES=100000

DROP_CACHES_INTERVAL=3	    #drop caches for every 3 seconds	

MIGRATION_BATCH_SIZE=8

MAX_MEM_SIZE="0"
FAST_MEM_SIZE="$3"
MIGRATION_THREADS_NUM="$4"
THP_MIGRATION="0"

MIGRATION_INTERVAL="$5"

WARPX_EXE="$6"
WARPX_PROBLEM="$7"

PROG_HOME="`dirname $0`"


echo "THP_MIGRATION=$THP_MIGRATION" 
echo "ENABLE_TRAFFIC_INJECTION=$ENABLE_TRAFFIC_INJECTION"
echo "FAST_MEM_SIZE=$FAST_MEM_SIZE MB" 
echo "MIGRATION_THREADS_NUM=$FAST_MEM_SIZE" 
echo "KILL_TIMEOUT=$KILL_TIMEOUT" 
echo "FAST_NODE=$FAST_NODE"
echo "SLOW_NODE=$SLOW_NODE"
echo "MIGRATION_INTERVAL=$MIGRATION_INTERVAL"
echo "WARPX_EXE=$WARPX_EXE"
echo "WARPX_PROBLEM=$WARPX_PROBLEM"



#
# cleanup
#
killall warpx_3d 2>/dev/zero
killall numa_launch 2>/dev/zero
killall nimble_control 2>/dev/zero


echo "Remove /sys/fs/cgroup/$CGROUP ..."
#echo ""|sudo tee /sys/fs/cgroup/"$CGROUP"/cgroup.procs 2>/dev/zero
sudo rmdir /sys/fs/cgroup/"$CGROUP" 2>/dev/zero
echo "Cleanup finished."

#
# do migration here
#

NIMBLE_CONTROL_OPTIONS="--exchange-pages"

handle_signal_ALRM() {
	
    check_status="`ps -ax|grep numa_launch|sed '/grep/d'`"

    if [ "$check_status" = "" ];then
        echo "Detected warpx exited."
        sudo killall warpx_3d 2>/dev/zero
        sudo rmdir /sys/fs/cgroup/"$CGROUP" 2>/dev/zero
        sudo rmdir /sys/fs/cgroup/test_* 2>/dev/zero
        exit
    fi

    child_pids="`ps -ax|grep warpx|sed '/grep/d'|sed '/mpirun/d'|sed '/numa_launch/d'|sed '/sh/d'|awk '{print $1}'`"

	#echo "Warpx pids: $child_pids"

	for pid in $child_pids;do
		#echo "pid $pid ..."


	    if [ "$pid" != "" ];then
		    #echo "Page migration start ..."
		    #Trigger Nimble kernel part to do migration
		    $PROG_HOME/nimble_control --pid=$pid --fast-mem-node=$FAST_NODE --slow-mem-node=$SLOW_NODE $NIMBLE_CONTROL_OPTIONS
		    #echo "Page migration start end: ret=$?"
	    fi
    done
    
    sleep $MIGRATION_INTERVAL
    
    # start again
    kill -ALRM $$
}

handle_signal_INT() {
    echo "Cleaning ..."
    killall warpx_3d 2>/dev/zero
    sudo rmdir /sys/fs/cgroup/"$CGROUP" 2>/dev/zero
    sudo rmdir /sys/fs/cgroup/test_* 2>/dev/zero
    exit
}

trap "handle_signal_ALRM" ALRM
trap "handle_signal_INT"  INT




echo "Set parameters ..." 

ulimit -n $MAX_OPEN_FILES
echo "Set maximum open files to $MAX_OPEN_FILES"


# remove old, if exists
sudo rmdir /sys/fs/cgroup/"$CGROUP" 2>/dev/zero
sudo rmdir /sys/fs/cgroup/test_* 2>/dev/zero

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
echo "Set vm.sysctl_enable_thp_migration=$THP_MIGRATION"


#Disable linux auto-NUMA migration
echo "Disable Linux auto-NUMA migration" 
sudo sysctl kernel.numa_balancing=0

sudo sysctl vm/migration_batch_size=$MIGRATION_BATCH_SIZE
echo "Set vm/migration_batch_size=$MIGRATION_BATCH_SIZE"

FAST_MEM_SIZE_BYTES="`expr $FAST_MEM_SIZE \\* 1024 \\* 1024`"

echo "$FAST_MEM_SIZE_BYTES" | sudo tee /sys/fs/cgroup/$CGROUP/memory.max_at_node:$FAST_NODE
echo "Set /sys/fs/cgroup/$CGROUP/memory.max_at_node:$FAST_NODE to $FAST_MEM_SIZE MB" 

sudo sysctl vm/limit_mt_num=$MIGRATION_THREADS_NUM
echo "Set vm/limit_mt_num=$MIGRATION_THREADS_NUM"

#sync
#sudo echo $DROP_CACHES_INTERVAL > /proc/sys/vm/drop_caches
#echo "Set /proc/sys/vm/drop_caches to $DROP_CACHES_INTERVAL"

pid=$$
echo "$pid" | sudo tee /sys/fs/cgroup/$CGROUP/cgroup.procs
echo "Set /sys/fs/cgroup/$CGROUP/cgroup.procs to $pid" 


#mkdir results_nimble

#echo "APP_CMD=$APP_CMD"
#APP_CMD="$PROG_HOME/numa_launch --cpu-node=$FAST_NODE --fast-mem-node=$FAST_NODE -- $WARPX_EXE $WARPX_PROBLEM" 

problem_name="`basename $WARPX_PROBLEM`"

OMP_NUM_THREADS=$OMP_THREADS mpirun -np $MPI_RANKS $PROG_HOME/numa_launch --cpu-node=$FAST_NODE --slow-mem-node=$SLOW_NODE --fast-mem-node=$FAST_NODE -- $WARPX_EXE $WARPX_PROBLEM &

#stdbuf -oL $APP_CMD 2>&1 | tee -a results_nimble/appoutput.txt &

sleep 5

child_pids="`jobs -p`"

echo "Started child pids: $child_pids" 

kill -ALRM $$

