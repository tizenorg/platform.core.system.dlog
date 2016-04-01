#ifndef _DLOG_IOCTL_H_
#define _DLOG_IOCTL_H_

#include <sys/ioctl.h>
#include <stdint.h>

#if DLOG_BACKEND_KMSG

struct kmsg_cmd_buffer_add {
	size_t size;
	unsigned short mode;
	int minor;
} __attribute__((packed));

#define KMSG_IOCTL_MAGIC        0xBB

/*
 * A ioctl interface for kmsg device.
 *
 * KMSG_CMD_BUFFER_ADD: Creates additional kmsg device based on its size
 *                      and mode. Minor of created device is put.
 * KMSG_CMD_BUFFER_DEL: Removes additional kmsg device based on its minor
 */
#define KMSG_CMD_BUFFER_ADD         _IOWR(KMSG_IOCTL_MAGIC, 0x00, struct kmsg_cmd_buffer_add)
#define KMSG_CMD_BUFFER_DEL         _IOW(KMSG_IOCTL_MAGIC, 0x01, int)

/*
 * A ioctl interface for kmsg* devices.
 *
 * KMSG_CMD_GET_BUF_SIZE:      Retrieve cyclic log buffer size associated with
 *                             device.
 * KMSG_CMD_GET_READ_SIZE_MAX: Retrieve max size of data read by kmsg read
 *                             operation.
 * KMSG_CMD_CLEAR:             Clears cyclic log buffer. After that operation
 *                             there is no data to read from buffer unless
 *                             logs are written.
 */
#define KMSG_CMD_GET_BUF_SIZE       _IOR(KMSG_IOCTL_MAGIC, 0x80, uint32_t)
#define KMSG_CMD_GET_READ_SIZE_MAX  _IOR(KMSG_IOCTL_MAGIC, 0x81, uint32_t)
#define KMSG_CMD_CLEAR              _IO(KMSG_IOCTL_MAGIC, 0x82)

#else

#define LOGGER_ENTRY_MAX_LEN		(4*1024)
#define LOGGER_ENTRY_MAX_PAYLOAD	(LOGGER_ENTRY_MAX_LEN - sizeof(struct logger_entry))

#define __LOGGERIO	0xAE
#define LOGGER_GET_LOG_BUF_SIZE		_IO(__LOGGERIO, 1) /* size of log */
#define LOGGER_GET_LOG_LEN		_IO(__LOGGERIO, 2) /* used log len */
#define LOGGER_GET_NEXT_ENTRY_LEN	_IO(__LOGGERIO, 3) /* next entry len */
#define LOGGER_FLUSH_LOG		_IO(__LOGGERIO, 4) /* flush log */

#endif

void clear_log(int fd);
void get_log_size(int fd, uint32_t *size);
void get_log_read_size_max(int fd, uint32_t *size);

#endif
