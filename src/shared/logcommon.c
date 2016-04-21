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

#include <string.h>

#include <logcommon.h>

#define MAX_PREFIX_SIZE 32
#define LINE_MAX        (MAX_PREFIX_SIZE+PATH_MAX)

log_id_t log_id_by_name(const char *name)
{
	if (0 == strcmp(name, "main"))
		return LOG_ID_MAIN;
	else if (0 == strcmp(name, "radio"))
		return LOG_ID_RADIO;
	else if (0 == strcmp(name, "system"))
		return LOG_ID_SYSTEM;
	else if (0 == strcmp(name, "apps"))
		return LOG_ID_APPS;
	else
		return -1;
}

char * log_name_by_id (int id)
{
	switch (id) {
		case 0: return "main";
		case 1: return "radio";
		case 2: return "system";
		case 3: return "apps";
		default: return "";
	}
}
