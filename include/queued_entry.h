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

struct logger_entry {
    uint16_t    len;    /* length of the payload */
    uint16_t    __pad;  /* no matter what, we get 2 bytes of padding */
    int32_t     pid;    /* generating process's pid */
    int32_t     tid;    /* generating process's tid */
    int32_t     sec;    /* seconds since Epoch */
    int32_t     nsec;   /* nanoseconds */
    char        msg[]; /* the entry's payload */
};

struct queued_entry_t {
	union {
		unsigned char buf[LOGGER_ENTRY_MAX_LEN + 1] __attribute__((aligned(4)));
		struct logger_entry entry __attribute__((aligned(4)));
	};
	struct queued_entry_t* next;
};

struct queued_entry_t* new_queued_entry();
void free_queued_entry(struct queued_entry_t* entry);
void free_queued_entry_list(struct queued_entry_t *queue);
int read_queued_entry_from_dev(int fd, struct queued_entry_t *entry);
struct queued_entry_t* pop_queued_entry(struct queued_entry_t** queue);
int cmp(struct queued_entry_t* a, struct queued_entry_t* b);
void enqueue(struct queued_entry_t** queue, struct queued_entry_t* entry);

#endif /* _QUEUED_ENTRY_H */
