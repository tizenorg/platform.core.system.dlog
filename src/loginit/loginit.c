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
#include <kmsg_ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>

#include <libudev.h>

#include <logcommon.h>
#include <dlog.h>

#define DEV_KMSG	"/dev/kmsg"

int g_minors[LOG_ID_MAX] = {-1, -1, -1, -1};

static void prepare_udev_monitor(struct udev_monitor **mon)
{
	struct udev *udev;

	udev = udev_new();
	if (!udev) {
		_E("udev_new() failed.\n");
		exit(EXIT_FAILURE);
	}

	*mon = udev_monitor_new_from_netlink(udev, "udev");
	if (!*mon) {
		_E("udev_monitor_new_from_netlink() failed.\n");
		exit(EXIT_FAILURE);
	}
	if (0 > udev_monitor_filter_add_match_subsystem_devtype(*mon, "mem", NULL)) {
		_E("udev_monitor_filter_add_match_subsystem_devtype() failed.\n");
		exit(EXIT_FAILURE);
	}
	if (0 > udev_monitor_enable_receiving(*mon)) {
		_E("udev_monitor_enable_receiving() failed.\n");
		exit(EXIT_FAILURE);
	}
}

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

static int change_owner(struct udev_monitor *mon)
{
	uid_t uid;
	gid_t gid;
	int fd;
	int received_devs;

	if (0 > get_uid("log", &uid))
		return -1;
	if (0 > get_gid("log", &gid))
		return -1;

	fd = udev_monitor_get_fd(mon);

	for (received_devs=0; received_devs < LOG_ID_MAX;) {
		fd_set fds;
		int ret;
		struct udev_device *dev;
		dev_t devnum;
		int i;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		ret = TEMP_FAILURE_RETRY(select(fd+1, &fds, NULL, NULL, NULL));
		if (ret < 0) {
			_E("select() failed. %s\n", strerror(errno));
			return -1;
		}

		dev = udev_monitor_receive_device(mon);
		if (!dev) {
			_E("udev_monitor_receive_device() failed.\n");
			return -1;
		}

		if (strcmp(udev_device_get_action(dev), "add")) {
			udev_device_unref(dev);
			continue;
		}

		devnum = udev_device_get_devnum(dev);
		for (i=0; i<LOG_ID_MAX; i++) {
			char buf[PATH_MAX];

			if (minor(devnum) != (unsigned int)g_minors[i])
				continue;

			if (0 > sprintf(buf, "%s%d", DEV_KMSG, g_minors[i])) {
				_E("sprintf failed\n");
				return -1;
			}
			if (0 > chown(buf, uid, gid)) {
				_E("chown on %s failed. %s\n", buf, strerror(errno));
				return -1;
			}
			received_devs++;
			break;
		}

		udev_device_unref(dev);
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
		       LOG_MAIN_CONF_PREFIX, DEV_KMSG, g_minors[LOG_ID_MAIN],
		       LOG_RADIO_CONF_PREFIX, DEV_KMSG, g_minors[LOG_ID_RADIO],
		       LOG_SYSTEM_CONF_PREFIX, DEV_KMSG, g_minors[LOG_ID_SYSTEM],
		       LOG_APPS_CONF_PREFIX, DEV_KMSG, g_minors[LOG_ID_APPS]);
}

int main()
{
	FILE *config_file;
	struct udev_monitor *mon;
	int kmsg_fd;

	prepare_udev_monitor(&mon);

	kmsg_fd = open(DEV_KMSG, O_RDWR);
	if (kmsg_fd < 0) {
		_E("Unable to open kmsg device. %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (0 > create_kmsg_devs(kmsg_fd))
		goto error;

	if (0 > change_owner(mon))
		goto error;

	config_file = fopen(KMSG_DEV_CONFIG_FILE, "w");
	if (!config_file) {
		_E("Unable to open %s config file\n", KMSG_DEV_CONFIG_FILE);
		goto error;
	}

	if (0 > write_config(config_file))
		goto error;

	return 0;

error:
	remove_kmsg_devs(kmsg_fd);
	exit(EXIT_FAILURE);
}
