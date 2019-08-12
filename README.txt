
1. Summary

This tool set is used for evaulating Linux Nimble memory migration algorithm, the author's webpage is on:
https://normal.zone/blog/2019-01-27-nimble-page-management/

https://github.com/ysarch-lab/nimble_page_management_asplos_2019


2. Build tools:

check releases
$ git tag --list

$ git clone https://github.com/sudormroot/memory_test_tools_linux.git 

$ cd memory_test_tools_linux/src 

$ make clean && make



3. extract_resnet_speed.sh
Used to collect generated evaluation data from test_resnet.sh

4. launch_testee.sh
Used to launch an evaluation application:

./launch_testee.sh [--enable-traffic-injection] [--kill-timeout=<Seconds-to-Kill>] --thp-migration=<1|0> [--max-mem-size=<Size-in-MB>] --fast-mem-size=<Size-in-MB> --migration-threads-num=<Migration-Threads-Number> <Cmd> <Arg1> <Arg2> ...

5. nimble_control
Send page migration calls to Nimble kernel part.

6. numa_launch
numactl seems not working well with Linux cgroupv2 unified mode and Nimble kernel, numa_launch is written to replace numactl.

7. numa_memory_access_time
Test NUMA machine memory access latency data.

8. numa_memory_hog
Eat/Reserve memory on specific NUMA node.

9. numa_memory_traffic_injector
Inject memory traffic on specific NUMA node.

10. test_resnet.sh
Scripts used to evaluate deep learning resnet network.
