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


#define _GNU_SOURCE
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
#include <linux/limits.h>


#include <logger.h>
#include <logprint.h>

#define DEFAULT_MAX_ROTATED_LOGS 4

struct queued_entry_t {
	union {
		unsigned char buf[LOGGER_ENTRY_MAX_LEN + 1] __attribute__((aligned(4)));
		struct logger_entry entry __attribute__((aligned(4)));
	};
	struct queued_entry_t* next;
};


static int cmp(struct queued_entry_t* a, struct queued_entry_t* b)
{
	int n = a->entry.sec - b->entry.sec;
	if (n != 0)
	{
		return n;
	}
	return a->entry.nsec - b->entry.nsec;
}


struct log_device_t {
	char* device;
	int fd;
	bool printed;
	struct queued_entry_t* queue;
	struct log_device_t* next;
};

struct cntx_str {
    log_format * logformat;
    bool nonblock;
    int tail_lines;
    const char * output_filename;
    int log_rotate_size_kbytes; // 0 means "no log rotation"
    int max_rotated_logs; // 0 means "unbounded"
    int outfd;
    off_t out_byte_count;
    int dev_count;
    char * log_file_dir;
    int is_clear_log;
    int getLogSize;
    int has_set_log_format;
    struct log_device_t * devices;
    struct log_device_t * dev;
    int mode;
};

static void init_cntx(struct cntx_str * cntx)
{
    cntx->logformat = NULL;
    cntx->nonblock = false;
    cntx->tail_lines = 0;
    cntx->output_filename = NULL;
    cntx->log_rotate_size_kbytes = 0;
    cntx->max_rotated_logs = DEFAULT_MAX_ROTATED_LOGS;
    cntx->outfd = -1;
    cntx->out_byte_count = 0;
    cntx->dev_count = 0;
    cntx->log_file_dir = NULL;
    cntx->is_clear_log = 0;
    cntx->getLogSize = 0;
    cntx->has_set_log_format = 0;
	cntx->devices = NULL;
    cntx->mode = O_RDONLY;
}

static void enqueue(struct log_device_t* device, struct queued_entry_t* entry)
{
	if( device->queue == NULL)
	{
		device->queue = entry;
	}
	else
	{
		struct queued_entry_t** e = &device->queue;
		while(*e && cmp(entry, *e) >= 0 )
		{
			e = &((*e)->next);
		}
		entry->next = *e;
		*e = entry;
    }
}

static int open_logfile (const char *pathname)
{
    return TEMP_FAILURE_RETRY( open(pathname, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH));
}

static void rotate_logs(struct cntx_str * cntx)
{
    int err;
	int i;
	char file0[256]={0};
	char file1[256]={0};

    // Can't rotate logs if we're not outputting to a file
    if (cntx->output_filename == NULL) {
        return;
    }

    close(cntx->outfd);

    for (i = cntx->max_rotated_logs ; i > 0 ; i--)
	{
		snprintf(file1, 255, "%s.%d", cntx->output_filename, i);

		if (i - 1 == 0) {
			snprintf(file0, 255, "%s", cntx->output_filename);
		} else {
			snprintf(file0, 255, "%s.%d", cntx->output_filename, i - 1);
		}

		err = rename (file0, file1);

		if (err < 0 && errno != ENOENT) {
			perror("while rotating log files");
		}
    }

    cntx->outfd = open_logfile (cntx->output_filename);

    if (cntx->outfd < 0) {
        perror ("couldn't open output file");
        exit(-1);
    }

    cntx->out_byte_count = 0;

}


static void processBuffer(
        struct cntx_str * cntx,
        struct log_device_t* dev,
        struct logger_entry *buf
        )
{
	int bytes_written = 0;
	int err;
	log_entry entry;
	char mgs_buf[1024];

	err = log_process_log_buffer(buf, &entry);

	if (err < 0) {
		goto error;
	}

	if (log_should_print_line(cntx->logformat, entry.tag, entry.priority)) {
		if (false && cntx->dev_count > 1) {
			// FIXME
			mgs_buf[0] = dev->device[0];
			mgs_buf[1] = ' ';
			bytes_written = write(cntx->outfd, mgs_buf, 2);
			if (bytes_written < 0)
			{
				perror("output error");
				exit(-1);
			}
		}

		bytes_written = log_print_log_line(cntx->logformat, cntx->outfd, &entry);

		if (bytes_written < 0)
		{
			perror("output error");
			exit(-1);
		}
	}

	cntx->out_byte_count += bytes_written;

	if (cntx->log_rotate_size_kbytes > 0 && (cntx->out_byte_count / 1024) >= cntx->log_rotate_size_kbytes) {
		if (cntx->nonblock) {
			exit(0);
		} else {
			rotate_logs(cntx);
		}
	}

error:
	//fprintf (stderr, "Error processing record\n");
	return;
}

