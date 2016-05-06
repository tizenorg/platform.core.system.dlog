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

#ifndef _QUEUED_ENTRY_H
#define _QUEUED_ENTRY_H

#include <logcommon.h>

#include <stdint.h>

#if DLOG_BACKEND_KMSG

struct logger_entry {
	char *buf;
	size_t buf_size;
	uint64_t ts_usec;
	char *msg_begin;
	char *msg_end;
	char *pid_begin;
	char *tid_begin;
	char *comm_begin;
	char *pri_begin;
	char *tag_begin;
};

struct queued_entry_t {
	struct logger_entry entry;
	struct queued_entry_t* next;
};

struct queued_entry_t* new_queued_entry(uint32_t log_read_size_max);
void free_queued_entry(struct queued_entry_t* entry);
void free_queued_entry_list(struct queued_entry_t *queue);
int read_queued_entry_from_dev(int fd, struct queued_entry_t *entry, uint32_t log_read_size_max);
int cmp(struct queued_entry_t* a, struct queued_entry_t* b);

#else

#include <dlog_ioctl.h>

struct logger_entry {
    uint16_t    len;    /* length of the payload */
    uint16_t    buf_id; /* ID of the log buffer */
    int32_t     pid;    /* generating process's pid */
    int32_t     tid;    /* generating process's tid */
    int32_t     sec;    /* seconds since Epoch */
    int32_t     nsec;   /* nanoseconds */
    char        msg[];  /* the entry's payload */
};

struct queued_entry_t {
	union {
		unsigned char buf[LOGGER_ENTRY_MAX_LEN + 1] __attribute__((aligned(4)));
		struct logger_entry entry __attribute__((aligned(4)));
	};
	struct queued_entry_t* next;
};

struct queued_entry_t* new_queued_entry(uint32_t for_compatibility);
void free_queued_entry(struct queued_entry_t* entry);
void free_queued_entry_list(struct queued_entry_t *queue);
int read_queued_entry_from_dev(int fd, struct queued_entry_t *entry, uint32_t for_compatibility);
int cmp(struct queued_entry_t* a, struct queued_entry_t* b);

#endif

#define RQER_SUCCESS   0
#define RQER_EINTR     1
#define RQER_EAGAIN    2
#define RQER_PARSE     3
#define RQER_EPIPE     4
//TODO: consider enum

struct queued_entry_t* pop_queued_entry(struct queued_entry_t** queue);
void enqueue(struct queued_entry_t** queue, struct queued_entry_t* entry);

#endif /* _QUEUED_ENTRY_H */
