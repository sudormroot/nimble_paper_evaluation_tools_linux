#!/bin/sh



result_home="evaluation_nimble_2"
warpx_exe="../../warpx_pmdk_test/warpx_build/warpx_3d"
problem_home="../../warpx_pmdk_test/warpx_problems_repo"

script_dir="`dirname $0`"


CPU_NODE="any"
FAST_NODE="0-1"
SLOW_NODE="2-3"
MPI_RANKS="6"
OMP_THREADS="8"
FAST_MEM_SIZE="128"
MIGRATION_THREADS="3"
MIGRATION_INTERVAL="1"
MAX_MANAGED_SIZE_MB="512"

test_problem(){
	FAST_MEM_SIZE="$1"
	problem="$2"

	problem_base="`basename $problem`"
	appout_dir="$result_home/$problem_base"
	appout="$appout_dir/appout.txt"

	echo "appout=$appout"

	rm -rf $appout_dir 2>/dev/zero
	mkdir -p $appout_dir 2>/dev/zero

	$script_dir/launch_warpx.sh --cpu-node=$CPU_NODE \
                            --fast-node=$FAST_NODE \
                            --slow-node=$SLOW_NODE \
                            --mpi-ranks-num=$MPI_RANKS \
                            --omp-threads-num=$OMP_THREADS \
                            --fast-mem-size=$FAST_MEM_SIZE \
                            --migration-threads-num=$MIGRATION_THREADS \
                            --migration-interval=$MIGRATION_INTERVAL \
                            --managed-size=$MAX_MANAGED_SIZE_MB \
                            $warpx_exe $problem | tee $appout
}

# fastmemsize is 12.8% of footprint


#test_problem 60020 $problem_home/3_species/laser_3d_512x512x4096_3_species
test_problem 40734 $problem_home/5_species/beam_3d_boost_512x512x4096_5_species

