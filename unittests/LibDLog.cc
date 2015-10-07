#include <cstdio>
#include <cstddef>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

#include <pthread.h>

#include "gtest.h"

#include "default.h"

#define TESTING

extern "C" {
#include "../src/shared/logcommon.c"
#include "../src/libdlog/log.c"
#include "../src/libdlog/logconfig.c"
#include "../src/libdlog/loglimiter.c"
}

static pthread_once_t libdlogPrepared = PTHREAD_ONCE_INIT;

class LibDLogTestSuite : public testing::Test
{
	void SetUp()
	{
		if (pthread_once(&libdlogPrepared, __dlog_init))
			exit(EXIT_FAILURE);

		if (log_fds[DEFAULT_TEST_ID] < 0)
			exit(EXIT_FAILURE);
		if (reinterpret_cast<unsigned>(write_to_log) != reinterpret_cast<unsigned>(__write_to_log_kernel))
			exit(EXIT_FAILURE);
	}

	void TearDown() { }
};

TEST_F(LibDLogTestSuite, dLogShouldLog)
{
	log_id_t id;
	int should_log = config.lc_limiter ?
			__log_limiter_pass_log(DEFAULT_TEST_TAG, DEFAULT_TEST_PRIO) :
			1;
	int retval;

	ASSERT_GT(should_log, 0);
	for (id = LOG_ID_MAIN; id < LOG_ID_MAX; id = static_cast<log_id_t>(id + 1)) {
		retval = dlog_should_log(id, DEFAULT_TEST_TAG, DEFAULT_TEST_PRIO);
		if ((should_log <= 0) || ((id != LOG_ID_APPS) && (!config.lc_plog)))
			ASSERT_EQ(retval, DLOG_ERROR_NOT_PERMITTED);
		else
			ASSERT_EQ(retval, DLOG_ERROR_NONE);
	}

	retval = dlog_should_log(static_cast<log_id_t>(-1), DEFAULT_TEST_TAG, DEFAULT_TEST_PRIO);
	ASSERT_EQ(retval, DLOG_ERROR_INVALID_PARAMETER);
	retval = dlog_should_log(LOG_ID_APPS, NULL, DEFAULT_TEST_PRIO);
	ASSERT_EQ(retval, DLOG_ERROR_INVALID_PARAMETER);
}

TEST_F(LibDLogTestSuite, dLogWriteToLogKernel)
{
	int should_log = config.lc_limiter ?
			__log_limiter_pass_log(DEFAULT_TEST_TAG, DEFAULT_TEST_PRIO) :
			1;
	int retval;
	const char * something = DEFAULT_TEST_MSG;

	ASSERT_GT(should_log, 0);
	retval = dlog_should_log(DEFAULT_TEST_ID, DEFAULT_TEST_TAG, DEFAULT_TEST_PRIO);
	ASSERT_EQ(retval, DLOG_ERROR_NONE);
	retval = __write_to_log_kernel(DEFAULT_TEST_ID, DEFAULT_TEST_PRIO, DEFAULT_TEST_TAG, something);
	ASSERT_GT(retval, static_cast<int>(strlen(something)));
}

TEST_F(LibDLogTestSuite, dLogPrintId)
{
	int retval;
	const char * something = DEFAULT_TEST_MSG;

	retval = __dlog_print(DEFAULT_TEST_ID, DEFAULT_TEST_PRIO, DEFAULT_TEST_TAG, "%s", something);
	ASSERT_GT(retval, static_cast<int>(strlen(something)));
}

static int libDLogTestSuite_test___dlog_vprint(log_id_t log_id, log_priority prio, const char *tag, const char *fmt, ...)
{
	va_list ap;
	int retval;

	va_start(ap, fmt);
	retval = __dlog_vprint(log_id, prio, tag, fmt, ap);
	va_end(ap);

	return retval;
}

TEST_F(LibDLogTestSuite, dLogVPrintId)
{
	int retval;
	const char * something = DEFAULT_TEST_MSG;

	retval = libDLogTestSuite_test___dlog_vprint(DEFAULT_TEST_ID, DEFAULT_TEST_PRIO, DEFAULT_TEST_TAG, "%s", something);
	ASSERT_GT(retval, static_cast<int>(strlen(something)));
}

TEST_F(LibDLogTestSuite, dLogPrint)
{
	int retval;
	const char * something = DEFAULT_TEST_MSG;

	retval = dlog_print(DEFAULT_TEST_PRIO, DEFAULT_TEST_TAG, "%s", something);
	ASSERT_GT(retval, static_cast<int>(strlen(something)));
}

static int libDLogTestSuite_test_dlog_vprint(log_priority prio, const char *tag, const char *fmt, ...)
{
	va_list ap;
	int retval;

	va_start(ap, fmt);
	retval = dlog_vprint(prio, tag, fmt, ap);
	va_end(ap);

	return retval;
}

TEST_F(LibDLogTestSuite, dLogVPrint)
{
	int retval;
	const char * something = DEFAULT_TEST_MSG;

	retval = libDLogTestSuite_test_dlog_vprint(DEFAULT_TEST_PRIO, DEFAULT_TEST_TAG, "%s", something);
	ASSERT_GT(retval, static_cast<int>(strlen(something)));
}

TEST_F(LibDLogTestSuite, dLogConfigCanProcessEmptyFile)
{
	int retval;
	struct log_config config = { 0 };
	FILE *f;

	retval = __log_config_read("/tmp", &config);
	ASSERT_EQ(retval, RET_ERROR);

	f = fopen("/tmp/dlog.fakeconfig", "w");
	ASSERT_TRUE(f);
	fprintf(f, "\n");
	fclose(f);

	retval = __log_config_read("/tmp/dlog.fakeconfig", &config);
	ASSERT_EQ(retval, RET_SUCCESS);
}
