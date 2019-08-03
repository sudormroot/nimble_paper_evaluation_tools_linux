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

#define MAX_MEM_SIZE	(32 << 20) 
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
	//
	int real_secs = 3;

	compiler_fence();
	mfence();

	t1 = rdtscp(); 
	mfence();

	sleep(real_secs);

	t2 = rdtscp();

	mfence();
	compiler_fence();

	printf("Calibration: real_secs = %d rdtscp %ld cycles %.2f s\n", real_secs, t2 - t1, cycles_to_nsecs(t2 - t1,freq) / 1000000000.00f);


	for(i = 0; i < aligned_size / sizeof(unsigned long); i++) {

		printf("i=%d\n", i);
		printf("aligned_ptr=%p\n", aligned_ptr);

		compiler_fence();
		mfence();

		clflush(aligned_ptr);

		mfence();  //serialize clflush
		//lfence();                      
		compiler_fence();

		//measure LOAD when cache miss
		t1 = rdtscp();                    

		mfence();                      
		compiler_fence();

		x = *((unsigned long *)aligned_ptr);             

		mfence();        
		compiler_fence();

		t2 = rdtscp();
		
		mfence();                      
		compiler_fence();

		//cache miss
		ms1 = t2 - t1;

		mfence();
		compiler_fence();
			
		printf( "Memory miss latency: %lu cycles %.2f ns\n", ms1, cycles_to_nsecs(ms1, freq));

		x++; 

		mfence();  
		compiler_fence();
		//lfence();  
		
		//measure LOAD when cache hit
		//
		t1 = rdtscp();
		
		mfence(); 
		compiler_fence();

		x = *((unsigned long *)aligned_ptr);

		mfence();  
		compiler_fence();

		t2 = rdtscp();

		mfence();
		compiler_fence();

		//cache hit
		hs1 = t2 - t1;

		mfence();
		compiler_fence();

		printf( "Memory hit latency: %lu cycles %.2f ns\n", hs1, cycles_to_nsecs(hs1, freq));


		mfence();  
		compiler_fence();
		//lfence();  
		t1 = rdtscp(); 

		//(void) rdtscp();
		mfence(); 
	        compiler_fence();

		//(void) rdtscp();
		mfence(); 
	        compiler_fence();

		t2 = rdtscp();

		mfence();  
		compiler_fence();
		os1 = t2 - t1;

		mfence();  
		compiler_fence();

		printf( "Overhead latency: %lu cycles %.2f ns\n", os1, cycles_to_nsecs(os1, freq)); 


		//printf( "Cache hit latency: %lu cycles %.2f ns\n", hs1 - os1, cycles_to_nsecs(hs1 - os1, freq)); 

		//printf( "Memory latency: %lu cycles %.2f ns\n", ms1 - os1, cycles_to_nsecs(ms1 - os1, freq)); 

		aligned_ptr += sizeof(unsigned long);

		//break;
	}

	numa_free_nodemask(cpu_node);
	numa_free_nodemask(mem_node);
	
	numa_free(ptr, size);
}

int main(int argc, char **argv)
{
	int cpu_node = 0;
	int mem_node = 0;

	int nodes_nr = 0;

	(void) nice(-20);
	
	setbuf(stdout, NULL);

	ndes_nr = numa_max_node();

	for(cpu_node = 0; cpu_node < nodes_nr; cpu_node++) {
		for(mem_node = 0; mem_node < nodes_nr; mem_node++) {
			printf("Measuring on cpu #%d mem #%d\n", cpu_node, mem_node);
			calculate_memory_access_time(cpu_node, mem_node);
		}
	}

	printf("Finished.\n");
	
	return 0;
}


