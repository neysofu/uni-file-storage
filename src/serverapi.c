/* glibc treats `timespec` and `realpath` as GNU extensions, not POSIX. */
#define _GNU_SOURCE

#include "serverapi.h"
#include "logc/src/log.h"
#include "utilities.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
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
file_contents(const char *filepath, void **buffer, size_t *size)
{
	assert(filepath);
	assert(buffer);
	assert(size);

	log_debug("Opening the file '%s'...", filepath);
	FILE *f = NULL;
	f = fopen(filepath, "rb");
	if (!f) {
		log_error("Couldn't open the given file.");
		errno = EIO;
		return -1;
	}
	fseek(f, 0L, SEEK_END);
	*size = ftell(f);
	*buffer = xmalloc(*size);
	fseek(f, 0, SEEK_SET);
	fread(*buffer, 1, *size, f);
	fclose(f);
	log_debug("Done reading the file '%s'.", filepath);
	return 0;
}

static int
on_io_err(void)
{
	log_error("Tried to read/write data, but there's been some I/O error. errno = %d",
	          errno);
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
	return write_bytes(fd, bytes, 8);
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
	char *pathname = buf_to_str(original_pathname, original_pathname_size);
	const char *filename = basename(pathname);
	char *path = xmalloc(strlen(dirname) + 1 + strlen(filename) + 1);
	strcpy(path, dirname);
	strcpy(path + strlen(dirname), "/");
	strcpy(path + strlen(dirname) + 1, filename);

	FILE *f = fopen(path, "wb");
	if (!f) {
		log_trace("Can't open '%s'.", path);
		return -1;
	}
	size_t num_bytes_written = fwrite(contents, 1, contents_size, f);
	if (num_bytes_written == 0) {
		log_trace("Can't file bytes to '%s'.", path);
		return -1;
	}
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
	err |= read_bytes(fd, buffer_response_code, 1);
	if (err < 0 || buffer_response_code[0] == RESPONSE_ERR) {
		return -1;
	}
	err |= read_bytes(fd, buffer_num_files, 8);
	if (err < 0) {
		return -1;
	}
	size_t num_files = big_endian_to_u64(buffer_num_files);
	/* Read actual file contents. */
	for (uint64_t i = 0; i < num_files; i++) {
		uint8_t buffer_lengths[16] = { '\0' };
		err |= read_bytes(state.fd, buffer_lengths, 16);
		if (err < 0) {
			return on_io_err();
		}
		size_t len_path = big_endian_to_u64(buffer_lengths);
		size_t len_contents = big_endian_to_u64(buffer_lengths + 8);
		void *buffer = xmalloc(len_path + len_contents);
		err |= read_bytes(state.fd, buffer, len_path + len_contents);
		if (!err) {
			free(buffer);
			return on_io_err();
		}
		err |= write_file_to_dir(
		  dirname, buffer, len_path, (char *)buffer + len_path, len_contents);
		if (err < 0) {
			free(buffer);
			return -1;
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
make_simple_request(enum ApiOp op, const char *pathname, int err_response_meaning)
{
	assert(pathname);

	state.last_operation = op;
	if (!state.connection_is_open) {
		return err_closed_connection();
	}
	int err = 0;
	err |= write_u64(state.fd, 1 + strlen(pathname));
	err |= write_op(state.fd, op);
	err |= write_bytes(state.fd, (void *)pathname, strlen(pathname));
	if (err < 0) {
		return on_io_err();
	}
	char buffer[1] = { '\0' };
	err |= read(state.fd, buffer, 1);
	if (err < 0) {
		return on_io_err();
	} else if (buffer[0] == RESPONSE_ERR) {
		log_error("Received a negative response from the server.");
		errno = err_response_meaning;
		return -1;
	}
	return 0;
}

/* A request type with support for two variable-length arguments:
 * - 8 byte header (length prefix).
 * - 1 byte operation code.
 * - 8 bytes, length N of 1st argument.
 * - 8 bytes, length M of 2nd argument.
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
	/* Request: */
	size_t message_size_in_bytes = 1 + 8 + 8 + arg1_size + arg2_size;
	int err = 0;
	err |= write_u64(state.fd, message_size_in_bytes);
	err |= write_op(state.fd, op);
	err |= write_u64(state.fd, arg1_size);
	err |= write_u64(state.fd, arg2_size);
	err |= write_bytes(state.fd, arg1, arg1_size);
	err |= write_bytes(state.fd, arg2, arg2_size);
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
		if (err) {
			log_warn("Attempt failed.");
			wait_msec(msec);
		} else {
			log_info("Attempt succeeded.");
			break;
		}
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
	int err = make_simple_request(API_OP_READ_FILE, pathname, ESTALE);
	if (err < 0) {
		return -1;
	}
	/* After the "simple" request, `readFile` also must read the file contents. */
	uint8_t buffer_len[8] = { 0 };
	err |= read_bytes(state.fd, buffer_len, 8);
	if (err < 0) {
		return on_io_err();
	}
	*size = big_endian_to_u64(buffer_len);
	*buf = xmalloc(*size);
	err |= read_bytes(state.fd, *buf, *size);
	if (err < 0) {
		return on_io_err();
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

	// FIXME: can't remember what this is supposed to be doing...

	/* Read header. */
	uint8_t buffer_response_code[1] = { '\0' };
	uint8_t buffer_num_files[8] = { '\0' };
	err |= read_bytes(state.fd, buffer_response_code, 1);
	if (err < 0 || buffer_response_code[0] == RESPONSE_ERR) {
		return -1;
	}
	err |= read_bytes(state.fd, buffer_num_files, 8);
	if (err < 0) {
		return -1;
	}
	size_t num_files = big_endian_to_u64(buffer_num_files);
	/* Read actual file contents. */
	for (uint64_t i = 0; i < num_files; i++) {
		uint8_t buffer_lengths[16] = { '\0' };
		err |= read_bytes(state.fd, buffer_lengths, 16);
		if (err < 0) {
			return on_io_err();
		}
		size_t len_path = big_endian_to_u64(buffer_lengths);
		size_t len_contents = big_endian_to_u64(buffer_lengths + 8);
		void *buffer = xmalloc(len_path + len_contents);
		err |= read_bytes(state.fd, buffer, len_path + len_contents);
		if (!err) {
			free(buffer);
			return on_io_err();
		}
		err |= write_file_to_dir(
		  dirname, buffer, len_path, (char *)buffer + len_path, len_contents);
		if (err < 0) {
			free(buffer);
			return -1;
		}
		free(buffer);
	}

	return 0;
}

int
openFile(const char *pathname, int flags)
{
	switch (flags) {
		case O_CREATE:
			return make_simple_request(API_OP_OPEN_FILE_CREATE, pathname, EINVAL);
		case O_LOCK:
			return make_simple_request(API_OP_OPEN_FILE_LOCK, pathname, EINVAL);
		case O_CREATE | O_LOCK:
			return make_simple_request(API_OP_OPEN_FILE_CREATE_LOCK, pathname, EINVAL);
		case 0:
			return make_simple_request(API_OP_OPEN_FILE, pathname, EINVAL);
		default:
			errno = EINVAL;
			return -1;
	}
}

int
lockFile(const char *pathname)
{
	return make_simple_request(API_OP_LOCK_FILE, pathname, EINVAL);
}

int
unlockFile(const char *pathname)
{
	return make_simple_request(API_OP_UNLOCK_FILE, pathname, EINVAL);
}

int
closeFile(const char *pathname)
{
	return make_simple_request(API_OP_CLOSE_FILE, pathname, EINVAL);
}

int
removeFile(const char *pathname)
{
	return make_simple_request(API_OP_REMOVE_FILE, pathname, EINVAL);
}

/******* API WRITE OPERATIONS
 * These are by far the most complicated, because they must take evictions into
 * account. */

int
writeFile(const char *filepath, const char *dirname)
{
	void *buffer = NULL;
	size_t buffer_size = 0;
	int err = 0;

	err = file_contents(filepath, &buffer, &buffer_size);
	if (err) {
		log_error("Can't get the file contents of '%s'.", filepath);
		return -1;
	}

	log_trace("`writeFile` on '%s', %llu bytes long", filepath, buffer_size);

	err = make_request_with_two_args(
	  API_OP_WRITE_FILE, filepath, strlen(filepath), buffer, buffer_size);
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
