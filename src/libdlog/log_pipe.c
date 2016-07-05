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
#include <logcommon.h>

extern int (*write_to_log)(log_id_t, log_priority, const char *tag, const char *msg);
extern pthread_mutex_t log_init_lock;
static int pipe_fd[LOG_ID_MAX];
static char log_pipe_path[LOG_ID_MAX][PATH_MAX];

static int connect_pipe(const char * path)
{
	int r;
	int fd;
	struct sockaddr_un sa = { .sun_family = AF_UNIX };
	struct dlog_control_msg ctrl_msg = DLOG_CTRL_REQ_PIPE;

	fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (fd < 0)
		return -errno;

	strncpy(sa.sun_path, path, sizeof(sa.sun_path));

	r = connect(fd, (struct sockaddr *) &sa, sizeof(sa));
	if (r < 0) {
		close(fd);
		return -errno;
	}

	r = write(fd, &ctrl_msg, ctrl_msg.length);
	if (r < 0) {
		close(fd);
		return -errno;
	}
	fd = recv_file_descriptor(fd);
	return fd;
}


static int __write_to_log_pipe(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
	ssize_t ret;
	char buf[LOG_MAX_SIZE];
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
	if (len > LOG_MAX_SIZE)
		return DLOG_ERROR_INVALID_PARAMETER;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	le->sec = ts.tv_sec;
	le->nsec = ts.tv_nsec;
	le->padding = 0;
	le->len = len;
	le->pid = getpid();
	le->tid = gettid();
	le->msg[0] = prio;

	memcpy(le->msg + 1, tag, tag_l);
	memcpy(le->msg + 1 + tag_l, msg, msg_l);

	if (le->len % 2)
		le->len += 1;
	ret = write(pipe_fd[log_id], buf, le->len);
	if (ret < 0 && errno == EPIPE) {
		pthread_mutex_lock(&log_init_lock);
		close(pipe_fd[log_id]);
		pipe_fd[log_id] = connect_pipe(log_pipe_path[log_id]);
		pthread_mutex_unlock(&log_init_lock);
		ret = write(pipe_fd[log_id], buf, le->len);
	}
	return ret;
}

void __dlog_init_backend()
{
	const char * conf_val;
	char conf_key [MAX_CONF_KEY_LEN];
	struct log_config conf;
	int i;

	/*
	 * We mask SIGPIPE signal because most applications do not install their
	 * own SIGPIPE handler. Default behaviour in SIGPIPE case is to abort the
	 * process. SIGPIPE occurs when e.g. dlog daemon closes read pipe endpoint
	 */
	signal(SIGPIPE, SIG_IGN);

	log_config_read(&conf);

	for (i = 0; i < LOG_ID_MAX; ++i) {
		snprintf(conf_key, sizeof(conf_key), "%s_write_sock", log_name_by_id(i));
		conf_val = log_config_get(&conf, conf_key);
		if (!conf_val) {
			syslog_critical_failure("DLog config lacks the \"%s\" entry!");
			return;
		}
		snprintf(log_pipe_path[i], PATH_MAX, "%s", conf_val);
		pipe_fd[i] = connect_pipe(log_pipe_path[i]);
	}
	log_config_free(&conf);
	write_to_log = __write_to_log_pipe;
}
