#!/bin/sh



result_home="evaluation_nimble"
warpx_exe="../../warpx_pmdk_test/warpx_build/warpx_3d"
problem_home="../../warpx_pmdk_test/warpx_problems_repo"

script_dir="`dirname $0`"


CPU_NODE=any
FAST_NODE="0-1"
SLOW_NODE="2-3"
MPI_RANKS="6"
OMP_THREADS="8"
FAST_MEM_SIZE="128"
MIGRATION_THREADS="3"
MIGRATION_INTERVAL="1"
MAX_MANAGED_SIZE_MB="512"

$script_dir/launch_warpx.sh --cpu-node=$CPU_NODE \
                            --fast-node=$FAST_NODE \
                            --slow-node=$SLOW_NODE \
                            --mpi-ranks-num=$MPI_RANKS \
                            --omp-threads-num=$OMP_THREADS \
                            --fast-mem-size=$FAST_MEM_SIZE \
                            --migration-threads-num=$MIGRATION_THREADS \
                            --migration-interval=$MIGRATION_INTERVAL \
                            --managed-size=$MAX_MANAGED_SIZE_MB \
                            $warpx_exe $problem_home/


