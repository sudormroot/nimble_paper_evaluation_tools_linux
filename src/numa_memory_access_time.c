/*
 *
 * Author: Jiaolin Luo 
 *
 * */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <immintrin.h>
#include <stdint.h>
#include <x86intrin.h>

#include <numa.h>


#define MAX_MEM_SIZE	(1024) 
#define ALIGN_SIZE		64



inline void mfence(void)
{
	asm volatile("mfence" ::: "memory");
}

inline void sfence(void)
{
	asm volatile("sfence" ::: "memory");
}

inline void lfence(void)
{
	asm volatile("lfence" ::: "memory");
}

inline void compiler_fence(void)
{
	asm volatile("" ::: "memory");
}

inline void clflush(volatile void *ptr)
{
    asm volatile ("clflush (%0)" :: "r"(ptr));
}


inline uint64_t rdtsc(void)
{
        volatile unsigned long lo,hi;

        asm volatile ( "rdtsc":"=a"(lo),"=d"(hi));

        return (uint64_t)hi << 32 | lo;
}

double measure_cpu_freq(void)
{
	uint64_t c1,c2;
	double freq;
	int interval = 2;

	struct timeval t1, t2;

	double elapsed;

	printf("Measuring CPU processor ...\n");

	gettimeofday(&t1, (struct timezone *) 0);
	c1 = rdtsc();

	sleep(interval);

	c2 = rdtsc();
	gettimeofday(&t2, (struct timezone *) 0);

	//in microsecond
	elapsed = (double)(t2.tv_sec - t1.tv_sec) * 1000000.00;

	+ (double)(t2.tv_usec - t1.tv_usec);

	if (t2.tv_usec < t1.tv_usec) {
		t2.tv_usec += 1000000;
		t2.tv_sec--;
	}

	elapsed += (double)(t2.tv_usec - t1.tv_usec);

	elapsed = elapsed / 1000000.00;

	freq = (double)(c2 - c1) / elapsed;

	printf("Measured processor frequency: %.2f Hz (%.2f GHz)\n", freq, freq / 1000000000.00f);

	return freq;
}


void calculate_memory_access_time(int cpu_node, int mem_node)
{
	int nr_nodes;

	volatile void *ptr = NULL;
	volatile void *aligned_ptr = NULL;

	size_t size = 0;
	size_t aligned_size = 0;

	struct bitmask *cpu_mask = NULL;
	struct bitmask *mem_mask = NULL;

	volatile uint64_t t1, t2, msl, hsl, osl;
	volatile register unsigned long x = 0;

	volatile double freq = 0;

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
		perror("numa_alloc_onnode() at node #%d failed\n", mem_node);
		exit(-1);
	}
	

	aligned_ptr = (void *) ((unsigned long)ptr - (unsigned long)ptr % sizeof(unsigned long));

	aligned_size = size - (unsigned long)ptr % sizeof(unsigned long);

	
	// lfence = load fence
	// sfence = save fence
	// mfence = save + load fence

	for(i = 0; i < aligned_size / sizeof(unsigned long); i++) {

		printf("aligned_ptr=%p\n", aligned_ptr);

		_mm_mfence();

		_mm_clflush(aligned_ptr);

		_mm_mfence();  //orderize _mm_clflush


		_mm_mfence();                      /* this properly orders both clflush and rdtscp*/
		_mm_lfence();                      /* mfence and lfence must be in this order + compiler barrier for rdtscp */
		t1 = __rdtsc();                    /* set timer */
		_mm_lfence();                      /* serialize __rdtscp with respect to trailing instructions + compiler barrier for rdtscp and the load */
		x = *((unsigned long *)aligned_ptr);             /* array[0] is a cache miss */
		/* measring the write miss latency to array is not meaningful because it's an implementation detail and the next write may also miss */
		/* no need for mfence because there are no stores in between */
		_mm_lfence();                      /* mfence and lfence must be in this order + compiler barrier for rdtscp and the load*/
		t2 = __rdtsc();
		_mm_lfence();                      /* serialize __rdtscp with respect to trailing instructions */
		msl = t2 - t1;

		asm volatile("": : :"memory");

		//printf( "x = %x \n", x);             /* prevent the compiler from optimizing the load */
		x++;

		//printf( "miss section latency = %lu \n", msl );   /* the latency of everything in between the two rdtscp */

		_mm_mfence();                      /* this properly orders both clflush and rdtscp*/
		_mm_lfence();                      /* mfence and lfence must be in this order + compiler barrier for rdtscp */
		t1 = __rdtsc();                 /* set timer */
		_mm_lfence();                      /* serialize __rdtscp with respect to trailing instructions + compiler barrier for rdtscp and the load */
		//temp = array[ 0 ];                 /* array[0] is a cache miss */
		x = *((unsigned long *)aligned_ptr);

		/* measring the write miss latency to array is not meaningful because it's an implementation detail and the next write may also miss */
		/* no need for mfence because there are no stores in between */
		_mm_lfence();                      /* mfence and lfence must be in this order + compiler barrier for rdtscp and the load */
		t2 = __rdtsc();
		_mm_lfence();                      /* serialize __rdtscp with respect to trailing instructions */
		hsl = t2 - t1;

		printf( "x = %x \n", x);            /* prevent the compiler from optimizing the load */
		printf( "Memory hit latency = %lu cycles\n", hsl );   /* the latency of everything in between the two rdtscp */


		_mm_mfence();                      /* this properly orders both clflush and rdtscp*/
		_mm_lfence();                      /* mfence and lfence must be in this order + compiler barrier for rdtscp */
		t1 = __rdtsc();                 /* set timer */
		_mm_lfence();                      /* serialize __rdtscp with respect to trailing instructions + compiler barrier for rdtscp */
		/* no need for mfence because there are no stores in between */
		_mm_lfence();                      /* mfence and lfence must be in this order + compiler barrier for rdtscp */
		t2 = __rdtsc();
		_mm_lfence();                      /* serialize __rdtscp with respect to trailing instructions */
		osl = t2 - t1;

		printf( "overhead latency = %lu \n", osl ); /* the latency of everything in between the two rdtscp */


		printf( "Measured L1 hit latency = %lu TSC cycles\n", hsl - osl ); /* hsl is always larger than osl */
		printf( "Measured main memory latency = %lu TSC cycles\n", msl - osl ); /* msl is always larger than osl and hsl */

		aligned_ptr += sizeof(unsigned long);
	}

	numa_free_nodemask(cpu_node);
	numa_free_nodemask(mem_node);
	
	numa_free(ptr0, size);
	numa_free(ptr1, size);
}

int main(int argc, char **argv)
{
	setbuf(stdout, NULL);

	calculate_memory_access_time(0, 0);
	calculate_memory_access_time(0, 1);
	calculate_memory_access_time(1, 0);
	calculate_memory_access_time(1, 1);

	printf("Finished.\n");
	
	return 0;
}


