#!/bin/bash



FAST_NODE="0"
SLOW_NODE="2"
CPU_NODE="$SLOW_NODE"

MPI_RANKS="1"
OMP_THREADS="1"

CGROUP="test_warpx_$$"

MAX_MEM_SIZE_BYTES=0

MAX_OPEN_FILES=100000

DROP_CACHES_INTERVAL=3	    #drop caches for every 3 seconds	

MIGRATION_BATCH_SIZE=8


PAGE_SIZE=4096
MAX_MANAGED_SIZE_MB=512   #unit is in MiB
MAX_MANAGED_PAGES=`echo "($MAX_MANAGED_SIZE_MB * 1048576) / $PAGE_SIZE" | bc`

MAX_MEM_SIZE="0" 
FAST_MEM_SIZE="0"
MIGRATION_THREADS_NUM="1"
THP_MIGRATION="0"

MIGRATION_INTERVAL="1"


PROG_HOME="`dirname $0`"

#############################################
#   Get parameters                          #
#############################################

show_usage(){
    echo "$0 [--cpu-node=<cpu-node>] --fast-node=<fast-node> --slow-node=<slow-node> --mpi-ranks-num=<mpi-ranks> --omp-threads-num=<omp-threads> --fast-mem-size=<fast-mem-size-in-mb> --migration-threads-num=<num-of-migration-threads> --migration-interval=<migration-interval-seconds> --managed-size=<managed-size-in-mb> <warpx_cmd> <problem>"
}

if [ "$#" = "0" ];then
    show_usage
    exit
fi

while [ 1 = 1 ] ; do
	case "$1" in
		-c|--cpu-node=*) 
			CPU_NODE=`echo ${1#*=}`
			shift 1;;
		-f|--fast-node=*) 
			FAST_NODE=`echo ${1#*=}`
			shift 1;;
		-s|--slow-node=*) 
			FAST_NODE=`echo ${1#*=}`
			shift 1;;
		-S|--managed-size=*) 
			MAX_MANAGED_SIZE_MB=`echo ${1#*=}`
			shift 1;;
		-r|--mpi-ranks-num=*) 
			MPI_RANKS=`echo ${1#*=}`
			shift 1;;
		-o|--omp-threads-num=*) 
			OMP_THREADS=`echo ${1#*=}`
			shift 1;;
		-T|--thp-migration=*) 
			THP_MIGRATION=`echo ${1#*=}`
			shift 1;;
		-M|--max-mem-size=*) 
			MAX_MEM_SIZE=`echo ${1#*=}`
			shift 1;;
		-m|--fast-mem-size=*) 
			FAST_MEM_SIZE=`echo ${1#*=}`
			shift 1;;
		-t|--migration-threads-num*) 
			MIGRATION_THREADS_NUM=`echo ${1#*=}`
			shift 1 ;;
		-t|--migration-interval*) 
			MIGRATION_INTERVAL=`echo ${1#*=}`
			shift 1 ;;
		--) 
			shift 1
			break ;;
		*) 
			break ;;
	esac
done


#if [ "$THP_MIGRATION" != "0" ] && [ "$THP_MIGRATION" != "1" ]; then
#	echo "Required --thp_migration=<1|0>"
#	show_usage
#	exit
#fi


if [ "$FAST_MEM_SIZE" = "0" ]; then
	echo "Required --fast-mem-size=<Size-in-MB>"
	show_usage
	exit
fi

#if [ "$MIGRATION_THREADS_NUM" = "0" ]; then
#	echo "Required --migration-threads-num=<Migration-Threads-Number>"
#	show_usage
#	exit
#fi


WARPX_EXE="$1"
WARPX_PROBLEM="$2"

if [ ! -x "$WARPX_EXE" ];then
    echo "warpx execution binary not found"
    show_usage
    exit
fi

if [ ! -f "$WARPX_PROBLEM" ];then
    echo "warpx input problem not found"
    show_usage
    exit
fi

