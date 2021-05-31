#define _POSIX_C_SOURCE 200809L

#include "utils.h"
#include <errno.h>
#include <time.h>
#include <unistd.h>

void
wait_msec(int msec)
{
	struct timespec interval;
	interval.tv_nsec = (msec % 1000) * 1000000;
	interval.tv_sec = msec / 1000;
	int result = 1;
	do {
		result = nanosleep(&interval, &interval);
	} while (result);
}

/** Evita letture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la lettura da fd leggo EOF
 *   \retval size se termina con successo
 */
int
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
int
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

uint64_t
big_endian_to_u64(char bytes[8])
{
	uint64_t n = 0;
	n += (uint64_t)(bytes[0]) << 56;
	n += (uint64_t)(bytes[1]) << 48;
	n += (uint64_t)(bytes[2]) << 40;
	n += (uint64_t)(bytes[3]) << 32;
	n += (uint64_t)(bytes[4]) << 24;
	n += (uint64_t)(bytes[5]) << 16;
	n += (uint64_t)(bytes[6]) << 8;
	n += (uint64_t)(bytes[7]);
	return n;
}

void
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
