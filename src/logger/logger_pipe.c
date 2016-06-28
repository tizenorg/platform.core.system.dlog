/*
 * Copyright (c) 2016, Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <pwd.h>
#include <grp.h>

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

#define LIST_FOREACH(head, i)					\
	for((i) = (head); i; (i) = (i)->next)

#define LIST_REMOVE(head, i, type)					\
	{												\
	struct type* tmp;								\
	tmp = i->prev;									\
	if (!tmp)										\
		head = i->next;								\
	else											\
		tmp->next = i->next;						\
													\
	if ((i)->next)									\
		(i)->next->prev = tmp;						\
	}

#define LIST_ADD(head, i)						\
	{											\
		if (!(head))							\
		head = i;								\
	else {											\
		(i)->next = head;						\
		head->prev = i;							\
		head = i;								\
	}											\
	}


#define LIST_ITEM_FREE(i, type)						\
	{												\
		struct type* tmp = i->next;					\
		type##_free(i);								\
		i = tmp;									\
	}

#define PIPE_REQUESTED_SIZE (256*4096)
#define FILE_PATH_SIZE (256)
#define INTERVAL_MAX 3600
#define BUFFER_MAX 65535
#define MAX_CONNECTION_Q 100

#define DELIMITER " "

enum reader_type {
	READER_SOCKET=1,
	READER_FILE
};

enum entity_type {
	ENTITY_WRITER = 1,
	ENTITY_BUFFER,
	ENTITY_CONTROL
};

enum writer_state {
	WRITER_SOCKET = 0,
	WRITER_PIPE
};

struct fd_entity {
	int type;
};

struct writer;

struct writer {
	struct fd_entity   _entity;
	int                socket_fd;
	int                pipe_fd[2];
	struct epoll_event event;
	int                rights;

	struct writer*     next;
	struct writer*     prev;
	struct log_buffer* buf_ptr;

	int                readed;
	char               state;
	char               buffer[LOG_MAX_SIZE];
};

struct reader;

struct reader {
	enum reader_type   type;
	struct log_file    file;
	int                current;
	int                buf_id;
	int                dumpcount;
	struct reader*     next;
	struct reader*     prev;
	int                partial_log_size;
	char               partial_log [LOG_MAX_SIZE];
};

struct log_buffer {
	struct fd_entity   _entity;
	int                listen_fd;
	struct epoll_event event;

	int                id;
	int                size;
	int                lines;
	int                head;
	int                tail;
	int                not_empty;
	int                buffered_len;
	int                elapsed;
	char               buffer[0];
};

struct control {
	struct fd_entity   _entity;
	int                fd;
	struct epoll_event event;
	struct log_buffer* buf_ptr;
	int                is_control;
	char               path [PATH_MAX];
};

struct logger {
	int                 epollfd;
	struct control      socket_wr  [LOG_ID_MAX];
	struct control      socket_ctl [LOG_ID_MAX];
	struct writer*      writers;
	struct reader**     readers;
	struct log_buffer** buffers;
	int                 max_buffered_bytes;
	int                 max_buffered_time;
	int                 should_timeout;
	log_format*         default_format;
};

static int parse_permissions (const char * str)
{
	int ret, parsed;
	char * parse_safety;

	if (!str)
		return S_IWUSR | S_IWGRP | S_IWOTH; // 0222: everyone can write

	parsed = strtol (str, & parse_safety, 8); // note, rights are octal
	if (parse_safety != (str + strlen (str)))
		return 0;

	ret = 0;

	// note that R and X are pretty useless, only W makes sense
	if (parsed & 00001) ret |= S_IXOTH;
	if (parsed & 00002) ret |= S_IWOTH;
	if (parsed & 00004) ret |= S_IROTH;
	if (parsed & 00010) ret |= S_IXGRP;
	if (parsed & 00020) ret |= S_IWGRP;
	if (parsed & 00040) ret |= S_IWGRP;
	if (parsed & 00100) ret |= S_IWUSR;
	if (parsed & 00200) ret |= S_IWUSR;
	if (parsed & 00400) ret |= S_IWUSR;
	if (parsed & 01000) ret |= S_ISVTX;
	if (parsed & 02000) ret |= S_ISGID;
	if (parsed & 04000) ret |= S_ISUID;

	return ret;
}

static int change_owners (const char * file, const char * user, const char * group)
{
	uid_t uid = -1;
	gid_t gid = -1;
	struct passwd * pwd = NULL;
	struct group  * grp = NULL;

	if (user)
		pwd = getpwnam (user);

	if (pwd)
		uid = pwd->pw_uid;

	if (group)
		grp = getgrnam (group);

	if (grp)
		gid = grp->gr_gid;

	return! chown (file, uid, gid); // ideally would be fchown, but that is broken
}

static int listen_fd_create (const char* path, int permissions)
{
	struct sockaddr_un server_addr;
	int sd;

	sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd == -1)
		return -errno;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path)-1);
	unlink(server_addr.sun_path);

	if (bind(sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
		goto failure;

	if (permissions)
		if (chmod (path, permissions) < 0) // ideally, fchmod would be used, but that does not work
			goto failure;

	if (listen(sd, MAX_CONNECTION_Q) < 0)
		goto failure;

	return sd;

failure:
	close (sd);
	return -errno;
}

static int add_fd_loop(int epollfd, int fd, struct epoll_event* event)
{
	return epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, event);
}

static int remove_fd_loop(int epollfd, int fd)
{
	return epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
}

static struct writer* writer_create(int fd, int rights)
{
	struct writer* w = calloc(1, sizeof(struct writer));

	if (!w)
		return w;

	w->event.data.ptr = w;
	w->event.events = EPOLLIN;
	w->socket_fd = fd;
	w->_entity.type = ENTITY_WRITER;
	w->rights = rights;
	return w;
}

static void writer_free(struct writer* w)
{
	switch (w->state) {
	case WRITER_SOCKET:
		close (w->socket_fd);
		break;
	case WRITER_PIPE:
		close (w->pipe_fd[0]);
		break;
	default:
		break;
	}
	free(w);
}

static struct log_buffer* buffer_create(int buf_id, int size)
{
	struct log_buffer* lb = calloc(1, sizeof(struct log_buffer) + size);

	if (!lb)
		return NULL;

	lb->id = buf_id;
	lb->size = size;

	return lb;
}

static void buffer_free(struct log_buffer** buffer)
{
	close((*buffer)->listen_fd);
	free(*buffer);
	*buffer = NULL;
}

static void copy_from_buffer(void* dst, int from, int count, struct log_buffer* b)
{
	int size = b->size;
	int s1 = (from + count) > size ? size - from : count;
	int s2 = count - s1;

	memcpy(dst, b->buffer + from, s1);
	memcpy((char*)dst + s1, b->buffer , s2);
}

static void copy_to_buffer(const void* src, int from, int count, struct log_buffer* b)
{
	int size = b->size;
	int s1 = (from + count) > size ? size - from : count;
	int s2 = count - s1;

	memcpy(b->buffer + from, src, s1);
	memcpy(b->buffer, (char*)src + s1, s2);
}

static int buffer_free_space(struct log_buffer* b)
{
	int head = b->head;
	int tail = b->tail;
	int free_space = b->size;
	if (head == tail && b->not_empty)
		free_space = 0;
	else if (head < tail)
		free_space = b->size - (tail-head);
	else if (head > tail)
		free_space = head - tail;

	return free_space;
}

static void buffer_append(const struct logger_entry* entry, struct log_buffer* b, struct reader* reader_head)
{
	while (buffer_free_space(b) <= entry->len) { // note that we use <= instead of < to make sure there's always some extra empty space. This guarantees that head != tail, which serves to disambiguate readers living there
		int old_head = b->head;
		struct reader * reader;
		struct logger_entry* t = (struct logger_entry*)(b->buffer + b->head);
		b->head += t->len;
		b->head %= b->size;
		-- b->lines;
		LIST_FOREACH(reader_head, reader)
			if (reader->current == old_head)
				reader->current = b->head;
	}

	copy_to_buffer(entry, b->tail, entry->len, b);
	b->tail += entry->len;
	b->tail %= b->size;
	b->buffered_len += entry->len;
	b->not_empty = 1;
	++ b->lines;
}

static void add_misc_file_info (int fd)
{
	const int32_t version = PIPE_FILE_FORMAT_VERSION;
	const int32_t endian = 0x12345678;
	int r;

	r = write (fd, &endian, 4);
	if (r <= 0)
		return;

	r = write (fd, &version, 4);
	if (r <= 0)
		return;
}

static int print_out_logs(struct reader* reader, struct log_buffer* buffer)
{
	int r, ret = 0;
	struct logger_entry* ple;
	char tmp [LOG_MAX_SIZE];
	int priority;
	char * tag;
	struct epoll_event ev = { .events = EPOLLOUT, .data.fd = reader->file.fd };
	int epoll_fd;
	int from = reader->current;
	int is_file = 0;

	epoll_fd = epoll_create1 (0);
	r = epoll_ctl (epoll_fd, EPOLL_CTL_ADD, reader->file.fd, &ev);
	if (r == -1 && errno == EPERM)
		is_file = 1;

	if (reader->partial_log_size) {
		if (!is_file && epoll_wait (epoll_fd, &ev, 1, 0) < 1)
			goto cleanup;

		do {
			r = write (reader->file.fd, reader->partial_log, reader->partial_log_size);
		} while (r < 0 && errno == EINTR);

		if (r <= 0)
			goto cleanup;

		if (r < reader->partial_log_size) {
			reader->partial_log_size -= r;
			memmove (reader->partial_log, reader->partial_log + r, reader->partial_log_size);
			goto cleanup;
		}

		reader->partial_log_size = 0;
	}

	while (from != buffer->tail) {
		ple = (struct logger_entry*)(buffer->buffer + from);
		copy_from_buffer(tmp, from, ple->len, buffer);
		ple = (struct logger_entry*)tmp;

		from += ple->len;
		from %= buffer->size;

		priority = ple->msg[0];
		if (priority < 0 || priority > DLOG_SILENT)
			continue;

		tag = ple->msg + 1;
		if (!strlen (tag))
			continue;

		if (!log_should_print_line(reader->file.format, tag, priority))
			continue;

		if (!is_file && epoll_wait (epoll_fd, &ev, 1, 0) < 1)
			goto cleanup;

		do {
			r = write (reader->file.fd, ple, ple->len);
		} while (r < 0 && errno == EINTR);

		if (r > 0)
			reader->file.size += r;

		if (r < ple->len) {
			reader->partial_log_size = ple->len - r;
			memcpy (reader->partial_log, ple + r, reader->partial_log_size);
			goto cleanup;
		} else if ((reader->file.rotate_size_kbytes > 0) && ((reader->file.size / 1024) >= reader->file.rotate_size_kbytes)) {
			rotate_logs(&reader->file);
			add_misc_file_info (reader->file.fd);
		}
	}

	if (reader->dumpcount)
		ret = 1;

cleanup:
	close (epoll_fd);
	reader->current = from;
	return ret;
}

static int send_pipe(int socket, int wr_pipe, int type)
{
	int r;
	struct dlog_control_msg tmp =
		{sizeof(struct dlog_control_msg), DLOG_REQ_PIPE, DLOG_FLAG_ANSW | type};
	struct msghdr msg = { 0 };
	char buf[CMSG_SPACE(sizeof(wr_pipe))];
	memset(buf, '\0', sizeof(buf));
	struct iovec io = { .iov_base = &tmp, .iov_len = sizeof(struct dlog_control_msg) };

	msg.msg_iov = &io;
	msg.msg_iovlen = 1;
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);

	struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(wr_pipe));

	memcpy(CMSG_DATA(cmsg), &wr_pipe, sizeof(wr_pipe));

	msg.msg_controllen = cmsg->cmsg_len;

	if ((r = sendmsg(socket, &msg, 0)) < 0)
		return r;

	close (wr_pipe);
	return 0;
}

static void writer_close_fd(struct logger* server, struct writer* wr)
{
	switch (wr->state) {
	case WRITER_SOCKET:
		remove_fd_loop(server->epollfd, wr->socket_fd);
		close (wr->socket_fd);
		break;
	case WRITER_PIPE:
		remove_fd_loop(server->epollfd, wr->pipe_fd[0]);
		close (wr->pipe_fd[0]);
		break;
	default:
		break;
	}
}

static void reader_free(struct reader* reader)
{
	if (reader->file.path)
		free(reader->file.path);

	close(reader->file.fd);
	log_format_free(reader->file.format);
	free(reader);
}

static int service_reader(struct logger* server, struct reader* reader)
{
	int buf_id = reader->buf_id;
	struct log_buffer* buffer = server->buffers[buf_id];
	int r = 0;

	r = print_out_logs(reader, buffer);
	return r;
}

static int parse_command_line(const char* cmdl, struct logger* server, struct writer* wr)
{
	char cmdline[512];
	int option, argc;
	char *argv [ARG_MAX];
	char *tok;
	char *tok_sv;
	int retval = 0;
	int wr_socket_fd = wr ? wr->socket_fd : -1;
	struct reader * reader;

	if (!server || !cmdl) return EINVAL;

	strncpy (cmdline, cmdl, 512);

	reader = calloc(1, sizeof(struct reader));
	if (!reader) return ENOMEM;

	tok = strtok_r(cmdline, DELIMITER, &tok_sv);
	if (!tok || strcmp(tok, "dlogutil")) return -1;

	argc = 0;
	while (tok && (argc < ARG_MAX)) {
		argv[argc++] = strdup(tok);
		tok = strtok_r(NULL, DELIMITER, &tok_sv);
	}

	reader->type = READER_FILE;
	reader->file.fd = -1;
	reader->file.path = NULL;
	reader->file.format = log_format_from_format(server->default_format);
	reader->file.rotate_size_kbytes = 0;
	reader->file.max_rotated = 0;
	reader->file.size = 0;
	reader->buf_id = 0;
	reader->dumpcount = 0;
	reader->partial_log_size = 0;

	while ((option = getopt(argc, argv, "cdt:gsf:r:n:v:b:")) != -1) {
		switch (option) {
			break;
		case 'd':
		case 't':
			reader->dumpcount = -1;
			break;
		case 'f':
			if (!optarg) {
				retval = -1;
				goto cleanup;
			}
			reader->file.path = strdup(optarg);
			break;
		case 'b':
			if (!optarg) {
				retval = -1;
				goto cleanup;
			}
			reader->buf_id = log_id_by_name (optarg);
			break;
		case 'r':
			if (!optarg || !isdigit(optarg[0])) {
				retval = -1;
				goto cleanup;
			}
			reader->file.rotate_size_kbytes = atoi(optarg);
			break;
		case 'n':
			if (!optarg || !isdigit(optarg[0])) {
				retval = -1;
				goto cleanup;
			}
			reader->file.max_rotated = atoi(optarg);
			break;
		default:
			// everything else gets handled in util directly
			break;
		}
	}

	if (wr && wr->buf_ptr)
		reader->buf_id = wr->buf_ptr->id;

	if ((reader->buf_id >= LOG_ID_MAX) || (reader->buf_id < 0)) {
		retval = -1;
		goto cleanup;
	}

	if (reader->file.path != NULL) {
		reader->file.fd = open (reader->file.path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		add_misc_file_info (reader->file.fd);
	} else {
		reader->file.rotate_size_kbytes = 0;
		reader->file.max_rotated = 0;
		int fds[2];
		if (!pipe2(fds, O_CLOEXEC | O_NONBLOCK)) {
			reader->file.fd = fds[1];
			send_pipe(wr_socket_fd, fds[0], DLOG_FLAG_READ);
		}
	}
	if (reader->file.fd < 0) {
		retval = -1;
		goto cleanup;
	}
	reader->current = server->buffers[reader->buf_id]->head;
	server->should_timeout |= (1 << reader->buf_id);
	server->buffers[reader->buf_id]->elapsed = server->max_buffered_time;
	LIST_ADD(server->readers[reader->buf_id], reader);

cleanup:

	while (argc--)
		free(argv[argc]);

	/* recycle for further usage */
	optarg = NULL;
	optind = 0;
	optopt = 0;

	if (retval) reader_free (reader);

	return retval;
}

