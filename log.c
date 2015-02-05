/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: t -*-
 * DLOG
 * Copyright (c) 2005-2008, The Android Open Source Project
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
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
#include <dlog.h>
#include <dlog-common.h>

#ifdef HAVE_SYSTEMD_JOURNAL
#define SD_JOURNAL_SUPPRESS_LOCATION 1
#include <syslog.h>
#include <systemd/sd-journal.h>
#else
#include <linux/limits.h>
#endif

static const size_t DLOG_BUF_SIZE = 4*1024;

static const char *const LOG_MAIN = "log_main";
static const char *const LOG_RADIO = "log_radio";
static const char *const LOG_SYSTEM = "log_system";
static const char *const LOG_APPS = "log_apps";

#define VALUE_MAX 2
#include <tzplatform_config.h>
#define PLATFORMLOG_ORG_CONF tzplatform_mkpath(TZ_SYS_ETC,"platformlog.conf")

static int platformlog = 0;
static int log_fds[(int)LOG_ID_MAX] = { -1, -1, -1, -1 };
static pthread_mutex_t log_init_lock = PTHREAD_MUTEX_INITIALIZER;

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
		log_fd = log_fds[log_id];
	else
		return -1; // for TC

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

	ret = writev(log_fd, vec, 3);

	return ret;
}


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
			return LOG_MAIN;
		case LOG_ID_RADIO:
			return LOG_RADIO;
		case LOG_ID_SYSTEM:
			return LOG_SYSTEM;
		case LOG_ID_APPS:
			return LOG_APPS;
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
#endif

static int __read_config(const char *file, int *value)
{
	int fd, ret;
	char val[VALUE_MAX];

	if (file == NULL || value == NULL)
		return 0;
	memset(val, 0, sizeof(val));
	fd = open(file, O_RDONLY);
	if (fd < 0) {
		return 0;
	}
	ret = read(fd, val, 1);
	close(fd);
	if (ret != 1)
		return 0;
	*value = atoi(val);
	return 1;
}

static void __configure(void)
{
	int ret;
	ret = __read_config(PLATFORMLOG_CONF_PATH, &platformlog);
	if (!ret)
		ret = __read_config(PLATFORMLOG_ORG_CONF, &platformlog);
	if (!ret)
		platformlog = 0;
}

typedef int (*type_write_to_log)(log_id_t, log_priority, const char *tag, const char *msg);

static type_write_to_log dlog_get_log_function(void)
{
	static type_write_to_log write_to_log = NULL;

	if (write_to_log)
		return write_to_log;

	pthread_mutex_lock(&log_init_lock);
	/* configuration */
	__configure();
#ifdef HAVE_SYSTEMD_JOURNAL
	write_to_log = __write_to_log_sd_journal;
#else
	char path_to_log[PATH_MAX];
	int ret;

	/* open device */
	ret = snprintf(path_to_log, sizeof(path_to_log), "/dev/%s", LOG_MAIN);
	if (sizeof(path_to_log) > ret)
		log_fds[LOG_ID_MAIN] = open(path_to_log, O_WRONLY);
	ret = snprintf(path_to_log, sizeof(path_to_log), "/dev/%s", LOG_RADIO);
	if (sizeof(path_to_log) > ret)
		log_fds[LOG_ID_RADIO] = open(path_to_log, O_WRONLY);
	ret = snprintf(path_to_log, sizeof(path_to_log), "/dev/%s", LOG_SYSTEM);
	if (sizeof(path_to_log) > ret)
		log_fds[LOG_ID_SYSTEM] = open(path_to_log, O_WRONLY);
	ret = snprintf(path_to_log, sizeof(path_to_log), "/dev/%s", LOG_APPS);
	if (sizeof(path_to_log) > ret)
		log_fds[LOG_ID_APPS] = open(path_to_log, O_WRONLY);

	if (log_fds[LOG_ID_MAIN] < 0 || log_fds[LOG_ID_RADIO] < 0) {
		write_to_log = __write_to_log_null;
	} else {
		write_to_log = __write_to_log_kernel;
	}

	if (log_fds[LOG_ID_SYSTEM] < 0)
		log_fds[LOG_ID_SYSTEM] = log_fds[LOG_ID_MAIN];
	if (log_fds[LOG_ID_APPS] < 0)
		log_fds[LOG_ID_APPS] = log_fds[LOG_ID_MAIN];
#endif
	pthread_mutex_unlock(&log_init_lock);

	return write_to_log;
}

int __dlog_vprint(log_id_t log_id, int prio, const char *tag, const char *fmt, va_list ap)
{
	char buf[DLOG_BUF_SIZE];
	type_write_to_log write_function;

	if (LOG_ID_MAX <= log_id)
		return 0;

	write_function = dlog_get_log_function();

	if (!write_function)
		return -1;

	if (log_id != LOG_ID_APPS && !platformlog)
		return 0;

	vsnprintf(buf, DLOG_BUF_SIZE, fmt, ap);

	return write_function(log_id, prio, tag, buf);
}

int __dlog_print(log_id_t log_id, int prio, const char *tag, const char *fmt, ...)
{
	va_list ap;
	char buf[DLOG_BUF_SIZE];
	type_write_to_log write_function;

	if (LOG_ID_MAX <= log_id)
		return 0;

	write_function = dlog_get_log_function();

	if (!write_function)
		return -1;

	if (log_id != LOG_ID_APPS && !platformlog)
		return 0;

	va_start(ap, fmt);
	vsnprintf(buf, DLOG_BUF_SIZE, fmt, ap);
	va_end(ap);

	return write_function(log_id, prio, tag, buf);
}

int _get_logging_on(void)
{
	return 1;
}
