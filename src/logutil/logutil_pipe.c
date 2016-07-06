/*
 * Copyright (c) 2016, Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>

#include <linux/limits.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <logpipe.h>
#include <logprint.h>
#include <log_file.h>
#include <logconfig.h>
#include <queued_entry.h>
#include <sys/inotify.h>
#include "logutil_doc.h"

/* should fit a whole command line with concatenated arguments (reasonably) */
#define MAX_LOGGER_REQUEST_LEN 2048

/* The size (in entries) of the sorting buffer. */
#define SORT_BUFFER_SIZE 16384

/* How large (in bytes) the pipe receiving buffer is. */
#define RECEIVE_BUFFER_SIZE 16384

/* Magic constant for checking file correctness */
#define ENDIANNESS_CHECK 0x12345678

static int buf_id = 0;
static int sock_fd = -1;
static int sort_timeout = 1000; // ms
static log_format * log_fmt;
static char inotify_buffer [sizeof (struct inotify_event) + NAME_MAX + 1];

static struct sorting_vector {
	struct logger_entry * data [SORT_BUFFER_SIZE];
	int size;
	int last_processed;
} logs;

static void push_log (struct logger_entry * p)
{
	int i;
	log_entry entry;

	log_process_log_buffer (p, & entry);
	if (!log_should_print_line (log_fmt, entry.tag, entry.priority))
		return;

	if ((logs.size + 1) % SORT_BUFFER_SIZE == logs.last_processed) {
		free (logs.data [logs.last_processed]);
		logs.last_processed = (logs.last_processed + 1) % SORT_BUFFER_SIZE;
	}

	for (i = logs.size; i > logs.last_processed || (logs.size < logs.last_processed && i <= logs.size); --i) {
		struct logger_entry * e = logs.data[(i ?: SORT_BUFFER_SIZE)-1];
		if (e->sec < p->sec || (e->sec == p->sec && e->nsec < p->nsec)) {
			logs.data[i] = p;
			logs.size = (logs.size + 1) % SORT_BUFFER_SIZE;
			return;
		} else {
			logs.data[i] = e;
		}
		if (!i)
			i = SORT_BUFFER_SIZE;
	}

	logs.data[i] = p;
	logs.size = (logs.size + 1) % SORT_BUFFER_SIZE;
}

static int connect_sock(const char * path)
{
	int r;
	int fd;
	struct sockaddr_un sa = { .sun_family = AF_UNIX };
	fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

	if (fd < 0)
		return -errno;

	strncpy(sa.sun_path, path, sizeof(sa.sun_path));

	r = connect(fd, (struct sockaddr *) &sa, sizeof(sa));
	if (r < 0) {
		close(fd);
		return -errno;
	}

	return fd;
}

static int do_clear()
{
	const int size = sizeof(struct dlog_control_msg);
	struct dlog_control_msg * const msg = calloc(1, size);

	msg->length = size;
	msg->request = DLOG_REQ_CLEAR;
	msg->flags = 0;
	if (write(sock_fd, msg, size) < 0) {
		printf("Error: could not send a CLEAR request to logger; socket write failed\n");
		return 1;
	}
	return 0;
}

static int do_getsize(struct log_config * conf)
{
	char conf_key[MAX_CONF_KEY_LEN];
	const char * conf_value;

	sprintf(conf_key, "%s_size", log_name_by_id(buf_id));
	conf_value = log_config_get(conf, conf_key);
	if (!conf_value) {
		printf("Error: could not get size of buffer #%d (%s); it has no config entry\n", buf_id, log_name_by_id(buf_id));
		return 1;
	}

	printf("Buffer #%d (%s) has size %d KB\n", buf_id, log_name_by_id(buf_id), atoi(conf_value) / 1024);
	return 0;
}

static int send_logger_request(int argc, char ** argv)
{
	char logger_request[MAX_LOGGER_REQUEST_LEN];
	int logger_request_len = snprintf(logger_request, MAX_LOGGER_REQUEST_LEN, "dlogutil");
	struct dlog_control_msg * msg;
	int i;

	for (i = 1; i < argc; ++i)
		logger_request_len += snprintf(logger_request + logger_request_len, MAX_LOGGER_REQUEST_LEN - logger_request_len, " %s", argv[i]);

	logger_request_len += sizeof(struct dlog_control_msg) + 1;

	msg = calloc(1, logger_request_len);
	msg->length = logger_request_len;
	msg->request = DLOG_REQ_HANDLE_LOGUTIL;
	msg->flags = 0;
	memcpy(msg->data, logger_request, logger_request_len - sizeof(struct dlog_control_msg));
	if (write(sock_fd, msg, logger_request_len) < 0) {
		printf("Error: could not send a logger request; socket write failed\n");
		return 0;
	}

	free(msg);
	return 1;
}

static int process_log(struct logger_entry *e, const struct timespec *now)
{
	int s = now->tv_sec - e->sec;
	int ns = now->tv_nsec - e->nsec;

	if (ns < 0) {
		ns += 1000000000;
		-- s;
	}

	if (sort_timeout < (s*1000 + ns/1000000)) {
		log_entry entry;
		log_process_log_buffer(e, & entry);
		log_print_log_line(log_fmt, 1, & entry);
		return 1;
	} else
		return 0;
}

