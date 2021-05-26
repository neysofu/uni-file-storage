#ifndef SOL_SERVER_DESERIALIZER
#define SOL_SERVER_DESERIALIZER

#include "serverapi_actions.h"
#include <stdlib.h>

struct Deserializer;

struct Message
{
	size_t size_in_bytes;
	void *raw;
	enum ActionType action_type;
	struct ActionArgs args;
};

/* Allocates a new `struct Deserializer` on the heap with an empty state. Returns
 * NULL on allocation failure. */
struct Deserializer *
deserializer_create(void);

int
deserializer_last_message(struct Deserializer *deserializer);

/* Returns a low bound on the number of missing bytes. */
size_t
deserializer_missing(const struct Deserializer *deserializer);

/* Returns a pointer to the next, uninitializer portion of its internal buffer. */
void *
deserializer_buffer(struct Deserializer *deserializer);

/* Frees all memory and the internal buffer of `deserializer`. */
void
deserializer_free(struct Deserializer *deserializer);

#endif