static void fd_change_flag(int fd, int flag, int set)
{
	int flags = fcntl(fd, F_GETFL);

	if (set)
		flags |= flag;
	else
		flags &= ~flag;

	fcntl(fd, F_SETFL, flags);
}

/*
	The following handlers use the following return value convention:
	* = 0 - success, but the reader connection needs to be closed (for example as a part of transition to another state).
	* = 1 - success, connection needs to be kept.
	* > 1 - failure of this particular request (value is +errno). Reader connection to be kept.
	* < 0 - failure of connection (value is -errno). Reader needs to be closed.
*/

static int service_writer_handle_req_util(struct logger* server, struct writer* wr, struct dlog_control_msg* msg)
{
	int r;
	if (msg->length <= sizeof(struct dlog_control_msg)
		|| msg->length >= LOG_MAX_SIZE-1)
		return EINVAL;

	msg->data[msg->length - sizeof(struct dlog_control_msg) -1] = 0;
	r = parse_command_line(msg->data, server, wr);

	if (r < 0)
		return r;

	if (wr->readed > msg->length) {
		wr->readed -= msg->length;
		memcpy(wr->buffer, wr->buffer + msg->length, wr->readed);
	} else
		wr->readed = 0;

	return 1;
}

static int service_writer_handle_req_clear(struct logger* server, struct writer* wr, struct dlog_control_msg* msg)
{
	struct reader* reader;

	if (msg->length != (sizeof(struct dlog_control_msg)))
		return EINVAL;

	if (!wr || !wr->buf_ptr)
		return EINVAL;

	wr->buf_ptr->head = wr->buf_ptr->tail = wr->buf_ptr->not_empty = 0;
	wr->buf_ptr->elapsed = wr->buf_ptr->buffered_len = wr->buf_ptr->lines = 0;

	LIST_FOREACH(server->readers[wr->buf_ptr->id], reader) {
		reader->current = 0;
	}

	if (wr->readed > msg->length) {
		wr->readed -= msg->length;
		memcpy(wr->buffer, wr->buffer + msg->length, wr->readed);
	} else
		wr->readed = 0;

	return 1;
}

