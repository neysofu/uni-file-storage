#ifndef SOL_SERVER_DESERIALIZER
#define SOL_SERVER_DESERIALIZER

#include "serverapi_actions.h"
#include <stdlib.h>

/* A deserializer for protocol messages. Implemented as a state machine under the
 * hood. */
struct Deserializer;

struct Message
{
	size_t size_in_bytes;
	void *raw;
	enum ActionType action_type;
	struct ActionArgs args;
};

/* Creates a new `struct Deserializer`. It returns NULL on allocation failure. */
struct Deserializer *
deserializer_create(void);

/* Parses the contents of `deserializer`'s internal buffer and returns a pointer
 * to a heap-allocated `struct Message` in case it is a full message, NULL
 * otherwise (i.e. it needs more data).
 *
 * After a `struct Message` is successfully returned from this function,
 * `deserializer` is not in a valid state anymore and should be immediately
 * destroyed via `deserializer_free`.
 * */
struct Message *
deserializer_detach(struct Deserializer *deserializer);

/* Returns a low bound on the amount of bytes missing until the message might be
 * complete. */
size_t
deserializer_missing(const struct Deserializer *deserializer);

/* Returns an internal buffer from `deserializer` that you can use as a sink for
 * incoming data. It is always guaranteed to be of the size dicated by
 * `deserializer_missing`. */
void *
deserializer_buffer(struct Deserializer *deserializer);

/* Frees all memory used by `deserializer`. */
void
deserializer_free(struct Deserializer *deserializer);

#endif
