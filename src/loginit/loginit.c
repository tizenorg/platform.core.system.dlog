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
#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <linux/limits.h>
#include <kmsg_ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>

#include <logcommon.h>
#include <dlog.h>

#define DEV_KMSG	"/dev/kmsg"

int minors[LOG_ID_MAX] = {-1, -1, -1, -1};

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
		minors[i] = cmd.minor;
	}
	return 0;
}

static void remove_kmsg_devs(int fd)
{
	int i;

	for (i=0; i<LOG_ID_MAX; i++) {
		if (minors[i]<0)
			continue;
		if (0 > ioctl(fd, KMSG_CMD_BUFFER_DEL, &minors[i]))
			_E("ioctl KMSG_CMD_BUFFER_DEL failed. %s\n", strerror(errno));
	}
}

static int get_uid(char *name, uid_t *uid)
{
	struct passwd *pw;

	errno = 0;
	pw = getpwnam(name);
	if (!pw) {
		_E("getpwnam failed. %s\n",
		   errno == 0 ? "User not found" : strerror(errno));
		return -1;
	}
	*uid = pw->pw_uid;
	return 0;
}

static int get_gid(char *name, gid_t *gid)
{
	struct group *gr;

	errno = 0;
	gr = getgrnam(name);
	if (!gr) {
		_E("getgrnam failed. %s\n",
		   errno == 0 ? "Group not found" : strerror(errno));
		return -1;
	}
	*gid = gr->gr_gid;
	return 0;
}

static int change_owner()
{
	int i;
	char buf[PATH_MAX];
	uid_t uid;
	gid_t gid;

	if (0 > get_uid("log", &uid))
		return -1;
	if (0 > get_gid("log", &gid))
		return -1;

	for (i=0; i<LOG_ID_MAX; i++) {
		if (0 > sprintf(buf, "%s%d", DEV_KMSG, minors[i])) {
			_E("sprintf failed\n");
			return -1;
		}
		if (0 > chown(buf, uid, gid)) {
			_E("chown on %s failed. %s\n", buf, strerror(errno));
			return -1;
		}
	}
	return 0;
}

static int write_config(FILE *config_file)
{
	return fprintf(config_file,
		       "%s%s%d\n"
		       "%s%s%d\n"
		       "%s%s%d\n"
		       "%s%s%d",
		       LOG_MAIN_CONF_PREFIX, DEV_KMSG, minors[LOG_ID_MAIN],
		       LOG_RADIO_CONF_PREFIX, DEV_KMSG, minors[LOG_ID_RADIO],
		       LOG_SYSTEM_CONF_PREFIX, DEV_KMSG, minors[LOG_ID_SYSTEM],
		       LOG_APPS_CONF_PREFIX, DEV_KMSG, minors[LOG_ID_APPS]);
}

int main()
{
	FILE *config_file;
	int kmsg_fd = open(DEV_KMSG, O_RDWR);
	if (kmsg_fd < 0) {
		_E("Unable to open kmsg device. %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (0 > create_kmsg_devs(kmsg_fd))
		goto error;

	config_file = fopen(KMSG_DEV_CONFIG_FILE, "w");
	if (!config_file) {
		_E("Unable to open %s config file\n", KMSG_DEV_CONFIG_FILE);
		goto error;
	}

	if (0 > change_owner())
		goto error;

	if (0 > write_config(config_file))
		goto error;

	return 0;

error:
	remove_kmsg_devs(kmsg_fd);
	exit(EXIT_FAILURE);
}
