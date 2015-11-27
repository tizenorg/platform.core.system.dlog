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

#include <stdint.h>
#include <stdio.h>

#ifdef DEBUG_ON
#define _D(...) printf(__VA_ARGS__)
#else
#define _D(...) do { } while (0)
#endif
#define _E(...) fprintf(stderr, __VA_ARGS__)

#define LOGGER_LOG_MAIN		"log_main"
#define LOGGER_LOG_RADIO	"log_radio"
#define LOGGER_LOG_SYSTEM	"log_system"
#define LOGGER_LOG_APPS	"log_apps"

#define LOGGER_ENTRY_MAX_LEN		(4*1024)
#define LOGGER_ENTRY_MAX_PAYLOAD	(LOGGER_ENTRY_MAX_LEN - sizeof(struct logger_entry))

#endif /* _LOGCOMMON_H */
