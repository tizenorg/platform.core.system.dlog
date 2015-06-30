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


#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <kmsg_ioctl.h>
#include <logcommon.h>

void clear_log(int fd)
{
	int ret = ioctl(fd, KMSG_CMD_CLEAR);
	if (ret < 0) {
		_E("ioctl KMSG_CMD_CLEAR failed. %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void get_log_size(int fd, uint32_t *size)
{
	int ret = ioctl(fd, KMSG_CMD_GET_BUF_SIZE, size);
	if (ret < 0) {
		_E("ioctl KMSG_CMD_GET_BUF_SIZE failed. %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void get_log_read_size_max(int fd, uint32_t *size)
{
	int ret = ioctl(fd, KMSG_CMD_GET_READ_SIZE_MAX, size);
	if (ret < 0) {
		_E("ioctl KMSG_CMD_GET_READ_SIZE_MAX failed. %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
}