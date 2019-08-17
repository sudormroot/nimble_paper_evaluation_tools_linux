#!/bin/sh

#
# Disable THP migration
# Set maximum memory size to 2048MB
# Set fast memory size to 128MB
#

CPUS_NUM="`cat /proc/cpuinfo|grep processor|wc -l`"

thp_migration=0

MEM_SIZE="`cat /proc/meminfo |grep MemTotal|cut -d: -f2|cut -dk -f1`"
#MEM_SIZE="`expr $MEM_SIZE / 1024`"


#echo "Detected memory size: $MEM_SIZE MB"
echo "Detected cpus       : $CPUS_NUM"




#30 minutes
#kill_timeout="`expr 30 * 60`"


PER_NODE_THREADS="`expr $CPUS_NUM / 2`"
#PER_NODE_MEM_SIZE="`expr $MEM_SIZE / 2`"


echo "PER_NODE_THREADS=$PER_NODE_THREADS"
#echo "PER_NODE_MEM_SIZE=$PER_NODE_MEM_SIZE"

#1G 2G 3G
#FASTMEM_SIZE_FROM="128"
#FASTMEM_SIZE_TO="8192"
#FASTMEM_SIZE_STEP="256"

FASTMEM_SIZE_LIST="`seq 128 256 8192`"


echo "FASTMEM_SIZE_LIST=`echo $FASTMEM_SIZE_LIST`"

program_home="`echo ~`"

program="$program_home/resnet-in-tensorflow/cifar10_train.py --num_residual_blocks=5 --report_freq=60 --train_steps=1500"


#num_residual_blocks : int. The total layers of the ResNet = 6 * num residual blocks + 2

for memsize in $FASTMEM_SIZE_LIST; do

	echo "fastmemsize=$memsize MB thp=0"

	#--kill-timeout=$kill_timeout
	./launch_testee.sh      --thp-migration=0 \
            	            --fast-mem-size=$memsize \
                	        --migration-threads-num=$PER_NODE_THREADS \
                    	    python $program "--version=test_fastmem_$memsize""_MB_thp_0"

	echo "fastmemsize=$memsize MB thp=1"

	./launch_testee.sh      --thp-migration=1 \
            	            --fast-mem-size=$memsize \
                	        --migration-threads-num=$PER_NODE_THREADS \
                    	    python $program "--version=test_fastmem_$memsize""_MB_thp_1"
done



