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

#include <numa.h>
#include <numaif.h>

#include "cpu_util.h"

int main(int argc, char **argv)
{


	struct bitmask *cpu_mask = NULL;
	struct bitmask *mem_mask = NULL;

	pid_t pid;


	setbuf(stdout, NULL);

	(void) nice(-20);

	cpu_mask = numa_allocate_nodemask();

	if(cpu_mask == NULL) {
		perror("numa_allocate_nodemask() failed\n");
		exit(-1);
	}
	
	(void) numa_bitmask_clearall(cpu_mask);

	numa_bitmask_setbit(cpu_mask, cpu_node);

		//numa_sched_setaffinity
	if (sched_setaffinity(pid, numa_bitmask_nbytes(cpu_mask), (cpu_set_t*)cpu_mask->maskp) < 0) {
		perror("sched_setaffinity() failed");
		exit(-1);
	}

	printf("Pid #%d is bound to cpu node #%d.\n", pid, cpu_node);

	
	freq = measure_cpu_freq();



	mem_mask = numa_allocate_nodemask();

	if(mem_mask == NULL) {
		perror("numa_allocate_nodemask() failed\n");
		exit(-1);
	}

	(void) numa_bitmask_clearall(mem_mask);

	numa_bitmask_setbit(mem_mask, mem_node);

	if (set_mempolicy(MPOL_BIND, mem_mask->maskp, mem_mask->size + 1) < 0) {
		perror("set_mempolicy() failed\n");
		exit(-1);
	}

	printf("Bind memory allocation to node #%d.\n", mem_node);


	return 0;
}

