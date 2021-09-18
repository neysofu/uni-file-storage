#define _POSIX_C_SOURCE 200809L
/* glibc treats `realpath` as a GNU extension, not POSIX. */
#define _GNU_SOURCE

#include "serverapi.h"
#include "logc/src/log.h"
#include "serverapi_utilities.h"
#include "utilities.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

/******* GLOBAL STATE */

struct ConnectionState
{
	bool connection_is_open;
	int fd;
	char *socket_name;
	int last_operation;
};

struct ConnectionState state = {
	false,
	-1,
	NULL,
	0xff,
};

/******* UTILITY FUNCTIONS */

static int
file_contents(const char *relative_path, char abs_path[], void **buffer, size_t *size)
{
	assert(relative_path);
	FILE *f = NULL;
	log_trace("Getting the absolute path of '%s'...", relative_path);
	char *s = realpath(relative_path, abs_path);
	if (!s) {
		log_error("Couldn't get the absolute path.");
		errno = ENAMETOOLONG;
		return -1;
	}
	log_trace("Done, it's '%s'.", s);
	f = fopen(abs_path, "r");
	if (!f) {
		log_error("Couldn't open the given file.");
		errno = EIO;
		return -1;
	}
	fseek(f, 0L, SEEK_END);
	*size = ftell(f);
	*buffer = xmalloc(*size);
	fread(*buffer, 1, *size, f);
	return 0;
}

static int
on_io_err(void)
{
	log_error("Tried to read/write data, but there's been some I/O error.");
	errno = EIO;
	return -1;
}

static int
write_op(int fd, enum ApiOp op)
{
	char op_byte = op;
	return write(fd, &op_byte, 1);
}

/* Writes a 64 bit unsigned number to `fd` as big endian. */
static int
write_u64(int fd, uint64_t data)
{
	char bytes[8];
	u64_to_big_endian(data, (uint8_t *)bytes);
	return writen(fd, bytes, 8);
}

static int
attempt_connection(const char *sockname)
{
	assert(sockname);
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		return -1;
	}
	state.fd = fd;
	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, sockname);
	state.socket_name = xmalloc(strlen(sockname) + 1);
	strcpy(state.socket_name, sockname);
	int err = connect(fd, (struct sockaddr *)(&addr), SUN_LEN(&addr));
	if (err < 0) {
		return -1;
	}
	state.connection_is_open = true;
	state.fd = fd;
	return 0;
}

/* When the connection is closed but was supposed to be open, this function:
 *  - sets `errno`;
 *  - logs a descriptive error message;
 *  - returns `-1` as an error code. */
static int
err_closed_connection(void)
{
	errno = ENOENT;
	log_error("No active connection.");
	return -1;
}

static int
write_file_to_dir(const char *dirname,
                  const void *original_pathname,
                  size_t original_pathname_size,
                  const void *contents,
                  size_t contents_size)
{
	FILE *f;
	f = fopen(dirname, "wb");
	if (!f) {
		return -1;
	}
	size_t num_bytes_written = fwrite(contents, 1, contents_size, f);
	fclose(f);
	return 0;
}

/* Reads a response from the server with a variable number of files and writes
 * them to `dirname`. */
static int
handle_response_with_files(int fd, const char *dirname)
{
	int err = 0;
	/* Read header. */
	uint8_t buffer_response_code[1] = { '\0' };
	uint8_t buffer_num_files[8] = { '\0' };
	err |= read(fd, buffer_response_code, 1);
	if (err < 0 || buffer_response_code[0] == RESPONSE_ERR) {
		return -1;
	}
	err |= readn(fd, buffer_num_files, 8);
	if (err < 0) {
		return -1;
	}
	size_t num_files = big_endian_to_u64(buffer_num_files);
	/* Read actual file contents. */
	for (uint64_t i = 0; i < num_files; i++) {
		uint8_t buffer_lengths[16] = { '\0' };
		err |= readn(state.fd, buffer_lengths, 16);
		if (err < 0) {
			return -1;
		}
		size_t len_path = big_endian_to_u64(buffer_lengths);
		size_t len_contents = big_endian_to_u64(buffer_lengths + 8);
		void *buffer = xmalloc(len_path + len_contents);
		err |= readn(state.fd, buffer, len_path + len_contents);
		if (!err) {
			free(buffer);
			return on_io_err();
		}
		err |= write_file_to_dir(
		  dirname, buffer, len_path, (char *)buffer + len_path, len_contents);
		if (err < 0) {
			free(buffer);
			return on_io_err();
		}
		free(buffer);
	}
	return 0;
}

/* A simple request is made up by:
 * - 8 byte header (length prefix).
 * - 1 byte operation code.
 * - N remaining bytes for a single variable-length argument.
 *
 * The response is just one single byte with a response code. */
static int
make_simple_request(enum ApiOp op, const char *pathname)
{
	state.last_operation = op;
	if (!state.connection_is_open) {
		return err_closed_connection();
	}
	int err = 0;
	err |= write_u64(state.fd, 1 + strlen(pathname));
	err |= write_op(state.fd, op);
	err |= writen(state.fd, (void *)pathname, strlen(pathname));
	if (err < 0) {
		return on_io_err();
	}
	char buffer[1] = { '\0' };
	err |= read(state.fd, buffer, 1);
	if (err < 0) {
		return on_io_err();
	}
	return 0;
}

