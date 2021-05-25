#include "deserializer.h"
#include <stdint.h>

#define LOW_BOUND_MIN_MESSAGE_SIZE_IN_BYTES 9

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
	size_t size_in_bytes_to_reach;
};

struct Deserializer *
deserializer_create(void)
{
	struct Deserializer *de = malloc(sizeof(struct Deserializer));
	if (!de) {
		return NULL;
	}
	de->buffer = malloc(sizeof(char) * LOW_BOUND_MIN_MESSAGE_SIZE_IN_BYTES);
	if (!de->buffer) {
		free(de);
		return NULL;
	}
	de->size_in_bytes = 0;
	de->size_in_bytes_to_reach = LOW_BOUND_MIN_MESSAGE_SIZE_IN_BYTES;
	return de;
}

struct Message *
deserializer_detach(struct Deserializer *de)
{
	if (de->size_in_bytes == de->size_in_bytes_to_reach) {
		struct Message *msg = malloc(sizeof(struct Message));
		if (!msg) {
			return NULL;
		}
		msg->raw = de->buffer;
		return msg;
	} else if (de->size_in_bytes >= 8) {
		size_t nominal_size = u64_from_big_endian(de->buffer);
		de->size_in_bytes_to_reach = 9 + nominal_size;
	}
}

size_t
deserializer_missing(const struct Deserializer *de)
{
	return de->size_in_bytes_to_reach - de->size_in_bytes;
}

void *
deserializer_buffer(struct Deserializer *de)
{
	return (char *)(de->buffer) + de->size_in_bytes;
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
