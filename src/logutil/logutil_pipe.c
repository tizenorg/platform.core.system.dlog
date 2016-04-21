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

static int recv_file_descriptor(int socket)
{
	struct msghdr message;
	struct iovec iov[1];
	struct cmsghdr *control_message = NULL;
	char ctrl_buf[CMSG_SPACE(sizeof(int))];
	char data[1];
	int res;

	memset(&message, 0, sizeof(struct msghdr));
	memset(ctrl_buf, 0, CMSG_SPACE(sizeof(int)));

	/* For the dummy data */
	iov[0].iov_base = data;
	iov[0].iov_len = sizeof(data);

	message.msg_name = NULL;
	message.msg_namelen = 0;
	message.msg_control = ctrl_buf;
	message.msg_controllen = CMSG_SPACE(sizeof(int));
	message.msg_iov = iov;
	message.msg_iovlen = 1;

	if((res = recvmsg(socket, &message, 0)) <= 0)
	return res;

	/* Iterate through header to find if there is a file descriptor */
	for(control_message = CMSG_FIRSTHDR(&message); control_message != NULL; control_message = CMSG_NXTHDR(&message, control_message)) {
		if( (control_message->cmsg_level == SOL_SOCKET) && (control_message->cmsg_type == SCM_RIGHTS) )
			return *(CMSG_DATA(control_message));
	}
	return -1;
}

int main (int argc, char ** argv)
{
	int misc = 0;  // flags for misc stuff that cannot get relegated to dlog_logger directly
	int buf_id = 0;
	char huge_string [2048];
	char sock_path [1024];
	const char * temp;
	int sock_fd, pipe_fd;
	char temp2 [20];
	int i, size;
	struct log_config conf;
	struct dlog_control_msg * msg;

	while (1) {
		int option;
		option = getopt(argc, argv, "cdt:gsf:r:n:v:b:D");

		if (option < 0) break;

		switch (option) {
		case 'c':
			misc += 1;
			break;
		case 'g':
			misc += 2;
			break;
		case 'b':
			buf_id = log_id_by_name (optarg);
			strncpy (huge_string, optarg, 2048);
			break;
		case 'f':
			misc += 4;
			break;
		default:
			// everything else gets relegated to dlog_logger
			break;
		}
	}

	log_config_read (&conf);

	temp = log_config_get(&conf, "pipe_control_socket");
	if (temp) sprintf(sock_path, "%s", temp);
	else {
		printf("config is bork\n");
		exit (1);
	}

	sock_fd = connect_sock (sock_path);

	if (misc & 1) {
		size = sizeof(struct dlog_control_msg) + 1;
		msg = calloc(1, size);
		msg->length = size;
		msg->request = DLOG_REQ_CLEAR;
		msg->flags = 0;
		msg->data[0] = buf_id;
		if (write (sock_fd, msg, size) < 0) exit (1);
		free (msg);
	}

	if (misc & 2) {
		sprintf(temp2, "%s_size", log_name_by_id (buf_id));
		if (!log_config_get(&conf, temp2)) {
			printf ( "There is no buffer \"%s\"\n"
				, huge_string
			);
		} else {
			printf ( "Buffer #%d (%s) has size %d KB\n"
				, buf_id
				, log_name_by_id (buf_id)
				, atoi (log_config_get (&conf, temp2)) / 1024
			);
		}
	}

	log_config_free (&conf);

	if (misc & 3) {
		close (sock_fd);
		return 0;
	}

	strcpy (huge_string, "dlogutil");
	for (i = 1; i < argc; ++i)
	{
		strcat (huge_string, " ");
		strcat (huge_string, argv[i]);
	}
	size = sizeof(struct dlog_control_msg) + strlen (huge_string) + 1;

	msg = calloc(1, size);
	msg->length = size;
	msg->request = DLOG_REQ_HANDLE_LOGUTIL;
	msg->flags = 0;
	memcpy(msg->data, huge_string, size - sizeof(struct dlog_control_msg));
	if (write (sock_fd, msg, size) < 0) exit(1);
	free (msg);

	if (misc & 4) {
		close (sock_fd);
		return 0;
	}

	pipe_fd = recv_file_descriptor (sock_fd);

	while (1) {
		char buff [LOG_BUF_SIZE];
		int r = read (pipe_fd, buff, LOG_BUF_SIZE);
		if (r <= 0) return 0;
		printf ("%s", buff);
	}

	return 0;
}
