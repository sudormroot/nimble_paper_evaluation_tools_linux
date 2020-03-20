#!/bin/sh

#./launch_warpx.sh <mpi-ranks> <omp-threads> <fast-mem-size-in-mb> <num-of-migration-threads> <migration-interval-seconds> <warpx_cmd> <problem>

./launch_warpx.sh 2 8 40 8 1 /home/jiaolin/warpx_pmdk_test/warpx_build/warpx_3d /home/jiaolin/warpx_pmdk_test/warpx_problems/laser-driven-acceleration/test_3d_64x64x512