static int service_writer_handle_req_pipe(struct logger* server, struct writer* wr, struct dlog_control_msg* msg)
{
	int r;
	if (msg->length != sizeof(struct dlog_control_msg))
		return EINVAL;

	if (pipe2(wr->pipe_fd, O_CLOEXEC | O_NONBLOCK) < 0)
		return -errno;

	wr->event.data.ptr = wr;
	wr->event.events = EPOLLIN;
	fd_change_flag(wr->pipe_fd[1], O_NONBLOCK, 0);
	r = send_pipe(wr->socket_fd, wr->pipe_fd[1], DLOG_FLAG_WRITE);
	if (r)
		return r;

	fcntl(wr->pipe_fd[1], F_SETPIPE_SZ, PIPE_REQUESTED_SIZE);
	add_fd_loop(server->epollfd, wr->pipe_fd[0], &wr->event);
	writer_close_fd(server, wr);
	wr->state = WRITER_PIPE;
	wr->readed = 0;
	return 0;
}

static int service_writer_socket(struct logger* server, struct writer* wr, struct epoll_event* event)
{
	int r = 0;
	struct dlog_control_msg* msg;

	if (event->events & EPOLLIN)
		r = read(wr->socket_fd, wr->buffer + wr->readed, LOG_MAX_SIZE - wr->readed);

	if ((r == 0 || r == -1) && event->events & EPOLLHUP)
		return EINVAL;

	do {
		if (r > 0)
			wr->readed += r;

		if (wr->readed < sizeof(msg->length))
			continue;

		msg = (struct dlog_control_msg*) wr->buffer;

		if (msg->length >= LOG_MAX_SIZE)
			return EINVAL;

		if (wr->readed < msg->length)
			continue;

		if (wr->rights) {
			switch (msg->request) {
			case DLOG_REQ_CLEAR:
			r = service_writer_handle_req_clear(server, wr, msg);
				break;
			case DLOG_REQ_HANDLE_LOGUTIL:
				r = service_writer_handle_req_util(server, wr, msg);
				break;
			default:
				return EINVAL;
			}
		} else {
			switch (msg->request) {
			case DLOG_REQ_PIPE:
				r = service_writer_handle_req_pipe(server, wr, msg);
				break;
			default:
				return EINVAL;
			}
		}

		if (r <= 0)
			return r;

		r = read(wr->socket_fd, wr->buffer + wr->readed, LOG_MAX_SIZE - wr->readed);
	} while (r > 0 || ((wr->readed >= sizeof(msg->length) && wr->readed >= msg->length)) );

	return (r >= 0 || (r < 0 && errno == EAGAIN))  ? 0 : r;
}

