
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


inline uint64_t rdtscp(void)
{
	volatile uint64_t tsc=0;

	asm volatile	("rdtscp; "         // serializing read of tsc
			"shl $32,%%rdx; "  // shift higher 32 bits stored in rdx up
			"or %%rdx,%%rax"   // and or onto rax
			: "=a"(tsc)        // output to tsc variable
			:
			: "%rcx", "%rdx"); // rcx and rdx are clobbered

	return tsc;

#if 0
    volatile unsigned  lo,hi;

    asm volatile
        ( "CPUID\n\t"
          "RDTSC\n\t"
          "mov %%edx, %0\n\t"
          "mov %%eax, %1\n\t"
          :
          "=r" (hi), "=r" (lo)
          ::
          "rax", "rbx", "rcx", "rdx"
        );

        return (uint64_t)hi << 32 | (uint64_t)lo;
#endif 
}

#endif /*CPU_UTIL_H*/
