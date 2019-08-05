
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>


#include "cpu_util.h"
#include "cpu_freq.h"

double measure_cpu_freq(void)
{
	uint64_t c1,c2;
	double freq;
	int interval = 2;

	struct timeval t1, t2;

	double elapsed;

	printf("Measuring CPU processor ...\n");

	gettimeofday(&t1, (struct timezone *) 0);
	c1 = rdtscp();

	sleep(interval);

	c2 = rdtscp();
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


