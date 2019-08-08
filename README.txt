
Build tools:

$ git clone https://github.com/sudormroot/memory_test_tools_linux.git
$ cd memory_test_tools_linux/src 
$ make clean && make

Launch your application:

./launch_testee.sh [--enable-traffic-injection] --thp-migration=<1|0> --max-mem-size=<Size-in-MB> --fast-mem-size=<Size-in-MB> --migration-threads-num=<Migration-Threads-Number> <Cmd> <Arg1> <Arg2> ...


