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
//#include <inttypes.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <fcntl.h>
//#include <sys/mman.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>

//#include <immintrin.h>
//#include <x86intrin.h>

#include <numa.h>
#include <numaif.h>

#include "cpu_util.h"

#define MAX_MEM_SIZE	(128) 
#define ALIGN_SIZE	64

void calculate_memory_access_time(int cpu_node, int mem_node)
{
	int nr_nodes;

	volatile void *ptr = NULL;
	volatile void *aligned_ptr = NULL;

	size_t size = 0;
	size_t aligned_size = 0;

	struct bitmask *cpu_mask = NULL;
	struct bitmask *mem_mask = NULL;

	volatile uint64_t t1, t2, ms1, hs1, os1;
	volatile register unsigned long x = 0;

	volatile double freq = 0;

	int i;

	nr_nodes = numa_max_node() + 1;

	//old_nodes = numa_bitmask_alloc(nr_nodes);
	//new_nodes = numa_bitmask_alloc(nr_nodes);
	//numa_bitmask_setbit(old_nodes, 1);
	//numa_bitmask_setbit(new_nodes, 0);


	pid_t pid = getpid();


	printf("Testing memory access time ...\n");
	printf("cpu_node   %-d\n", cpu_node);
	printf("mem_node   %-d\n", mem_node);
	printf("pid        %-d\n", pid);
	printf("NUMA nodes %-d\n", nr_nodes);

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


	//prepare cache-line aligned memory
	//
	size = MAX_MEM_SIZE; 
	
	ptr = numa_alloc_onnode(size, mem_node);

	if(ptr == NULL) {
		perror("numa_alloc_onnode() failed\n");
		exit(-1);
	}
	

	aligned_ptr = (void *) ((unsigned long)ptr - (unsigned long)ptr % sizeof(unsigned long));

	aligned_size = size - (unsigned long)ptr % sizeof(unsigned long);

	
	// lfence = load fence
	// sfence = save fence
	// mfence = save + load fence

	for(i = 0; i < aligned_size / sizeof(unsigned long); i++) {

		printf("aligned_ptr=%p\n", aligned_ptr);

		mfence();

		clflush(aligned_ptr);

		mfence();  //serialize clflush
		//lfence();                      
		
		compiler_fence();

		t1 = rdtsc();                    

		mfence();                      

		compiler_fence();

		x = *((unsigned long *)aligned_ptr);             

		lfence();        
		
		compiler_fence();

		t2 = rdtsc();
		
		lfence();                      

		compiler_fence();

		ms1 = t2 - t1;

		compiler_fence();
		
		x++;

		mfence();  
		lfence();  
		
		t1 = rdtsc();
		
		lfence(); 
		compiler_fence();

		x = *((unsigned long *)aligned_ptr);

		lfence();  
		t2 = rdtsc();
		lfence();

		compiler_fence();
		hs1 = t2 - t1;

		printf( "Memory hit latency: %lu cycles %.2f ns\n", hs1, cycles_to_nsecs(hs1, freq));


		mfence();  
		lfence();  
		t1 = rdtsc(); 
		lfence();  
		lfence(); 
		t2 = rdtsc();
		lfence();  
		os1 = t2 - t1;

		printf( "Overhead latency: %lu cycles %.2f ns\n", os1, cycles_to_nsecs(os1, freq)); 


		printf( "Measured L1 hit latency: %lu cycles %.2f ns\n", hs1 - os1, cycles_to_nsecs(hs1 - os1, freq)); 

		printf( "Measured main memory latency: %lu cycles %.2f ns\n", ms1 - os1, cycles_to_nsecs(ms1 - os1, freq)); 

		aligned_ptr += sizeof(unsigned long);

		break;
	}

	numa_free_nodemask(cpu_node);
	numa_free_nodemask(mem_node);
	
	numa_free(ptr, size);
}

int main(int argc, char **argv)
{

	if(nice(-20)) {
		perror("Warning, failed in nice().\n");
	}

	calculate_memory_access_time(0, 0);
	//calculate_memory_access_time(0, 1);
	//calculate_memory_access_time(1, 0);
	//calculate_memory_access_time(1, 1);

	printf("Finished.\n");
	
	return 0;
}


