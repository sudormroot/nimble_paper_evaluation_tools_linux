#!/bin/sh

#./launch_warpx.sh <mpi-ranks> <omp-threads> <fast-mem-size-in-mb> <num-of-migration-threads> <migration-interval-seconds> <warpx_cmd> <problem>


mkdir /home/jiaolin/warpx_pmdk_test/results_nimble 2>/dev/zero
chown -R jiaolin /home/jiaolin/warpx_pmdk_test/results_nimble 2>/dev/zero

result_home="/home/jiaolin/warpx_pmdk_test/results_nimble"
warpx_exe="/home/jiaolin/warpx_pmdk_test/warpx_build/warpx_3d"
problem_home="/home/jiaolin/warpx_pmdk_test/warpx_problems/laser-driven-acceleration"


problem="test_3d_64x64x512_steps_200"
problem_dir="$problem""_nimble_mpi_2_omp_8_migth_8_migint_1_steps_200"
fastmemsize="40"
mkdir $result_home/$problem_dir 2>/dev/zero
./launch_warpx.sh 2 8 $fastmemsize 8 1 $warpx_exe "$problem_home/$problem" > $result_home/$problem_dir/appoutput.txt


problem="test_3d_128x128x1024_steps_200" 
problem_dir="$problem""_nimble_mpi_2_omp_8_migth_8_migint_1_steps_200"
fastmemsize="310"
mkdir $result_home/$problem_dir 2>/dev/zero
./launch_warpx.sh 2 8 $fastmemsize 8 1 $warpx_exe "$problem_home/$problem" > $result_home/$problem_dir/appoutput.txt


problem="test_3d_256x256x2048_steps_200"
problem_dir="$problem""_nimble_mpi_2_omp_8_migth_8_migint_1_steps_200"
fastmemsize="2500"
mkdir $result_home/$problem_dir 2>/dev/zero
./launch_warpx.sh 2 8 $fastmemsize 8 1 $warpx_exe "$problem_home/$problem" > $result_home/$problem_dir/appoutput.txt


problem="test_3d_512x512x4096_steps_200"
problem_dir="$problem""_nimble_mpi_2_omp_8_migth_8_migint_1_steps_200"
fastmemsize="20000"
mkdir $result_home/$problem_dir 2>/dev/zero
./launch_warpx.sh 2 8 $fastmemsize 8 1 $warpx_exe "$problem_home/$problem" > $result_home/$problem_dir/appoutput.txt


problem="test_3d_864x864x7200" # 10 steps
problem_dir="$problem""_nimble_mpi_2_omp_8_migth_8_migint_1_steps_10"
fastmemsize="100000"
mkdir $result_home/$problem_dir 2>/dev/zero
./launch_warpx.sh 2 8 $fastmemsize 8 1 $warpx_exe "$problem_home/$problem" > $result_home/$problem_dir/appoutput.txt


problem="test_3d_960x960x7680_steps_10" # 10 steps
problem_dir="$problem""_nimble_mpi_2_omp_8_migth_8_migint_1_steps_10"
fastmemsize="135000"
mkdir $result_home/$problem_dir 2>/dev/zero
./launch_warpx.sh 2 8 $fastmemsize 8 1 $warpx_exe "$problem_home/$problem" > $result_home/$problem_dir/appoutput.txt

chown -R jiaolin:jiaolin /home/jiaolin/warpx_pmdk_test/results_nimble 2>/dev/zero

echo "--- Finished ---"
