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
#include <log_file.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

void open_logfile(struct log_file *file)
{
	file->fd = open(file->path,
			O_WRONLY | O_APPEND | O_CREAT,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

	if (file->fd < 0) {
		_E("could not open log file %s (%d)",
		   file->path, errno);
		exit(EXIT_FAILURE);
	}
}

void rotate_logs(struct log_file *file)
{
	int i, ret;
	char path0[PATH_MAX];
	char path1[PATH_MAX];

	close(file->fd);
	for (i = file->max_rotated ; i > 0 ; i--) {
		snprintf(path1, PATH_MAX, "%s.%d", file->path, i);
		if (i - 1 == 0)
			snprintf(path0, PATH_MAX, "%s",  file->path);
		else
			snprintf(path0, PATH_MAX, "%s.%d", file->path, i - 1);
		ret = rename(path0, path1);
		if (ret < 0 && errno != ENOENT)
			_E("while rotating log file: %s", file->path);
	}
	/* open log file again */
	open_logfile(file);
	file->size = 0;
}
