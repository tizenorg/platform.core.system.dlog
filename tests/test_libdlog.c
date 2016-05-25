#define LOG_TAG "SRPOL_LOGGER"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <dlog.h>
#include <logcommon.h>

void * func (void * data)
{
	int i;

	for (i = 0; i < 10000; ++i)
		LOGE ("Multithreading test %d", i);

}

int main (int argc, char **argv)
{
	if (argc == 1) {
		printf ("\t- checking oversized buffer\n");
		{
			char huge_buffer [LOG_MAX_SIZE * 3];

			memset (huge_buffer, 'a', LOG_MAX_SIZE);
			memset (huge_buffer + LOG_MAX_SIZE, 'b', LOG_MAX_SIZE);
			memset (huge_buffer + LOG_MAX_SIZE * 2, 'c', LOG_MAX_SIZE);
			huge_buffer [LOG_MAX_SIZE * 3 - 1] = '\0';

			LOGE ("%s", huge_buffer);
		}

		printf ("\t- checking garbage data\n");
		{
			char buffer [LOG_MAX_SIZE];
			int i;

			for (i = 0; i < LOG_MAX_SIZE; ++i) {
				buffer [i] = rand() & 255;
			}

			LOGE ("%s", buffer);
		}

		printf ("\t- checking multithreading\n");
		{
			int i = 0;
			pthread_t threads [50];

			for (i = 0; i < 50; ++i)
				pthread_create(threads + i, NULL, func, NULL);

			for (i = 0; i < 50; ++i)
				pthread_join (threads[i], NULL);
		}

		return 0;
	} else {
		int mode = 0;
		int count = atoi(argv[1]);

		while (count--) {
			if (mode) // alternate logging levels to allow testing of filters
				LOGE("%s", "test data, level: error");
			else
				LOGI("%s", "test data, level: info");

			mode = !mode;
		}
	}

	return 0;
}

