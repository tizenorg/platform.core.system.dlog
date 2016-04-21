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

#ifndef DLOG_BACKEND_KMSG
int main () { return 0; }
#else

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

int g_minors[LOG_ID_MAX] = {-1, -1, -1, -1};

static int create_kmsg_devs(int fd)
{
	int i;
	struct kmsg_cmd_buffer_add cmd = {
		.size = 1024*256,
		.mode = 0662,
	};

	for (i=0; i<LOG_ID_MAX; i++) {
		if (0 > ioctl(fd, KMSG_CMD_BUFFER_ADD, &cmd)) {
			_E("ioctl KMSG_CMD_BUFFER_ADD failed. %s\n",
			   strerror(errno));
			return -1;
		}
		g_minors[i] = cmd.minor;
	}
	return 0;
}

static void remove_kmsg_devs(int fd)
{
	int i;

	for (i=0; i<LOG_ID_MAX; i++) {
		if (g_minors[i]<0)
			continue;
		if (0 > ioctl(fd, KMSG_CMD_BUFFER_DEL, &g_minors[i]))
			_E("ioctl KMSG_CMD_BUFFER_DEL failed. %s\n", strerror(errno));
	}
}

int main()
{
	int kmsg_fd, i;
	struct log_config conf;

	log_config_read (&conf);

	kmsg_fd = open(CONFIG_FILENAME, O_RDWR);
	if (kmsg_fd < 0) {
		_E("Unable to open kmsg device. %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (0 > create_kmsg_devs(kmsg_fd))
		goto error;


	for (i = 0; i < LOG_ID_MAX; ++i) {
		char temp [512];
		snprintf(temp, 512, "/dev/kmsg%d", g_minors[i]);
		log_config_set (&conf, log_name_by_id(i), temp);
	}

	log_config_write (&conf);

	return 0;

error:
	remove_kmsg_devs(kmsg_fd);
	exit(EXIT_FAILURE);
}
#endif
