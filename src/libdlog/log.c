/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: t -*-
 * DLOG
 * Copyright (c) 2005-2008, The Android Open Source Project
 * Copyright (c) 2012-2015 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * @file	log.c
 * @version	0.1
 * @brief	This file is the source file of dlog interface
 */

#define _GNU_SOURCE

#include "config.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <errno.h>
#include <linux/limits.h>
#include <dlog.h>
#include <dlog-common.h>

#ifdef HAVE_SYSTEMD_JOURNAL
#define SD_JOURNAL_SUPPRESS_LOCATION 1
#include <syslog.h>
#include <systemd/sd-journal.h>
#endif

static const size_t      g_dlog_buf_size = 4*1024;
static const char *const g_log_main = "log_main";
static const char *const g_log_radio = "log_radio";
static const char *const g_log_system = "log_system";
static const char *const g_log_apps = "log_apps";

#include <tzplatform_config.h>

static pthread_mutex_t g_log_init_lock = PTHREAD_MUTEX_INITIALIZER;

#ifndef HAVE_SYSTEMD_JOURNAL

static int g_log_fds[(int)LOG_ID_MAX] = { -1, -1, -1, -1 };

#endif

#ifdef HAVE_SYSTEMD_JOURNAL

static int dlog_pri_to_journal_pri(log_priority prio)
{
	switch (prio)
	{
		case DLOG_UNKNOWN:
		case DLOG_DEFAULT:
		case DLOG_VERBOSE:
		case DLOG_DEBUG:
			return LOG_DEBUG;
		case DLOG_INFO:
			return LOG_INFO;
		case DLOG_WARN:
			return LOG_WARNING;
		case DLOG_ERROR:
			return LOG_ERR;
		case DLOG_FATAL:
			return LOG_CRIT;
		case DLOG_SILENT:
			return -1;
		default:
			return -EINVAL;
	}
}

static inline const char* dlog_id_to_string(log_id_t log_id)
{
	switch (log_id)
	{
		case LOG_ID_MAIN:
			return g_log_main;
		case LOG_ID_RADIO:
			return g_log_radio;
		case LOG_ID_SYSTEM:
			return g_log_system;
		case LOG_ID_APPS:
			return g_log_apps;
		default:
			return NULL;
	}
}

static int __write_to_log_sd_journal(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
	/* XXX: sd_journal_sendv() with manually filed iov-s might be faster */
	return sd_journal_send("MESSAGE=%s", msg,
				   "PRIORITY=%i", dlog_pri_to_journal_pri(prio),
				   "DLOG_PRIORITY=%d", prio,
				   "DLOG_TAG=%s", tag,
				   "DLOG_ID=%s", dlog_id_to_string(log_id),
				   NULL);
}

#else

static int __write_to_log_null(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
	return -1;
}
static int __write_to_log_kernel(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
	ssize_t ret;
	int log_fd;
	struct iovec vec[3];

	if (log_id < LOG_ID_MAX)
		log_fd = g_log_fds[log_id];
	else
		return -1;  /* for TC */

	if (!tag)
		tag = "";

	if (!msg)
		return -1;

	vec[0].iov_base	= (unsigned char *) &prio;
	vec[0].iov_len	= 1;
	vec[1].iov_base	= (void *) tag;
	vec[1].iov_len	= strlen(tag) + 1;
	vec[2].iov_base	= (void *) msg;
	vec[2].iov_len	= strlen(msg) + 1;

	ret = TEMP_FAILURE_RETRY( writev(log_fd, vec, 3));

	return ret;
}

#endif

static void __configure(void)
{
	return 0;
}

typedef int (*type_write_to_log)(log_id_t, log_priority, const char *tag, const char *msg);

static type_write_to_log dlog_get_log_function(void)
{
	static type_write_to_log g_write_to_log = NULL;

	if (g_write_to_log)
		return g_write_to_log;

	pthread_mutex_lock(&g_log_init_lock);

	if (!g_write_to_log) {

	/* configuration */
		__configure();
#ifdef HAVE_SYSTEMD_JOURNAL
		g_write_to_log = __write_to_log_sd_journal;
#else
		char path_to_log[PATH_MAX];
		int ret;

		/* open device */
		ret = snprintf(path_to_log, sizeof(path_to_log), "/dev/%s", g_log_main);
		if (sizeof(path_to_log) > ret)
			g_log_fds[LOG_ID_MAIN] = TEMP_FAILURE_RETRY( open(path_to_log, O_WRONLY));
		ret = snprintf(path_to_log, sizeof(path_to_log), "/dev/%s", g_log_radio);
		if (sizeof(path_to_log) > ret)
			g_log_fds[LOG_ID_RADIO] = TEMP_FAILURE_RETRY( open(path_to_log, O_WRONLY));
		ret = snprintf(path_to_log, sizeof(path_to_log), "/dev/%s", g_log_system);
		if (sizeof(path_to_log) > ret)
			g_log_fds[LOG_ID_SYSTEM] = TEMP_FAILURE_RETRY( open(path_to_log, O_WRONLY));
		ret = snprintf(path_to_log, sizeof(path_to_log), "/dev/%s", g_log_apps);
		if (sizeof(path_to_log) > ret)
			g_log_fds[LOG_ID_APPS] = TEMP_FAILURE_RETRY( open(path_to_log, O_WRONLY));

		if (g_log_fds[LOG_ID_MAIN] < 0 || g_log_fds[LOG_ID_RADIO] < 0) {
			g_write_to_log = __write_to_log_null;
		} else {
			g_write_to_log = __write_to_log_kernel;
		}

		if (g_log_fds[LOG_ID_SYSTEM] < 0)
			g_log_fds[LOG_ID_SYSTEM] = g_log_fds[LOG_ID_MAIN];
		if (g_log_fds[LOG_ID_APPS] < 0)
			g_log_fds[LOG_ID_APPS] = g_log_fds[LOG_ID_MAIN];
#endif
	}
	pthread_mutex_unlock(&g_log_init_lock);

	return g_write_to_log;
}

int __dlog_vprint(log_id_t log_id, int prio, const char *tag, const char *fmt, va_list ap)
{
	char buf[g_dlog_buf_size];
	type_write_to_log write_function;

	if (LOG_ID_MAX <= log_id)
		return 0;

	write_function = dlog_get_log_function();

	if (!write_function)
		return -1;


	vsnprintf(buf, g_dlog_buf_size, fmt, ap);

	return write_function(log_id, prio, tag, buf);
}

int __dlog_print(log_id_t log_id, int prio, const char *tag, const char *fmt, ...)
{
	va_list ap;
	char buf[g_dlog_buf_size];
	type_write_to_log write_function;

	if (LOG_ID_MAX <= log_id)
		return 0;

	write_function = dlog_get_log_function();

	if (!write_function)
		return -1;


	va_start(ap, fmt);
	vsnprintf(buf, g_dlog_buf_size, fmt, ap);
	va_end(ap);

	return write_function(log_id, prio, tag, buf);
}

int _get_logging_on(void)
{
	return 1;
}
