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

#if DLOG_BACKEND_JOURNAL

#include <syslog.h>
#include <systemd/sd-journal.h>

#elif (DLOG_BACKEND_KMSG) || (DLOG_BACKEND_LOGGER)

#include <logcommon.h>
#include <queued_entry.h>
#include <log_file.h>
#include <logprint.h>
#include <dlog_ioctl.h>

#define DEFAULT_LOG_ROTATE_SIZE_KBYTES 16
#define DEFAULT_MAX_ROTATED_LOGS 4
#define MAX_QUEUED 4096

static bool g_nonblock = false;
static int g_tail_lines = 0;

static struct log_file g_log_file = {
	NULL,
	-1,
	0,
	DEFAULT_LOG_ROTATE_SIZE_KBYTES,
	DEFAULT_MAX_ROTATED_LOGS,
	NULL,
};

static int g_dev_count = 0;

struct log_device_t {
	char* device;
	int fd;
	bool printed;
	uint32_t log_read_size_max;
	struct queued_entry_t* queue;
	struct log_device_t* next;
};

static char g_devs[LOG_ID_MAX][PATH_MAX];

static void processBuffer(struct log_device_t* dev, struct logger_entry *buf)
{
	int bytes_written = 0;
	int err;
	log_entry entry;
	char mgs_buf[1024];

	err = log_process_log_buffer(buf, &entry);

	if (err < 0) {
		goto error;
	}

	if (log_should_print_line(g_log_file.format, entry.tag, entry.priority)) {
		if (false && g_dev_count > 1) {
			mgs_buf[0] = dev->device[0];
			mgs_buf[1] = ' ';
			bytes_written = write(g_log_file.fd, mgs_buf, 2);
			if (bytes_written < 0) {
				_E("output error\n");
				exit(-1);
			}
		}

		bytes_written = log_print_log_line(g_log_file.format, g_log_file.fd, &entry);

		if (bytes_written < 0) {
			_E("output error\n");
			exit(-1);
		}
	}

	g_log_file.size += bytes_written;

	if (g_log_file.rotate_size_kbytes > 0 && (g_log_file.size / 1024) >= g_log_file.rotate_size_kbytes) {
		if (g_nonblock) {
			exit(0);
		} else if (g_log_file.path) {
			rotate_logs(&g_log_file);
		}
	}

error:
	return;
}

static void chooseFirst(struct log_device_t* dev, struct log_device_t** firstdev)
{
	for (*firstdev = NULL; dev != NULL; dev = dev->next) {
		if (dev->queue != NULL && (*firstdev == NULL ||
				   	cmp(dev->queue, (*firstdev)->queue) < 0)) {
			*firstdev = dev;
		}
	}
}

static void maybePrintStart(struct log_device_t* dev) {
	if (!dev->printed) {
		dev->printed = true;
		if (g_dev_count > 1 ) {
			char buf[1024];
			snprintf(buf, sizeof(buf), "--------- beginning of %s\n", dev->device);
			if (write(g_log_file.fd, buf, strlen(buf)) < 0) {
				_E("output error\n");
				exit(-1);
			}
		}
	}
}

static void skipNextEntry(struct log_device_t* dev) {
	maybePrintStart(dev);
	struct queued_entry_t* entry = pop_queued_entry(&dev->queue);
	free_queued_entry(entry);
}

static void printNextEntry(struct log_device_t* dev)
{
	maybePrintStart(dev);
	processBuffer(dev, &dev->queue->entry);
	skipNextEntry(dev);
}


