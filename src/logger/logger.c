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
#include <limits.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include <logcommon.h>
#include <queued_entry.h>
#include <log_file.h>
#include <logprint.h>
#include <dlog_ioctl.h>
#include <logconfig.h>

#define COMMAND_MAX 5
#define DELIMITER " "
#define MAX_ARGS 16
#define MAX_ROTATED 4
#define MAX_QUEUED 4096
#define BUFFER_MAX 100
#define INTERVAL_MAX 60*60

#define ARRAY_SIZE(name) (sizeof(name)/sizeof(name[0]))

struct log_command {
	char *filename;
	int option_file;
	int option_buffer;
	int rotate_size;
	int max_rotated;
	int devices[LOG_ID_MAX];
	log_format *format;
};

struct log_work {
	struct log_file file;
	bool printed;
	struct log_work *next;
};

struct log_task_link {
	struct log_work *work;
	struct log_task_link *next;
};

struct log_device {
	int id;
	int fd;
	uint32_t log_read_size_max;
	struct queued_entry_t *queue;
	struct log_task_link *task;
	struct log_device *next;
};

static struct log_config conf;

static struct log_work *works;
static struct log_device *devices;
static int device_list[] = {
	[LOG_ID_MAIN] = false,
	[LOG_ID_RADIO] = false,
	[LOG_ID_SYSTEM] = false,
	[LOG_ID_APPS] = false,
	[LOG_ID_MAX] = false,
};

static int buffer_size = 0;
static int min_interval = 0;

/*
 * check device registration on watch device list
 */
static int check_device(int id)
{
	if (id == LOG_ID_INVALID || LOG_ID_MAX <= id)
		return -1;

	return (device_list[id] == true) ? 0 : -1;
}

/*
 * register device to watch device list
 */
static int register_device(int id)
{
	if (id == LOG_ID_INVALID || LOG_ID_MAX <= id)
		return -1;
	device_list[id] = true;

	return 0;
}

/*
 * process to print log
 * and check the log file size to rotate
 */
static void process_buffer(struct log_device *dev, struct logger_entry *buf)
{
	int bytes_written, err;
	log_entry entry;
	struct log_work *logwork;
	struct log_task_link *task;

	err = log_process_log_buffer(buf, &entry);
	if (err < 0)
		goto exit;

	for (task = dev->task; task; task = task->next) {
		logwork = task->work;
		if (log_should_print_line(logwork->file.format,
					entry.tag, entry.priority)) {
			bytes_written =
				log_print_log_line(logwork->file.format,
						logwork->file.fd, &entry);
			if (bytes_written < 0) {
				_E("work error");
				exit(EXIT_FAILURE);
			}
			logwork->file.size += bytes_written;
		}
		if (logwork->file.rotate_size_kbytes > 0 &&
				(logwork->file.size / 1024) >= logwork->file.rotate_size_kbytes) {
			rotate_logs(&logwork->file);
		}
	}

exit:
	return;
}

/*
 * choose first device by log_entry
 */
static void choose_first(struct log_device *dev, struct log_device **firstdev)
{
	for (*firstdev = NULL; dev != NULL; dev = dev->next) {
		if (dev->queue != NULL &&
				(*firstdev == NULL ||
				 cmp(dev->queue,
					 (*firstdev)->queue) < 0)) {
			*firstdev = dev;
		}
	}
}

/*
 * print beginnig string into the log files
 */
static void maybe_print_start(struct log_device *dev)
{
	struct log_work *logwork;
	struct log_task_link *task;
	char buf[1024];

	for (task = dev->task; task; task = task->next) {
		logwork = task->work;
		if (!logwork->printed) {
			logwork->printed = true;
			snprintf(buf, sizeof(buf),
					"--------- beginning of %s\n",
					log_config_get(&conf, log_name_by_id(dev->id)));
			if (write(logwork->file.fd, buf, strlen(buf)) < 0) {
				_E("maybe work error");
				exit(EXIT_FAILURE);
			}
		}
	}
}

/*
 * skip log_entry
 */
static void skip_next_entry(struct log_device *dev)
{
	maybe_print_start(dev);
	struct queued_entry_t* entry = pop_queued_entry(&dev->queue);
	free_queued_entry(entry);
}

/*
 * print log_entry
 */
static void print_next_entry(struct log_device *dev)
{
	maybe_print_start(dev);
	process_buffer(dev, &dev->queue->entry);
	skip_next_entry(dev);
}

/*
 * do logging
 */
