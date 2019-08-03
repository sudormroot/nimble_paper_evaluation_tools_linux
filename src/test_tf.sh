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
#   Use memhog to slow NUMA node #1         #
#############################################

echo "Inject traffic to slow memory on NUMA node #1 ..."

memhog_pids=""
i=0

while [ $i -lt $MEMHOG_NR ];do
	
done


#############################################
#   Use memhog to slow NUMA node #1         #
#############################################
pid=$$

# 1GB in numa node #0
sudo echo "1G" > /sys/fs/cgroup/$CGROUP/memory.max_at_node:0
echo "Current PID is $pid, attach current process and children into cgroup two-level-memory ..."

echo "$pid" > /sys/fs/cgroup/two-level-memory/cgroup.procs


