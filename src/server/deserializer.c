#include "deserializer.h"

struct Deserializer
{
	void *buffer;
	size_t size_in_bytes;
	size_t nominal_size_in_bytes;
};

size_t
deserializer_missing(const struct Deserializer *deserializer)
{
	return deserializer->nominal_size_in_bytes - deserializer->size_in_bytes;
}

void *
deserializer_buffer(struct Deserializer *deserializer)
{
	return (char *)(deserializer->buffer) + deserializer->size_in_bytes;
}
