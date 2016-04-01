/*
 * Copyright (c) 2005-2008, The Android Open Source Project
 * Copyright (c) 2009-2015, Samsung Electronics Co., Ltd. All rights reserved.
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <linux/limits.h>
#include <dlog_ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>

#include <libudev.h>

#include <logcommon.h>
#include <dlog.h>

#ifdef DLOG_BACKEND_KMSG

#define DEV_KMSG	"/dev/kmsg"

int g_minors[LOG_ID_MAX] = {-1, -1, -1, -1};

static int create_kmsg_devs(int fd)
{
	int i;
	struct kmsg_cmd_buffer_add cmd = {
		.size = 1024*256,
		.mode = 0662,
	};

	for (i = 0; i < LOG_ID_MAX; i++) {
		if (0 > ioctl(fd, KMSG_CMD_BUFFER_ADD, &cmd)) {
			_E("ioctl KMSG_CMD_BUFFER_ADD failed. (%d)\n", errno);
			return -1;
		}
		g_minors[i] = cmd.minor;
	}
	return 0;
}

static void remove_kmsg_devs(int fd)
{
	int i;

	for (i = 0; i < LOG_ID_MAX; i++) {
		if (g_minors[i] < 0)
			continue;
		if (0 > ioctl(fd, KMSG_CMD_BUFFER_DEL, &g_minors[i]))
			_E("ioctl KMSG_CMD_BUFFER_DEL failed. (%d)\n", errno);
	}
}

static int write_config_kmsg(FILE *config_file)
{
	return fprintf(config_file,
			"%s%s\n"
			"%s%s%d\n"
			"%s%s%d\n"
			"%s%s%d\n"
			"%s%s%d",
			LOG_TYPE_CONF_PREFIX, "kmsg",
			LOG_MAIN_CONF_PREFIX, DEV_KMSG, g_minors[LOG_ID_MAIN],
			LOG_RADIO_CONF_PREFIX, DEV_KMSG, g_minors[LOG_ID_RADIO],
			LOG_SYSTEM_CONF_PREFIX, DEV_KMSG, g_minors[LOG_ID_SYSTEM],
			LOG_APPS_CONF_PREFIX, DEV_KMSG, g_minors[LOG_ID_APPS]);
}

#elif DLOG_BACKEND_LOGGER

static void write_config_logger(FILE *config_file)
{
	fprintf(config_file,
			"%s%s\n"
			"%s%s\n"
			"%s%s\n"
			"%s%s\n"
			"%s%s",
			LOG_TYPE_CONF_PREFIX,   "logger",
			LOG_MAIN_CONF_PREFIX,   "/dev/log_main",
			LOG_RADIO_CONF_PREFIX,  "/dev/log_radio",
			LOG_SYSTEM_CONF_PREFIX, "/dev/log_system",
			LOG_APPS_CONF_PREFIX,   "/dev/log_apps");
}

#elif DLOG_BACKEND_JOURNAL

static void write_config_journal(FILE *config_file)
{
	fprintf(config_file, "%s%s",
			LOG_TYPE_CONF_PREFIX, "journal");
}

#endif

int main()
{
	FILE *config_file = fopen(KMSG_DEV_CONFIG_FILE, "w");
	if (!config_file) {
		_E("Unable to open %s config file\n", KMSG_DEV_CONFIG_FILE);
		exit(EXIT_FAILURE);
	}

#ifdef DLOG_BACKEND_KMSG
	int kmsg_fd;

	kmsg_fd = open(DEV_KMSG, O_RDWR);
	if (kmsg_fd < 0) {
		_E("Unable to open kmsg device. %d\n", errno);
		exit(EXIT_FAILURE);
	}

	if (0 > create_kmsg_devs(kmsg_fd))
		goto error;

	if (0 > write_config_kmsg(config_file))
		goto error;

	return 0;

error:
	remove_kmsg_devs(kmsg_fd);
	exit(EXIT_FAILURE);

#elif DLOG_BACKEND_LOGGER
	write_config_logger(config_file);
	return 0;

#elif DLOG_BACKEND_JOURNAL
	write_config_journal(config_file);
	return 0;

#endif
}