static void read_log_lines(struct log_device_t* devices)
{
	struct log_device_t* dev;
	int max = 0;
	int queued_lines = 0;
	bool sleep = false; // for exit immediately when log buffer is empty and g_nonblock value is true.

	int result;
	fd_set readset;

	for (dev=devices; dev; dev = dev->next) {
		if (dev->fd > max) {
			max = dev->fd;
		}
	}

	while (1) {
		do {
			struct timeval timeout = { 0, 5000 /* 5ms */ }; // If we oversleep it's ok, i.e. ignore EINTR.
			FD_ZERO(&readset);
			for (dev=devices; dev; dev = dev->next) {
				FD_SET(dev->fd, &readset);
			}
			result = select(max + 1, &readset, NULL, NULL, sleep ? NULL : &timeout);
		} while (result == -1 && errno == EINTR);

		if (result >= 0) {
			for (dev=devices; dev; dev = dev->next) {
				if (FD_ISSET(dev->fd, &readset)) {
					int ret;
					struct queued_entry_t *entry =
						new_queued_entry(dev->log_read_size_max);
					/* NOTE: driver guarantees we read exactly one full entry */
					ret = read_queued_entry_from_dev(dev->fd, entry,
									 dev->log_read_size_max);
					if (ret == RQER_EINTR)
						goto next;
					else if (ret == RQER_EAGAIN)
						break;
					else if(ret == RQER_PARSE) {
						printf("Wrong formatted message is written.\n");
						continue;
					}
					/* EPIPE is not an error: it signals the cyclic buffer having
					 * made a full turn and overwritten previous data */
					else if(ret == RQER_EPIPE)
						continue;

					enqueue(&dev->queue, entry);
					++queued_lines;
					if (g_nonblock && MAX_QUEUED < queued_lines) {
						while (true) {
							chooseFirst(devices, &dev);
							if (dev == NULL)
								break;
							printNextEntry(dev);
							--queued_lines;
						}
						break;
					}
				}
			}

			if (result == 0) {
				/* we did our short timeout trick and there's nothing new
				 print everything we have and wait for more data */
				sleep = true;
				while (true) {
					chooseFirst(devices, &dev);
					if (dev == NULL) {
						break;
					}
					if (g_tail_lines == 0 || queued_lines <= g_tail_lines) {
						printNextEntry(dev);
					} else {
						skipNextEntry(dev);
					}
					--queued_lines;
				}

				/* the caller requested to just dump the log and exit */
				if (g_nonblock) {
					exit(0);
				}
			} else {
				/* print all that aren't the last in their list */
				sleep = false;
				while (g_tail_lines == 0 || queued_lines > g_tail_lines) {
					chooseFirst(devices, &dev);
					if (dev == NULL || dev->queue->next == NULL) {
						break;
					}
					if (g_tail_lines == 0) {
						printNextEntry(dev);
					} else {
						skipNextEntry(dev);
					}
					--queued_lines;
				}
			}
		}
next:
		;
	}
}

static void setup_output()
{

	if (g_log_file.path == NULL) {
		g_log_file.fd = STDOUT_FILENO;

	} else {
		struct stat statbuf;

		open_logfile(&g_log_file);
		if (fstat(g_log_file.fd, &statbuf) == -1)
			g_log_file.size = 0;
		else
			g_log_file.size = statbuf.st_size;
	}
}

static int set_log_format(const char * formatString)
{
	static log_print_format format;

	format = log_format_from_string(formatString);

	if (format == FORMAT_OFF) {
		/* FORMAT_OFF means invalid string */
		return -1;
	}

	log_set_print_format(g_log_file.format, format);

	return 0;
}

static void show_help(const char *cmd)
{
	fprintf(stderr,"Usage: %s [options] [filterspecs]\n", cmd);

	fprintf(stderr, "options include:\n"
			"  -s              Set default filter to silent.\n"
			"                  Like specifying filterspec '*:s'\n"
			"  -f <filename>   Log to file. Default to stdout\n"
			"  -r [<kbytes>]   Rotate log every kbytes. (16 if unspecified). Requires -f\n"
			"  -n <count>      Sets max number of rotated logs to <count>, default 4\n"
			"  -v <format>     Sets the log print format, where <format> is one of:\n\n"
			"                  brief(by default) process tag thread raw time threadtime long\n\n"
			"  -c              clear (flush) the entire log and exit, conflicts with '-g'\n"
			"  -d              dump the log and then exit (don't block)\n"
			"  -t <count>      print only the most recent <count> lines (implies -d)\n"
			"  -g              get the size of the log's ring buffer and exit, conflicts with '-c'\n"
			"  -b <buffer>     request alternate ring buffer\n"
			"                  ('main' (default), 'radio', 'system')");


	fprintf(stderr,"\nfilterspecs are a series of \n"
			"  <tag>[:priority]\n\n"
			"where <tag> is a log component tag (or * for all) and priority is:\n"
			"  V    Verbose\n"
			"  D    Debug\n"
			"  I    Info\n"
			"  W    Warn\n"
			"  E    Error\n"
			"  F    Fatal\n"
			"  S    Silent (supress all output)\n"
			"\n'*' means '*:D' and <tag> by itself means <tag>:V\n"
			"If no filterspec is found, filter defaults to '*:I'\n\n");
}


