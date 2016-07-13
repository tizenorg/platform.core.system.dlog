/*
 * DLOG
 * Copyright (c) 2005-2008, The Android Open Source Project
 * Copyright (c) 2012-2015 Samsung Electronics Co., Ltd.
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

#ifndef _LOGCOMMON_H
#define _LOGCOMMON_H

#include <linux/limits.h>
#include <stdint.h>
#include <stdio.h>
#include <dlog.h>

#ifdef DEBUG_ON
#define _D(...) printf(__VA_ARGS__)
#else
#define _D(...) do { } while (0)
#endif
#define _E(...) fprintf(stderr, __VA_ARGS__)

/*
 * LOG_ATOMIC_SIZE is calculated according to kernel value
 * 976 = 1024(size of log line) - 48(size of max prefix length)
 */
#define LOG_INFO_SIZE   16
#define LOG_ATOMIC_SIZE 976
#define LOG_BUF_SIZE    1024
#define LOG_MAX_SIZE    4076

#define gettid() syscall(SYS_gettid)

char * log_name_by_id(log_id_t id);
log_id_t log_id_by_name(const char *name);

void syslog_critical_failure(const char * message);
int recv_file_descriptor(int socket);
#endif /* _LOGCOMMON_H */
