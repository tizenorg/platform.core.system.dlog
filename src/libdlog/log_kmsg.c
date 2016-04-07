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

extern int (*write_to_log)(log_id_t, log_priority, const char *tag, const char *msg) __attribute__((visibility ("hidden")));

static int log_fds[(int)LOG_ID_MAX] = { -1, -1, -1, -1 };
static char log_devs[LOG_ID_MAX][PATH_MAX];

static int __write_to_log_kmsg(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
	ssize_t ret;
	int log_fd;
	size_t len;
	char buf[LOG_BUF_SIZE];

	if (log_id < LOG_ID_MAX)
		log_fd = log_fds[log_id];
	else
		return DLOG_ERROR_INVALID_PARAMETER;

	if (prio < DLOG_VERBOSE || prio >= DLOG_PRIO_MAX)
		return DLOG_ERROR_INVALID_PARAMETER;

	if (!tag)
		tag = "";

	if (!msg)
		return DLOG_ERROR_INVALID_PARAMETER;

	len = snprintf(buf, LOG_BUF_SIZE, "%s;%d;%s", tag, prio, msg);

	ret = write(log_fd, buf, len > LOG_BUF_SIZE ? LOG_BUF_SIZE : len);
	if (ret < 0)
		ret = -errno;

	return ret;
}

void __attribute__((visibility ("hidden"))) __dlog_init_backend()
{
	if (0 == get_log_dev_names(log_devs, NULL)) {
		log_fds[LOG_ID_MAIN]   = open(log_devs[LOG_ID_MAIN],   O_WRONLY);
		log_fds[LOG_ID_SYSTEM] = open(log_devs[LOG_ID_SYSTEM], O_WRONLY);
		log_fds[LOG_ID_RADIO]  = open(log_devs[LOG_ID_RADIO],  O_WRONLY);
		log_fds[LOG_ID_APPS]   = open(log_devs[LOG_ID_APPS],   O_WRONLY);
	}
	if (log_fds[LOG_ID_MAIN] >= 0)
		write_to_log = __write_to_log_kmsg;

	if (log_fds[LOG_ID_RADIO]  < 0)
		log_fds[LOG_ID_RADIO]  = log_fds[LOG_ID_MAIN];
	if (log_fds[LOG_ID_SYSTEM] < 0)
		log_fds[LOG_ID_SYSTEM] = log_fds[LOG_ID_MAIN];
	if (log_fds[LOG_ID_APPS]   < 0)
		log_fds[LOG_ID_APPS]   = log_fds[LOG_ID_MAIN];
}

