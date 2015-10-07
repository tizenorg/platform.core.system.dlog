#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>

#include <pthread.h>
#include <libudev.h>

#include "gtest.h"

#define TESTING

extern "C" {
static __thread int *g_minors;
#include "../src/loginit/loginit.c"
}

static pthread_once_t udevMonitorPrepared = PTHREAD_ONCE_INIT;
static struct udev_monitor *mon = NULL;

static void prepareUdevMonitor()
{
	prepare_udev_monitor(&mon);
	if (!mon)
		exit(EXIT_FAILURE);
}

class LogInitTestSuite : public testing::Test
{
	int kmsg_fd;
	int gMinors[LOG_ID_MAX];

	void SetUp()
	{
		int i;

		if (pthread_once(&udevMonitorPrepared, prepareUdevMonitor))
			exit(EXIT_FAILURE);

		if (!mon)
			exit(EXIT_FAILURE);

		kmsg_fd = open(DEV_KMSG, O_RDWR);
		if (kmsg_fd < 0)
			exit(EXIT_FAILURE);

		for (i = 0; i < LOG_ID_MAX; i++)
			gMinors[i] = -1;
		g_minors = gMinors;
		if (create_kmsg_devs(kmsg_fd)) {
			close(kmsg_fd);
			exit(EXIT_FAILURE);
		}

		for (i = 0; i < LOG_ID_MAX; i++)
			if (gMinors[i] < 0) {
				TearDown();
				exit(EXIT_FAILURE);
			}
	}

	void TearDown()
	{
		if (kmsg_fd < 0)
			exit(EXIT_FAILURE);

		g_minors = gMinors;
		remove_kmsg_devs(kmsg_fd);
		close(kmsg_fd);
	}

	public: int *getMinors()
	{
		return gMinors;
	}
};

TEST_F(LogInitTestSuite, canChangeOwner)
{
	int retval;

	g_minors = getMinors();
	retval = change_owner(mon);
	ASSERT_EQ(retval, 0);
}

TEST_F(LogInitTestSuite, canWriteConfig)
{
	FILE *f = fopen("/tmp/dlog.conf", "w");
	int retval;

	ASSERT_TRUE(f);

	g_minors = getMinors();
	retval = write_config(f);
	ASSERT_GT(retval, 0);

	fclose(f);
}
