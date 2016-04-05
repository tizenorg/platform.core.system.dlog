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

#define SD_JOURNAL_SUPPRESS_LOCATION 1
#include <syslog.h>
#include <systemd/sd-journal.h>

extern int (*write_to_log)(log_id_t, log_priority, const char *tag, const char *msg) __attribute__((visibility ("hidden")));

static inline int dlog_pri_to_journal_pri(log_priority prio)
{
	static int pri_table[DLOG_PRIO_MAX] = {
		[DLOG_UNKNOWN] = LOG_DEBUG,
		[DLOG_DEFAULT] = LOG_DEBUG,
		[DLOG_VERBOSE] = LOG_DEBUG,
		[DLOG_DEBUG]   = LOG_DEBUG,
		[DLOG_INFO]    = LOG_INFO,
		[DLOG_WARN]    = LOG_WARNING,
		[DLOG_ERROR]   = LOG_ERR,
		[DLOG_FATAL]   = LOG_CRIT,
		[DLOG_SILENT]  = -1,
	};

	if (prio < 0 || prio >= DLOG_PRIO_MAX)
		return DLOG_ERROR_INVALID_PARAMETER;

	return pri_table[prio];
}

static inline const char* dlog_id_to_string(log_id_t log_id)
{
	static const char* id_table[LOG_ID_MAX] = {
		[LOG_ID_MAIN]   = "log_main",
		[LOG_ID_RADIO]  = "log_radio",
		[LOG_ID_SYSTEM] = "log_system",
		[LOG_ID_APPS]   = "log_apps",
	};

	if (log_id < 0 || log_id >= LOG_ID_MAX || !id_table[log_id])
		return "UNKNOWN";

	return id_table[log_id];
}


static int __write_to_log_journal(log_id_t log_id, log_priority prio, const char *tag, const char *msg)
{
	const char *lid_str = dlog_id_to_string(log_id);

	pid_t tid = (pid_t)syscall(SYS_gettid);

	if(!msg)
		return DLOG_ERROR_INVALID_PARAMETER;

	if(strncmp(lid_str, "UNKNOWN", 7) == 0)
		return DLOG_ERROR_INVALID_PARAMETER;

	if(prio < DLOG_VERBOSE || prio >= DLOG_PRIO_MAX)
		return DLOG_ERROR_INVALID_PARAMETER;

	if(!tag)
		tag = "";

	struct iovec vec[5];
	char _msg   [LOG_BUF_SIZE + 8];
	char _prio  [LOG_BUF_SIZE + 9];
	char _tag   [LOG_BUF_SIZE + 8];
	char _log_id[LOG_BUF_SIZE + 7];
	char _tid   [LOG_BUF_SIZE + 4];

	snprintf(_msg, LOG_BUF_SIZE + 8, "MESSAGE=%s", msg);
	vec[0].iov_base = (void *)_msg;
	vec[0].iov_len = strlen(vec[0].iov_base);

	snprintf(_prio, LOG_BUF_SIZE + 9, "PRIORITY=%i", dlog_pri_to_journal_pri(prio));
	vec[1].iov_base = (void *)_prio;
	vec[1].iov_len = strlen(vec[1].iov_base);

	snprintf(_tag, LOG_BUF_SIZE + 8, "LOG_TAG=%s", tag);
	vec[2].iov_base = (void *)_tag;
	vec[2].iov_len = strlen(vec[2].iov_base);

	snprintf(_log_id, LOG_BUF_SIZE + 7, "LOG_ID=%s", lid_str);
	vec[3].iov_base = (void *)_log_id;
	vec[3].iov_len = strlen(vec[3].iov_base);

	snprintf(_tid, LOG_BUF_SIZE + 4, "TID=%d", tid);
	vec[4].iov_base = (void *)_tid;
	vec[4].iov_len = strlen(vec[4].iov_base);

	return sd_journal_sendv(vec, 5);
}

void  __attribute__((visibility ("hidden"))) __dlog_init_backend()
{
	write_to_log = __write_to_log_journal;
}

