/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
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
/**
 * @file	log-common.c
 * @version	0.1
 * @brief	This file is the source file of dlog commons
 */

#define _GNU_SOURCE

#include <dlog-common.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int read_platformlog()
{
	int fd, ret;
	char buf[1];
	fd = TEMP_FAILURE_RETRY ( open(PLATFORMLOG_CONF_PATH, O_RDONLY) );
        if (fd == -1)
		return -1;

	ret = TEMP_FAILURE_RETRY ( read(fd, buf, 1) );
	close(fd);
	if (ret != 1)
		return -1;
	switch (buf[0])
	{
		case '0':
			return 0;
		case '1':
			return 1;
		default:
			return -1;
	}
}