static void handle_pipe(int pipe_fd, int dump, int inotify_fd)
{
	char buff[RECEIVE_BUFFER_SIZE];
	int index = 0;
	int filled = 1;
	int r;
	int eof_is_real = (inotify_fd == -1);
	int accepting_logs = 1;
	struct timespec start_time;

	int epollfd, is_file = 0;
	struct epoll_event ev = { .events = EPOLLIN, .data.fd = pipe_fd };

	epollfd = epoll_create1(0);
	r = epoll_ctl (epollfd, EPOLL_CTL_ADD, pipe_fd, &ev);
	if (r == -1 && errno == EPERM) {
		is_file = 1;
		if (inotify_fd != -1)
			epoll_ctl (epollfd, EPOLL_CTL_ADD, inotify_fd, &ev);
	}

	clock_gettime (CLOCK_MONOTONIC, &start_time);

	fcntl (pipe_fd, F_SETFL, fcntl (pipe_fd, F_GETFL, 0) | O_NONBLOCK);

	for (;;) {
		struct logger_entry *e;
		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);

		if (dump > 0) {
			if (!accepting_logs) {
				int available = logs.size - logs.last_processed;
				if (available < 0)
					available += SORT_BUFFER_SIZE;
				if (dump < available)
					available = dump;
				logs.last_processed = (logs.size - available);
				if (logs.last_processed < 0)
					logs.last_processed += SORT_BUFFER_SIZE;
				dump = 0;
			}
		} else
			while (!filled && logs.last_processed != logs.size)
				if (process_log (logs.data[logs.last_processed], &now)) {
					free(logs.data[logs.last_processed]);
					logs.last_processed = (logs.last_processed + 1) % SORT_BUFFER_SIZE;
				} else
					break;

		if (!accepting_logs) {
			if (logs.last_processed == logs.size)
				break;
			else
				continue;
		}

		if (!is_file && epoll_wait(epollfd, &ev, 1, 100) < 1) {
			filled = 0;
			continue;
		}

		if (is_file && epoll_wait (epollfd, &ev, 1, 0) == 1) {
			r = read (inotify_fd, inotify_buffer, sizeof(inotify_buffer));
			struct inotify_event * temp = (struct inotify_event *) inotify_buffer;
			if (r >= sizeof(struct inotify_event) && temp->mask == IN_MOVE_SELF)
				eof_is_real = 1;
		}

		r = read(pipe_fd, buff + index, RECEIVE_BUFFER_SIZE - index);

		if (r < 0) {
			if (errno != EAGAIN)
				return;
			else {
				filled = 0;
				continue;
			}
		}

		if (r == 0) {
			filled = 0;
			if (eof_is_real)
				accepting_logs = 0;
			continue;
		}

		filled = 1;
		index += r;

		e = (struct logger_entry *)buff;
		while (index > 2 && index >= e->len) {
			struct logger_entry *temp = malloc(e->len);
			if (!e->len)
				break;
			memcpy(temp, buff, e->len);
			index -= e->len;
			memmove(buff, buff + e->len, RECEIVE_BUFFER_SIZE - e->len);
			push_log(temp);
		}
	}
}

int handle_file (char const * filename, int dump, int inotify_fd)
{
	fd_set readfds;
	int endian, version;
	int r;
	int fd;

	fd = open(filename, O_RDONLY);

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	/* Check whether stdin contains anything.
	read() would be blocking if nothing was redirected into stdin
	ergo we need to know about that beforehand. */
	if (!select(fd + 1, &readfds, NULL, NULL, &timeout))
		return 0;

	r = read(fd, &endian, 4);
	if (r <= 0)
		return 0;

	if (endian != ENDIANNESS_CHECK) {
		fprintf (stderr, "Unsupported endianness!\n");
		return 0;
	}

	r = read(fd, &version, 4);
	if (r <= 0)
		return 0;

	if (version != PIPE_FILE_FORMAT_VERSION) {
		fprintf (stderr, "Obsolete file format version!\n");
		return 0;
	}

	handle_pipe(fd, dump, inotify_fd);
	return 1;
}

int handle_continuous (char const *filename)
{
	int inotify_fd = inotify_init1 (IN_NONBLOCK);
	while (1) {
		int wd = inotify_add_watch (inotify_fd, filename, IN_MOVE_SELF);
		handle_file (filename, -1, inotify_fd);
		inotify_rm_watch (inotify_fd, wd);
	}
}