static int service_writer_pipe(struct logger* server, struct writer* wr, struct epoll_event* event)
{
	struct logger_entry* entry;
	int r = 0;

	if (event->events & EPOLLIN) {
		r = read (wr->pipe_fd[0], wr->buffer + wr->readed, LOG_MAX_SIZE - wr->readed);

		if (r == -1 && errno == EAGAIN)
			return 0;
		else if ((r == 0 || r== -1) && event->events & EPOLLHUP)
			return EINVAL;
		else if (r == 0)
			return -EBADF;

		wr->readed += r;

		entry = (struct logger_entry*)wr->buffer;
		while ((wr->readed >= sizeof(entry->len)) && (entry->len <= wr->readed)) {
			buffer_append(entry, server->buffers[entry->buf_id], server->readers[entry->buf_id]);
			wr->readed -= entry->len;
			server->should_timeout |= (1<<entry->buf_id);
			memmove(wr->buffer, wr->buffer + entry->len, LOG_MAX_SIZE - entry->len);
		}
	} else if (event->events & EPOLLHUP)
		return -EBADF;

	return 0;
}

static int service_writer(struct logger* server, struct writer* wr, struct epoll_event* event)
{
	switch (wr->state) {
	case WRITER_SOCKET:
		return service_writer_socket(server, wr, event);
	case WRITER_PIPE:
		return service_writer_pipe(server, wr, event);
	}

	return 0;
}