echo "THP_MIGRATION=$THP_MIGRATION" 
#echo "ENABLE_TRAFFIC_INJECTION=$ENABLE_TRAFFIC_INJECTION"
echo "FAST_MEM_SIZE=$FAST_MEM_SIZE MB" 
echo "MIGRATION_BATCH_SIZE=$MIGRATION_BATCH_SIZE"
echo "MIGRATION_THREADS_NUM=$MIGRATION_THREADS_NUM" 
#echo "KILL_TIMEOUT=$KILL_TIMEOUT" 
echo "CPU_NODE=$CPU_NODE"
echo "FAST_NODE=$FAST_NODE"
echo "SLOW_NODE=$SLOW_NODE"
echo "MIGRATION_INTERVAL=$MIGRATION_INTERVAL"
echo "WARPX_EXE=$WARPX_EXE"
echo "WARPX_PROBLEM=$WARPX_PROBLEM"
echo "MAX_MANAGED_SIZE_MB=$MAX_MANAGED_SIZE_MB MiB"
echo "MAX_MANAGED_PAGES=$MAX_MANAGED_PAGES"

#exit

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

        #fastnode_list="`echo $FAST_NODE | sed 's/-/ /g'`"
        fastnode_num="`echo $FAST_NODE |grep -o '-'|wc -l`"

        if [ "$fastnode_num" = "0" ];then
            fastnode_num=1
        fi

	    if [ "$pid" != "" ];then

            killall nimble_control 2>/dev/zero

            for i in "`seq $fastnode_num`";do
                fastnode="`echo $FAST_NODE|cut -d- -f$i`"
                slownode="`echo $SLOW_NODE|cut -d- -f$i`"
		        $PROG_HOME/nimble_control --pid=$pid --managed-pages=$MAX_MANAGED_PAGES --fast-mem-node=$fastnode --slow-mem-node=$slownode $NIMBLE_CONTROL_OPTIONS &
            done

		    #echo "Page migration start ..."
		    #Trigger Nimble kernel part to do migration
		    #$PROG_HOME/nimble_control --pid=$pid --fast-mem-node=0 --slow-mem-node=2 $NIMBLE_CONTROL_OPTIONS
		    #$PROG_HOME/nimble_control --pid=$pid --fast-mem-node=1 --slow-mem-node=3 $NIMBLE_CONTROL_OPTIONS
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



fastnode_list="`echo $FAST_NODE | sed 's/-/ /g'`"
fastnode_num="`echo $FAST_NODE |grep -o '-'|wc -l`"

if [ "$fastnode_num" = "0" ];then
    fastnode_num=1
fi

FAST_MEM_SIZE_BYTES="`echo $FAST_MEM_SIZE_BYTES / $fastnode_num |bc`"
FAST_MEM_SIZE="`echo $FAST_MEM_SIZE / $fastnode_num |bc`"


for fastnode in $fastnode_list;do
    echo "$FAST_MEM_SIZE_BYTES" | sudo tee /sys/fs/cgroup/$CGROUP/memory.max_at_node:$fastnode
    echo "Set /sys/fs/cgroup/$CGROUP/memory.max_at_node:$fastnode to $FAST_MEM_SIZE MB" 

    #echo "$FAST_MEM_SIZE_BYTES" | sudo tee /sys/fs/cgroup/$CGROUP/memory.max_at_node:1
    #echo "Set /sys/fs/cgroup/$CGROUP/memory.max_at_node:1 to $FAST_MEM_SIZE MB" 
done

sudo sysctl vm/limit_mt_num=$MIGRATION_THREADS_NUM
echo "Set vm/limit_mt_num=$MIGRATION_THREADS_NUM"

#sync
#sudo echo $DROP_CACHES_INTERVAL > /proc/sys/vm/drop_caches
#echo "Set /proc/sys/vm/drop_caches to $DROP_CACHES_INTERVAL"

pid=$$
echo "$pid" | sudo tee /sys/fs/cgroup/$CGROUP/cgroup.procs
echo "Set /sys/fs/cgroup/$CGROUP/cgroup.procs to $pid" 



problem_name="`basename $WARPX_PROBLEM`"

OMP_NUM_THREADS=$OMP_THREADS mpirun -np $MPI_RANKS $PROG_HOME/numa_launch --cpu-node=$CPU_NODE --slow-mem-node=$SLOW_NODE --fast-mem-node=$FAST_NODE -- $WARPX_EXE $WARPX_PROBLEM &

#stdbuf -oL $APP_CMD 2>&1 | tee -a results_nimble/appoutput.txt &

sleep 5

child_pids="`jobs -p`"

echo "Started child pids: $child_pids" 

kill -ALRM $$

