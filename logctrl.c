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
 * @file    logctrl.c
 * @version 0.1
 * @brief   This file is the source file of dlogctrl
 */

#define _GNU_SOURCE

#include <dlog-common.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

static void show_help()
{
	printf("dlogctrl {get|set} platformlog [0|1]\n\n");
	printf("Gets or sets platformlog option which determines "
	       "whether logs with lower id than LOG_ID_APPS are written.\n");
}

static int write_platformlog(long platformlog)
{
	int fd, ret;
	char buf[1];

	if (platformlog == 0)
		buf[0] = '0';
	else
		buf[0] = '1';

	fd = TEMP_FAILURE_RETRY( open(PLATFORMLOG_CONF_PATH, O_WRONLY));
        if (fd == -1)
		return -1;

	ret =  TEMP_FAILURE_RETRY( write(fd, buf, 1));
	close(fd);
	if (ret == -1)
		return -1;
	return 0;
}

int main(int argc, char **argv)
{
	if (argc == 1) {
		show_help();
		return 0;
	}
	if (argc < 3) {
		fprintf(stderr, "Not enough args!\n\n");
		goto err;
	}
	if (strncmp(argv[2], "platformlog", strlen("platformlog")) != 0) {
		fprintf(stderr, "Bad args!\n\n");
		goto err;
	}
	if (strncmp(argv[1], "get", strlen("get")) == 0) {
		int platformlog = read_platformlog();
		if (platformlog == -1) {
			fprintf(stderr, "Cannot read platformlog!\n\n");
			goto err;
		}
		printf("%d", platformlog);
	} else if (strncmp(argv[1], "set", strlen("set")) == 0) {
		if (argc < 4) {
			fprintf(stderr, "Not enough args!\n\n");
			goto err;
		}
		long platformlog = strtol(argv[3], NULL, 10);
		if (platformlog != 0 && platformlog != 1) {
			fprintf(stderr, "Bad args!\n\n");
			goto err;
		}
		if (write_platformlog(platformlog) == -1) {
			fprintf(stderr, "Cannot write platformlog!\n\n");
			goto err;
		}
	} else {
		fprintf(stderr, "Bad args!\n\n");
		goto err;
	}
	return EXIT_SUCCESS;

err:
	show_help();
	return EXIT_FAILURE;
}
