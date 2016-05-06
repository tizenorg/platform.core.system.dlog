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
	else										\
	{											\
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
#define LOG_CONTROL_SOCKET "/var/log/control.sock"
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

	int                readed;
	char               state;
	char               buffer[LOG_BUF_SIZE];
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
	char               path [PATH_MAX];
};

struct logger {
	int                 epollfd;
	struct control      control;
	struct writer*      writers;
	struct reader**     readers;
	struct log_buffer** buffers;
	int                 max_buffered_bytes;
	int                 max_buffered_time;
	int                 should_timeout;
	log_format*         default_format;
};

static int listen_fd_create(const char* path)
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
		return -errno;

	if (listen(sd, MAX_CONNECTION_Q) < 0)
		return -errno;

	return sd;
}

static int add_fd_loop(int epollfd, int fd, struct epoll_event* event)
{
	return	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, event);
}

static int remove_fd_loop(int epollfd, int fd)
{
	return	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
}

static struct writer* writer_create(int fd)
{
	struct writer* w = calloc(1, sizeof(struct writer));

	if (!w)
		return w;

	w->event.data.ptr = w;
	w->event.events = EPOLLIN;
	w->socket_fd = fd;
	w->_entity.type = ENTITY_WRITER;
	return w;
}

static void writer_free(struct writer* w)
{
	switch (w->state)
	{
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

static struct log_buffer* buffer_create(int buf_id, int size, const char * path)
{
	struct log_buffer* lb = calloc(1, sizeof(struct log_buffer) + size);

	if (!lb)
		return NULL;

	if ((lb->listen_fd = listen_fd_create(path)) < 0)
		goto err;

	lb->event.data.ptr = lb;
	lb->event.events = EPOLLIN;
	lb->_entity.type = ENTITY_BUFFER;
	lb->id = buf_id;
	lb->size = size;

	return lb;
err:
	free(lb);
	return NULL;
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

static void buffer_append(const struct logger_entry* entry, struct log_buffer* b)
{
	while (buffer_free_space(b) < entry->len) {
		struct logger_entry* t = (struct logger_entry*)(b->buffer + b->head);
		b->head += t->len;
		b->head %= b->size;
		-- b->lines;
	}

	copy_to_buffer(entry, b->tail, entry->len, b);
	b->tail += entry->len;
	b->tail %= b->size;
	b->buffered_len += entry->len;
	b->not_empty = 1;
	++ b->lines;
}

static int print_out_logs(struct log_file* file, struct log_buffer* buffer, int from, int dumpcount)
{
	int r;
	int written;
	log_entry entry;
	struct logger_entry* ple;
	char tmp[LOG_BUF_SIZE];
	int skip = (dumpcount > 0) ? (buffer->lines - dumpcount) : 0;

	while (from !=	buffer->tail) {
		ple = (struct logger_entry*)(buffer->buffer + from);
		if (ple->len + from >= buffer->size) {
			copy_from_buffer(tmp, from, ple->len, buffer);
			ple = (struct logger_entry*)tmp;
		}

		from += ple->len;
		from %= buffer->size;
		r = log_process_log_buffer(ple, &entry);
		if (r)
			return r;

		if (skip --> 0)
			continue;

		if (!log_should_print_line(file->format, entry.tag, entry.priority))
			continue;

		written = log_print_log_line(file->format, file->fd, &entry);
		if (written < 0)
			return 1;

		file->size += written;

		if ((file->rotate_size_kbytes > 0) && ((file->size / 1024) >= file->rotate_size_kbytes))
			rotate_logs(file);
	}
	return 0;
}

static int send_pipe(int socket, int wr_pipe, int type)
{
	int r;
	struct dlog_control_msg tmp = (struct dlog_control_msg)
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

static int send_data(int socket, struct dlog_control_msg* m, const void*  data, int len)
{
	int r;
	struct msghdr msg = { 0 };

	struct iovec io[2] = {
		(struct iovec){
			.iov_base = m,
			.iov_len = sizeof(struct dlog_control_msg)
		},
		(struct iovec){
			.iov_base = (void*)data,
			.iov_len = len
		}
	};

	msg.msg_iov = io;
	msg.msg_iovlen = 2;

	if ((r = sendmsg(socket, &msg, 0)) < 0)
		return r;

	return 0;
}

static void writer_close_fd(struct logger* server, struct writer* wr)
{
	switch (wr->state)
	{
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

	r = print_out_logs(&reader->file, buffer, reader->current, reader->dumpcount);
	reader->current = buffer->tail;
	return r;
}

static int parse_command_line(const char* cmdl, struct logger* server, int wr_socket_fd)
{
	char cmdline[512];
	int option, argc;
	char *argv [ARG_MAX];
	char *tok;
	char *tok_sv;
	int silent = 0;
	int retval = 0;
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

	while ((option = getopt(argc, argv, "cgsdt:f:r:n:v:b:")) != -1) {
		switch (option) {
		case 'c':
		case 'g':
			// already handled in dlogutil
			break;
		case 't':
			if (!optarg) {
				retval = -1;
				goto cleanup;
			}
			reader->dumpcount = atoi(optarg);
			break;
		case 'd':
			if (!reader->dumpcount) reader->dumpcount = -1;
			break;
		case 's':
			silent = 1;
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
		case 'v':
			if (!optarg) {
				retval = -1;
				goto cleanup;
			} else {
				log_print_format print_format;
				print_format = log_format_from_string(optarg);
				if (print_format == FORMAT_OFF)
					break;

				log_set_print_format(reader->file.format, print_format);
			}
			break;
		default:
			break;
		}
	}

	if ((reader->buf_id >= LOG_ID_MAX) || (reader->buf_id < 0)) {
		retval = -1;
		goto cleanup;
	}

	if (!silent) {
		if (argc != optind) {
			while (optind < argc) {
				log_add_filter_string(reader->file.format, argv[optind++]);
			}
		} else {
			log_add_filter_string(reader->file.format, "*:d");
		}
	} else {
		log_add_filter_string(reader->file.format, "*:s");
	}

	if (reader->file.path != NULL) {
		reader->file.fd = open (reader->file.path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	} else {
		reader->file.rotate_size_kbytes = 0;
		reader->file.max_rotated = 0;
		int fds[2];
		if (!pipe2(fds, O_CLOEXEC)) {
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

static int service_writer_handle_req_remove(struct logger* server,struct writer* wr, struct dlog_control_msg* msg)
{
	char * str = msg->data;
	int buf_id;
	struct reader * reader;

	if (msg->length < (sizeof(struct dlog_control_msg)))
		return EINVAL;

	msg->data[msg->length - sizeof(struct dlog_control_msg) -1] = 0;
	buf_id = log_id_by_name(str);
	if (buf_id < 0 || buf_id >= LOG_ID_MAX)
		return EINVAL;

	str = msg->data + strlen(str) + 1;
	LIST_FOREACH(server->readers[buf_id], reader) {
		if (!str ? !reader->file.path : !strcmp(str, reader->file.path)) {
			LIST_REMOVE(server->readers[buf_id], reader, reader);
			LIST_ITEM_FREE(reader, reader);
			break;
		}
	}

	if (wr->readed > msg->length) {
		wr->readed -= msg->length;
		memcpy(wr->buffer, wr->buffer + msg->length, wr->readed);
	} else
		wr->readed = 0;

	return 1;
}

static int service_writer_handle_req_util(struct logger* server,struct writer* wr, struct dlog_control_msg* msg)
{
	int r;
	if (msg->length <= sizeof(struct dlog_control_msg)
		|| msg->length >= LOG_BUF_SIZE-1)
		return EINVAL;

	msg->data[msg->length - sizeof(struct dlog_control_msg) -1] = 0;
	r = parse_command_line(msg->data, server, wr->socket_fd);
	if (r < 0)
		return r;

	if (wr->readed > msg->length) {
		wr->readed -= msg->length;
		memcpy(wr->buffer, wr->buffer + msg->length, wr->readed);
	} else
		wr->readed = 0;

	return 1;
}

static int service_writer_handle_req_clear(struct logger* server,struct writer* wr, struct dlog_control_msg* msg)
{
	int buf_id;
	struct reader* reader;
	struct log_buffer* lb;

	if (msg->length != (sizeof(struct dlog_control_msg) + 1))
		return EINVAL;

	buf_id = msg->data[0];
	if (buf_id < 0 || buf_id >= LOG_ID_MAX)
		return EINVAL;

	lb = server->buffers[buf_id];
	lb->head = lb->tail = lb->not_empty = 0;
	lb->elapsed = lb->buffered_len = lb->lines = 0;

	LIST_FOREACH(server->readers[buf_id], reader) {
		reader->current = 0;
	}

	if (wr->readed > msg->length) {
		wr->readed -= msg->length;
		memcpy(wr->buffer, wr->buffer + msg->length, wr->readed);
	} else
		wr->readed = 0;

	return 1;
}

static int service_writer_handle_req_size(struct logger* server,struct writer* wr, struct dlog_control_msg* msg)
{
	int r;
	int buf_id;
	if (msg->length != (sizeof(struct dlog_control_msg) + 1))
		return EINVAL;

	buf_id = msg->data[0];
	if (buf_id < 0 || buf_id >= LOG_ID_MAX)
		return EINVAL;

	msg->flags |= DLOG_FLAG_ANSW;
	r = send_data (wr->socket_fd,
				   msg,
				   &server->buffers[buf_id]->size,
				   sizeof(server->buffers[buf_id]->size));

	if (r)
		return r;

	if (wr->readed > msg->length) {
		wr->readed -= msg->length;
		memcpy(wr->buffer, wr->buffer + msg->length, wr->readed);
	} else
		wr->readed = 0;

	return 1;
}

static int service_writer_handle_req_pipe(struct logger* server,struct writer* wr, struct dlog_control_msg* msg)
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

static int service_writer_socket(struct logger* server,struct writer* wr, struct epoll_event* event)
{
	int r = 0;
	struct dlog_control_msg* msg;

	if (event->events & EPOLLIN)
		r = read(wr->socket_fd, wr->buffer + wr->readed, LOG_BUF_SIZE - wr->readed);

	if ((r == 0 || r == -1) && event->events & EPOLLHUP)
		return EINVAL;

	do
	{
		if (r > 0)
			wr->readed += r;

		if (wr->readed < sizeof(msg->length))
			continue;

		msg = (struct dlog_control_msg*) wr->buffer;

		if (msg->length >= LOG_BUF_SIZE)
			return EINVAL;

		if (wr->readed < msg->length)
			continue;

		switch (msg->request) {
		case DLOG_REQ_PIPE :
			r = service_writer_handle_req_pipe(server, wr, msg);
			break;
		case DLOG_REQ_BUFFER_SIZE :
			r = service_writer_handle_req_size(server, wr, msg);
			break;
		case DLOG_REQ_CLEAR:
			r = service_writer_handle_req_clear(server, wr, msg);
			break;
		case DLOG_REQ_HANDLE_LOGUTIL:
			r = service_writer_handle_req_util(server, wr, msg);
			break;
		case DLOG_REQ_REMOVE_WRITER:
			r = service_writer_handle_req_remove(server, wr, msg);
			break;
		default:
			return EINVAL;
		}

		if (r <= 0)
			return r;

		r = read(wr->socket_fd, wr->buffer + wr->readed, LOG_BUF_SIZE - wr->readed);
	} while (r > 0 || ((wr->readed >= sizeof(msg->length) && wr->readed >= msg->length)) );

	return (r >= 0 || (r < 0 && errno == EAGAIN))  ? 0 : r;
}

static int service_writer_pipe(struct logger* server,struct writer* wr, struct epoll_event* event)
{
	struct logger_entry* entry;
	int r = 0;

	if (event->events & EPOLLIN) {
		r = read (wr->pipe_fd[0], wr->buffer + wr->readed, LOG_BUF_SIZE - wr->readed);

		if (r == -1 && errno == EAGAIN)
			return 0;
		else if ((r == 0 || r== -1) && event->events & EPOLLHUP)
			return EINVAL;
		else if (r == 0)
			return -EBADF;

		wr->readed += r;

		entry = (struct logger_entry*)wr->buffer;
		while ((wr->readed >= sizeof(entry->len)) && (entry->len <= wr->readed)) {
			buffer_append(entry, server->buffers[entry->buf_id]);
			wr->readed -= entry->len;
			server->should_timeout |= (1<<entry->buf_id);
			memmove(wr->buffer, wr->buffer + entry->len, LOG_BUF_SIZE - entry->len);
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
	struct reader*		reader = NULL;

	for (i = 0; i < LOG_ID_MAX; i++) {
		buffers[i]->elapsed += time_elapsed;
		if (buffers[i]->buffered_len > server->max_buffered_bytes ||
			buffers[i]->elapsed > server->max_buffered_time) {
			LIST_FOREACH(server->readers[i], reader) {
				r = service_reader(server, reader);
				if (r || reader->dumpcount) {
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

static struct logger* logger_create(char* path, struct log_config *conf)
{
	struct logger* l = calloc(1, sizeof(struct logger));
	int i = 0;
	int j;
	int size = 0;
	char conf_line [32];
	const char *conf_value;

	if (!l)
		goto err1;

	l->epollfd = epoll_create1(0);
	if (l->epollfd == -1)
		goto err1;

	strncpy(l->control.path, path, sizeof(l->control.path));

	if ((l->control.fd = listen_fd_create(l->control.path)) < 0)
		goto err1;

	l->control._entity.type = ENTITY_CONTROL;
	l->control.event.data.ptr = &l->control;
	l->control.event.events = EPOLLIN;

	if (!(l->default_format = log_format_new()))
		goto err2;
	log_add_filter_string(l->default_format, "*:d");

	if (!(l->buffers = calloc(LOG_ID_MAX, sizeof(struct log_buffer*))))
		goto err2;

	if (!(l->readers = calloc(LOG_ID_MAX, sizeof(struct reader*))))
		goto err3;

	add_fd_loop(l->epollfd, l->control.fd, &l->control.event);

	for (i = 0; i < LOG_ID_MAX; i++) {
		snprintf(conf_line, 32, "%s_size", log_name_by_id(i));
		conf_value = log_config_get (conf, conf_line);
		if (!conf_value)
			goto err4;

		size = atoi (conf_value);

		conf_value = log_config_get(conf, log_name_by_id(i));
		if (!conf_value)
			goto err4;

		if (!(l->buffers[i] = buffer_create(i, size, conf_value)))
			goto err4;
	}

	return l;

err4:
	for (j = 0; j < i; j++)
		buffer_free(&l->buffers[j]);
err3:
	free(l->buffers);
err2:
	close(l->control.fd);
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

	for (j = 0; j < LOG_ID_MAX; j++)
		buffer_free(&l->buffers[j]);

	free(l->buffers);
	close(l->control.fd);
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
	case ENTITY_CONTROL:
	{
		int sock_pipe = accept4(((struct control*)event->data.ptr)->fd, NULL, NULL, SOCK_NONBLOCK);
		if (sock_pipe >= 0) {
			struct writer* writer = writer_create(sock_pipe);

			if (writer)
			{
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

	for (i = 0; i < LOG_ID_MAX; i++)
	{
		int diff = server->max_buffered_time - server->buffers[i]->elapsed;
		if (diff > 0 && (diff < timeout || timeout == -1))
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

	for (i = 0; i < LOG_ID_MAX; i++)
		add_fd_loop (server->epollfd, server->buffers[i]->listen_fd, &server->buffers[i]->event);

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
	const char * tmp;
	char tmp2 [64];
	int i;
	char path_buffer[1024];
	path_buffer[0] = 0;
	if (*server)
		logger_free(*server);

	log_config_read (&conf);
	tmp = log_config_get(&conf, "pipe_control_socket");

	if (!tmp)
		sprintf(path_buffer, LOG_CONTROL_SOCKET);
	else
		sprintf(path_buffer, "%s", tmp);

	*server = logger_create(path_buffer, &conf);
	if (!(*server))
		return EINVAL;
	i = 0;

	while (1) {
		sprintf(tmp2, "dlog_logger_conf_%d", i);
		tmp = log_config_get(&conf, tmp2);
		if (!tmp) break;
		parse_command_line (tmp, *server, -1);
		++i;
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
