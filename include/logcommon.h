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

#define	LOG_MAIN_CONF_PREFIX    "LOG_MAIN="
#define LOG_RADIO_CONF_PREFIX   "LOG_RADIO="
#define LOG_SYSTEM_CONF_PREFIX  "LOG_SYSTEM="
#define LOG_APPS_CONF_PREFIX    "LOG_APPS="
#define LOG_TYPE_CONF_PREFIX	"LOG_TYPE="

#define LOG_BUF_SIZE    1024
#define LOG_CONFIG_FILE "/opt/etc/dlog.conf"

int get_log_dev_names(char devs[LOG_ID_MAX][PATH_MAX]);

log_id_t log_id_by_name(const char *name);

#endif /* _LOGCOMMON_H */
