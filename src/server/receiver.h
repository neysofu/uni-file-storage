#ifndef SOL_SERVER_RECEIVER
#define SOL_SERVER_RECEIVER

#include "deserializer.h"
#include "serverapi_utilities.h"
#include <stdlib.h>

/* Opaque data structure that simplifies the following actions:
 *  - reading incoming data from client connections.
 *  - automatically accepting new connections. */
struct Receiver;

/* The data type of incoming messages. */
struct Message
{
	int fd;
	struct Buffer buffer;
	struct Message *next;
};

/* Creates a new `struct Receiver` that listens for incoming connections on
 * `socket_fd`, with `num_workers` worker threads. */
struct Receiver *
receiver_create(int socket_fd, unsigned num_workers);

/* After disabling new connections via this function, `receiver_poll` will only
 * keep listening on existing connections. */
void
receiver_disable_new_connections(struct Receiver *receiver);

/* Suspends the execution of the current thread until one or more incoming
 * messages are available. After this call, `*messages` will point to a
 * heap-allocated array of all available messages. The caller has the
 * responsability to free all memory used by `messages` after use.
 *
 * New connection attempts are automatically accepted. It returns 0 on success
 * and -1 on failure. */
int
receiver_poll(struct Receiver *receiver);

/* Frees all memory and system resources used by `receiver`. */
void
receiver_free(struct Receiver *receiver);

#endif
