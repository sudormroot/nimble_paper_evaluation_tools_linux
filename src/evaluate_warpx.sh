#!/bin/sh

result_home="/home/jiaolin/warpx_pmdk_test/results_nimble"
warpx_exe="/home/jiaolin/warpx_pmdk_test/warpx_build/warpx_3d"
problem_home="/home/jiaolin/warpx_pmdk_test/warpx_problems/warpx_problems_repo"

rm -rf $result_home 2>/dev/zero

CPUNODES="0-1"
FASTNODES="0-1"