static void do_logger(struct log_device *dev)
{
	time_t commit_time = 0, current_time = 0;
	struct log_device *pdev;
	int result;
	fd_set readset;
	bool sleep = false;
	int queued_lines = 0;
	int max = 0;

	if (min_interval)
		commit_time = current_time = time(NULL);

	for (pdev = dev; pdev; pdev = pdev->next) {
		if (pdev->fd > max)
			max = pdev->fd;
	}

	while (1) {
		do {
			struct timeval timeout = { 0, 5000 /* 5ms */ };
			FD_ZERO(&readset);
			for (pdev = dev; pdev; pdev = pdev->next)
				FD_SET(pdev->fd, &readset);
			result = select(max + 1, &readset, NULL, NULL,
					sleep ? NULL : &timeout);
		} while (result == -1 && errno == EINTR);

		if (result < 0)
			continue;

		for (pdev = dev; pdev; pdev = pdev->next) {
			if (FD_ISSET(pdev->fd, &readset)) {
				int ret;

				struct queued_entry_t *entry =
					new_queued_entry(pdev->log_read_size_max);

				ret = read_queued_entry_from_dev(pdev->fd, entry,
								 pdev->log_read_size_max);

				if (ret == RQER_EINTR)
					goto next;
				else if (ret == RQER_EAGAIN)
					break;
				else if (ret == RQER_PARSE) {
					printf("Wrong formatted message is written. \n");
					continue;
				}
#ifdef DLOG_KMSG_BACKEND
				/* In KMSG, EPIPE is not an error: it signals the cyclic buffer
				 * having made a full turn and overwritten previous data */
				else if (ret == RQER_EPIPE)
					continue;
#endif
				enqueue(&pdev->queue, entry);
				++queued_lines;

				if (MAX_QUEUED < queued_lines) {
					_D("queued_lines = %d\n", queued_lines);
					while (true) {
						choose_first(dev, &pdev);
						if (pdev == NULL)
							break;
						print_next_entry(pdev);
						--queued_lines;
					}
					if (min_interval)
						commit_time = time(NULL);
					break;
				}
			}
		}

		if (min_interval) {
			current_time = time(NULL);
			if (current_time - commit_time < min_interval &&
					queued_lines < buffer_size) {
				sleep = true;
				continue;
			}
		}

		if (result == 0) {
			sleep = true;
			while (true) {
				choose_first(dev, &pdev);
				if (pdev == NULL)
					break;
				print_next_entry(pdev);
				--queued_lines;
				if (min_interval)
					commit_time = current_time;
			}
		} else {
			/* print all that aren't the last in their list */
			sleep = false;
			while (true) {
				choose_first(dev, &pdev);
				if (pdev == NULL || pdev->queue->next == NULL)
					break;
				print_next_entry(pdev);
				--queued_lines;
				if (min_interval)
					commit_time = current_time;
			}
		}
next:
		;
	}
}


/*
 * create a work
 */
static struct log_work *work_new(void)
{
	struct log_work *work;

	work = malloc(sizeof(struct log_work));
	if (work == NULL) {
		_E("failed to malloc log_work\n");
		return NULL;
	}
	work->file.path = NULL;
	work->file.fd = -1;
	work->printed = false;
	work->file.size = 0;
	work->next = NULL;
	_D("work alloc %p\n", work);

	return work;
}

/*
 * add a new log_work to the tail of chain
 */
static int work_add_to_tail(struct log_work *work, struct log_work *nwork)
{
	struct log_work *tail = work;

	if (!nwork)
		return -1;

	if (work == NULL) {
		work = nwork;
		return 0;
	}

	while (tail->next)
		tail = tail->next;
	tail->next = nwork;

	return 0;
}

/*
 * add a new work task to the tail of chain
 */
static void work_add_to_device(struct log_device *dev, struct log_work *work)
{
	struct log_task_link *tail;

	if (!dev || !work)
		return;
	_D("dev %p work %p\n", dev, work);
	if (dev->task == NULL) {
		dev->task =
			(struct log_task_link *)
			malloc(sizeof(struct log_task_link));
		if (dev->task == NULL) {
			_E("failed to malloc log_task_link\n");
			return;
		}
		tail = dev->task;
	} else {
		tail = dev->task;
		while (tail->next)
			tail = tail->next;
		tail->next =
			(struct log_task_link *)
			malloc(sizeof(struct log_task_link));
		if (tail->next == NULL) {
			_E("failed to malloc log_task_link\n");
			return;
		}
		tail = tail->next;
	}
	tail->work = work;
	tail->next = NULL;
}

/*
 * free work file descriptor
 */
