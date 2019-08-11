/*
 *
 * Author: Jiaolin Luo 
 *
 * */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/wait.h>

#include <numa.h>
#include <numaif.h>

#include "cpu_util.h"
#include "cpu_freq.h"


static struct option long_options [] = 
{
	{"cpu-node", 	required_argument, 0, 'C'},
	{"mem-node", 	required_argument, 0, 'M'},
	{"mem-size", 	required_argument, 0, 'm'},
	{"thread-num", 	required_argument, 0, 'T'},
	{0,0,0,0}
};

static void inject_memory_traffic(void *ptr, size_t size)
{
	int i = 0;
	char *pos = NULL;
	char c;

	printf("Injecting traffic, pid=%d ...\n",getpid());

	while(1) {
		pos = (char *)ptr + i++ % size;
		mfence();
		c = *pos;
		mfence();
		*pos = i % 255;
		clflush(pos);
		mfence();
	}

}

int main(int argc, char **argv)
{

	struct bitmask *cpu_mask = NULL;
	struct bitmask *mem_mask = NULL;

	
	int mem_node = 0;
	int cpu_node = 0;

	int nodes_nr;

	size_t mem_size=0;

	char c;
	int option_index = 0;


	void *ptr = NULL;

	int thread_num = 0;

	pid_t *child_pids = NULL;
	pid_t pid;

	int i = 0;

	int child_status;

	double freq;  /*CPU freq*/

	time_t seed;


	srand((unsigned) time(&seed));

	setbuf(stdout, NULL);

	if(argc != 5) {
		printf("%s --cpu-node=<NUMA CPU Node> --mem-node=<NUMA Memory Node> --mem-size=<Size-in-MB> --thread-num=<Thread-Number>\n", argv[0]);
		exit(0);
	}


	while ((c = getopt_long(argc, argv, "C:M:m:T:", long_options, &option_index)) != -1) {
		switch (c) {
			//case 0:
			//	if (long_options[option_index].flag != 0)
			//		break;
			//
			//	printf ("option %s = %s\n", long_options[option_index].name, optarg?optarg:"NULL");
			//	break;
			case 'C':
				cpu_mask = numa_parse_nodestring(optarg);
				printf("cpu-node: %s\n", optarg);
				cpu_node = atoi(optarg);
				break;
			case 'M':
				mem_mask = numa_parse_nodestring(optarg);
				printf("mem-node: %s\n", optarg);
				mem_node = atoi(optarg);
				break;
			case 'm':
				mem_size = atol(optarg) << 20;
				printf("mem-size: %s MB\n", optarg);
				break;
			case 'T':
				thread_num = atol(optarg);
				printf("thread-num: %s MB\n", optarg);
				break;
			default:
				abort();
		}
	}


	printf("Inject traffic on memory node %d, executing on cpu node %d, ...\n", mem_node, cpu_node);

	nice(-20);

	/*
	 * Bind process to cpu node
	 * */

		
	if(numa_run_on_node_mask_all(cpu_mask)) {
		perror("numa_run_on_node_mask_all() failed");
		exit(-1);
	}

	//printf("Pid #%d is bound to cpu node #%d.\n", getpid(), cpu_node);

	

	freq = measure_cpu_freq();


	/*
	 * Bind process to cpu node
	 * */


	if (set_mempolicy(MPOL_BIND, mem_mask->maskp, mem_mask->size + 1) < 0) {
		perror("set_mempolicy() failed\n");
		exit(-1);
	}

	//printf("Bind memory allocation to node #%d.\n", mem_node);


	/*
	 *
	 * prepare cache-line aligned memory
	 *
	 * */
	
	
	ptr = numa_alloc_onnode(mem_size, mem_node);

	if(ptr == NULL) {
		perror("numa_alloc_onnode() failed\n");
		exit(-1);
	}
	
	/*
	 * Spawn child processes
	 *
	 * */

	child_pids = numa_alloc_onnode(sizeof(pid_t) * thread_num, mem_node);

	if(child_pids == NULL) {
		perror("numa_alloc_onnode() failed\n");
		exit(-1);
	}

	memset(child_pids, 0, sizeof(pid_t) * thread_num);

	for(i = 0; i < thread_num; i++) {

		pid = fork();

		if(pid != 0) {
			// in parent
			child_pids[i] = pid;
		} else {
			// in child

			inject_memory_traffic(ptr, mem_size);

			exit(0); //make CC happy
		}
	}

	//make CC happy
	for(i = 0; i < thread_num; i++)
		waitpid(child_pids[i], &child_status, 0);


	numa_free_nodemask(cpu_mask);
	numa_free_nodemask(mem_mask);

	numa_free(ptr, mem_size);
	numa_free(child_pids, sizeof(pid_t) * thread_num);

	return 0;
}

