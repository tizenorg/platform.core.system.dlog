/*
 * Copyright (c) 2016, Samsung Electronics Co., Ltd. All rights reserved.
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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#define READ_BUFFER_SIZE  	512          	// Maximum message size. Truncation is inflicted upon longer messages
#define MAX_ANTISPAM_FDS  	1024         	// How many streams are handled at once. No connections are accepted above this limit
#define ANTISPAM_MAX_LINES	10           	// How many log lines are read per iteration. Higher values increase speed, but also starvation.
#define LOG_FILE_PATH     	"/dev/null"  	// Filename into which to write
#define SOCK_PATH         	"./seqp.pipe"	// Filename of the socket
#define LOG_TIME_STORE_MS 	50           	// Maximum age of a log in ms. Received elder logs are immediately flushed to file without sorting
#define LOG_STORE_SIZE    	2048         	// How many logs are stored for sorting. Adding more flushes the eldest

#define OPTIMIZED 0
// 0 = malloc for each entry, 1 = one large storage malloc'd, 2 = large storage on the stack
// measurements suggest 0 > 1

typedef struct {
	int timestamp;
	int len;
#if OPTIMIZED == 0
	char * buf;
#else
	int buf;
#endif
} log_entry;

int main ()
{
#if OPTIMIZED > 0
#if OPTIMIZED == 1
	char * sloty;
#else
	char sloty [LOG_STORE_SIZE * READ_BUFFER_SIZE];
#endif
	int wolne_sloty[LOG_STORE_SIZE];
	int wolne_sloty_end = LOG_STORE_SIZE;
#endif
	int r;
	int i, j, k;
	int timestamp, timestamp_msg;
	int epoll_fd, log_fd, sock_fd;
	log_entry log_storage [LOG_STORE_SIZE];
	char read_buffer [READ_BUFFER_SIZE];
	int storage_start = 0, storage_end = 0;
	int fd_list [MAX_ANTISPAM_FDS];
	int fd_list_end = 0;
	struct timespec time_stamp;
	struct epoll_event ev;
	struct msghdr hdr;
	struct iovec iov;
	struct sockaddr_un sa = {
		.sun_family = AF_UNIX
	};
	socklen_t slen;

#if OPTIMIZED == 1
	sloty = malloc (LOG_STORE_SIZE * READ_BUFFER_SIZE);
	if (!sloty) {
		printf("PIPE: sloty malloc failed\n");
		exit (EXIT_FAILURE);
	}
#endif

#if OPTIMIZED > 0
	for (i = 0; i < LOG_STORE_SIZE; ++i) {
		wolne_sloty[i] = i;
	}
#endif

	log_fd = open
		( LOG_FILE_PATH
		, O_WRONLY | O_CREAT | O_APPEND
		, 0640
	);

	epoll_fd = epoll_create1 (EPOLL_CLOEXEC);

	sock_fd = socket
		( AF_UNIX
		, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC
		, 0
	);
	unlink (SOCK_PATH);

	strncpy
		( sa.sun_path
		, SOCK_PATH
		, sizeof (sa.sun_path)
	);

	bind
		( sock_fd
		, (struct sockaddr *) & sa
		, sizeof (sa)
	);

	listen (sock_fd, SOMAXCONN);

	if ((sock_fd < 0) || (epoll_fd < 0) || (log_fd < 0)) {
		fprintf
			( stderr
			, "PIPE: initialisation failure! FD:\n"
			  "\tSocket  %d\n"
			  "\tePoll   %d\n"
			  "\tLogFile %d\n"
			, sock_fd
			, epoll_fd
			, log_fd
		);
		exit (EXIT_FAILURE);
	}

	ev.events = EPOLLIN;
	ev.data.fd = sock_fd;
	epoll_ctl
		( epoll_fd
		, EPOLL_CTL_ADD
		, sock_fd
		, & ev
	);

	hdr.msg_name = NULL;
	hdr.msg_namelen = 0;
	hdr.msg_iov = &iov;
	hdr.msg_iovlen = 1;
	hdr.msg_control = NULL;
	hdr.msg_controllen = 0;
	hdr.msg_flags = 0;
	iov.iov_base = read_buffer;
	iov.iov_len = READ_BUFFER_SIZE - 1; // LEAVE SOME ROOM TO TERMINATE THE LOG WITH A NEWLINE IF NEED BE.

	while (1) {
		if (fd_list_end < MAX_ANTISPAM_FDS) {
			do {
				r = epoll_wait
					( epoll_fd
					, & ev
					, 1
					, 1
				);
				if (r) {
					if (ev.data.fd != sock_fd) {
						if (ev.events & EPOLLIN) {
							fd_list [fd_list_end++] = ev.data.fd;
						}
					} else {
						slen = sizeof (sa);
						i = accept4
							( sock_fd
							, (struct sockaddr *) & sa
							, & slen
							, SOCK_NONBLOCK | SOCK_CLOEXEC
						);
						if (i >= 0)
						{
							fcntl
								( i
								, F_SETFL
								, fcntl (i, F_GETFL, 0) | O_NONBLOCK
							);
							ev.data.fd = i;
							ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
							epoll_ctl
								( epoll_fd
								, EPOLL_CTL_ADD
								, i
								, & ev
							);
						}
					}
				}
			} while (r && (fd_list_end < MAX_ANTISPAM_FDS));
		}

		for (j = 0; j < fd_list_end; ++j) {
			i = ANTISPAM_MAX_LINES;

			do {
				r = recvmsg
					( fd_list[j]
					, & hdr
					, 0
				);
				if (r > 0) {
					switch (read_buffer[r-1]) {
						case '\0':
							read_buffer[r-1] = '\n';
						case '\n':
							break;
						default:
							read_buffer[r++] = '\n';
					}
					clock_gettime
						( CLOCK_MONOTONIC
						, & time_stamp
					);
					timestamp
						= time_stamp.tv_sec  * 1000
						+ time_stamp.tv_nsec / 1000000
					;
					timestamp_msg = atoi(read_buffer);

					if (timestamp > timestamp_msg + LOG_TIME_STORE_MS) {
						write
							( log_fd
							, read_buffer
							, r
						);
					} else {
						if (storage_start ? ((storage_end + 1) == storage_start) : (storage_end == (LOG_STORE_SIZE - 1))) {
							write
								( log_fd
#if OPTIMIZED == 0
								, log_storage[storage_start].buf
#else
								, sloty + (READ_BUFFER_SIZE * log_storage[storage_start].buf)
#endif
								, log_storage[storage_start].len
							);

#if OPTIMIZED == 0
								free (log_storage[storage_start].buf);
#else
								wolne_sloty[wolne_sloty_end++] = log_storage[storage_start].buf;
#endif
							if (++storage_start == LOG_STORE_SIZE) storage_start = 0;
						}
						k = storage_end++;
						if (storage_end == LOG_STORE_SIZE) storage_end = 0;
						while (1) {
							if ((k == storage_start) || (timestamp_msg >= log_storage[k ? (k-1) : (LOG_STORE_SIZE-1)].timestamp)) {
#if OPTIMIZED == 0
									log_storage[k].buf = malloc (r);
									strncpy (log_storage[k].buf, read_buffer, r);
#else
									log_storage[k].buf = wolne_sloty[--wolne_sloty_end];
									strncpy (sloty + (log_storage[k].buf * READ_BUFFER_SIZE), read_buffer, r);
#endif
								log_storage[k].len = r;
								log_storage[k].timestamp = timestamp_msg;
								break;
							} else {
								if (!k) {
									log_storage[k] = log_storage[LOG_STORE_SIZE - 1];
									k = LOG_STORE_SIZE;
								} else {
									log_storage[k] = log_storage[k - 1];
								}
								--k;
							}
						}
					}
				}
			} while (--i && (r > 0));
			if (r < 0 && errno == EAGAIN) {
				ev.data.fd = fd_list[j];
				ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
				epoll_ctl
					( epoll_fd
					, EPOLL_CTL_MOD
					, fd_list[j]
					, & ev
				);
				fd_list[j--] = fd_list [--fd_list_end];
			} else if (r == 0) {
				close (fd_list[j]);
				epoll_ctl
				( epoll_fd
				, EPOLL_CTL_DEL
					, fd_list[j]
					, NULL
				);
				fd_list[j--] = fd_list[--fd_list_end];
			}
		}

		clock_gettime
			( CLOCK_MONOTONIC
			, & time_stamp
		);
		timestamp
			= time_stamp.tv_sec  * 1000
			+ time_stamp.tv_nsec / 1000000
			- LOG_TIME_STORE_MS
		;

		while ((storage_start != storage_end) && (timestamp > log_storage[storage_start].timestamp)) {
			write
				( log_fd
#if OPTIMIZED == 0
				, log_storage[storage_start].buf
#else
				, sloty + (READ_BUFFER_SIZE * log_storage[storage_start].buf)
#endif
				, log_storage[storage_start].len
			);

#if OPTIMIZED == 0
			free (log_storage[storage_start].buf);
#else
			wolne_sloty[wolne_sloty_end++] = log_storage[storage_start].buf;
#endif
			if (++storage_start == LOG_STORE_SIZE) storage_start = 0;
		}
	}

	return EXIT_FAILURE; // should never arrive here
}