static void work_free(struct log_work *work)
{
	if (!work)
		return;
	if (work->file.path) {
		free(work->file.path);
		work->file.path = NULL;
		if (work->file.fd != -1) {
			close(work->file.fd);
			work->file.fd = -1;
		}
	}
	log_format_free(work->file.format);
	work->file.format = NULL;
	free(work);
	work = NULL;
}

/*
 * free all the nodes after the "work" and includes itself
 */
static void work_chain_free(struct log_work *work)
{
	if (!work)
		return;
	while (work->next) {
		struct log_work *tmp = work->next;
		work->next = tmp->next;
		work_free(tmp);
	}
	work_free(work);
	work = NULL;
}

/*
 * create a new log_device instance
 * and open device
 */
static struct log_device *device_new(int id)
{
	struct log_device *dev;

	if (id == LOG_ID_INVALID || LOG_ID_MAX <= id)
		return NULL;
	dev = malloc(sizeof(struct log_device));
	if (dev == NULL) {
		_E("failed to malloc log_device\n");
		return NULL;
	}
	dev->id = id;
	dev->fd = open(log_config_get(&conf, log_name_by_id(id)), O_RDONLY);
	if (dev->fd < 0) {
		_E("Unable to open log device %s (%d)",
				log_config_get(&conf, log_name_by_id(id)),
				errno);
		free(dev);
		return NULL;
	}
	_D("device new id %d fd %d\n", dev->id, dev->fd);
	dev->task = NULL;
	dev->queue = NULL;
	dev->next = NULL;

	get_log_read_size_max(dev->fd, &dev->log_read_size_max);
	_D("device %s log read size max %u\n",
	   log_config_get(&conf, log_name_by_id(id)), dev->log_read_size_max);

#if DLOG_BACKEND_KMSG
	off_t off;

	off = lseek(dev->fd, 0, SEEK_DATA);
	if (off == -1) {
		_E("Unable to lseek device %s. %d\n",
		   log_config_get(&conf, log_name_by_id(id)), errno);
		exit(EXIT_FAILURE);
	}
#endif

	return dev;
}

/*
 * add a new log_device to the tail of chain
 */
static int device_add_to_tail(struct log_device *dev, struct log_device *ndev)
{
	struct log_device *tail = dev;

	if (!dev || !ndev)
		return -1;

	while (tail->next)
		tail = tail->next;
	tail->next = ndev;

	return 0;
}

/*
 * add a new log_device or add to the tail of chain
 */
static void device_add(int id)
{
	if (check_device(id) < 0)
		return;

	if (!devices) {
			devices = device_new(id);
			if (devices == NULL) {
				_E("failed to device_new: %s\n",
						log_config_get(&conf, log_name_by_id(id)));
				exit(EXIT_FAILURE);
			}
	} else {
		if (device_add_to_tail(devices, device_new(id)) < 0) {
			_E("failed to device_add_to_tail: %s\n",
					log_config_get(&conf, log_name_by_id(id)));
			exit(EXIT_FAILURE);
		}
	}
	return;
}

/*
 * free one log_device  and it doesn't take care of chain so it
 * may break the chain list
 */
static void device_free(struct log_device *dev)
{
	if (!dev)
		return;
	if (dev->queue) {
		free_queued_entry_list(dev->queue);
		dev->queue = NULL;
	}
	if (dev->task) {
		while (dev->task->next) {
			struct log_task_link *tmp =
				dev->task->next;
			dev->task->next = tmp->next;
			free(tmp);
		}
		free(dev->task);
		dev->task = NULL;
	}
	free(dev);
	dev = NULL;
}

/*
 * free all the nodes after the "dev" and includes itself
 */
static void device_chain_free(struct log_device *dev)
{
	if (!dev)
		return;
	while (dev->next) {
		struct log_device *tmp = dev->next;
		dev->next = tmp->next;
		device_free(tmp);
	}
	device_free(dev);
	dev = NULL;
}

/*
 * parse command line
 * using getopt function
 */