static void service_all_readers(struct logger* server, int time_elapsed)
{
	int i = 0;
	int r = 0;
	struct log_buffer** buffers = server->buffers;
	struct reader* reader = NULL;

	for (i = 0; i < LOG_ID_MAX; i++) {
		buffers[i]->elapsed += time_elapsed;
		if (buffers[i]->buffered_len >= server->max_buffered_bytes ||
			buffers[i]->elapsed >= server->max_buffered_time) {
			LIST_FOREACH(server->readers[i], reader) {
				r = service_reader(server, reader);
				if (r) {
					LIST_REMOVE(server->readers[i], reader, reader);
					LIST_ITEM_FREE(reader, reader);
					if (!reader)
						break;
				}
			}

			buffers[i]->buffered_len = 0;
			buffers[i]->elapsed = 0;
			server->should_timeout &= ~(1<<i);
		}
	}
}

static struct logger* logger_create (struct log_config *conf)
{
	struct logger* l = calloc(1, sizeof(struct logger));
	int i = 0;
	int size = 0;
	char conf_key [MAX_CONF_KEY_LEN];
	const char *conf_value, *conf_value2;
	int permissions;

	if (!l)
		goto err1;

	l->epollfd = epoll_create1(0);
	if (l->epollfd == -1)
		goto err1;

