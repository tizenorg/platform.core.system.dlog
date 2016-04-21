#include <logcommon.h>

#define DLOG_CTRL_REQ_PIPE {sizeof(struct dlog_control_msg), DLOG_REQ_PIPE, 0}

enum {
	DLOG_REQ_PIPE=1,
	DLOG_REQ_BUFFER_SIZE,
	DLOG_REQ_CLEAR,
	DLOG_REQ_HANDLE_LOGUTIL,
	DLOG_REQ_REMOVE_WRITER,
	DLOG_REQ_MAX
};

enum {
	DLOG_FLAG_ANSW  = 0x01,
	DLOG_FLAG_WRITE = 0x02,
	DLOG_FLAG_READ  = 0x04
};

struct dlog_control_msg {
	char length;
	char request;
	char flags;
	char data[0];
};

struct log_buffer_desc {
	const char* path;
	const int   size;
	const int   flag;
};

struct dlog_crtl_file {
	unsigned char  buf_id;
	unsigned short rot_size;
	unsigned short max_size;
	char           path[0];
} __attribute__ ((packed));
