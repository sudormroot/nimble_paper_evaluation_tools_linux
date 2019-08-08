#!/bin/sh

#
# Disable THP migration
# Set maximum memory size to 2048MB
# Set fast memory size to 128MB
#

threads_num="`cat /proc/cpuinfo|grep processor|wc -l`"

thp_migration=0

max_mem_size="`cat /proc/meminfo |grep MemTotal|cut -d: -f2|cut -dk -f1`"
max_mem_size="`expr $max_mem_size / 1024`"

fast_mem_size=1024

echo "Detected memory size: $max_mem_size MB"
echo "Detected cpus       : $threads_num"

python_program="~/resnet-in-tensorflow/cifar10_train.py"



if [ ! -f "$python_program" ];then
	echo "$python_program not found"
	exit
fi

./launch_testee.sh      --thp_migration=$thp_migration \
                        --max-mem-size=$max_mem_size \
                        --fast-mem-size=$fast_mem_size \
                        --migration-threads-num=$threads_num \
                        python $python_program