	if (!(l->default_format = log_format_new()))
		goto err2;
	log_add_filter_string(l->default_format, "*:d");

	if (!(l->buffers = calloc(LOG_ID_MAX, sizeof(struct log_buffer*))))
		goto err3;

	if (!(l->readers = calloc(LOG_ID_MAX, sizeof(struct reader*))))
		goto err4;

	for (i = 0; i < LOG_ID_MAX; i++) {

		snprintf (conf_key, MAX_CONF_KEY_LEN, "%s_ctl_sock", log_name_by_id (i));
		conf_value = log_config_get (conf, conf_key);
		if (!conf_value)
			goto err5;
		strncpy (l->socket_ctl[i].path, conf_value, sizeof(l->socket_ctl[i].path));

		snprintf (conf_key, MAX_CONF_KEY_LEN, "%s_ctl_sock_rights", log_name_by_id (i));
		conf_value = log_config_get (conf, conf_key);

		permissions = parse_permissions (conf_value);
		if (!permissions)
			goto err5;

		if ((l->socket_ctl[i].fd = listen_fd_create(l->socket_ctl[i].path, permissions)) < 0)
			goto err5;

		snprintf (conf_key, MAX_CONF_KEY_LEN, "%s_ctl_sock_owner", log_name_by_id (i));
		conf_value  = log_config_get (conf, conf_key);
		snprintf (conf_key, MAX_CONF_KEY_LEN, "%s_ctl_sock_group", log_name_by_id (i));
		conf_value2 = log_config_get (conf, conf_key);

		if (!change_owners (l->socket_ctl[i].path, conf_value, conf_value2))
			goto err6;

		l->socket_ctl[i]._entity.type = ENTITY_CONTROL;
		l->socket_ctl[i].event.data.ptr = &l->socket_ctl[i];
		l->socket_ctl[i].event.events = EPOLLIN;
		l->socket_ctl[i].is_control = 1;

		add_fd_loop (l->epollfd, l->socket_ctl[i].fd, &l->socket_ctl[i].event);

		snprintf (conf_key, MAX_CONF_KEY_LEN, "%s_write_sock", log_name_by_id (i));
		conf_value = log_config_get (conf, conf_key);
		if (!conf_value)
			goto err6;
		strncpy(l->socket_wr[i].path, conf_value, sizeof(l->socket_wr[i].path));

		snprintf (conf_key, MAX_CONF_KEY_LEN, "%s_write_sock_rights", log_name_by_id (i));
		conf_value = log_config_get (conf, conf_key);

		permissions = parse_permissions (conf_value);
		if (!permissions)
			goto err6;

		if ((l->socket_wr[i].fd = listen_fd_create(l->socket_wr[i].path, permissions)) < 0)
			goto err6;

		snprintf (conf_key, MAX_CONF_KEY_LEN, "%s_write_sock_owner", log_name_by_id (i));
		conf_value  = log_config_get (conf, conf_key);
		snprintf (conf_key, MAX_CONF_KEY_LEN, "%s_write_sock_group", log_name_by_id (i));
		conf_value2 = log_config_get (conf, conf_key);

		if (!change_owners (l->socket_ctl[i].path, conf_value, conf_value2))
			goto err7;

		l->socket_wr[i]._entity.type = ENTITY_CONTROL;
		l->socket_wr[i].event.data.ptr = &l->socket_wr[i];
		l->socket_wr[i].event.events = EPOLLIN;
		l->socket_wr[i].is_control = 0;

		add_fd_loop (l->epollfd, l->socket_wr[i].fd, &l->socket_wr[i].event);

		snprintf (conf_key, MAX_CONF_KEY_LEN, "%s_size", log_name_by_id(i));
		conf_value = log_config_get (conf, conf_key);
		if (!conf_value)
			goto err7;

		size = atoi (conf_value);

		if (!(l->buffers[i] = buffer_create(i, size)))
			goto err7;

		l->socket_wr[i].buf_ptr = l->buffers[i];
		l->socket_ctl[i].buf_ptr = l->buffers[i];
	}

