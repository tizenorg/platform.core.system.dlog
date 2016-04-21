/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: t -*-
 * DLOG
 * Copyright (c) 2005-2008, The Android Open Source Project
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "config.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <dlog.h>
#include <logcommon.h>
#include "loglimiter.h"
#include "logconfig.h"
#ifdef FATAL_ON
#include <assert.h>
#endif

int (*write_to_log)(log_id_t, log_priority, const char *tag, const char *msg) __attribute__((visibility ("hidden")));
static pthread_mutex_t log_init_lock = PTHREAD_MUTEX_INITIALIZER;
static struct log_config config;
extern void __dlog_init_backend(void) __attribute__((visibility ("hidden")));

static int __write_to_log_null(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
	return DLOG_ERROR_NOT_PERMITTED;
}

static void __configure(void)
{
	if (0 > __log_config_read(LOG_CONFIG_FILE, &config)) {
		config.lc_limiter = 0;
		config.lc_plog = 0;
	}

	if (config.lc_limiter) {
		if (0 > __log_limiter_initialize())
			config.lc_limiter = 0;
	}
}

static void __dlog_init(void)
{
	pthread_mutex_lock(&log_init_lock);
	write_to_log = __write_to_log_null;
	__configure();
	__dlog_init_backend();
	pthread_mutex_unlock(&log_init_lock);
}

void __dlog_fatal_assert(int prio)
{
#ifdef FATAL_ON
	assert(!(prio == DLOG_FATAL));
#endif
}

static int dlog_should_log(log_id_t log_id, const char* tag, int prio)
{
	int should_log;

#ifndef DLOG_DEBUG_ENABLE
	if (prio <= DLOG_DEBUG)
		return DLOG_ERROR_INVALID_PARAMETER;
#endif
	if (!tag)
		return DLOG_ERROR_INVALID_PARAMETER;

	if (log_id < 0 || LOG_ID_MAX <= log_id)
		return DLOG_ERROR_INVALID_PARAMETER;

	if (log_id != LOG_ID_APPS && !config.lc_plog)
		return DLOG_ERROR_NOT_PERMITTED;

	if (config.lc_limiter) {
		should_log = __log_limiter_pass_log(tag, prio);

		if (!should_log) {
			return DLOG_ERROR_NOT_PERMITTED;
		} else if (should_log < 0) {
			write_to_log(log_id, prio, tag,
			             "Your log has been blocked due to limit of log lines per minute.");
			return DLOG_ERROR_NOT_PERMITTED;
		}
	}

	return DLOG_ERROR_NONE;
}

int __dlog_vprint(log_id_t log_id, int prio, const char *tag, const char *fmt, va_list ap)
{
	int ret;
	char buf[LOG_BUF_SIZE];

	if (write_to_log == NULL)
		__dlog_init();

	ret = dlog_should_log(log_id, tag, prio);

	if (ret < 0)
		return ret;

	vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
	ret = write_to_log(log_id, prio, tag, buf);
#ifdef FATAL_ON
	__dlog_fatal_assert(prio);
#endif
	return ret;
}

int __dlog_print(log_id_t log_id, int prio, const char *tag, const char *fmt, ...)
{
	int ret;
	va_list ap;
	char buf[LOG_BUF_SIZE];

	if (write_to_log == NULL)
		__dlog_init();

	ret = dlog_should_log(log_id, tag, prio);

	if (ret < 0)
		return ret;

	va_start(ap, fmt);
	vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
	va_end(ap);

	ret = write_to_log(log_id, prio, tag, buf);
#ifdef FATAL_ON
	__dlog_fatal_assert(prio);
#endif
	return ret;
}

int dlog_vprint(log_priority prio, const char *tag, const char *fmt, va_list ap)
{
	char buf[LOG_BUF_SIZE];

	if (write_to_log == NULL)
		__dlog_init();

	vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);

	return write_to_log(LOG_ID_APPS, prio, tag, buf);
}

int dlog_print(log_priority prio, const char *tag, const char *fmt, ...)
{
	va_list ap;
	char buf[LOG_BUF_SIZE];

	if (write_to_log == NULL)
		__dlog_init();

	va_start(ap, fmt);
	vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
	va_end(ap);

	return write_to_log(LOG_ID_APPS, prio, tag, buf);
}

void __attribute__((destructor)) __dlog_fini(void)
{
	__log_limiter_destroy();
}
