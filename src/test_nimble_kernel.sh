#!/bin/bash


CGROUP="two-level-memory"
MAXMEM_SIZE_LIMIT="32G"
FASTMEM_SIZE_LIMIT="`seq 1 1 24`"  # from 1,2,3,....,24 respectively

CORES_NR="`cat /proc/cpuinfo|grep processor|wc -l`"
MAXTHREAD_NR="`seq 1 1 $CORES_NR`" # testing threads


APP_CMD="find /bin" 

MEMHOG="/usr/bin/memhog"
MEMHOG_NR=3

#############################################
#   Set basic memory allocation policies    #
#############################################


echo "Set basic memory allocation policies ..."
# create customized control-group named two-level-memory
sudo mkdir /sys/fs/cgroup/$CGROUP

# enable memory control
sudo echo "+memory" > /sys/fs/cgroup/cgroup.subtree_control

sudo echo "$MAXMEM_SIZE_LIMIT" > /sys/fs/cgroup/$CGROUP/memory.max
echo "MAXMEM_SIZE_LIMIT=$MAXMEM_SIZE_LIMIT"

#############################################
#   Add current process to cgroup           #
#############################################


pid=$$
echo "$pid" > /sys/fs/cgroup/two-level-memory/cgroup.procs

#############################################
#   Use memhog to slow NUMA node #1         #
#############################################

echo "Inject traffic to slow memory on NUMA node #1 ..."

numa_memory_traffic_injector --cpu-node=1 --mem-node=1 --mem-size=128 --thread-num=8


#############################################
#   Use memhog to slow NUMA node #1         #
#############################################

sudo sysctl vm.sysctl_enable_thp_migration=1

for mem in $FASTMEM_SIZE_LIMIT; do
	
	sudo echo "$mem""G" > /sys/fs/cgroup/two-level-memory/memory.max_at_node:0

	for threads in $MAXTHREAD_NR; do
		sudo sysctl vm/limit_mt_num=$threads

		sudo sysctl vm.sysctl_enable_thp_migration=1
		echo "mem=$mem threads=$threads ..."

		exec $APP_CMD

		sudo sysctl vm.sysctl_enable_thp_migration=0
	done
done

sudo sysctl vm.sysctl_enable_thp_migration=1

