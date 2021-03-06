#ifndef SOL_SERVER_DESERIALIZER
#define SOL_SERVER_DESERIALIZER

#include "serverapi.h"
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/* A deserializer for protocol messages. Implemented as a state machine under the
 * hood. */
struct Deserializer;

struct Buffer
{
	size_t size_in_bytes;
	void *raw;
};

/* Creates a new `struct Deserializer` and returns a pointer to it. */
struct Deserializer *
deserializer_create(void);

/* Frees all memory used by `deserializer`. */
void
deserializer_free(struct Deserializer *deserializer);

/* Checks that the bytes already parsed are fully valid. */
bool
deserializer_validate(const struct Deserializer *deserializer);

/* Parses the contents of `deserializer`'s internal buffer and returns a pointer
 * to a heap-allocated `struct Buffer` in case it is a full message, NULL
 * otherwise (i.e. it needs more data). `new_bytes` is the number of bytes that
 * were inserted into the internal buffer since the last call to this function.
 *
 * After a `struct Buffer` is successfully returned from this function,
 * `deserializer` is not valid anymore (freed) and pointers to it should be
 * discarded.
 * */
struct Buffer *
deserializer_detach(struct Deserializer *deserializer, size_t new_bytes);

/* Returns a low bound on the amount of bytes missing until the message might be
 * complete. */
size_t
deserializer_missing(const struct Deserializer *deserializer);

/* Returns an internal buffer from `deserializer` that you can use as a sink for
 * incoming data. It is always guaranteed to be of the size indicated by
 * `deserializer_missing`. */
void *
deserializer_buffer(struct Deserializer *deserializer);

#endif
