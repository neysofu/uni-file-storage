#include "receiver.h"
#include <poll.h>
#include <stdlib.h>

struct Receiver
{
	int max_fd;
	unsigned active_connections_count;
	struct pollfd *active_connections;
	void **buffers;
};

struct Receiver *
receiver_create(int socket_descriptor)
{
	struct Receiver *receiver = malloc(sizeof(struct Receiver));
	if (!receiver) {
		return NULL;
	}
	receiver->max_fd = socket_descriptor;
	receiver->active_connections_count = 0;
	receiver->active_connections = malloc(sizeof(struct pollfd));
	if (!receiver->active_connections) {
		free(receiver);
		return NULL;
	}
	receiver->active_connections[0].events = 0;
	receiver->active_connections[0].fd = socket_descriptor;
	receiver->active_connections[0].revents = 0;
	receiver->buffers = NULL;
	return receiver;
}

int
receiver_wait_read(struct Receiver *receiver)
{
	int fd = poll(receiver->active_connections + 1, receiver->active_connections_count, -1);
	return fd;
}

int
receiver_next_data(struct Receiver *receiver)
{}

void
receiver_free(struct Receiver *receiver)
{
	if (!receiver) {
		return;
	}
	free(receiver->active_connections);
	free(receiver);
}

int
receiver_add(struct Receiver *receiver, int fd)
{
	receiver->active_connections_count++;
	receiver->active_connections =
	  realloc(receiver->active_connections,
	          sizeof(struct pollfd) * receiver->active_connections_count);
	receiver->active_connections[receiver->active_connections_count].events = 0;
	receiver->active_connections[receiver->active_connections_count].fd = fd;
	receiver->active_connections[receiver->active_connections_count].revents = 0;
	receiver->buffers =
	  realloc(receiver->buffers, sizeof(void *) * receiver->active_connections_count);
	return 0;
}