static int parse_command_line(char const *linebuffer, struct log_command *cmd)
{
	int i, ret, id, argc;
	char *argv[MAX_ARGS];
	char *tok, *saveptr, *cmdline;

	if (linebuffer == NULL || cmd == NULL)
		return -1;
	/* copy command line */
	cmdline = strdup(linebuffer);
	tok = strtok_r(cmdline, DELIMITER, &saveptr);
	/* check the availability of command line
	   by comparing first word with dlogutil*/
	if (!tok || strcmp(tok, "dlogutil")) {
		_D("Ignore this line (%s)\n", linebuffer);
		free(cmdline);
		return -1;
	}
	_D("Parsing this line (%s)\n", linebuffer);
	/* fill the argc and argv
	   for extract option from command line */
	argc = 0;
	while (tok && (argc < MAX_ARGS)) {
		argv[argc] = strdup(tok);
		tok = strtok_r(NULL, DELIMITER, &saveptr);
		argc++;
	}
	free(cmdline);

	/* initialize the command struct with the default value */
	memset(cmd->devices, 0, sizeof(cmd->devices));
	cmd->option_file = false;
	cmd->option_buffer = false;
	cmd->filename = NULL;
	cmd->rotate_size = 0;
	cmd->max_rotated = MAX_ROTATED;
	cmd->format = (log_format *)log_format_new();

	/* get option and fill the command struct */
	while ((ret = getopt(argc, argv, "f:r:n:v:b:")) != -1) {
		switch (ret) {
		case 'f':
			cmd->filename = strdup(optarg);
			_D("command filename %s\n", cmd->filename);
			cmd->option_file = true;
			break;
		case 'b':
			id = log_id_by_name(optarg);
			_D("command device name %s id %d\n", optarg, id);
			if (id == LOG_ID_INVALID)
				break;
			cmd->option_buffer = true;
			/* enable to log in device on/off struct */
			cmd->devices[id] = true;
			/* enable to open in global device on/off struct */
			register_device(id);
			break;
		case 'r':
			if (!isdigit(optarg[0]))
				goto exit_free;
			cmd->rotate_size = atoi(optarg);
			_D("command rotate size %d\n", cmd->rotate_size);
			break;
		case 'n':
			if (!isdigit(optarg[0]))
				goto exit_free;
			cmd->max_rotated = atoi(optarg);
			_D("command max rotated %d\n", cmd->max_rotated);
			break;
		case 'v':
			{
				log_print_format print_format;
				print_format = log_format_from_string(optarg);
				if (print_format == FORMAT_OFF) {
					_E("failed to add format\n");
					goto exit_free;
				}
				_D("command format %s\n", optarg);
				log_set_print_format(cmd->format, print_format);
			}
			break;
		default:
			break;
		}
	}
	/* add filter string, when command line have tags */
	if (argc != optind) {
		for (i = optind ; i < argc ; i++) {
			ret = log_add_filter_string(cmd->format, argv[i]);
			_D("command add fileter string %s\n", argv[i]);
			if (ret < 0) {
				_E("Invalid filter expression '%s'\n", argv[i]);
				goto exit_free;
			}
		}
	} else {
		ret = log_add_filter_string(cmd->format, "*:d");
		if (ret < 0) {
			_E("Invalid silent filter expression\n");
			goto exit_free;
		}
	}
	/* free argv */
	for (i = 0; i < argc; i++)
		free(argv[i]);

	if (cmd->option_file == false)
		goto exit_free;
	/* If it have not the -b option,
	   set the default devices to open and log */
	if (cmd->option_buffer == false) {
		_D("set default device\n");
		cmd->devices[LOG_ID_MAIN] = true;
		cmd->devices[LOG_ID_SYSTEM] = true;
		register_device(LOG_ID_MAIN);
		register_device(LOG_ID_SYSTEM);
	}
	/* for use getopt again */
	optarg = NULL;
	optind = 1;
	optopt = 0;

	return 0;

exit_free:
	if (cmd->filename)
		free(cmd->filename);
	return -1;
}

/*
 * parse command from configuration file
 * and return the command list with number of command
 * if an command was successfully parsed, then it returns number of parsed command.
 * on error or not founded, 0 is returned
 */
static int parse_command(struct log_command *command_list)
{
	struct log_config conf;
	char conf_key [MAX_CONF_KEY_LEN];
	const char * conf_val;
	int ncmd;

	if (!log_config_read (&conf))
		return 0;

	ncmd = 0;
	while (1) {
		sprintf (conf_key, "dlog_logger_conf_%d", ncmd);
		conf_val = log_config_get (&conf, conf_key);
		if (!conf_val)
			break;
		if (parse_command_line(conf_val, &command_list[ncmd]) == 0)
			ncmd++;
		if (COMMAND_MAX <= ncmd)
			break;
	}

	log_config_free (&conf);

	return ncmd;
}

/*
 * free dynamically allocated memory
 */
static void cleanup(void)
{
	work_chain_free(works);
	device_chain_free(devices);
}

/*
 * SIGINT, SIGTERM, SIGQUIT signal handler
 */
