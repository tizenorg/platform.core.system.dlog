/*
 * DLOG
 * Copyright (c) 2005-2008, The Android Open Source Project
 * Copyright (c) 2012-2015 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <logcommon.h>
#include <queued_entry.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#if DLOG_BACKEND_KMSG

struct queued_entry_t* new_queued_entry(uint32_t log_read_size_max)
{
	struct queued_entry_t* entry = (struct queued_entry_t *)malloc(sizeof(struct queued_entry_t));
	if (entry == NULL) {
		_E("Can't malloc queued_entry_t\n");
		exit(EXIT_FAILURE);
	}
	memset(entry, 0, sizeof(struct queued_entry_t));
	entry->entry.buf = (char*)malloc(log_read_size_max+1);
	if (entry->entry.buf == NULL) {
		_E("Can't malloc queued_entry_t entry buf \n");
		exit(EXIT_FAILURE);
	}
	return entry;
}

void free_queued_entry(struct queued_entry_t *entry)
{
	if (!entry)
		return;
	if (entry->entry.buf)
		free(entry->entry.buf);
	free(entry);
}

void free_queued_entry_list(struct queued_entry_t *queue)
{
	while (queue->next) {
		struct queued_entry_t *entry = queue;
		queue = queue->next;
		free_queued_entry(entry);
	}
	free_queued_entry(queue);
}

static char* parse_dict_val(char *buf, char **val)
{
	char *cptr;

	*val = strchr(buf, '=');
	if (*val == NULL)
		return NULL;
	(*val)++;
	cptr = strchr(*val, '\n');
	if (cptr) {
		*cptr = '\0';
		cptr++;
	}
	return cptr;
}

static int parse_entry_raw(struct logger_entry *entry)
{
	char *cptr;
	char *ts_begin;

	cptr = strchr(entry->buf, ',');
	if (cptr == NULL)
		return -1;

	ts_begin = strchr(cptr+1, ',');
	if (ts_begin == NULL)
		return -1;
	ts_begin++;
	cptr = strchr(ts_begin, ',');
	if (cptr == NULL)
		return -1;
	*cptr = '\0';

	errno = 0;
	entry->ts_usec = strtoull(ts_begin, NULL, 10);
	if (errno)
		return -1;

	entry->tag_begin = strchr(cptr+1, ';');
	if (entry->tag_begin == NULL)
		return -1;
	entry->tag_begin++;

	entry->pri_begin = strchr(entry->tag_begin, ';');
	if (entry->pri_begin == NULL)
		return -1;
	*entry->pri_begin = '\0';
	entry->pri_begin++;

	entry->msg_begin = strchr(entry->pri_begin, ';');
	if (entry->msg_begin == NULL)
		return -1;
	*entry->msg_begin = '\0';
	entry->msg_begin++;

	entry->msg_end = strchr(entry->msg_begin, '\n');
	if (entry->msg_end == NULL)
		return -1;
	*entry->msg_end = '\0';

	cptr = parse_dict_val(entry->msg_end+1, &entry->pid_begin);
	if (cptr == NULL)
		return -1;

	cptr = parse_dict_val(cptr, &entry->tid_begin);
	if (cptr == NULL)
		return -1;

	cptr = parse_dict_val(cptr, &entry->comm_begin);
	if (cptr == NULL)
		return -1;
	return 0;
}

int read_queued_entry_from_dev(int fd, struct queued_entry_t *entry,
			       uint32_t log_read_size_max) {
	int ret = read(fd, entry->entry.buf, log_read_size_max);

	if (ret < 0) {
		int err = errno;
		free_queued_entry(entry);
		switch (err) {
		case EINTR:
			return RQER_EINTR;
		case EAGAIN:
			return RQER_EAGAIN;
		case EPIPE:
			return RQER_EPIPE;
		default:
			_E("read: %d", errno);
			exit(EXIT_FAILURE);

		}
	} else if (!ret) {
		_E("read: Unexpected EOF!\n");
		exit(EXIT_FAILURE);
	}

	entry->entry.buf_size = ret;
	entry->entry.buf[entry->entry.buf_size] = '\0';

	if (0 != parse_entry_raw(&entry->entry)) {
		_D("parsing log failed\n");
		free_queued_entry(entry);
		return RQER_PARSE;
	}

	return RQER_SUCCESS;
}

int cmp(struct queued_entry_t* a, struct queued_entry_t* b)
{
	if (a->entry.ts_usec < b->entry.ts_usec)
		return -1;
	else if (a->entry.ts_usec > b->entry.ts_usec)
		return 1;
	return 0;
}

#else

struct queued_entry_t* new_queued_entry(uint32_t for_compatibility)
{
	struct queued_entry_t* entry = (struct queued_entry_t *)malloc(sizeof(struct queued_entry_t));
	if (entry == NULL) {
		_E("Can't malloc queued_entry_t\n");
		exit(EXIT_FAILURE);
	}
	entry->next = NULL;
	return entry;
}

void free_queued_entry(struct queued_entry_t *entry)
{
	if (entry)
		free(entry);
}

void free_queued_entry_list(struct queued_entry_t *queue)
{
	while (queue->next) {
		struct queued_entry_t *entry = queue;
		queue = queue->next;
		free_queued_entry(entry);
	}
	free_queued_entry(queue);
}

int read_queued_entry_from_dev(int fd, struct queued_entry_t *entry, uint32_t for_compatibility)
{
	int ret = read(fd, entry->buf, LOGGER_ENTRY_MAX_LEN);
	if (ret < 0) {
		int err = errno;
		free_queued_entry(entry);
		switch (err) {
		case EINTR:
			return RQER_EINTR;
		case EAGAIN:
			return RQER_EAGAIN;
		default:
			_E("read: error is occurred (%d)", errno);
			exit(EXIT_FAILURE);
		}
	} else if (!ret) {
		_E("read: Unexpected EOF!\n");
		exit(EXIT_FAILURE);
	} else if (entry->entry.len != ret - sizeof(struct logger_entry)) {
		_E("read: unexpected length. Expected %d, got %d\n",
		   entry->entry.len, ret - (int)sizeof(struct logger_entry));
		exit(EXIT_FAILURE);
	}
	entry->entry.msg[entry->entry.len] = '\0';
	return 0;
}

int cmp(struct queued_entry_t* a, struct queued_entry_t* b)
{
	int n = a->entry.sec - b->entry.sec;
	if (n != 0) {
		return n;
	}
	return a->entry.nsec - b->entry.nsec;
}

#endif

struct queued_entry_t* pop_queued_entry(struct queued_entry_t** queue)
{
	struct queued_entry_t *entry = *queue;
	*queue = (*queue)->next;
	return entry;
}

void enqueue(struct queued_entry_t** queue, struct queued_entry_t* entry)
{
	if (*queue == NULL)
		*queue = entry;
	else {
		struct queued_entry_t** e = queue;

		while (*e && cmp(entry, *e) >= 0)
			e = &((*e)->next);

		entry->next = *e;
		*e = entry;
	}
}

