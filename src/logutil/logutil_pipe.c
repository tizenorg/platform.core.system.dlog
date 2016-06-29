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
#define MAX_LOGGER_REQUEST_LEN 2048

int buf_id = 0;
int sock_fd = -1;

static int connect_sock(const char * path)
{
	int r;
	int fd;
	struct sockaddr_un sa = { .sun_family = AF_UNIX };
	fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

	if (fd < 0)
		return -errno;

	strncpy(sa.sun_path, path, sizeof(sa.sun_path));

	r = connect(fd, (struct sockaddr *) &sa, sizeof(sa));
	if (r < 0) {
		close(fd);
		return -errno;
	}

	return fd;
}

static int do_clear()
{
	const int size = sizeof(struct dlog_control_msg) + 1;
	struct dlog_control_msg * const msg = calloc(1, size);

	msg->length = size;
	msg->request = DLOG_REQ_CLEAR;
	msg->flags = 0;
	msg->data[0] = buf_id;
	if (write(sock_fd, msg, size) < 0) {
		printf("Error: could not send a CLEAR request to logger; socket write failed\n");
		return 1;
	}
	return 0;
}

static int do_getsize(struct log_config * conf)
{
	char conf_key[MAX_CONF_KEY_LEN];
	const char * conf_value;

	sprintf(conf_key, "%s_size", log_name_by_id(buf_id));
	conf_value = log_config_get(conf, conf_key);
	if (!conf_value) {
		printf("Error: could not get size of buffer #%d (%s); it has no config entry\n", buf_id, log_name_by_id(buf_id));
		return 1;
	}

	printf("Buffer #%d (%s) has size %d KB\n", buf_id, log_name_by_id(buf_id), atoi(conf_value) / 1024);
	return 0;
}

static int send_logger_request(int argc, char ** argv)
{
	char logger_request[MAX_LOGGER_REQUEST_LEN];
	int logger_request_len = snprintf(logger_request, MAX_LOGGER_REQUEST_LEN, "dlogutil");
	struct dlog_control_msg * msg;
	int i;

	for (i = 1; i < argc; ++i)
		logger_request_len += snprintf(logger_request + logger_request_len, MAX_LOGGER_REQUEST_LEN - logger_request_len, " %s", argv[i]);

	logger_request_len += sizeof(struct dlog_control_msg) + 1;

	msg = calloc(1, logger_request_len);
	msg->length = logger_request_len;
	msg->request = DLOG_REQ_HANDLE_LOGUTIL;
	msg->flags = 0;
	memcpy(msg->data, logger_request, logger_request_len - sizeof(struct dlog_control_msg));
	if (write(sock_fd, msg, logger_request_len) < 0) {
		printf("Error: could not send a logger request; socket write failed\n");
		return 0;
	}
	
	free(msg);
	return 1;
}

static void handle_pipe(int pipe_fd)
{
	char buff[LOG_MAX_SIZE];
	int r;
	for (;;) {
		r = read(pipe_fd, buff, LOG_MAX_SIZE);
		if (r <= 0 && errno != EAGAIN)
			return;
		buff[r] = '\0';
		printf("%s", buff);
	}
}

int main(int argc, char ** argv)
{
	char buffer_name[MAX_CONF_VAL_LEN] = "";
	const char * sock_path;
	int pipe_fd;
	struct log_config conf;
	int should_clear = 0;
	int should_getsize = 0;
	int into_file = 0;

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
			strncpy(buffer_name, optarg, MAX_CONF_VAL_LEN);
			break;
		case 'f':
			into_file = 1;
			break;
		default:
			// everything else gets relegated to dlog_logger
			break;
		}
	}

	if (strlen(buffer_name) && (((buf_id = log_id_by_name(buffer_name)) < 0) || (buf_id >= LOG_ID_MAX))) {
		printf("There is no buffer \"%s\"\n", buffer_name);
		return 1;
	}

	log_config_read(&conf);

	if (should_getsize)
		return do_getsize(&conf);

	if ((sock_path = log_config_get(&conf, "pipe_control_socket")) == NULL) {
		printf("Error: dlog config is broken, lacks the pipe_control_socket entry\n");
		return 1;
	}

	log_config_free(&conf);

	if ((sock_fd = connect_sock(sock_path)) < 0) {
		printf("Error: socket connection failed\n");
		return 1;
	}

	if (should_clear)
		return do_clear();

	if (!send_logger_request(argc, argv)) {
		printf("Error: could not send request to logger daemon\n");
		return 1;
	}

	if (into_file) {
		close(sock_fd);
		return 0;
	}

	if ((pipe_fd = recv_file_descriptor(sock_fd)) < 0) {
		printf("Error: failed to receive a logging pipe\n");
		return 1;
	}

	handle_pipe(pipe_fd);

	return 0;
}
