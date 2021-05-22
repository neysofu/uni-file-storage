#define _POSIX_C_SOURCE 200809L

#include "serverapi.h"
#include "utils.h"
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define SOCKNAME "./cs_sock"
#define MAXBACKLOG 32

struct ConnectionState
{
	bool connection_is_open;
	int fd;
	char *socket_name;
};

struct ConnectionState state = (struct ConnectionState){
	.connection_is_open = false,
	.fd = -1,
	.socket_name = NULL,
};

/** Evita letture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la lettura da fd leggo EOF
 *   \retval size se termina con successo
 */
static inline int
readn(long fd, void *buf, size_t size)
{
	size_t left = size;
	int r;
	char *bufptr = (char *)buf;
	while (left > 0) {
		if ((r = read((int)fd, bufptr, left)) == -1) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (r == 0)
			return 0; // EOF
		left -= r;
		bufptr += r;
	}
	return size;
}

/** Evita scritture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la scrittura la write ritorna 0
 *   \retval  1   se la scrittura termina con successo
 */
static inline int
writen(long fd, void *buf, size_t size)
{
	size_t left = size;
	int r;
	char *bufptr = (char *)buf;
	while (left > 0) {
		if ((r = write((int)fd, bufptr, left)) == -1) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (r == 0)
			return 0;
		left -= r;
		bufptr += r;
	}
	return 1;
}

enum Operation
{
	OP_OPEN_FILE,
	OP_READ_FILE,
	OP_READ_N_FILES,
	OP_WRITE_FILE,
	OP_APPEND_TO_FILE,
	OP_LOCK_FILE,
	OP_UNLOCK_FILE,
	OP_CLOSE_FILE,
	OP_REMOVE_FILE,
};

void
write_op(int fd, enum Operation op)
{
	char op_byte = op;
	write(fd, &op_byte, 1);
}

static void
u64_to_big_endian(uint64_t data, char bytes[8])
{

	bytes[0] = data >> 56;
	bytes[1] = data >> 48;
	bytes[2] = data >> 40;
	bytes[3] = data >> 32;
	bytes[4] = data >> 24;
	bytes[5] = data >> 16;
	bytes[6] = data >> 8;
	bytes[7] = data;
}

void
write_u64_big_endian(int fd, uint64_t data)
{
	char bytes[8];
	u64_to_big_endian(data, bytes);
	write(fd, bytes, 8);
}

void
write_string(int fd, const char *data)
{
	write_u64_big_endian(fd, strlen(data));
	writen(fd, data, strlen(data));
}

static int
write_op_with_string(int fd, enum Operation op, const char *operand)
{
	write_op(fd, op);
	write_string(fd, operand);
}

int
openConnection(const char *sockname, int msec, const struct timespec abstime)
{
	if (state.connection_is_open) {
		return -1;
	}
	while (1) {
		int socket_descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
		state.fd = socket_descriptor;
		struct sockaddr_un socket_address;
		memset(&socket_address, 0, sizeof(socket_address));
		socket_address.sun_family = AF_UNIX;
		state.socket_name = malloc(strlen(sockname) + 1);
		strcpy(state.socket_name, sockname);
		strcpy(socket_address.sun_path, sockname);
		bind(socket_descriptor,
		     &socket_address,
		     ((struct sockaddr_un *)(NULL))->sun_path + strlen(socket_address.sun_path));
		listen(socket_descriptor, 1);
		wait_msec(msec);
	}
	return 0;
}

int
closeConnection(const char *sockname)
{
	if (!state.connection_is_open) {
		errno = EPERM;
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
openFile(const char *pathname, int flags)
{
	if (!state.connection_is_open) {
		return -1;
	}
	write_op_with_string(state.fd, OP_OPEN_FILE, pathname);
	return 0;
}

int
readFile(const char *pathname, void **buf, size_t *size)
{
	if (!state.connection_is_open) {
		return -1;
	}
	write_op_with_string(state.fd, OP_READ_FILE, pathname);
	return 0;
}

int
readNFiles(int n, const char *dirname)
{
	assert(n >= 0);
	if (!state.connection_is_open) {
		return -1;
	}
	write_op(state.fd, OP_READ_N_FILES);
	write_string(state.fd, dirname);
	write_u64_big_endian(state.fd, n);
	return 0;
}

int
writeFile(const char *pathname, const char *dirname)
{
	if (!state.connection_is_open) {
		return -1;
	}
	write_op(state.fd, OP_WRITE_FILE);
	write_string(state.fd, pathname);
	write_string(state.fd, dirname);
	return 0;
}

int
appendToFile(const char *pathname, void *buf, size_t size, const char *dirname)
{
	if (!state.connection_is_open) {
		return -1;
	}
	write_op(state.fd, OP_APPEND_TO_FILE);
	write_string(state.fd, pathname);
	write_string(state.fd, dirname);
	return 0;
}

int
lockFile(const char *pathname)
{
	if (!state.connection_is_open) {
		return -1;
	}
	write_op(state.fd, OP_LOCK_FILE);
	write_string(state.fd, pathname);
	return 0;
}

int
unlockFile(const char *pathname)
{
	if (!state.connection_is_open) {
		return -1;
	}
	write_op(state.fd, OP_UNLOCK_FILE);
	write_string(state.fd, pathname);
	return 0;
}

int
closeFile(const char *pathname)
{
	if (!state.connection_is_open) {
		return -1;
	}
	write_op(state.fd, OP_CLOSE_FILE);
	write_string(state.fd, pathname);
	return 0;
}

int
removeFile(const char *pathname)
{
	if (!state.connection_is_open) {
		return -1;
	}
	write_op(state.fd, OP_REMOVE_FILE);
	write_string(state.fd, pathname);
	return 0;
}
