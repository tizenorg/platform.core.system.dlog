#include <fcntl.h>
#include <unistd.h>

#include <dlog.h>
#include <logcommon.h>
#include <logconfig.h>

extern int (*write_to_log)(log_id_t, log_priority, const char *tag, const char *msg);

static int log_fds[(int)LOG_ID_MAX] = { -1, -1, -1, -1 };

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
	struct log_config conf;
	log_id_t buf_id;

	if (!log_config_read (&conf))
		return;

	for (buf_id = 0; buf_id < LOG_ID_MAX; ++buf_id) {
		const char * const buffer_name = log_config_get(&conf, log_name_by_id(buf_id));

		if (!buffer_name) {
			char err_message [MAX_CONF_VAL_LEN + 64];
			snprintf (err_message, MAX_CONF_VAL_LEN + 64, "LOG BUFFER #%d %s HAS NO PATH SET IN CONFIG", buf_id, buffer_name);
			syslog_critical_failure (err_message);
			return;
		}

		log_fds[buf_id] = open(buffer_name, O_WRONLY);
	}

	log_config_free (&conf);

	if (log_fds[LOG_ID_MAIN] < 0) {
		syslog_critical_failure ("COULD NOT OPEN MAIN LOG");
		return;
	}

	write_to_log = __write_to_log_kmsg;

	for (buf_id = 0; buf_id < LOG_ID_MAX; ++buf_id)
		if ((buf_id != LOG_ID_MAIN) && (log_fds[buf_id] < 0))
			log_fds[buf_id] = log_fds[LOG_ID_MAIN];
}

