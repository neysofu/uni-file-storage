#include "deserializer.h"
#include <stdint.h>

#define HEADER_SIZE_IN_BYTES 8

uint64_t
u64_from_big_endian(void *bytes)
{
	uint8_t *data = bytes;
	uint64_t n = 0;
	n += data[0] << 56;
	n += data[1] << 48;
	n += data[2] << 40;
	n += data[3] << 32;
	n += data[4] << 24;
	n += data[5] << 16;
	n += data[6] << 8;
	n += data[7] << 0;
	return n;
}

struct Deserializer
{
	void *buffer;
	size_t size_in_bytes;
};

struct Deserializer *
deserializer_create(void)
{
	struct Deserializer *de = malloc(sizeof(struct Deserializer));
	if (!de) {
		return NULL;
	}
	de->buffer = malloc(sizeof(char) * HEADER_SIZE_IN_BYTES);
	if (!de->buffer) {
		free(de);
		return NULL;
	}
	de->size_in_bytes = 0;
	return de;
}

struct Buffer *
deserializer_detach(struct Deserializer *de, size_t new_bytes)
{
	de->size_in_bytes += new_bytes;
	if (de->size_in_bytes < HEADER_SIZE_IN_BYTES) {
		return NULL;
	}
	uint64_t length_prefix = u64_from_big_endian(de->buffer);
	if (de->size_in_bytes == length_prefix) {
		struct Buffer *buf = malloc(sizeof(struct Buffer));
		if (!buf) {
			return NULL;
		}
		buf->raw = de->buffer;
		buf->size_in_bytes = de->size_in_bytes;
		free(de);
		return buf;
	} else {
		return NULL;
	}
}

size_t
deserializer_missing(const struct Deserializer *de)
{
	if (de->size_in_bytes < HEADER_SIZE_IN_BYTES) {
		return HEADER_SIZE_IN_BYTES - de->size_in_bytes;
	} else {
		uint64_t length_prefix = u64_from_big_endian(de->buffer);
		return length_prefix - de->size_in_bytes - HEADER_SIZE_IN_BYTES;
	}
}

void *
deserializer_buffer(struct Deserializer *de)
{
	return (char *)(de->buffer) + de->size_in_bytes;
}