static void chooseFirst(struct log_device_t* dev, struct log_device_t** firstdev)
{
	for (*firstdev = NULL; dev != NULL; dev = dev->next) {
		if (dev->queue != NULL && (*firstdev == NULL || cmp(dev->queue, (*firstdev)->queue) < 0))
		{
			*firstdev = dev;
		}
	}
}

static void maybePrintStart(
        struct cntx_str * cntx,
        struct log_device_t* dev) {
	if (!dev->printed) {
		dev->printed = true;
		if (cntx->dev_count > 1 ) {
			char buf[1024];
			snprintf(buf, sizeof(buf), "--------- beginning of %s\n", dev->device);
			if (write(cntx->outfd, buf, strlen(buf)) < 0) {
				perror("output error");
				exit(-1);
			}
		}
	}
}

static void skipNextEntry(
        struct cntx_str * cntx,
        struct log_device_t* dev) {
	maybePrintStart(cntx, dev);
	struct queued_entry_t* entry = dev->queue;
	dev->queue = entry->next;
	free(entry);
}

static void printNextEntry(
        struct cntx_str * cntx,
        struct log_device_t* dev)
{
	maybePrintStart(cntx, dev);
	processBuffer(cntx, dev, &dev->queue->entry);
	skipNextEntry(cntx, dev);
}


static void read_log_lines(
        struct cntx_str * cntx)
{
	struct log_device_t* dev;
	int max = 0;
	int ret;
	int queued_lines = 0;
	bool sleep = false; // for exit immediately when log buffer is empty and nonblock value is true.

	int result;
	fd_set readset;

	for (dev=cntx->devices; dev; dev = dev->next) {
		if (dev->fd > max) {
			max = dev->fd;
		}
	}

	while (1) {
		do {
			struct timeval timeout = { 0, 5000 /* 5ms */ }; // If we oversleep it's ok, i.e. ignore EINTR.
			FD_ZERO(&readset);
			for (dev=cntx->devices; dev; dev = dev->next) {
				FD_SET(dev->fd, &readset);
			}
			result = select(max + 1, &readset, NULL, NULL, sleep ? NULL : &timeout);
		} while (result == -1 && errno == EINTR);

        if (result >= 0) {
            for (dev=cntx->devices; dev; dev = dev->next) {
                if (FD_ISSET(dev->fd, &readset)) {
                    struct queued_entry_t* entry = (struct queued_entry_t *)malloc(sizeof( struct queued_entry_t));
					if (entry == NULL) {
						fprintf(stderr,"Can't malloc queued_entry\n");
						exit(-1);
					}
					entry->next = NULL;

                    /* NOTE: driver guarantees we read exactly one full entry */
                    ret = TEMP_FAILURE_RETRY( read(dev->fd, entry->buf, LOGGER_ENTRY_MAX_LEN));
                    if (ret < 0) {
                        if (errno == EINTR) {
                            free(entry);
                            goto next;
                        }
                        if (errno == EAGAIN) {
                            free(entry);
                            break;
                        }
                        perror("dlogutil read");
                        exit(EXIT_FAILURE);
                    }
                    else if (!ret) {
                        free(entry);
                        fprintf(stderr, "read: Unexpected EOF!\n");
                        exit(EXIT_FAILURE);
					}
					else if (entry->entry.len != ret - sizeof(struct logger_entry)) {
                        free(entry);
						fprintf(stderr, "read: unexpected length. Expected %d, got %d\n",
								entry->entry.len, ret - sizeof(struct logger_entry));
						exit(EXIT_FAILURE);
					}


                    entry->entry.msg[entry->entry.len] = '\0';

                    enqueue(dev, entry);
                    ++queued_lines;
                }
            }

            if (result == 0) {
                // we did our short timeout trick and there's nothing new
                // print everything we have and wait for more data
                sleep = true;
                while (true) {
                    chooseFirst(cntx->devices, &dev);
                    if (dev == NULL) {
                        break;
                    }
                    if (cntx->tail_lines == 0 || queued_lines <= cntx->tail_lines) {
                        printNextEntry(cntx, dev);
                    } else {
                        skipNextEntry(cntx, dev);
                    }
                    --queued_lines;
                }

                // the caller requested to just dump the log and exit
                if (cntx->nonblock) {
                    exit(0);
                }
            } else {
                // print all that aren't the last in their list
                sleep = false;
                while (cntx->tail_lines == 0 || queued_lines > cntx->tail_lines) {
                    chooseFirst(cntx->devices, &dev);
                    if (dev == NULL || dev->queue->next == NULL) {
                        break;
                    }
                    if (cntx->tail_lines == 0) {
                        printNextEntry(cntx, dev);
                    } else {
                        skipNextEntry(cntx, dev);
                    }
                    --queued_lines;
                }
            }
        }
next:
        ;
    }
}


