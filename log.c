/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: t -*-
 * DLOG
 * Copyright (c) 2005-2008, The Android Open Source Project
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
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
/**
 * @file	log.c
 * @version	0.1
 * @brief	This file is the source file of dlog interface
 */

#include <dlog.h>
#include <dlog-common.h>

#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <systemd/sd-journal.h>

static const size_t DLOG_BUF_SIZE = 4*1024;

static const char *const LOG_MAIN = "log_main";
static const char *const LOG_RADIO = "log_radio";
static const char *const LOG_SYSTEM = "log_system";
static const char *const LOG_APPS = "log_apps";

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

static const char* dlog_id_to_string(log_id_t log_id)
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

static int check_write(log_id_t log_id)
{
	char buf[1];
	int fd;
	ssize_t ret;

	if (log_id == LOG_ID_APPS)
		return 1;

	if (log_id < 0 || log_id >= LOG_ID_MAX)
		return -1;

	return read_platformlog();
}

static int write_to_log(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
	return sd_journal_send("MESSAGE=%s", msg,
			       "PRIORITY=%i", dlog_pri_to_journal_pri(prio),
			       "DLOG_TAG=%s", tag,
			       "DLOG_ID=%s", dlog_id_to_string(log_id),
			       NULL);
}

int __dlog_vprint(log_id_t log_id, int prio, const char *tag, const char *fmt, va_list ap)
{
	int ret;
	char buf[DLOG_BUF_SIZE];

	ret = check_write(log_id);
	if (ret != 1)
		return ret;

	vsnprintf(buf, DLOG_BUF_SIZE, fmt, ap);

	return write_to_log(log_id, prio, tag, buf);
}

int __dlog_print(log_id_t log_id, int prio, const char *tag, const char *fmt, ...)
{
	int ret;
	va_list ap;
	char buf[DLOG_BUF_SIZE];

	ret = check_write(log_id);
	if (ret != 1)
		return ret;

	va_start(ap, fmt);
	vsnprintf(buf, DLOG_BUF_SIZE, fmt, ap);
	va_end(ap);

	return write_to_log(log_id, prio, tag, buf);
}
