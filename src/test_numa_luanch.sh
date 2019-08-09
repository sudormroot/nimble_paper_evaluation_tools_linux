#!/bin/sh

sudo mkdir /sys/fs/cgroup/test 2>/dev/zero

memsize="$1"

if [ "$memsize" = "" ];then
	echo "$0 <fastmem-size-in-mb>"
	exit
fi

sudo ./numa_launch --cgroup=/sys/fs/cgroup/test --cpu-node=1 --fast-mem-size=$memsize --fast-mem-node=1 --slow-mem-node=0 -- python ~/resnet-in-tensorflow/cifar10_train.py --num_residual_blocks=5 --report_freq=10
