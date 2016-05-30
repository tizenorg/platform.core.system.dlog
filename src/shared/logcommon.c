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
#include <syslog.h>

#include <logcommon.h>

#define MAX_PREFIX_SIZE 32
#define LINE_MAX        (MAX_PREFIX_SIZE+PATH_MAX)

static struct {
	log_id_t id;
	char * name;
} logid_map [] = {
	{ .id = LOG_ID_MAIN,   .name = "main" },
	{ .id = LOG_ID_RADIO,  .name = "radio" },
	{ .id = LOG_ID_SYSTEM, .name = "system" },
	{ .id = LOG_ID_APPS,   .name = "apps" },
};

log_id_t log_id_by_name(const char *name)
{
	log_id_t i;

	for (i = 0; i < LOG_ID_MAX; ++i)
		if (!strcmp (name, logid_map[i].name))
			return logid_map[i].id;

	return LOG_ID_INVALID;
}

char * log_name_by_id (log_id_t id)
{
	log_id_t i;

	for (i = 0; i < LOG_ID_MAX; ++i)
		if (id == logid_map[i].id)
			return logid_map[i].name;

	return "";
}

/* Sends a message to syslog in case dlog has a critical failure and cannot make its own log */
void syslog_critical_failure (const char * message)
{
	openlog (NULL, LOG_PERROR | LOG_CONS | LOG_PID | LOG_NDELAY, 0);
	syslog (LOG_CRIT, "DLOG CRITIAL FAILURE: %s", message);
	closelog ();
}
