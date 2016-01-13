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
#define SD_JOURNAL_SUPPRESS_LOCATION 1
#include <syslog.h>
#include <systemd/sd-journal.h>
#ifdef FATAL_ON
#include <assert.h>
#endif

#define LOG_BUF_SIZE	1024

#define VALUE_MAX 2
#define LOG_CONFIG_FILE "/opt/etc/dlog.conf"

static int log_fds[(int)LOG_ID_MAX] = { -1, -1, -1, -1 };
static char log_devs[LOG_ID_MAX][PATH_MAX];
static int (*write_to_log)(log_id_t, log_priority, const char *tag, const char *msg) = NULL;
static pthread_mutex_t log_init_lock = PTHREAD_MUTEX_INITIALIZER;
static struct log_config config;

static inline int dlog_pri_to_journal_pri(log_priority prio)
{
	static int pri_table[DLOG_PRIO_MAX] = {
		[DLOG_UNKNOWN] = LOG_DEBUG,
		[DLOG_DEFAULT] = LOG_DEBUG,
		[DLOG_VERBOSE] = LOG_DEBUG,
		[DLOG_DEBUG] = LOG_DEBUG,
		[DLOG_INFO] = LOG_INFO,
		[DLOG_WARN] = LOG_WARNING,
		[DLOG_ERROR] = LOG_ERR,
		[DLOG_FATAL] = LOG_CRIT,
		[DLOG_SILENT] = -1,
	};

	if (prio < 0 || prio >= DLOG_PRIO_MAX)
		return DLOG_ERROR_INVALID_PARAMETER;

	return pri_table[prio];
}

static inline const char* dlog_id_to_string(log_id_t log_id)
{
	static const char* id_table[LOG_ID_MAX] = {
		[LOG_ID_MAIN]   = "log_main",
		[LOG_ID_RADIO]  = "log_radio",
		[LOG_ID_SYSTEM] = "log_system",
		[LOG_ID_APPS]   = "log_apps",
	};

	if (log_id < 0 || log_id >= LOG_ID_MAX || !id_table[log_id])
		return "UNKNOWN";

	return id_table[log_id];
}

static int __write_to_log_sd_journal(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
	const char *lid_str = dlog_id_to_string(log_id);

	pid_t tid = (pid_t)syscall(SYS_gettid);

	if(!msg)
		return DLOG_ERROR_INVALID_PARAMETER;

	if(strncmp(lid_str, "UNKNOWN", 7) == 0)
		return DLOG_ERROR_INVALID_PARAMETER;

	if(prio < DLOG_VERBOSE || prio >= DLOG_PRIO_MAX)
		return DLOG_ERROR_INVALID_PARAMETER;

	if(!tag)
		tag = "";

	struct iovec vec[5];
	char _msg[LOG_BUF_SIZE + 8];
	char _prio[LOG_BUF_SIZE + 9];
	char _tag[LOG_BUF_SIZE + 8];
	char _log_id[LOG_BUF_SIZE + 7];
	char _tid[LOG_BUF_SIZE + 4];

	snprintf(_msg, LOG_BUF_SIZE + 8, "MESSAGE=%s", msg);
	vec[0].iov_base = (void *)_msg;
	vec[0].iov_len = strlen(vec[0].iov_base);

	snprintf(_prio, LOG_BUF_SIZE + 9, "PRIORITY=%i", dlog_pri_to_journal_pri(prio));
	vec[1].iov_base = (void *)_prio;
	vec[1].iov_len = strlen(vec[1].iov_base);

	snprintf(_tag, LOG_BUF_SIZE + 8, "LOG_TAG=%s", tag);
	vec[2].iov_base = (void *)_tag;
	vec[2].iov_len = strlen(vec[2].iov_base);

	snprintf(_log_id, LOG_BUF_SIZE + 7, "LOG_ID=%s", lid_str);
	vec[3].iov_base = (void *)_log_id;
	vec[3].iov_len = strlen(vec[3].iov_base);

	snprintf(_tid, LOG_BUF_SIZE + 4, "TID=%d", tid);
	vec[4].iov_base = (void *)_tid;
	vec[4].iov_len = strlen(vec[4].iov_base);

	return sd_journal_sendv(vec, 5);
}

static int __write_to_log_null(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
	return DLOG_ERROR_NOT_PERMITTED;
}

static int __write_to_log_kernel(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
	ssize_t ret;
	int log_fd;
	size_t len;
	char buf[LOG_BUF_SIZE];

	if (log_id < LOG_ID_MAX)
		log_fd = log_fds[log_id];
	else
		return DLOG_ERROR_INVALID_PARAMETER;

	if (prio < DLOG_VERBOSE || prio >= DLOG_PRIO_MAX)
		return DLOG_ERROR_INVALID_PARAMETER;

	if (!tag)
		tag = "";

	if (!msg)
		return DLOG_ERROR_INVALID_PARAMETER;

	len = snprintf(buf, LOG_BUF_SIZE, "%s;%d;%s", tag, prio, msg);

	ret = write(log_fd, buf, len > LOG_BUF_SIZE ? LOG_BUF_SIZE : len);
	if (ret < 0)
	    ret = -errno;

	return ret;
}

static void __configure(void)
{
	if (0 > __log_config_read(LOG_CONFIG_FILE, &config)) {
		config.lc_limiter = 0;
		config.lc_plog = 0;
	}

	if (config.lc_limiter) {
		if (0 > __log_limiter_initialize()) {
			config.lc_limiter = 0;
		}
	}
}

static void __dlog_init(void)
{
	pthread_mutex_lock(&log_init_lock);
	__configure();

	switch (dlog_mode_detect()) {
		case DLOG_MODE_JOURNAL:
			write_to_log = __write_to_log_sd_journal;
			break;
		case DLOG_MODE_KMSG:
		case DLOG_MODE_ANDROID_LOGGER:
			if (0 == get_log_dev_names(log_devs)) {
				log_fds[LOG_ID_MAIN]   = open(log_devs[LOG_ID_MAIN],   O_WRONLY);
				log_fds[LOG_ID_SYSTEM] = open(log_devs[LOG_ID_SYSTEM], O_WRONLY);
				log_fds[LOG_ID_RADIO]  = open(log_devs[LOG_ID_RADIO],  O_WRONLY);
				log_fds[LOG_ID_APPS]   = open(log_devs[LOG_ID_APPS],   O_WRONLY);
			}
			if (log_fds[LOG_ID_MAIN] < 0) {
				write_to_log = __write_to_log_null;
			} else {
				write_to_log = __write_to_log_kernel;
			}
			if (log_fds[LOG_ID_RADIO] < 0)
				log_fds[LOG_ID_RADIO]  = log_fds[LOG_ID_MAIN];
			if (log_fds[LOG_ID_SYSTEM] < 0)
				log_fds[LOG_ID_SYSTEM] = log_fds[LOG_ID_MAIN];
			if (log_fds[LOG_ID_APPS] < 0)
				log_fds[LOG_ID_APPS]   = log_fds[LOG_ID_MAIN];
			break;
		default: assert (0); break;
	}
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
