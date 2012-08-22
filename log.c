/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <stdio.h>
#include <errno.h>
#include <dlog.h>

#define LOG_BUF_SIZE	1024

#define LOG_MAIN	"log_main"
#define LOG_RADIO	"log_radio"
#define LOG_SYSTEM	"log_system"
#define LOG_APPS	"log_apps"


static int log_fds[(int)LOG_ID_MAX] = { -1, -1, -1, -1 };

static int g_debug_level= DLOG_SILENT;

static int __dlog_init(log_id_t, log_priority, const char *tag, const char *msg);
static int (*write_to_log)(log_id_t, log_priority, const char *tag, const char *msg) = __dlog_init;
#ifdef HAVE_PTHREADS
static pthread_mutex_t log_init_lock = PTHREAD_MUTEX_INITIALIZER;
#endif


static int __write_to_log_null(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
    return -1;
}

static int __write_to_log_kernel(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
	ssize_t ret;
	int log_fd;
	struct iovec vec[3];

	if(log_id >= LOG_ID_APPS && prio<g_debug_level)
	{
		return 0;
	}
	if( log_id < LOG_ID_MAX )
		log_fd = log_fds[log_id];
	else
		return -1; // for TC

	if (!tag)
		  tag = "";

	vec[0].iov_base	= (unsigned char *) &prio;
	vec[0].iov_len	= 1;
	vec[1].iov_base	= (void *) tag;
	vec[1].iov_len	= strlen(tag) + 1;
	vec[2].iov_base	= (void *) msg;
	vec[2].iov_len	= strlen(msg) + 1;

	ret = writev(log_fd, vec, 3);

	return ret;
}

void init_debug_level(void)
{
	char *debuglevel=getenv("TIZEN_DEBUG_LEVEL");
	if(!debuglevel) {
#ifndef NDEBUG
		fprintf(stderr, "Not matched env. variable, TIZEN_DEBUG_LEVEL");
#endif
		return;
	}
	g_debug_level=atoi(debuglevel);
#ifndef NDEBUG
	fprintf(stderr, "debug level init %d(%s) \n",g_debug_level,debuglevel);
#endif
}
static int __dlog_init(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
#ifdef HAVE_PTHREADS
		pthread_mutex_lock(&log_init_lock);
#endif
	// get filtering info

	// open device
	if( write_to_log == __dlog_init)
	{
		log_fds[LOG_ID_MAIN] = open("/dev/"LOG_MAIN, O_WRONLY);
		log_fds[LOG_ID_RADIO] = open("/dev/"LOG_RADIO, O_WRONLY);
		log_fds[LOG_ID_SYSTEM] = open("/dev/"LOG_SYSTEM, O_WRONLY);
		log_fds[LOG_ID_APPS] = open("/dev/"LOG_APPS, O_WRONLY);

		init_debug_level();

		if( log_fds[LOG_ID_MAIN] < 0 || log_fds[LOG_ID_RADIO] < 0 )
		{
			fprintf(stderr, "open log dev is failed\n");
			write_to_log = __write_to_log_null;
		}
		else
			write_to_log = __write_to_log_kernel;

		if( log_fds[LOG_ID_SYSTEM] < 0 )
		{
			log_fds[LOG_ID_SYSTEM] = log_fds[LOG_ID_MAIN];
		}

		if( log_fds[LOG_ID_APPS] < 0 )
		{
			log_fds[LOG_ID_APPS] = log_fds[LOG_ID_MAIN];
		}
	}
#ifdef HAVE_PTHREADS
    pthread_mutex_unlock(&log_init_lock);
#endif
	return write_to_log(log_id, prio, tag, msg);
}

int __dlog_vprint(log_id_t log_id, int prio, const char *tag, const char *fmt, va_list ap)
{
    char buf[LOG_BUF_SIZE];

    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);

    return write_to_log(log_id, prio, tag, buf);
}

int __dlog_print(log_id_t log_id, int prio, const char *tag, const char *fmt, ...)
{
    va_list ap;
    char buf[LOG_BUF_SIZE];

    va_start(ap, fmt);
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);

    return write_to_log(log_id, prio, tag, buf);
}