	return l;

err7:
	close (l->socket_wr[i].fd);
err6:
	close (l->socket_ctl[i].fd);
err5:
	while (i--) {
		close (l->socket_ctl[i].fd);
		close (l->socket_wr[i].fd);
		buffer_free (&l->buffers[i]);
	}
	free (l->readers);
err4:
	free (l->buffers);
err3:
	log_format_free (l->default_format);
err2:
	close (l->epollfd);
err1:
	free(l);
	return NULL;
}

static void logger_free(struct logger* l)
{
	int j;

	if (!l) return;

	log_format_free(l->default_format);

	for (j = 0; j < LOG_ID_MAX; j++) {
		struct writer* i;
		while((i = l->writers)) {
			LIST_REMOVE(l->writers, i, writer);
			writer_free(i);
		}
	}

	for (j = 0; j < LOG_ID_MAX; j++) {
		struct reader* reader;
		while((reader = l->readers[j])) {
			LIST_REMOVE(l->readers[j], reader, reader);
			reader_free(reader);
		}
	}

	for (j = 0; j < LOG_ID_MAX; j++) {
		close (l->socket_ctl[j].fd);
		close (l->socket_wr[j].fd);
		buffer_free(&l->buffers[j]);
	}

	free(l->buffers);
	free(l);
}

static int add_reader(struct logger* server, int buf_id, int fd, enum reader_type type, const char* path, int rot_size, int max_rot)
{
	struct reader* r = calloc(1, sizeof(struct reader));

	if (!r)
		return ENOMEM;

	r->type = type;
	r->file.fd = fd;
	r->buf_id = buf_id;
	if (path != NULL) {
		r->file.path = calloc(1, strlen(path));
		strcpy(r->file.path, path);
	}
	r->file.format = log_format_from_format(server->default_format);
	r->file.rotate_size_kbytes = rot_size;
	r->file.max_rotated = max_rot;
	r->file.size = 0;

	LIST_ADD(server->readers[buf_id], r);

	return 0;
}

