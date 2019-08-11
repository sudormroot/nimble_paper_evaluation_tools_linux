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
#include <time.h>

#include <numa.h>
#include <numaif.h>

#include "cpu_util.h"
#include "cpu_freq.h"

#define MAX_MEM_SIZE	(8 << 20) 
#define ALIGN_SIZE	64

static volatile double freq = 0;

void measure_memory_access_time(int cpu_node, int mem_node)
{
	int nr_nodes;

	volatile void *ptr = NULL;
	volatile void *aligned_ptr = NULL;

	size_t size = 0;
	size_t aligned_size = 0;

	struct bitmask *cpu_mask = NULL;
	struct bitmask *mem_mask = NULL;

	char node_str[16];

	volatile uint64_t c1, c2, mem_cycles, hit_cycles, overhead_cycles, _hit_cycles, _mem_cycles;
	volatile register unsigned long x = 0;


	double *hit_latencies;
	double *mem_latencies;

	double *hit_ptr;
	double *mem_ptr;

	double hit_avg = 0;
	double mem_avg = 0;

	double hit_max = 0;
	double hit_min = 0;

	double mem_max = 0;
	double mem_min = 0;

	double sum1, sum2;


	int i;

	nr_nodes = numa_max_node() + 1;


	pid_t pid = getpid();


//	printf("Testing memory access time ...\n");
//	printf("cpu_node   %-d\n", cpu_node);
//	printf("mem_node   %-d\n", mem_node);
//	printf("pid        %-d\n", pid);
//	printf("NUMA nodes %-d\n", nr_nodes);



	/*
	 * Bind process to cpu node
	 * */

	memset(node_str, 0, sizeof(node_str));

	(void) snprintf(node_str, sizeof(node_str), "%d", cpu_node);

	cpu_mask = numa_parse_nodestring(node_str);

	if(cpu_mask == NULL) {
		perror("numa_parse_nodestring() failed\n");
		exit(-1);
	}
	
	if(numa_run_on_node_mask_all(cpu_mask)) {
		perror("numa_run_on_node_mask_all() failed");
		exit(-1);
	}

//	printf("Pid #%d is bound to cpu node #%d.\n", pid, cpu_node);

	



	/*
	 * Bind process to cpu node
	 * */

	memset(node_str, 0, sizeof(node_str));

	(void) snprintf(node_str, sizeof(node_str), "%d", mem_node);

	mem_mask = numa_parse_nodestring(node_str);

	if(mem_mask == NULL) {
		perror("numa_parse_nodestring() failed\n");
		exit(-1);
	}


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
	
	size = MAX_MEM_SIZE; 
	
	ptr = numa_alloc_onnode(size, mem_node);

	if(ptr == NULL) {
		perror("numa_alloc_onnode() failed\n");
		exit(-1);
	}
		
	//warm up
	for(i = 0; i < size; i++) {
		*((unsigned char *)(ptr + i)) = rand() % 255;
		clflush((void*)(ptr + i));
		mfence();
	}


	aligned_ptr = (void *) ((unsigned long)ptr - (unsigned long)ptr % sizeof(unsigned long));

	aligned_size = size - (unsigned long)ptr % sizeof(unsigned long);

	
	hit_latencies = malloc(sizeof(double) * (aligned_size / sizeof(unsigned long)));

	mem_latencies = malloc(sizeof(double) * (aligned_size / sizeof(unsigned long)));

	if(hit_latencies == NULL || mem_latencies == NULL) {
		perror("malloc() failed\n");
		exit(-1);
	}

	memset(hit_latencies, 0, sizeof(double) * (aligned_size / sizeof(unsigned long)));
	memset(mem_latencies, 0, sizeof(double) * (aligned_size / sizeof(unsigned long)));


	// lfence = load fence
	// sfence = save fence
	// mfence = save + load fence

	for(i = 0; i < aligned_size / sizeof(unsigned long); i++) {

		hit_ptr = &hit_latencies[i];
		mem_ptr = &mem_latencies[i];

		if(i == 0 || i % 1024 == 0)
			printf(".");

		//printf("i=%d\n", i);
		//printf("aligned_ptr=%p\n", aligned_ptr);

		compiler_fence();
		mfence();

		clflush(aligned_ptr);

		mfence();  //serialize clflush
		//lfence();                      
		compiler_fence();

		//measure LOAD when cache miss
		c1 = rdtscp();                    

		mfence();                      
		compiler_fence();

		x = *((unsigned long *)aligned_ptr);             

		mfence();        
		compiler_fence();

		c2 = rdtscp();
		
		mfence();                      
		compiler_fence();

		//cache miss
		mem_cycles = c2 - c1;

		mfence();
		compiler_fence();
			
		//printf( "Memory miss latency: %lu cycles %.2f ns\n", mem_cycles, cycles_to_nsecs(mem_cycles, freq));

		x++; 

		mfence();  
		compiler_fence();
		//lfence();  
		
		//measure LOAD when cache hit
		//
		c1 = rdtscp();
		
		mfence(); 
		compiler_fence();

		x = *((unsigned long *)aligned_ptr);

		mfence();  
		compiler_fence();

		c2 = rdtscp();

		mfence();
		compiler_fence();

		//cache hit
		hit_cycles = c2 - c1;

		mfence();
		compiler_fence();

		//printf( "Memory hit latency: %lu cycles %.2f ns\n", hit_cycles, cycles_to_nsecs(hit_cycles, freq));


		mfence();  
		compiler_fence();
		//lfence();  
		c1 = rdtscp(); 

		//(void) rdtscp();
		mfence(); 
	        compiler_fence();

		//(void) rdtscp();
		mfence(); 
	        compiler_fence();

		c2 = rdtscp();

		mfence();  
		compiler_fence();
		overhead_cycles = c2 - c1;

		mfence();  
		compiler_fence();

		//printf( "Overhead latency: %lu cycles %.2f ns\n", overhead_cycles, cycles_to_nsecs(overhead_cycles, freq)); 


		if(hit_cycles > overhead_cycles)
			_hit_cycles = hit_cycles - overhead_cycles;
		else
			_hit_cycles = hit_cycles;

		if(mem_cycles > overhead_cycles)
			_mem_cycles = mem_cycles - overhead_cycles;
		else
			_mem_cycles = mem_cycles;

		*hit_ptr = cycles_to_nsecs(_hit_cycles, freq);

		*mem_ptr = cycles_to_nsecs(_mem_cycles, freq); 


		if(*hit_ptr < hit_min || i == 0)
			hit_min = *hit_ptr;

		if(*hit_ptr > hit_max)
			hit_max = *hit_ptr;

		if(*mem_ptr < mem_min || i == 0)
			mem_min = *mem_ptr;

		if(*mem_ptr > mem_max)
			mem_max = *mem_ptr;


		//printf( "Cache hit latency: %lu cycles %.2f ns min %.2f ns max %.2f ns\n", _hit_cycles, *hit_ptr, hit_min, hit_max);

		//printf( "Memory latency: %lu cycles %.2f ns min %.2f ns max %.2f ns\n", _mem_cycles, *mem_ptr, mem_min, mem_max);


		aligned_ptr += sizeof(unsigned long);

		//break;
	}

	sum1 = 0;
	sum2 = 0;

	for(i = 0; i < aligned_size / sizeof(unsigned long); i++) {

		hit_ptr = &hit_latencies[i];
		mem_ptr = &mem_latencies[i];

		sum1 += *hit_ptr;
		sum2 += *mem_ptr;

	}

	

	hit_avg = sum1 / (double) (aligned_size / sizeof(unsigned long));
	mem_avg = sum2 / (double) (aligned_size / sizeof(unsigned long));

	printf("\n");
	printf("Cache hit average     : %.2f ns min %.2f ns max %.2f ns\n", hit_avg, hit_min, hit_max);
	printf("Memory access average : %.2f ns min %.2f ns max %.2f ns\n", mem_avg, mem_min, mem_max);
	printf("\n");

	free(hit_latencies);
	free(mem_latencies);

	numa_free_nodemask(cpu_mask);
	numa_free_nodemask(mem_mask);
	
	numa_free((void *)ptr, size);
}

int main(int argc, char **argv)
{
	int cpu_node = 0;
	int mem_node = 0;

	int nodes_nr = 0;

	uint64_t c1, c2;
	int real_secs = 3;

	time_t seed;

	srand((unsigned) time(&seed));

	(void) nice(-20);
	
	setbuf(stdout, NULL);

	nodes_nr = numa_max_node() + 1;

	printf("NUMA nodes: %d\n", nodes_nr);

	freq = measure_cpu_freq();

	compiler_fence();
	mfence();

	c1 = rdtscp(); 
	mfence();

	sleep(real_secs);

	c2 = rdtscp();

	mfence();
	compiler_fence();

	printf("Frequency calibration: real_secs = %d rdtscp %ld cycles %.2f s\n", real_secs, c2 - c1, cycles_to_nsecs(c2 - c1,freq) / 1000000000.00f);


	for(cpu_node = 0; cpu_node < nodes_nr; cpu_node++) {
		for(mem_node = 0; mem_node < nodes_nr; mem_node++) {
			printf("Measuring cpu #%d mem #%d\n", cpu_node, mem_node);
			measure_memory_access_time(cpu_node, mem_node);
		}
	}

	printf("Finished.\n");
	
	return 0;
}


