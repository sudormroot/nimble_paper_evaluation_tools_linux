#!/bin/sh

sudo mkdir /sys/fs/cgroup/test 2>/dev/zero

FAST_MEM_SIZE="$1"
FAST_NODE=1
SLOW_NODE=0

if [ "$FAST_MEM_SIZE" = "" ];then
	echo "$0 <fastmem-size-in-mb>"
	exit
fi

FAST_MEM_SIZE_BYTES="`expr $FAST_MEM_SIZE \\* 1024 \\* 1024`"

echo "$FAST_MEM_SIZE_BYTES" | sudo tee /sys/fs/cgroup/$CGROUP/memory.max_at_node:$FAST_NODE
echo "Set /sys/fs/cgroup/$CGROUP/memory.max_at_node:$FAST_NODE to $FAST_MEM_SIZE MB" 

echo "$$" | sudo tee /sys/fs/cgroup/$CGROUP/cgroup.procs
echo "Set /sys/fs/cgroup/$CGROUP/cgroup.procs to $$" 

#sudo ./numa_launch --cgroup=/sys/fs/cgroup/test --cpu-node=1 --fast-mem-size=$memsize --fast-mem-node=1 --slow-mem-node=0 -- python ~/resnet-in-tensorflow/cifar10_train.py --num_residual_blocks=5 --report_freq=10
./numa_launch --cpu-node=$FAST_NODE --fast-mem-node=$FAST_NODE --slow-mem-node=$SLOW_NODE -- python ~/resnet-in-tensorflow/cifar10_train.py --num_residual_blocks=5 --report_freq=10
