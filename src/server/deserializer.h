#ifndef SOL_SERVER_PARSER
#define SOL_SERVER_PARSER

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

struct Deserializer *
deserializer_create(void);

void
deserializer_clean(struct Deserializer *deserializer);

int
deserializer_last_message(struct Deserializer *deserializer);

size_t
deserializer_missing(const struct Deserializer *deserializer);

void *
deserializer_buffer(struct Deserializer *deserializer);

void
deserializer_free(struct Deserializer *deserializer);

#endif
