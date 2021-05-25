#ifndef SOL_SERVER_RECEIVER
#define SOL_SERVER_RECEIVER

#include "serverapi_actions.h"
#include <stdlib.h>

/* Opaque data structure that simplifies the following:
 *  - reading incoming data from client connections; and
 *  - automatically accepting new connections.
 */
struct Receiver;

/* The data type of messages as processed by `struct Receiver`. */
struct Message
{
	int fd;
	size_t size_in_bytes;
	void *raw;
	enum ActionType action_type;
	struct ActionArgs args;
};

/* Creates a new `struct Receiver` that listens for incoming connections on
 * `socket_fd`. */
struct Receiver *
receiver_create(int socket_fd);

/* Suspends the execution of the current thread until one or more incoming
 * messages are available. After this call, `*messages` will point to a
 * heap-allocated array of all available messages. The caller has the
 * responsability to free all memory used by `messages` after use.
 *
 * New connection attempts are automatically accepted. It returns 0 on success
 * and -1 on failure. */
int
receiver_poll(struct Receiver *receiver, struct Message **messages);

/* Frees all memory and system resources used by `receiver`. */
void
receiver_free(struct Receiver *receiver);

#endif
