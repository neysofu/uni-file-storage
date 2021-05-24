#include "deserializer.h"

struct Deserializer
{
	void *buffer;
	size_t size_in_bytes;
	size_t nominal_size_in_bytes;
};

struct Deserializer *
deserializer_create(void)
{
	struct Deserializer *deserializer = malloc(sizeof(struct Deserializer));
	if (!deserializer) {
		return NULL;
	}
	deserializer->buffer = malloc(sizeof(char) * 9);
	deserializer->size_in_bytes = 0;
	deserializer->nominal_size_in_bytes = 0;
	return deserializer;
}

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

void
deserializer_free(struct Deserializer *deserializer)
{
	if (!deserializer) {
		return;
	}
	free(deserializer->buffer);
	free(deserializer);
}
