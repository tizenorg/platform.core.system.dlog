#define _GNU_SOURCE

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

#define SOCK_PATH_SEQP     "./seqp.pipe"

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))

int unix_open
	( int type
	, const char * path
) {
	int r;
	int fd;
	struct sockaddr_un sa = {
		.sun_family = AF_UNIX
	};

	fd = socket
		( AF_UNIX
		, type | SOCK_CLOEXEC
		, 0
	);

	if (fd < 0) {
		return -errno;
	}

	strncpy
		( sa.sun_path
		, path
		, ARRAY_SIZE (sa.sun_path)
	);

	r = connect
		( fd
		, (struct sockaddr *) & sa
		, sizeof (sa)
	);
	if (r < 0) {
		close (fd);
		return -errno;
	}

	return fd;
}

// sizeof(msg) == 256
char msg [] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

int main (int argc, char ** argv)
{
	int i, j, widelec;
	int sock_fd;
	int milliseconds;
	struct timespec time_stamp;
	struct epoll_event ev;
	struct msghdr hdr;
        struct iovec iov;

        hdr.msg_name = NULL;
        hdr.msg_namelen = 0;
        hdr.msg_iov = &iov;
        hdr.msg_iovlen = 1;
        hdr.msg_control = NULL;
        hdr.msg_controllen = 0;
        hdr.msg_flags = 0;
        iov.iov_base = msg;
        iov.iov_len = 256;

	sock_fd = unix_open (SOCK_SEQPACKET, SOCK_PATH_SEQP);

	clock_gettime (CLOCK_MONOTONIC, &time_stamp);
	j = time_stamp.tv_sec * 1000 + time_stamp.tv_nsec / 1000000;
	printf("START: %d ms\n", j);

	for (i = 0; i < 104448; ++i) {
		clock_gettime (CLOCK_MONOTONIC, &time_stamp);
		j = time_stamp.tv_sec * 1000 + time_stamp.tv_nsec / 1000000;

		j = sprintf (msg, "%d", j);
		msg[j] = ';';
		sendmsg (sock_fd, &hdr, 0);
	}

	close(sock_fd);

        clock_gettime (CLOCK_MONOTONIC, &time_stamp);
        j = time_stamp.tv_sec * 1000 + time_stamp.tv_nsec / 1000000;
        printf("%d END: %d ms\n", widelec, j);

	return EXIT_SUCCESS;
}

