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

static char* get_dev_from_line(char *line, char *prefix)
{
	size_t len = strlen(prefix);
	if (strncmp(line, prefix, len))
		return NULL;
	return line + len;
}

int get_log_dev_names(char devs[LOG_ID_MAX][PATH_MAX])
{
	int i;
	FILE *config_file;
	char line[LINE_MAX];

	for (i = 0; i < LOG_ID_MAX; i++)
		devs[i][0] = '\0';

	config_file = fopen(KMSG_DEV_CONFIG_FILE, "r");
	if (!config_file)
		return -1;

	while (fgets(line, LINE_MAX, config_file)) {
		char *dev;
		int len = strlen(line);

		if (line[0] == '\n' || line[0] == '#')
			continue;

		if (line[len-1] == '\n')
			line[len-1] = '\0';

		dev = get_dev_from_line(line, LOG_TYPE_CONF_PREFIX);
		if (dev)
			continue;

		dev = get_dev_from_line(line, LOG_MAIN_CONF_PREFIX);
		if (dev) {
			strncpy(devs[LOG_ID_MAIN], dev, PATH_MAX);
			continue;
		}
		dev = get_dev_from_line(line, LOG_RADIO_CONF_PREFIX);
		if (dev) {
			strncpy(devs[LOG_ID_RADIO], dev, PATH_MAX);
			continue;
		}
		dev = get_dev_from_line(line, LOG_SYSTEM_CONF_PREFIX);
		if (dev) {
			strncpy(devs[LOG_ID_SYSTEM], dev, PATH_MAX);
			continue;
		}
		dev = get_dev_from_line(line, LOG_APPS_CONF_PREFIX);
		if (dev) {
			strncpy(devs[LOG_ID_APPS], dev, PATH_MAX);
			continue;
		}

		goto error;
	}

	for (i = 0; i < LOG_ID_MAX; i++) {
		if (devs[i][0] == '\0')
			goto error;
	}

	return 0;

error:
	fclose(config_file);
	return -1;
}

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
