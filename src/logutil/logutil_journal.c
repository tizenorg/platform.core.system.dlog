/*
 * Copyright (c) 2005-2008, The Android Open Source Project
 * Copyright (c) 2009-2015, Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "dlog.h"

#include <syslog.h>
#include <systemd/sd-journal.h>

int main (int argc, char **argv)
{
	int r;
	sd_journal *j;

	static const char pri_table[DLOG_PRIO_MAX] = {
		[DLOG_VERBOSE] = 'V',
		[LOG_DEBUG] = 'D',
		[LOG_INFO] = 'I',
		[LOG_WARNING] = 'W',
		[LOG_ERR] = 'E',
		[LOG_CRIT] = 'F',
	};

	r = sd_journal_open(&j, SD_JOURNAL_LOCAL_ONLY);
	if (r < 0) {
		fprintf(stderr, "Failed to open journal: %s\n", strerror(-r));

		return 1;
	}

	fprintf(stderr, "read\n");
	SD_JOURNAL_FOREACH(j) {
		const char *priority, *log_tag, *tid,  *message;
		size_t l;

		r = sd_journal_get_data(j, "PRIORITY", (const void **)&priority, &l);
		if (r < 0)
			continue;

		r = sd_journal_get_data(j, "LOG_TAG", (const void **)&log_tag, &l);
		if (r < 0)
			continue;

		r = sd_journal_get_data(j, "TID", (const void **)&tid, &l);
		if (r < 0)
			continue;

		r = sd_journal_get_data(j, "MESSAGE", (const void **)&message, &l);
		if (r < 0)
			continue;

		fprintf(stdout, "%c/%s(%5d): %s\n", pri_table[atoi(priority+9)], log_tag+8, atoi(tid+4), message+8);
	}

	fprintf(stderr, "wait\n");
	for (;;) {
		const char *log_tag, *priority, *tid, *message;
		size_t l;

		if (sd_journal_seek_tail(j) < 0) {
			fprintf(stderr, "Couldn't find journal");
		} else if (sd_journal_previous(j) > 0) {
			r = sd_journal_get_data(j, "PRIORITY", (const void **)&priority, &l);
			if (r < 0)
				continue;

			r = sd_journal_get_data(j, "LOG_TAG", (const void **)&log_tag, &l);
			if (r < 0)
				continue;

			r = sd_journal_get_data(j, "TID", (const void **)&tid, &l);
			if (r < 0)
				continue;

			r = sd_journal_get_data(j, "MESSAGE", (const void **)&message, &l);
			if (r < 0)
				continue;

			fprintf(stdout, "%c/%s(%5d): %s", pri_table[atoi(priority+9)], log_tag+8, atoi(tid+4), message+8);
			fprintf(stdout, "\n");

			r = sd_journal_wait(j, (uint64_t) -1);
			if (r < 0) {
				fprintf(stderr, "Couldn't wait for journal event");
				break;
			}
		}
	}

	sd_journal_close(j);

	return 0;
}