void get_file_info (char *filename, struct log_config *conf, int buf_id)
{
	int i = 0;
	char conf_key [MAX_CONF_KEY_LEN];
	char const *conf_val;
	char command_line [MAX_CONF_VAL_LEN];
	int argc;
	char *argv[ARG_MAX];
	char *tok, *tok_sv;
	int option;
	int current_buf_id;

	while (1) {
		snprintf (conf_key, sizeof (conf_key), "dlog_logger_conf_%d", i++);
		conf_val = log_config_get (conf, conf_key);
		if (!conf_val)
			break;

		strncpy (command_line, conf_val, MAX_CONF_VAL_LEN);
		argc = 0;
		tok = strtok_r (command_line, " ", &tok_sv);
		while (tok && argc < ARG_MAX) {
			argv[argc++] = strdup (tok);
			tok = strtok_r (NULL, " ", &tok_sv);
		}

		current_buf_id = -123;
		while ((option = getopt(argc, argv, "cdt:gsf:r:n:v:b:")) != -1) {
			switch (option) {
				case 'f':
					strncpy (filename, optarg, MAX_CONF_VAL_LEN);
					break;
				case 'b':
					current_buf_id = log_id_by_name (optarg);
					break;
				default:
					break;
			}
		}
		if (current_buf_id == -123)
			current_buf_id = 0;

		while (argc--)
			free (argv [argc]);

		optarg = NULL;
		optopt = 0;
		optind = 0;

		if (current_buf_id == buf_id)
			return;
		else
			filename[0] = '\0';
	}
}

int main(int argc, char ** argv)
{
	char buffer_name[MAX_CONF_VAL_LEN] = "";
	char file_input_name [MAX_CONF_VAL_LEN] = "";
	const char * sock_path;
	const char * conf_value;
	int pipe_fd;
	struct log_config conf;
	int should_clear = 0;
	int should_getsize = 0;
	int dump = 0;
	int into_file = 0;
	int silence = 0;
	char conf_key [MAX_CONF_KEY_LEN];

	log_fmt = log_format_new ();
	log_set_print_format (log_fmt, FORMAT_KERNELTIME);

	while (1) {
		static struct option long_options [] = {
			{"dumpfile", required_argument, 0, 0},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};
		int long_option_id = -1;
		int option = getopt_long (argc, argv, "cdt:gsf:r:n:v:b:h", long_options, &long_option_id);

		if (option < 0)
			break;

		switch (option) {
		case 0:
			switch (long_option_id) {
			case 0:
				strncpy (file_input_name, optarg, MAX_CONF_VAL_LEN);
				break;
			case 1:
				show_help (argv[0]);
				return 1;
			}
			break;
		case 'h':
			show_help (argv[0]);
			return 1;
		case 'd':
			dump = -1;
			break;
		case 't':
			dump = atoi (optarg);
			break;
		case 'c':
			should_clear = 1;
			break;
		case 'g':
			should_getsize = 1;
			break;
		case 'b':
			strncpy(buffer_name, optarg, MAX_CONF_VAL_LEN);
			break;
		case 'f':
			into_file = 1;
			break;
		case 'v':
			log_set_print_format (log_fmt, log_format_from_string (optarg));
			break;
		case 's':
			silence = 1;
			break;
		default:
			// everything else gets relegated to dlog_logger
			break;
		}
	}

	if (!silence)
		if (optind < argc)
			while (optind < argc)
				log_add_filter_string(log_fmt, argv[optind++]);
		else
			log_add_filter_string(log_fmt, "*:D");
	else
		log_add_filter_string(log_fmt, "*:s");

	optind = 0;
	optopt = 0;
	optarg = NULL;

	logs.size = 0;
	logs.last_processed = 0;

	if (strlen(buffer_name) && (((buf_id = log_id_by_name (buffer_name)) < 0) || (buf_id >= LOG_ID_MAX))) {
		printf ("There is no buffer \"%s\"\n", buffer_name);
		return 1;
	}

	if (strlen (file_input_name) && !dump)
		dump = -1;

	log_config_read(&conf);

	conf_value = log_config_get (&conf, "util_sorting_time_window");
	if (conf_value)
		sort_timeout = strtol (conf_value, NULL, 10);
	if (sort_timeout <= 0)
		sort_timeout = 1000;

	if (should_getsize)
		return do_getsize(&conf);

	snprintf (conf_key, sizeof(conf_key), "%s_ctl_sock", log_name_by_id (buf_id));

	if ((sock_path = log_config_get(&conf, conf_key)) == NULL) {
		printf("Error: dlog config is broken, lacks the \"%s\" entry\n", conf_key);
		return 1;
	}

	if (!dump)
		get_file_info (file_input_name, &conf, buf_id);

	log_config_free(&conf);

	if (!dump && strlen (file_input_name)) {
		handle_continuous (file_input_name);
		return 0;
	}

	if (strlen (file_input_name) && handle_file (file_input_name, dump, -1))
		return 0;

	if ((sock_fd = connect_sock(sock_path)) < 0) {
		printf("Error: socket connection failed\n");
		return 1;
	}

	if (should_clear)
		return do_clear();

	if (!send_logger_request(argc, argv)) {
		printf("Error: could not send request to logger daemon\n");
		return 1;
	}

	if (into_file) {
		close(sock_fd);
		return 0;
	}

	if ((pipe_fd = recv_file_descriptor(sock_fd)) < 0) {
		printf("Error: failed to receive a logging pipe\n");
		return 1;
	}

	handle_pipe(pipe_fd, dump, -1);

	return 0;
}
