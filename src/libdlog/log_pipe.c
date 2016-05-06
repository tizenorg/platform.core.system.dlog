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
#include <logpipe.h>
#include <logprint.h>
#include <logconfig.h>

#define gettid() syscall(SYS_gettid)


extern int (*write_to_log)(log_id_t, log_priority, const char *tag, const char *msg) __attribute__((visibility ("hidden")));
extern pthread_mutex_t log_init_lock __attribute__((visibility ("hidden")));
static int pipe_fd = -1;
static char log_pipe_path [PATH_MAX];


static int connect_pipe(const char * path)
{
	int r;
	int fd;
	struct sockaddr_un sa = { .sun_family = AF_UNIX };
	struct dlog_control_msg ctrl_msg = DLOG_CTRL_REQ_PIPE;
	fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

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
	struct logger_entry* le = (struct logger_entry*)buf;
	struct timespec ts;
	int len;
    int tag_l;
    int msg_l;

	if (log_id >= LOG_ID_MAX
	|| prio < DLOG_VERBOSE
	|| prio >= DLOG_PRIO_MAX
	|| !msg)
		return DLOG_ERROR_INVALID_PARAMETER;

	if (!tag)
		tag = "";

    tag_l = strlen(tag) + 1;
    msg_l = strlen(msg) + 1;
	len = 1 + msg_l + tag_l + sizeof(struct logger_entry);
    if (len > LOG_BUF_SIZE)
        return DLOG_ERROR_INVALID_PARAMETER;

	clock_gettime (CLOCK_MONOTONIC, &ts);
	le->sec = ts.tv_sec;
	le->nsec = ts.tv_nsec;
	le->buf_id = log_id;
    le->len = len;
	le->pid = getpid();
	le->tid = gettid();
	le->msg[0] = prio;

    memcpy(le->msg + 1, tag, tag_l);
    memcpy(le->msg + 1+ tag_l, msg, msg_l);

	if (le->len % 2)
		le->len += 1;
	ret = write (pipe_fd, buf, le->len);
	if (ret < 0 && errno == EPIPE) {
		pthread_mutex_lock(&log_init_lock);
		close (pipe_fd);
		pipe_fd = connect_pipe(log_pipe_path);
		pthread_mutex_unlock(&log_init_lock);
		ret = write (pipe_fd, buf, le->len);
	}
	return ret;
}

void __attribute__((visibility ("hidden")))  __dlog_init_backend()	{
	const char * temp;
	struct log_config conf;
	signal(SIGPIPE, SIG_IGN);

	log_config_read (&conf);
	temp = log_config_get(&conf, "pipe_control_socket");
	if (temp) sprintf(log_pipe_path, "%s", temp);
	else sprintf(log_pipe_path, " ");
	log_config_free (&conf);

	pipe_fd = connect_pipe(log_pipe_path);
	write_to_log = __write_to_log_pipe;
}