static int dispatch_event(struct logger* server, struct fd_entity* entity, struct epoll_event* event)
{
	int r = 1;
	switch(entity->type) {
	case ENTITY_WRITER:
		r = service_writer(server, (struct writer*)entity, event);
		if (r) {
			LIST_REMOVE (server->writers, ((struct writer*)entity), writer);
			writer_free((struct writer*)entity);
		}
		break;
	case ENTITY_CONTROL: {
		struct control* ctrl = (struct control*) event->data.ptr;
		int sock_pipe = accept4(ctrl->fd, NULL, NULL, SOCK_NONBLOCK);
		if (sock_pipe >= 0) {
			struct writer* writer = writer_create(sock_pipe, ctrl->is_control);

			if (writer) {
				writer->buf_ptr = ((struct control*)event->data.ptr)->buf_ptr;
				LIST_ADD(server->writers, writer);
				add_fd_loop(server->epollfd, writer->socket_fd, &writer->event);
			} else
				close (sock_pipe);
		}
		break;
	}
	case ENTITY_BUFFER:
	{
		struct log_buffer* buffer = (struct log_buffer*)event->data.ptr;
		int sock = accept4(buffer->listen_fd, NULL, NULL, SOCK_NONBLOCK);
		if (sock >= 0)
			r = add_reader(server, buffer->id, sock, READER_SOCKET, NULL, 0, 0);

		if (r)
			close(sock);
		break;
	}
	}
	return 0;
}

static int logger_get_timeout(struct logger* server)
{
	int timeout = -1;
	int i = 0;

	if (!server->should_timeout)
		return timeout;

	for (i = 0; i < LOG_ID_MAX; i++) {
		int diff = server->max_buffered_time - server->buffers[i]->elapsed;
		if (diff >= 0 && (diff < timeout || timeout == -1))
			timeout = diff;
	}
	return timeout;
}

static int do_logger(struct logger* server)
{
	int nfds, i;
	int time_left = 0;
	struct timeval tv1;
	struct timeval tv2;

	const int max_events = 1024;
	struct epoll_event events[1024];

	for(;;) {
		gettimeofday(&tv1, NULL);
		time_left = logger_get_timeout(server);
		nfds = epoll_wait(server->epollfd, events, max_events, time_left);

		if (nfds < 0 && errno == EINTR) {
			gettimeofday(&tv2, NULL);
			time_left = (tv2.tv_sec - tv1.tv_sec)*1000 + (tv2.tv_usec - tv1.tv_usec)/1000;
		} else if (nfds < 0)
			goto err;

		for (i = 0; i < nfds; i++) {
			struct fd_entity* entity = (struct fd_entity*) events[i].data.ptr;
			dispatch_event(server, entity, &events[i]);
		}
		service_all_readers(server, time_left);
	}
	return 0;

err:
	logger_free(server);
	return errno;
}

static int parse_configs(struct logger** server)
{
	struct log_config conf;
	const char * conf_val;
	char conf_key [MAX_CONF_KEY_LEN];
	int i = 0;
	if (*server)
		logger_free(*server);

	log_config_read (&conf);

	*server = logger_create (&conf);
	if (!(*server))
		return EINVAL;

	while (1) {
		sprintf (conf_key, "dlog_logger_conf_%d", i++);
		conf_val = log_config_get (&conf, conf_key);
		if (!conf_val)
			break;
		parse_command_line (conf_val, *server, NULL);
	}

	log_config_free (&conf);
	return 0;
}

static void help () {
	printf
		( "Usage: %s [options]\n"
		  "\t-h	   Show this help\n"
		  "\t-b N  Set the size of the log buffer (in bytes)\n"
		  "\t-t N  Set time between writes to file (in seconds)\n"
		, program_invocation_short_name
	);
}

static int parse_args (int argc, char ** argv, struct logger * server)
{
	int temp, option;
	while ((option = getopt(argc, argv, "hb:t:")) != -1) {
		switch (option) {
		case 't':
			if (!isdigit(optarg[0])) {
				help();
				return EINVAL;
			}

			temp = atoi(optarg);
			if (temp < 0) temp = 0;
			if (temp > INTERVAL_MAX) temp = INTERVAL_MAX;
			server->max_buffered_time = temp;

			break;
		case 'b':
			if (!isdigit(optarg[0])) {
				help();
				return EINVAL;
			}

			temp = atoi(optarg);
			if (temp < 0) temp = 0;
			if (temp > BUFFER_MAX) temp = BUFFER_MAX;
			server->max_buffered_bytes = temp;

			break;
		case 'h':
			help();
			return EINVAL;
		default:
			return EINVAL;
		}
	}
	optind = 0;
	optopt = 0;
	optarg = NULL;
	return 0;
}

int main(int argc, char** argv)
{
	int r;
	struct logger* server = NULL;
	signal(SIGPIPE, SIG_IGN);

	r = parse_configs(&server);
	if (r) {
		logger_free (server);
		return r;
	}

	r = parse_args (argc, argv, server);
	if (r) {
		logger_free (server);
		return r;
	}

	return do_logger(server);
}
