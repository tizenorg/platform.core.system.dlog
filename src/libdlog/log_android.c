#include <fcntl.h>
#include <sys/socket.h>

#include <dlog.h>
#include <logcommon.h>
#include <logconfig.h>

extern int (*write_to_log)(log_id_t, log_priority, const char *tag, const char *msg);

static int log_fds[(int)LOG_ID_MAX] = { -1, -1, -1, -1 };

static int __write_to_log_android(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
	ssize_t ret;
	int log_fd;
	struct iovec vec[3];

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

	vec[0].iov_base = (unsigned char *) &prio;
	vec[0].iov_len  = 1;
	vec[1].iov_base = (void *) tag;
	vec[1].iov_len  = strlen(tag) + 1;
	vec[2].iov_base = (void *) msg;
	vec[2].iov_len  = strlen(msg) + 1;

	ret = writev(log_fd, vec, 3);
	if (ret < 0)
		ret = -errno;

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

	write_to_log = __write_to_log_android;

	for (buf_id = 0; buf_id < LOG_ID_MAX; ++buf_id)
		if ((buf_id != LOG_ID_MAIN) && (log_fds[buf_id] < 0))
			log_fds[buf_id] = log_fds[LOG_ID_MAIN];
}