/* A request type with support for two variable-length arguments:
 * - 8 byte header (length prefix).
 * - 1 byte operation code.
 * - 8 bytes, length of 1st argument.
 * - 8 bytes, length of 2nd argument.
 * - N bytes for 1st argument.
 * - M bytes for 2nd argument.
 *
 * The response is operation-specific. */
static int
make_request_with_two_args(enum ApiOp op,
                           const void *arg1,
                           size_t arg1_size,
                           const void *arg2,
                           size_t arg2_size)
{
	state.last_operation = op;
	if (!state.connection_is_open) {
		return err_closed_connection();
	}
	log_trace("Sending file contents to server...");
	int err = 0;
	/* Request: */
	size_t message_size_in_bytes = 1 + 8 + 8 + arg1_size + arg2_size;
	err |= write_u64(state.fd, message_size_in_bytes);
	err |= write_op(state.fd, op);
	err |= write_u64(state.fd, arg1_size);
	err |= write_u64(state.fd, arg2_size);
	err |= writen(state.fd, arg1, arg1_size);
	err |= writen(state.fd, arg2, arg2_size);
	if (err < 0) {
		return on_io_err();
	}
	log_trace("Done.");
	return 0;
}

/******* API IMPLEMENTATIONS */

int
openConnection(const char *sockname, int msec, const struct timespec abstime)
{
	state.last_operation = API_OP_OPEN_CONNECTION;
	assert(sockname);
	if (state.connection_is_open) {
		return err_closed_connection();
	}
	while (true) {
		struct timespec now;
		clock_gettime(CLOCK_REALTIME, &now);
		if (now.tv_sec > abstime.tv_sec && abstime.tv_sec > 0) {
			errno = ETIMEDOUT;
			return -1;
		}
		log_info("New connection attempt.");
		int err = attempt_connection(sockname);
		if (!err) {
			break;
		}
		wait_msec(msec);
	}
	return 0;
}

int
closeConnection(const char *sockname)
{
	state.last_operation = API_OP_CLOSE_CONNECTION;
	if (!state.connection_is_open) {
		errno = ENOENT;
		return -1;
	} else if (strcmp(state.socket_name, sockname) != 0) {
		errno = EPERM;
		return -1;
	}
	int result = close(state.fd);
	if (result == -1) {
		return result;
	}
	free(state.socket_name);
	state.connection_is_open = false;
	state.socket_name = NULL;
	state.fd = -1;
	return 0;
}

int
readFile(const char *pathname, void **buf, size_t *size)
{
	int err = make_simple_request(API_OP_READ_FILE, pathname);
	if (err < 0) {
		return -1;
	}
	/* After the "simple" request, `readFile` also must read the file contents. */
	uint8_t buffer_len[8] = { 0 };
	err |= readn(state.fd, buffer_len, 8);
	*size = big_endian_to_u64(buffer_len);
	if (err < 0) {
		return -1;
	}
	*buf = xmalloc(*size);
	err |= readn(state.fd, *buf, *size);
	if (err < 0) {
		return -1;
	}
	return 0;
}

int
readNFiles(int n, const char *dirname)
{
	assert(dirname);
	state.last_operation = API_OP_READ_N_FILES;
	if (n < 0) {
		errno = EINVAL;
		return -1;
	}
	if (!state.connection_is_open) {
		return err_closed_connection();
	}
	int err = 0;
	/* We'll write 9 bytes: 1 operation code and 8 argument bytes. */
	err |= write_u64(state.fd, 1 + 8);
	err |= write_op(state.fd, API_OP_READ_N_FILES);
	err |= write_u64(state.fd, n);
	if (err < 0) {
		return on_io_err();
	}
	return handle_response_with_files(state.fd, dirname);
}

int
openFile(const char *pathname, int flags)
{
	return 0;
	make_simple_request(API_OP_OPEN_FILE | flags, pathname);
}

int
lockFile(const char *pathname)
{
	return make_simple_request(API_OP_LOCK_FILE, pathname);
}

int
unlockFile(const char *pathname)
{
	return make_simple_request(API_OP_UNLOCK_FILE, pathname);
}

int
closeFile(const char *pathname)
{
	return make_simple_request(API_OP_CLOSE_FILE, pathname);
}

int
removeFile(const char *pathname)
{
	return make_simple_request(API_OP_REMOVE_FILE, pathname);
}

/******* API WRITE OPERATIONS
 * These are by far the most complicated, because they must take evictions into
 * account. */

int
writeFile(const char *pathname, const char *dirname)
{
	char abs_path[PATH_MAX];
	void *buffer = NULL;
	size_t buffer_size = 0;
	int err = file_contents(pathname, abs_path, &buffer, &buffer_size);
	if (err) {
		return -1;
	}
	err |= make_request_with_two_args(
	  API_OP_WRITE_FILE, abs_path, strlen(abs_path), buffer, buffer_size);
	if (err < 0) {
		return -1;
	}
	return handle_response_with_files(state.fd, dirname);
}

int
appendToFile(const char *pathname, void *buffer, size_t buffer_size, const char *dirname)
{
	char abs_path[PATH_MAX];
	char *s = realpath(pathname, abs_path);
	if (!s) {
		return -1;
	}
	int err = make_request_with_two_args(
	  API_OP_APPEND_TO_FILE, abs_path, strlen(abs_path), buffer, buffer_size);
	if (err) {
		return -1;
	}
	return handle_response_with_files(state.fd, dirname);
}
