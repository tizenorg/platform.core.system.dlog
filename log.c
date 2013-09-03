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
#include <stdio.h>
#include <errno.h>
#include <dlog.h>
#ifdef HAVE_SYSTEMD_JOURNAL
#define SD_JOURNAL_SUPPRESS_LOCATION 1
#include <syslog.h>
#include <systemd/sd-journal.h>
#endif
#define LOG_BUF_SIZE	1024

#define LOG_MAIN	"log_main"
#define LOG_RADIO	"log_radio"
#define LOG_SYSTEM	"log_system"
#define LOG_APPS	"log_apps"

#define VALUE_MAX 2
#define PLATFORMLOG_CONF "/tmp/.platformlog.conf"
#define PLATFORMLOG_ORG_CONF "/opt/etc/platformlog.conf"

static int platformlog = 0;
static int log_fds[(int)LOG_ID_MAX] = { -1, -1, -1, -1 };
static int (*write_to_log)(log_id_t, log_priority, const char *tag, const char *msg) = NULL;
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

static char dlog_pri_to_char (log_priority pri)
{
	static const char pri_table[DLOG_PRIO_MAX] = {
		[DLOG_VERBOSE] = 'V',
		[DLOG_DEBUG] = 'D',
		[DLOG_INFO] = 'I',
		[DLOG_WARN] = 'W',
		[DLOG_ERROR] = 'E',
		[DLOG_FATAL] = 'F',
		[DLOG_SILENT] = 'S',
	};

	if (pri < 0 || DLOG_PRIO_MAX <= pri || !pri_table[pri])
		return '?';
	return pri_table[pri];
}

#ifdef HAVE_SYSTEMD_JOURNAL
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
		return -EINVAL;

	return pri_table[prio];
}

static inline const char* dlog_id_to_string(log_id_t log_id)
{
	static const char* id_table[LOG_ID_MAX] = {
		[LOG_ID_MAIN]   = LOG_MAIN,
		[LOG_ID_RADIO]  = LOG_RADIO,
		[LOG_ID_SYSTEM] = LOG_SYSTEM,
		[LOG_ID_APPS]   = LOG_APPS,
	};

	if (log_id < 0 || log_id >= LOG_ID_MAX || !id_table[log_id])
		return "UNKNOWN";

	return id_table[log_id];
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

static int __read_config(char *file, int *value)
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
	ret = __read_config(PLATFORMLOG_CONF, &platformlog);
	if (!ret)
		ret = __read_config(PLATFORMLOG_ORG_CONF, &platformlog);
	if (!ret)
		platformlog = 0;
}

static void __dlog_init(void)
{
	pthread_mutex_lock(&log_init_lock);
	/* configuration */
	__configure();
	/* open device */
	log_fds[LOG_ID_MAIN] = open("/dev/"LOG_MAIN, O_WRONLY);
	log_fds[LOG_ID_RADIO] = open("/dev/"LOG_RADIO, O_WRONLY);
	log_fds[LOG_ID_SYSTEM] = open("/dev/"LOG_SYSTEM, O_WRONLY);
	log_fds[LOG_ID_APPS] = open("/dev/"LOG_APPS, O_WRONLY);
	if (log_fds[LOG_ID_MAIN] < 0 || log_fds[LOG_ID_RADIO] < 0) {
		write_to_log = __write_to_log_null;
	} else {
#ifdef HAVE_SYSTEMD_JOURNAL
		write_to_log = __write_to_log_sd_journal;
#else
		write_to_log = __write_to_log_kernel;
#endif
	}

	if (log_fds[LOG_ID_SYSTEM] < 0)
		log_fds[LOG_ID_SYSTEM] = log_fds[LOG_ID_MAIN];
	if (log_fds[LOG_ID_APPS] < 0)
		log_fds[LOG_ID_APPS] = log_fds[LOG_ID_MAIN];
	pthread_mutex_unlock(&log_init_lock);
}

int __dlog_vprint(log_id_t log_id, int prio, const char *tag, const char *fmt, va_list ap)
{
	char buf[LOG_BUF_SIZE];

	if (LOG_ID_MAX <= log_id)
		return 0;

	if (write_to_log == NULL)
		__dlog_init();

	if (log_id != LOG_ID_APPS && !platformlog)
		return 0;

	vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);

	return write_to_log(log_id, prio, tag, buf);
}

int __dlog_print(log_id_t log_id, int prio, const char *tag, const char *fmt, ...)
{
	va_list ap;
	char buf[LOG_BUF_SIZE];

	if (LOG_ID_MAX <= log_id)
		return 0;

	if (write_to_log == NULL)
		__dlog_init();

	if (log_id != LOG_ID_APPS && !platformlog)
		return 0;

	va_start(ap, fmt);
	vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
	va_end(ap);

	return write_to_log(log_id, prio, tag, buf);
}

int _get_logging_on(void)
{
	return 1;
}
