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
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <getopt.h>

#include <numa.h>
#include <numaif.h>

#include "cpu_util.h"

static struct option long_options [] = 
{
	{"node", 	required_argument, 0, 'n'},
	{"size", 	required_argument, 0, 's'},
	{0,0,0,0}
};

int main(int argc, char **argv)
{

	
	int node = 0;

	size_t size=0;

	char c;
	int option_index = 0;

	void *ptr = NULL;

	setbuf(stdout, NULL);

	if(argc != 3) {
		printf("%s --node=<NUMA Node> --size=<Size-in-MB>\n", argv[0]);
		exit(0);
	}


	while ((c = getopt_long(argc, argv, "n:s:", long_options, &option_index)) != -1) {
		switch (c) {
			case 'm':
				size = atol(optarg) << 20;
				printf("size: %s MB\n", optarg);
				break;
			case 'n':
				node = atoi(optarg);
				printf("node: %s\n", optarg);
				break;
			default:
				abort();
		}
	}

	ptr = numa_alloc_onnode(size, node);

	if(ptr == NULL) {
		perror("numa_alloc_onnode() failed\n");
		exit(-1);
	}
	
	printf("%ld MB memory is allocated on NUMA node #%d\n", size, node);

	while(1) {
		sleep(100);
	}

	numa_free(ptr, size);

	return 0;
}


