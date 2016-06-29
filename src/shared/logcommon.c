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

#include <string.h>
#include <syslog.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <logcommon.h>

#define MAX_PREFIX_SIZE 32
#define LINE_MAX        (MAX_PREFIX_SIZE+PATH_MAX)

static struct {
	log_id_t id;
	char * name;
} logid_map[] = {
	{ .id = LOG_ID_MAIN,   .name = "main" },
	{ .id = LOG_ID_RADIO,  .name = "radio" },
	{ .id = LOG_ID_SYSTEM, .name = "system" },
	{ .id = LOG_ID_APPS,   .name = "apps" },
};

log_id_t log_id_by_name(const char *name)
{
	log_id_t i;

	for (i = 0; i < LOG_ID_MAX; ++i)
		if (!strcmp(name, logid_map[i].name))
			return logid_map[i].id;

	return LOG_ID_INVALID;
}

char * log_name_by_id(log_id_t id)
{
	log_id_t i;

	for (i = 0; i < LOG_ID_MAX; ++i)
		if (id == logid_map[i].id)
			return logid_map[i].name;

	return "";
}

/* Sends a message to syslog in case dlog has a critical failure and cannot make its own log */
void syslog_critical_failure(const char * message)
{
	openlog(NULL, LOG_PERROR | LOG_CONS | LOG_PID | LOG_NDELAY, 0);
	syslog(LOG_CRIT, "DLOG CRITIAL FAILURE: %s", message);
	closelog();
}

int recv_file_descriptor(int socket)
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

	if ((res = recvmsg(socket, &message, 0)) <= 0)
	return res;

	/* Iterate through header to find if there is a file descriptor */
	for (control_message = CMSG_FIRSTHDR(&message); control_message != NULL; control_message = CMSG_NXTHDR(&message, control_message)) {
		if ((control_message->cmsg_level == SOL_SOCKET) && (control_message->cmsg_type == SCM_RIGHTS))
			return *(CMSG_DATA(control_message));
	}
	return -1;
}