static void sig_handler(int signo)
{
	_D("sig_handler\n");
	cleanup();
	exit(EXIT_SUCCESS);
}

static int help(void)
{

	printf("%s [OPTIONS...] \n\n"
			"Logger, records log messages to files.\n\n"
			"  -h      Show this help\n"
			"  -b N    Set the number of logs to keep logs in memory buffer before writing files (default:0, MAX:100)\n"
			"  -t N    Set minimum interval time to write files (default:0, MAX:3600, seconds)\n",
			program_invocation_short_name);

	return 0;
}

static int parse_argv(int argc, char *argv[])
{
	int ret = 1, option;

	while ((option = getopt(argc, argv, "hb:t:")) != -1) {
		switch (option) {
		case 't':
			if (!isdigit(optarg[0])) {
				ret = -EINVAL;
				printf("Wrong argument!\n");
				help();
				goto exit;
			}
			min_interval = atoi(optarg);
			if (min_interval < 0 || INTERVAL_MAX < min_interval)
				min_interval = 0;
			ret = 1;
			break;
		case 'b':
			if (!isdigit(optarg[0])) {
				ret = -EINVAL;
				printf("Wrong argument!\n");
				help();
				goto exit;
			}
			buffer_size = atoi(optarg);
			if (buffer_size < 0 || BUFFER_MAX < buffer_size)
				buffer_size = 0;
			ret = 1;
			break;
		case 'h':
			help();
			ret = 0;
			goto exit;
		default:
			ret = -EINVAL;
		}
	}
exit:
	optarg = NULL;
	optind = 1;
	optopt = 0;
	return ret;
}


int main(int argc, char **argv)
{
	/* Nothing to do if the backend is journal */
#if DLOG_BACKEND_JOURNAL
	return 0;
#endif

	int i, r, ncmd;
	struct stat statbuf;
	struct log_device *dev;
	struct log_work *work;
	struct log_command command_list[COMMAND_MAX];
	struct sigaction act;

	/* set the signal handler for free dynamically allocated memory. */
	memset(&act, 0, sizeof(struct sigaction));
	sigemptyset(&act.sa_mask);
	act.sa_handler = (void *)sig_handler;
	act.sa_flags = 0;

	if (sigaction(SIGQUIT, &act, NULL) < 0)
		_E("failed to sigaction for SIGQUIT");
	if (sigaction(SIGINT, &act, NULL) < 0)
		_E("failed to sigaction for SIGINT");
	if (sigaction(SIGTERM, &act, NULL) < 0)
		_E("failed to sigaction for SIGTERM");

	if (argc != 1) {
		r = parse_argv(argc, argv);
		if (r <= 0)
			goto exit;
	}
	/* parse command from command configuration file. */
	ncmd = parse_command(command_list);
	/* If it have nothing command, exit. */
	if (!ncmd)
		goto exit;

	if (!log_config_read (&conf))
		goto exit;

	/* create log device */
	device_add(LOG_ID_MAIN);
	device_add(LOG_ID_SYSTEM);
	device_add(LOG_ID_RADIO);

	/* create work from the parsed command */
	for (i = 0; i < ncmd; i++) {
		work = work_new();
		_D("work new\n");
		if (work == NULL) {
			_E("failed to work_new\n");
			goto clean_exit;
		}
		/* attatch the work to global works variable */
		if (work_add_to_tail(works, work) < 0) {
			_E("failed to work_add_to_tail\n");
			goto clean_exit;
		}
		/* 1. set filename, fd and file current size */
		work->file.path = command_list[i].filename;
		if (work->file.path == NULL) {
			_E("file name is NULL");
			goto clean_exit;
		}
		open_logfile(&work->file);
		if (fstat(work->file.fd, &statbuf) == -1)
			work->file.size = 0;
		else
			work->file.size = statbuf.st_size;

		/* 2. set size limits for log files */
		work->file.rotate_size_kbytes = command_list[i].rotate_size;

		/* 3. set limit on the number of rotated log files */
		work->file.max_rotated = command_list[i].max_rotated;

		/* 4. set log_format to filter logs*/
		work->file.format = command_list[i].format;

		/* 5. attatch the work to device task for logging */
		dev = devices;
		while (dev) {
			if (command_list[i].devices[dev->id] == true) {
				work_add_to_device(dev, work);
			}
			dev = dev->next;
		}
	}

	/* do log */
	do_logger(devices);

clean_exit:
	work_chain_free(works);
	device_chain_free(devices);
exit:
	return 0;
}