/*
 * free one log_device_t and it doesn't take care of chain so it
 * may break the chain list
 */
static void log_devices_free(struct log_device_t *dev)
{
	if (!dev)
		return;

	if (dev->device)
		free(dev->device);

	if (dev->queue)
		free_queued_entry_list(dev->queue);

	free(dev);
	dev = NULL;
}


/*
 * free all the nodes after the "dev" and includes itself
 */
static void log_devices_chain_free(struct log_device_t *dev)
{
	if (!dev)
		return;

	while (dev->next) {
		struct log_device_t *tmp = dev->next;
		dev->next = tmp->next;
		log_devices_free(tmp);
	}

	log_devices_free(dev);
	dev = NULL;
}


/*
 * create a new log_device_t instance but don't care about
 * the device node accessable or not
 */
static struct log_device_t *log_devices_new(const char *path)
{
	struct log_device_t *new;

	if (!path || strlen(path) <= 0)
		return NULL;

	new = malloc(sizeof(*new));
	if (!new) {
		_E("out of memory\n");
		return NULL;
	}

	new->device = strdup(path);
	new->fd = -1;
	new->printed = false;
	new->queue = NULL;
	new->next = NULL;

	return new;
}


/*
 * add a new device to the tail of chain
 */
static int log_devices_add_to_tail(struct log_device_t *devices, struct log_device_t *new)
{
	struct log_device_t *tail = devices;

	if (!devices || !new)
		return -1;

	while (tail->next)
		tail = tail->next;

	tail->next = new;
	g_dev_count++;

	return 0;
}

#endif

#if DLOG_BACKEND_JOURNAL

