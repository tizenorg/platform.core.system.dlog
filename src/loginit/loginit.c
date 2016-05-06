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
#include <logconfig.h>
#include <dlog.h>

#define DEV_KMSG	"/dev/kmsg"

int g_minors[LOG_ID_MAX] = {-1, -1, -1, -1};

static int create_kmsg_devs(int fd, struct log_config *conf)
{
	int i;
	struct kmsg_cmd_buffer_add cmd = {
		.mode = 0662,
	};

	for (i = 0; i < LOG_ID_MAX; i++) {
		char key [MAX_CONF_KEY_LEN];
		const char *size_str;
		char *size_str_end;

		snprintf (key, MAX_CONF_KEY_LEN, "%s_size", log_name_by_id(i));
		size_str = log_config_get (conf, key);
		if (!size_str)
			return -1;

		errno = 0;
		cmd.size = strtol(size_str, &size_str_end, 10);
		if (errno || (cmd.size <= 0) || (*size_str_end != '\0'))
			return -1;

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

int main()
{
	int kmsg_fd, i;
	struct log_config conf_in;
	struct log_config conf_out;

	log_config_read (&conf_in, get_config_filename (CONFIG_COMMON));
	memset (&conf_out, 0, sizeof(struct log_config));

	kmsg_fd = open(DEV_KMSG, O_RDWR);
	if (kmsg_fd < 0) {
		_E("Unable to open kmsg device. %d\n", errno);
		exit(EXIT_FAILURE);
	}

	if (0 > create_kmsg_devs(kmsg_fd, &conf_in))
		goto error;

	for (i = 0; i < LOG_ID_MAX; ++i) {
		char key [MAX_CONF_KEY_LEN];
		char val [MAX_CONF_VAL_LEN];

		snprintf(key, MAX_CONF_KEY_LEN, "%s_size", log_name_by_id(i));
		snprintf(val, MAX_CONF_VAL_LEN, "%s", log_config_get (&conf_in, key));
		log_config_push (&conf_out, key, val);

		snprintf(val, MAX_CONF_VAL_LEN, "%s%d", DEV_KMSG, g_minors[i]);
		log_config_push (&conf_out, log_name_by_id(i), val);
	}

	log_config_write (&conf_out, get_config_filename (CONFIG_KMSG));

	return 0;

error:
	remove_kmsg_devs(kmsg_fd);
	exit(EXIT_FAILURE);
}
