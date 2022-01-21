#include "deserializer.h"
#include "global_state.h"
#include "utilities.h"
#include <stdint.h>
#include <stdio.h>

#define HEADER_SIZE_IN_BYTES 8

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

size_t
deserializer_missing(const struct Deserializer *de)
{
	if (de->size_in_bytes < HEADER_SIZE_IN_BYTES) {
		/* At least the header bytes are missing. */
		return HEADER_SIZE_IN_BYTES - de->size_in_bytes;
	} else {
		/* We have enough data to see exactly how many bytes we need */
		uint64_t length_prefix = big_endian_to_u64(de->buffer);
		return length_prefix + HEADER_SIZE_IN_BYTES - de->size_in_bytes;
	}
}

void *
deserializer_buffer(struct Deserializer *de)
{
	size_t missing = deserializer_missing(de);
	glog_trace("The deserializer at %p needs %zu more bytes.", de, missing);
	de->buffer = xrealloc(de->buffer, de->size_in_bytes + missing);
	return (char *)(de->buffer) + de->size_in_bytes;
}
