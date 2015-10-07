#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdint.h>

#include <dlog.h>

#include "gtest.h"

#include "default.h"

#define TESTING

extern "C" {
#include "../src/shared/kmsg_ioctl.c"
#include "../src/shared/logprint.c"
#include "../src/shared/queued_entry.c"
#include "../src/shared/log_file.c"
#include "../src/logger/logger.c"
}

class LoggerTestSuite : public testing::Test
{
	void SetUp()
	{
		logger.devices = NULL;
		logger.works = NULL;
		if (_main())
			exit(EXIT_FAILURE);
		if (!logger.devices)
			exit(EXIT_FAILURE);
	}

	void TearDown()
	{
		cleanup();
	}
};

TEST_F(LoggerTestSuite, oneIteration)
{
	do_logger(logger.devices);
}

TEST_F(LoggerTestSuite, getLogSize)
{
	uint32_t size = 0;

	do_logger(logger.devices);

	get_log_size(logger.devices->fd, &size);
	ASSERT_GT(size, 0u);
}

TEST_F(LoggerTestSuite, getLogReadSizeMax)
{
	uint32_t size = 0;
	int retval;
	const char * something = DEFAULT_TEST_MSG;

	do_logger(logger.devices);

	clear_log(logger.devices->fd);

	retval = __dlog_print(static_cast<log_id_t>(logger.devices->id), DEFAULT_TEST_PRIO,
				DEFAULT_TEST_TAG, "%s", something);
	ASSERT_GT(retval, static_cast<int>(strlen(something)));

	get_log_read_size_max(logger.devices->fd, &size);
	ASSERT_GT(size, 0u);
}
