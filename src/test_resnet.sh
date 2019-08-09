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
FASTMEM_SIZE_LIST="`seq 1024 256 4096`"


echo "FASTMEM_SIZE_LIST=`echo $FASTMEM_SIZE_LIST`"

program_home="`echo ~`"

program="$program_home/resnet-in-tensorflow/cifar10_train.py --num_residual_blocks=5 --report_freq=60 --train_steps=2000"


#num_residual_blocks : int. The total layers of the ResNet = 6 * num residual blocks + 2

for memsize in $FASTMEM_SIZE_LIST; do

	echo "memsize=$memsize MB"

	#--kill-timeout=$kill_timeout
	./launch_testee.sh      --thp-migration=0 \
            	            --fast-mem-size=$memsize \
                	        --migration-threads-num=$PER_NODE_THREADS \
                    	    python $program

	./launch_testee.sh      --thp-migration=1 \
            	            --fast-mem-size=$memsize \
                	        --migration-threads-num=$PER_NODE_THREADS \
                    	    python $program
done



