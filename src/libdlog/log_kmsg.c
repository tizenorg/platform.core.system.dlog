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

void __dlog_init_backend()
{
	struct log_config conf;
	log_id_t buf_id;

	if (!log_config_read (&conf))
		return;

	for (buf_id = 0; buf_id < LOG_ID_MAX; ++buf_id) {
		const char * const buffer_name = log_config_get(&conf, log_name_by_id(buf_id));

		if (!buffer_name)
			return;

		log_fds[buf_id] = open(buffer_name, O_WRONLY);
	}

	log_config_free (&conf);

	if (log_fds[LOG_ID_MAIN] < 0)
		return;

	write_to_log = __write_to_log_kmsg;

	for (buf_id = 0; buf_id < LOG_ID_MAX; ++buf_id)
		if ((buf_id != LOG_ID_MAIN) && (log_fds[buf_id] < 0))
			log_fds[buf_id] = log_fds[LOG_ID_MAIN];
}

