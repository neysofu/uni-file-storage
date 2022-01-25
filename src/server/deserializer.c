#include "deserializer.h"
#include "global_state.h"
#include "utilities.h"
#include <stdint.h>
#include <stdio.h>

#define HEADER_SIZE_IN_BYTES 16
#define HEADER_MAGIC_CODE_SIZE_IN_BYTES 8
#define HEADER_MAGIC_CODE 0x86b2f464f65e01ULL

struct Deserializer
{
	void *buffer;
	size_t size_in_bytes;
};

struct Deserializer *
deserializer_create(void)
{
	struct Deserializer *de = xmalloc(sizeof(struct Deserializer));
	de->buffer = NULL;
	de->size_in_bytes = 0;
	return de;
}

void
deserializer_free(struct Deserializer *de)
{
	if (!de) {
		return;
	}
	free(de->buffer);
	free(de);
}

struct Buffer *
deserializer_detach(struct Deserializer *de, size_t new_bytes)
{
	de->size_in_bytes += new_bytes;
	if (deserializer_missing(de) > 0) {
		return NULL;
	} else {
		struct Buffer *buf = xmalloc(sizeof(struct Buffer));
		buf->raw = de->buffer;
		buf->size_in_bytes = de->size_in_bytes;
		de->buffer = NULL;
		de->size_in_bytes = 0;
		return buf;
	}
}

bool
deserializer_validate(const struct Deserializer *de)
{
	if (de->size_in_bytes < HEADER_MAGIC_CODE_SIZE_IN_BYTES) {
		return true;
	} else {
		uint64_t magic_code = big_endian_to_u64(de->buffer);
		return magic_code == (uint64_t)HEADER_MAGIC_CODE;
	}
}

size_t
deserializer_missing(const struct Deserializer *de)
{
	if (de->size_in_bytes < HEADER_SIZE_IN_BYTES) {
		/* At least the header bytes are missing. */
		return HEADER_SIZE_IN_BYTES - de->size_in_bytes;
	} else {
		/* We have enough data to see exactly how many bytes we need */
		uint64_t length_prefix =
		  big_endian_to_u64((uint8_t *)de->buffer + HEADER_MAGIC_CODE_SIZE_IN_BYTES);
		return length_prefix + HEADER_SIZE_IN_BYTES - de->size_in_bytes;
	}
}

void *
deserializer_buffer(struct Deserializer *de)
{
	size_t missing = deserializer_missing(de);
	de->buffer = xrealloc(de->buffer, de->size_in_bytes + missing);
	return (char *)(de->buffer) + de->size_in_bytes;
}
