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

	size_t total=0;

	setbuf(stdout, NULL);

	if(argc != 3) {
		printf("%s --node=<NUMA Node> --size=<Size-in-MB>\n", argv[0]);
		exit(0);
	}


	while ((c = getopt_long(argc, argv, "n:s:", long_options, &option_index)) != -1) {
		switch (c) {
			case 's':
				size = atol(optarg);
				printf("size: %d MB\n", size);
				break;
			case 'n':
				node = atoi(optarg);
				printf("node: %d\n", node);
				break;
			default:
				abort();
		}
	}
	
	total=0;

	while(1) {
		
		if(total >= size)
			break;

		ptr = numa_alloc_onnode(1 << 20, node);

		if(ptr == NULL) {
			sleep(1);
			continue;
		}

		total += 1;

		printf("Total %ld MB memory is occupied on NUMA node #%d\n", total, node);

	}

	while(1) {
		sleep(100);
	}

	exit(0);
}


