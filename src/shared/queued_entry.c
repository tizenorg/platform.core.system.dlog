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

struct queued_entry_t* new_queued_entry()
{
	struct queued_entry_t* entry = (struct queued_entry_t *)malloc(sizeof( struct queued_entry_t));
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

int read_queued_entry_from_dev(int fd, struct queued_entry_t *entry) {
	int ret = read(fd, entry->buf, LOGGER_ENTRY_MAX_LEN);
	if (ret < 0) {
		int err = errno;
		free_queued_entry(entry);
		if (err == EINTR || err == EAGAIN)
			return err;
		_E("read: %s", strerror(errno));
		exit(EXIT_FAILURE);
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

struct queued_entry_t* pop_queued_entry(struct queued_entry_t** queue)
{
	struct queued_entry_t *entry = *queue;
	*queue = (*queue)->next;
	return entry;
}

int cmp(struct queued_entry_t* a, struct queued_entry_t* b)
{
	int n = a->entry.sec - b->entry.sec;
	if (n != 0) {
		return n;
	}
	return a->entry.nsec - b->entry.nsec;
}

void enqueue(struct queued_entry_t** queue, struct queued_entry_t* entry)
{
	if (*queue == NULL) {
		*queue = entry;
	} else {
		struct queued_entry_t** e = queue;
		while (*e && cmp(entry, *e) >= 0 ) {
			e = &((*e)->next);
		}
		entry->next = *e;
		*e = entry;
	}
}
