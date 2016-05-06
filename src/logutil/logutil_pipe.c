/*
 * Copyright (c) 2016, Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <linux/limits.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <logpipe.h>
#include <logprint.h>
#include <log_file.h>
#include <logconfig.h>

/* should fit a whole command line with concatenated arguments (reasonably) */
#define HUGE_STR_LEN 2048

static int connect_sock(const char * path)
{
	int r;
	int fd;
	struct sockaddr_un sa = { .sun_family = AF_UNIX };
	fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

	if (fd < 0)
		return -errno;

	strncpy(sa.sun_path, path, sizeof (sa.sun_path));

	r = connect(fd, (struct sockaddr *) &sa, sizeof(sa));
	if (r < 0) {
		close (fd);
		return -errno;
	}

	return fd;
}

int main (int argc, char ** argv)
{
	int buf_id = 0;
	char huge_string [HUGE_STR_LEN] = "";
	char sock_path [MAX_CONF_VAL_LEN];
	const char * temp;
	int sock_fd, pipe_fd;
	int i, size;
	struct log_config conf;
	struct dlog_control_msg * msg;
	int should_clear = 0;
	int should_getsize = 0;
	int into_file = 0;
	int current_huge_str_len = 0;

	while (1) {
		int option = getopt(argc, argv, "cdt:gsf:r:n:v:b:");

		if (option < 0)
			break;

		switch (option) {
		case 'c':
			should_clear = 1;
			break;
		case 'g':
			should_getsize = 1;
			break;
		case 'b':
			strncpy (huge_string, optarg, 2048);
			break;
		case 'f':
			into_file = 1;
			break;
		default:
			// everything else gets relegated to dlog_logger
			break;
		}
	}

	if (!strlen(huge_string)) {
		buf_id = 0;
	} else {
		buf_id = log_id_by_name (huge_string);
		if (buf_id < 0 || buf_id >= LOG_ID_MAX) {
			printf ("There is no buffer \"%s\"\n", huge_string);
			exit (1);
		}
	}

	log_config_read (&conf);

	temp = log_config_get(&conf, "pipe_control_socket");
	if (temp) {
		sprintf(sock_path, "%s", temp);
	} else {
		printf("Error: dlog config is broken, lacks the pipe_control_socket entry\n");
		exit (1);
	}

	sock_fd = connect_sock (sock_path);
	if (sock_fd < 0) {
		printf("Error: socket connection failed\n");
		exit(1);
	}

	if (should_clear) {
		size = sizeof(struct dlog_control_msg) + 1;
		msg = calloc(1, size);
		msg->length = size;
		msg->request = DLOG_REQ_CLEAR;
		msg->flags = 0;
		msg->data[0] = buf_id;
		if (write (sock_fd, msg, size) < 0) {
			printf("Error: logger buffer clear request failed on socket write attempt\n");
			exit (1);
		}
		free (msg);
	}

	if (should_getsize) {
		char conf_key [MAX_CONF_KEY_LEN];
		sprintf(conf_key, "%s_size", log_name_by_id (buf_id));
		printf ("Buffer #%d (%s) has size %d KB\n", buf_id, log_name_by_id (buf_id), atoi (log_config_get (&conf, conf_key)) / 1024);
	}

	log_config_free (&conf);

	if (should_clear || should_getsize) {
		close (sock_fd);
		return 0;
	}

	current_huge_str_len = snprintf (huge_string, HUGE_STR_LEN, "dlogutil");
	for (i = 1; i < argc; ++i)
	{
		current_huge_str_len += snprintf (huge_string + current_huge_str_len, HUGE_STR_LEN - current_huge_str_len, " %s", argv[i]);
	}
	size = sizeof(struct dlog_control_msg) + current_huge_str_len + 1;

	msg = calloc(1, size);
	msg->length = size;
	msg->request = DLOG_REQ_HANDLE_LOGUTIL;
	msg->flags = 0;
	memcpy(msg->data, huge_string, size - sizeof(struct dlog_control_msg));
	if (write (sock_fd, msg, size) < 0) {
		printf("Error: sending a handle logutil request failed on socket write\n");
		exit(1);
	}
	free (msg);

	if (into_file) {
		close (sock_fd);
		return 0;
	}

	pipe_fd = recv_file_descriptor (sock_fd);
	if (pipe_fd < 0) {
		printf("Error: failed to receive a logging pipe\n");
		exit (1);
	}

	while (1) {
		char buff [LOG_BUF_SIZE];
		int r = read(pipe_fd, buff, LOG_BUF_SIZE);
		if (r <= 0 && errno != EAGAIN)
			return 0;
		buff[r] = '\0';
		printf ("%s", buff);
	}

	return 0;
}
