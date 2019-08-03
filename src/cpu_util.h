
/*
 *
 * Author: Jiaolin Luo
 *
 * */
#ifndef CPU_UTIL_H
#define CPU_UTIL_H

inline double cycles_to_nsecs(double cycles, double freq)
{
	return (cycles / freq) * 1000000000.00f;
}

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

inline double measure_cpu_freq(void)
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


#endif /*CPU_UTIL_H*/
