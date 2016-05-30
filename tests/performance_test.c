#define LOG_TAG "SRPOL_LOGGER"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include <dlog.h>

#define THREADS 50
#define LOGS_PER_THREAD 10000
#define TOTAL_LOGS (THREADS * LOGS_PER_THREAD)

#define SECOND_TO_MICROSECOND 1000000

void * func (void * data)
{
	int i;

	for (i = 0; i < LOGS_PER_THREAD; ++i)
		LOGE ("Logging test %d", i);
}

int main (int argc, char **argv)
{
	int i = 0;
	struct timespec start, end;
	double elapsed;
	pthread_t threads [50];

	clock_gettime (CLOCK_MONOTONIC, &start);

	for (i = 0; i < 50; ++i)
		pthread_create(threads + i, NULL, func, NULL);
	for (i = 0; i < 50; ++i)
		pthread_join (threads[i], NULL);

	clock_gettime (CLOCK_MONOTONIC, &end);
	elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1E9;
	printf("%d threads %d logs each (total logs %d). Total time %lfs (%lf us each)\n", THREADS, LOGS_PER_THREAD, TOTAL_LOGS, elapsed, SECOND_TO_MICROSECOND * elapsed / TOTAL_LOGS);

	return 0;
}
