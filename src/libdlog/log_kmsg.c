#include <fcntl.h>
#include <unistd.h>

#include <dlog.h>
#include <logcommon.h>

extern int (*write_to_log)(log_id_t, log_priority, const char *tag, const char *msg);

static int log_fds[(int)LOG_ID_MAX] = { -1, -1, -1, -1 };
static char log_devs[LOG_ID_MAX][PATH_MAX];

static int __write_to_log_kmsg(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
	ssize_t ret;
	int log_fd;
	ssize_t prefix_len, written_len, last_msg_len;
	char buf[LOG_ATOMIC_SIZE];
	const char *msg_ptr = msg;

	if (log_id > LOG_ID_INVALID && log_id < LOG_ID_MAX)
		log_fd = log_fds[log_id];
	else
		return DLOG_ERROR_INVALID_PARAMETER;

	if (prio < DLOG_VERBOSE || prio >= DLOG_PRIO_MAX)
		return DLOG_ERROR_INVALID_PARAMETER;

	if (!tag)
		tag = "";

	if (!msg)
		return DLOG_ERROR_INVALID_PARAMETER;

	ret = 0;
	prefix_len = strlen(tag) + 3;
	while (1) {
		last_msg_len = snprintf(buf, LOG_ATOMIC_SIZE, "%s;%d;%s", tag, prio, msg_ptr);
		written_len = write(log_fd, buf,
				last_msg_len < LOG_ATOMIC_SIZE - 1 ? last_msg_len : LOG_ATOMIC_SIZE - 1);

		if (written_len < 0)
			return -errno;

		ret += (written_len - prefix_len);
		msg_ptr += (written_len - prefix_len);

		if (*(msg_ptr) == '\0')
			break;
	}

	return ret;
}

void __dlog_init_backend()
{
	if (0 == get_log_dev_names(log_devs)) {
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