static int clear_log(int logfd)
{
    return ioctl(logfd, LOGGER_FLUSH_LOG);
}

/* returns the total size of the log's ring buffer */
static int get_log_size(int logfd)
{
    return ioctl(logfd, LOGGER_GET_LOG_BUF_SIZE);
}

/* returns the readable size of the log's ring buffer (that is, amount of the log consumed) */
static int get_log_readable_size(int logfd)
{
    return ioctl(logfd, LOGGER_GET_LOG_LEN);
}

static void setup_output(struct cntx_str * cntx)
{

	if (cntx->output_filename == NULL) {
		cntx->outfd = STDOUT_FILENO;

	} else {
		struct stat statbuf;

		cntx->outfd = open_logfile (cntx->output_filename);

		if (cntx->outfd < 0) {
			perror ("couldn't open output file");
			exit(-1);
		}
		if (fstat(cntx->outfd, &statbuf) == -1)
			cntx->out_byte_count = 0;
		else
			cntx->out_byte_count = statbuf.st_size;
	}
}

static int set_log_format(
        struct cntx_str * cntx,
        const char * formatString)
{
	static log_print_format format;

	format = log_format_from_string(formatString);

	if (format == FORMAT_OFF) {
		// FORMAT_OFF means invalid string
		return -1;
	}

	log_set_print_format(cntx->logformat, format);

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

	if (dev->queue) {
		while (dev->queue->next) {
			struct queued_entry_t *tmp = dev->queue->next;
			dev->queue->next = tmp->next;
			free(tmp);
		}
		free(dev->queue);
	}

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
		fprintf(stderr, "out of memory\n");
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
static int log_devices_add_to_tail(
        struct cntx_str * cntx,
        struct log_device_t *new)
{
	struct log_device_t *tail = cntx->devices;

	if (!(cntx->devices) || !new)
		return -1;

	while (tail->next)
		tail = tail->next;

	tail->next = new;
	cntx->dev_count++;

	return 0;
}

void parse_opts(
        struct cntx_str * cntx,
        int argc,
        char **argv)
{
    int err;

    if (argc == 2 && 0 == strcmp(argv[1], "--help")) {
        show_help(argv[0]);
        exit(0);
    }

    for (;;) {
        int ret;

        ret = getopt(argc, argv, "cdt:gsf:r:n:v:b:D");

        if (ret < 0) {
            break;
        }

        switch(ret) {
            case 's':
                // default to all silent
                log_add_filter_rule(cntx->logformat, "*:s");
            break;

            case 'c':
                cntx->is_clear_log = 1;
                cntx->mode = O_WRONLY;
            break;

            case 'd':
                cntx->nonblock = true;
            break;

            case 't':
                cntx->nonblock = true;
                cntx->tail_lines = atoi(optarg);
            break;


            case 'g':
                cntx->getLogSize = 1;
            break;

			case 'b': {
						  char buf[PATH_MAX + 1];
                          int ret;

                          if ((!optarg) || (!*optarg) || strchr(optarg, '/')) {
							  fprintf(stderr,"Invalid log device\n");
							  exit(-1);
                          }
						  ret = snprintf(buf, sizeof(buf), "%s%s", cntx->log_file_dir, optarg);
						  if ((ret < 1) || (ret >= sizeof(buf))) {
							  fprintf(stderr,"Invalid log device: %s\n", buf);
							  exit(-1);
						  }
						  cntx->dev = log_devices_new(buf);
						  if ((cntx->dev) == NULL) {
							  fprintf(stderr,"Can't add log device: %s\n", buf);
							  exit(-1);
						  }
						  if (cntx->devices) {
							  if (log_devices_add_to_tail(cntx, cntx->dev)) {
								  fprintf(stderr, "Open log device %s failed\n", buf);
								  exit(-1);
							  }
						  } else {
							  cntx->devices = cntx->dev;
							  cntx->dev_count = 1;
						  }
					  }
            break;

            case 'f':
                // redirect output to a file

                cntx->output_filename = optarg;

            break;

            case 'r':
                    if (!isdigit(optarg[0])) {
                        fprintf(stderr,"Invalid parameter to -r\n");
                        show_help(argv[0]);
                        exit(-1);
                    }
                    cntx->log_rotate_size_kbytes = atoi(optarg);
            break;

            case 'n':
                if (!isdigit(optarg[0])) {
                    fprintf(stderr,"Invalid parameter to -r\n");
                    show_help(argv[0]);
                    exit(-1);
                }

                cntx->max_rotated_logs = atoi(optarg);
            break;

            case 'v':
                err = set_log_format (cntx, optarg);
                if (err < 0) {
                    fprintf(stderr,"Invalid parameter to -v\n");
                    show_help(argv[0]);
                    exit(-1);
                }

                cntx->has_set_log_format = 1;
            break;

			default:
				fprintf(stderr,"Unrecognized Option\n");
				show_help(argv[0]);
				exit(-1);
			break;
		}
	}

    return;
}

void add_to_devices(struct cntx_str * cntx)
{
	cntx->devices = log_devices_new("/dev/"LOGGER_LOG_MAIN);
	if (cntx->devices == NULL) {
		fprintf(stderr,"Can't add log device: %s\n", LOGGER_LOG_MAIN);
		exit(-1);
	}
    cntx->dev_count = 1;

    int accessmode =
              (cntx->mode == O_RDONLY) ? R_OK : 0
            | (cntx->mode == O_WRONLY) ? W_OK : 0;

    // only add this if it's available
    if (0 == access("/dev/"LOGGER_LOG_SYSTEM, accessmode)) {
        if (log_devices_add_to_tail(cntx, log_devices_new("/dev/"LOGGER_LOG_SYSTEM))) {
            fprintf(stderr,"Can't add log device: %s\n", LOGGER_LOG_SYSTEM);
            exit(-1);
        }
    }
    if (0 == access("/dev/"LOGGER_LOG_APPS, accessmode)) {
        if (log_devices_add_to_tail(cntx, log_devices_new("/dev/"LOGGER_LOG_APPS))) {
            fprintf(stderr,"Can't add log device: %s\n", LOGGER_LOG_APPS);
            exit(-1);
        }
    }

    return;
}

void add_to_filters(
        struct cntx_str * cntx,
        int argc,
        char **argv)
{
	int i;
    int err;

	if(argc == optind )
	{
		// Add from environment variable
		log_add_filter_string(cntx->logformat, "*:d");
	}
	else
	{
		for (i = optind ; i < argc ; i++) {
			err = log_add_filter_string(cntx->logformat, argv[i]);

			if (err < 0) {
				fprintf (stderr, "Invalid filter expression '%s'\n", argv[i]);
				show_help(argv[0]);
				exit(-1);
			}
		}
	}
    return;
}

void open_devices(struct cntx_str * cntx)
{
    cntx->dev = cntx->devices;
    while (cntx->dev) {
        cntx->dev->fd = TEMP_FAILURE_RETRY( open(cntx->dev->device, cntx->mode));
        if (cntx->dev->fd < 0) {
            fprintf(stderr, "Unable to open log device '%s': %s\n",
                cntx->dev->device, strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (cntx->is_clear_log) {
            int ret;
            ret = clear_log(cntx->dev->fd);
            if (ret) {
                perror("ioctl");
                exit(EXIT_FAILURE);
            }
        }

        if (cntx->getLogSize) {
            int size, readable;

            size = get_log_size(cntx->dev->fd);
            if (size < 0) {
                perror("ioctl");
                exit(EXIT_FAILURE);
            }

            readable = get_log_readable_size(cntx->dev->fd);
            if (readable < 0) {
                perror("ioctl");
                exit(EXIT_FAILURE);
            }

            printf("%s: ring buffer is %dKb (%dKb consumed), "
                   "max entry is %db, max payload is %db\n", cntx->dev->device,
                   size / 1024, readable / 1024,
                   (int) LOGGER_ENTRY_MAX_LEN, (int) LOGGER_ENTRY_MAX_PAYLOAD);
        }

        cntx->dev = cntx->dev->next;
    }
    return;
}

int main(int argc, char **argv)
{
    int err;
    struct cntx_str cntx;

    init_cntx(&cntx);
    cntx.logformat = (log_format *)log_format_new();
    cntx.log_file_dir = "/dev/log_";

    parse_opts(&cntx, argc, argv);

    /* get log size conflicts with write mode */
	if (cntx.getLogSize && cntx.mode != O_RDONLY) {
		show_help(argv[0]);
		exit(-1);
	}

	if (!(cntx.devices)) {
        add_to_devices(&cntx);
    }

    if (cntx.log_rotate_size_kbytes != 0 && cntx.output_filename == NULL)
	{
		fprintf(stderr,"-r requires -f as well\n");
		show_help(argv[0]);
		exit(-1);
	}

    setup_output(&cntx);

	if (cntx.has_set_log_format == 0) {
		err = set_log_format(&cntx, "brief");
        if (err < 0) 
    	    fprintf(stderr, "failed to set log format to brief\n");
	}

	fprintf(stderr,
            "arc = %d, optind = %d ,Kb %d, rotate %d\n",
            argc,
            optind,
            cntx.log_rotate_size_kbytes,
            cntx.max_rotated_logs);

    add_to_filters(&cntx, argc, argv);

    open_devices(&cntx);

    if (cntx.getLogSize) {
        return 0;
    }

    if (cntx.is_clear_log) {
        return 0;
    }

    read_log_lines(&cntx);

	log_devices_chain_free(cntx.devices);

    return 0;
}
