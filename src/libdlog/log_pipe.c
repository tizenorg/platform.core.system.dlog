#include "config.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <dlog.h>
#include <logcommon.h>
#include "loglimiter.h"
#include "logconfig.h"


int (*write_to_log)(log_id_t, log_priority, const char *tag, const char *msg) __attribute__((visibility ("hidden"))) = NULL;
int pipe_fd;

static int recv_file_descriptor(int socket) /* Socket from which the file descriptor is read */
{
	int sent_fd;
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
			return *((int *) CMSG_DATA(control_message));
	}
	return -1;
}

static int connect_pipe(const char * path)
{
	int r;
	int fd;
	struct sockaddr_un sa = { .sun_family = AF_UNIX };
	struct dlog_control_msg ctrl_msg = DLOG_CTRL_REQ_PIPE;
	fd = socket(AF_UNIX, type | SOCK_CLOEXEC, 0);

	if (fd < 0)
		return -errno;

	strncpy(sa.sun_path, path, sizeof (sa.sun_path));

	r = connect(fd, (struct sockaddr *) &sa, sizeof(sa));
	if (r < 0) {
		close (fd);
		return -errno;
	}

	r = write (fd, &ctrl_msg, ctrl_msg.length);
	if (r < 0) {
		close (fd);
		return -errno;
	}
	fd = recv_file_descriptor(fd);
	return fd;
}


static int __write_to_log_pipe(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
	ssize_t ret;
	char buf [LOG_BUF_SIZE];
	struct timespec ts;
	int timestamp;

	if (log_id >= LOG_ID_MAX
	|| prio < DLOG_VERBOSE
	|| prio >= DLOG_PRIO_MAX
	|| !msg)
		return DLOG_ERROR_INVALID_PARAMETER;

	if (!tag)
		tag = "";

	clock_gettime (CLOCK_MONOTONIC, &ts);
	timestamp = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

	snprintf
		( buf
		, LOG_BUF_SIZE
		, "%d;%d;%s;%s\n"
		, timestamp
		, prio
		, tag
		, msg
	);
	ret = write (pipe_fd, buf, len);
	if (ret < 0 && errno == EPIPE) {
		pipe_fd = connect_pipe(LOG_PIPE_PATH);
	}
	ret = write (pipe_fd, buf, len);
	return ret;
}

void __dlog_init_backend() __attribute__((visibility ("hidden"))) {
	signal(SIGPIPE, SIG_IGN);
	pipe_fd = connect_pipe();
	write_to_log = __write_to_log_pipe;
}

