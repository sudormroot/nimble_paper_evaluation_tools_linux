#!/bin/sh


result_dirs_list="`echo result_dir-*`"


min="0"
max="0"
avg="0"
sum="0"
n="0"

extract_data_one_dir(){
	result_dir="$1"

	training_speed_list="`cat $result_dir/applog.txt |grep examples|awk '{print $8}'|sed 's/(//g'`"
	fastmem_size="`cat $result_dir/config.txt |grep FAST_MEM_SIZE|cut -d= -f2`"

	for x in $training_speed_list;do

        echo "speed=$x"

        if [ "$min" = "0" ] || [ "$min" \< "$x" ];then
                min="$x"
        fi

        if [ "$max" = "0" ] || [ "$max" \< "$x" ];then
                max="$x"
        fi

        sum="`echo $sum + $x | bc`"

        n="`expr $n + 1`"
	done

	avg="`echo $sum / $n | bc`"

	echo "fastmem_size=$fastmem_size MB, training speed min=$min max=$max avg=$avg"
}

for dir in $result_dirs_list;do
	echo "dir=$dir"
	extract_data_one_dir $dir
done