int main_journal(int argc, char **argv)
{
	int r;
	sd_journal *j;

	static const char pri_table[DLOG_PRIO_MAX] = {
		[LOG_DEBUG] = 'D',
		[LOG_INFO] = 'I',
		[LOG_WARNING] = 'W',
		[LOG_ERR] = 'E',
		[LOG_CRIT] = 'F',
	};

	r = sd_journal_open(&j, SD_JOURNAL_LOCAL_ONLY);
	if (r < 0) {
		fprintf(stderr, "Failed to open journal (%d)\n", -r);

		return 1;
	}

	fprintf(stderr, "read\n");
	SD_JOURNAL_FOREACH(j) {
		const char *priority, *log_tag, *tid,  *message;
		size_t l;
		int prio_idx;

		r = sd_journal_get_data(j, "PRIORITY", (const void **)&priority, &l);
		if (r < 0)
			continue;

		prio_idx = atoi(priority+9);
		if (prio_idx < LOG_CRIT || prio_idx > LOG_DEBUG) {
			fprintf(stdout, "Wrong priority");
			continue;
		}

		r = sd_journal_get_data(j, "LOG_TAG", (const void **)&log_tag, &l);
		if (r < 0)
			continue;

		r = sd_journal_get_data(j, "TID", (const void **)&tid, &l);
		if (r < 0)
			continue;

		r = sd_journal_get_data(j, "MESSAGE", (const void **)&message, &l);
		if (r < 0)
			continue;

		fprintf(stdout, "%c/%s(%5d): %s\n", pri_table[prio_idx], log_tag+8, atoi(tid+4), message+8);
	}

	fprintf(stderr, "wait\n");
	for (;;) {
		const char *log_tag, *priority, *tid, *message;
		size_t l;
		int prio_idx;

		if (sd_journal_seek_tail(j) < 0) {
			fprintf(stderr, "Couldn't find journal");
		} else if (sd_journal_previous(j) > 0) {
			r = sd_journal_get_data(j, "PRIORITY", (const void **)&priority, &l);
			if (r < 0)
				continue;

			prio_idx = atoi(priority+9);
			if (prio_idx < LOG_CRIT || prio_idx > LOG_DEBUG) {
				fprintf(stdout, "Wrong priority");
				continue;
			}

			r = sd_journal_get_data(j, "LOG_TAG", (const void **)&log_tag, &l);
			if (r < 0)
				continue;

			r = sd_journal_get_data(j, "TID", (const void **)&tid, &l);
			if (r < 0)
				continue;

			r = sd_journal_get_data(j, "MESSAGE", (const void **)&message, &l);
			if (r < 0)
				continue;

			fprintf(stdout, "%c/%s(%5d): %s", pri_table[prio_idx], log_tag+8, atoi(tid+4), message+8);
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

#elif (DLOG_BACKEND_KMSG) || (DLOG_BACKEND_LOGGER)

int main_others(int argc, char **argv)
{
	int err;
	int has_set_log_format = 0;
	int is_clear_log = 0;
	int needs_file_path = 0;
	int getLogSize = 0;
	int mode = O_RDONLY;
	int accessmode = R_OK;
	int i;
	struct log_device_t* devices = NULL;
	struct log_device_t* dev;
	log_id_t id;

	g_log_file.format = (log_format *)log_format_new();

	if (argc == 2 && 0 == strcmp(argv[1], "--test")) {
		logprint_run_tests();
		exit(0);
	}

	if (argc == 2 && 0 == strcmp(argv[1], "--help")) {
		show_help(argv[0]);
		exit(0);
	}

	if (0 != get_log_dev_names(g_devs)) {
		_E("Unable to read initial configuration\n");
		exit(-1);
	}

	for (;;) {
		int ret;

		ret = getopt(argc, argv, "cdt:gsf:r:n:v:b:D");

		if (ret < 0)
			break;

		switch (ret) {
		case 's':
			/* default to all silent */
			log_add_filter_rule(g_log_file.format, "*:s");
			break;

		case 'c':
			is_clear_log = 1;
			mode = O_WRONLY;
			break;

		case 'd':
			g_nonblock = true;
			break;

		case 't':
			g_nonblock = true;
			g_tail_lines = atoi(optarg);
			break;


		case 'g':
			getLogSize = 1;
			break;

		case 'b': {
					  id = log_id_by_name(optarg);
					  if (id < 0) {
						  _E("Unknown log buffer %s\n", optarg);
						  exit(-1);
					  }

					  dev = log_devices_new(g_devs[id]);
					  if (dev == NULL) {
						  _E("Can't add log device: %s\n", g_devs[id]);
						  exit(-1);
					  }
					  if (devices) {
						  if (log_devices_add_to_tail(devices, dev)) {
							  _E("Open log device %s failed\n", g_devs[id]);
							  exit(-1);
						  }
					  } else {
						  devices = dev;
						  g_dev_count = 1;
					  }
				  }
				  break;

		case 'f':
				  /* redirect output to a file */
				  g_log_file.path = optarg;

				  break;

		case 'r':
				  if (!isdigit(optarg[0])) {
					  _E("Invalid parameter to -r\n");
					  show_help(argv[0]);
					  exit(-1);
				  }
				  g_log_file.rotate_size_kbytes = atoi(optarg);
				  needs_file_path = 1;
				  break;

		case 'n':
				  if (!isdigit(optarg[0])) {
					  _E("Invalid parameter to -r\n");
					  show_help(argv[0]);
					  exit(-1);
				  }

				  g_log_file.max_rotated = atoi(optarg);
				  break;

		case 'v':
				  err = set_log_format (optarg);
				  if (err < 0) {
					  _E("Invalid parameter to -v\n");
					  show_help(argv[0]);
					  exit(-1);
				  }

				  has_set_log_format = 1;
				  break;

		default:
				  _E("Unrecognized Option\n");
				  show_help(argv[0]);
				  exit(-1);
				  break;
		}
	}

	/* get log size conflicts with write mode */
	if (getLogSize && mode == O_WRONLY) {
		show_help(argv[0]);
		exit(-1);
	}

	if (!devices) {
		devices = log_devices_new(g_devs[LOG_ID_MAIN]);
		if (devices == NULL) {
			_E("Can't add log device: %s\n", g_devs[LOG_ID_MAIN]);
			exit(-1);
		}
		g_dev_count = 1;

		if (mode == O_WRONLY)
			accessmode = W_OK;

		/* only add this if it's available */
		if (0 == access(g_devs[LOG_ID_SYSTEM], accessmode)) {
			if (log_devices_add_to_tail(devices, log_devices_new(g_devs[LOG_ID_SYSTEM]))) {
				_E("Can't add log device: %s\n", g_devs[LOG_ID_SYSTEM]);
				exit(-1);
			}
		}
		if (0 == access(g_devs[LOG_ID_APPS], accessmode)) {
			if (log_devices_add_to_tail(devices, log_devices_new(g_devs[LOG_ID_APPS]))) {
				_E("Can't add log device: %s\n", g_devs[LOG_ID_APPS]);
				exit(-1);
			}
		}

	}

	if (needs_file_path != 0 && g_log_file.path == NULL) {
		_E("-r requires -f as well\n");
		show_help(argv[0]);
		exit(-1);
	}

	setup_output();


	if (has_set_log_format == 0)
		err = set_log_format("brief");

	_E("arc = %d, optind = %d ,Kb %d, rotate %d\n", argc, optind, g_log_file.rotate_size_kbytes, g_log_file.max_rotated);

	if (argc == optind) {
		/* Add from environment variable
		char *env_tags_orig = getenv("DLOG_TAGS");*/
		log_add_filter_string(g_log_file.format, "*:d");
	} else {

		for (i = optind ; i < argc ; i++) {
			err = log_add_filter_string(g_log_file.format, argv[i]);

			if (err < 0) {
				_E("Invalid filter expression '%s'\n", argv[i]);
				show_help(argv[0]);
				exit(-1);
			}
		}
	}
	dev = devices;
	while (dev) {
		dev->fd = open(dev->device, mode);
		if (dev->fd < 0) {
			_E("Unable to open log device %s (%d)",
					dev->device, errno);
			exit(EXIT_FAILURE);
		}

		if (is_clear_log)
			clear_log(dev->fd);

		get_log_read_size_max(dev->fd, &dev->log_read_size_max);

#if DLOG_BACKEND_KMSG
		if (mode != O_WRONLY) {
			off_t off;
			off = lseek(dev->fd, 0, SEEK_DATA);
			if (off == -1) {
				_E("Unable to lseek device %s. %s\n",
						dev->device, strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
#endif

		if (getLogSize || g_nonblock) {
			uint32_t size;

			get_log_size(dev->fd, &size);

			g_log_file.rotate_size_kbytes += size / 1024;
			printf("%s: cyclic ring buffer is %uKb. "
			       "Max read entry log payload size is %uKb\n",
			       dev->device, size/1024,
			       dev->log_read_size_max/1024);
		}

		dev = dev->next;
	}

	if (getLogSize)
		return 0;

	if (is_clear_log)
		return 0;

	read_log_lines(devices);

	log_devices_chain_free(devices);

	return 0;
}

#endif

int main(int argc, char **argv)
{
#if DLOG_BACKEND_JOURNAL
	return main_journal(argc, argv);
#elif (DLOG_BACKEND_KMSG) || (DLOG_BACKEND_LOGGER)
	return main_others(argc, argv);
#else
	printf("Backend isn't set at the build time.\n");
	return 1;
#endif
}
