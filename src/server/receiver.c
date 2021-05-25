#include "receiver.h"
#include "deserializer.h"
#include <poll.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

struct Receiver
{
	int max_fd;
	unsigned active_sockets_count;
	struct pollfd *active_sockets;
	struct Deserializer **deserializers;
	void **buffers;
};

struct Receiver *
receiver_create(int socket_descriptor)
{
	struct Receiver *r = malloc(sizeof(struct Receiver));
	if (!r) {
		return NULL;
	}
	r->max_fd = socket_descriptor;
	r->active_sockets_count = 1;
	r->active_sockets = malloc(sizeof(struct pollfd));
	if (!r->active_sockets) {
		free(r);
		return NULL;
	}
	r->active_sockets[0].events = 0;
	r->active_sockets[0].fd = socket_descriptor;
	r->active_sockets[0].revents = 0;
	r->buffers = NULL;
	return r;
}

bool
receiver_has_new_connection(const struct Receiver *r)
{
	return r->active_sockets[0].revents & POLLIN;
}

int
receiver_poll(struct Receiver *receiver, struct Message **messages)
{
	/* Block until something happens. */
	int num_reads = poll(receiver->active_sockets, receiver->active_sockets_count, -1);
	if (num_reads == -1) {
		return -1;
	}
	if (receiver_has_new_connection(receiver)) {
		return 0;
		num_reads--;
	}
	size_t fd_counter = 0;
	/* We start counting from 1 because the first element contains the root
	 * socket connection and we're not interested in that. */
	for (size_t i = 1; i < receiver->active_sockets_count; i++) {
		if (receiver->active_sockets[i].revents & POLLIN) {
			fds[fd_counter++] = i;
			int connection_fd = receiver->active_sockets[i].fd;
			void *buffer = deserializer_buffer(receiver->deserializers[i]);
			size_t missing_bytes = deserializer_missing(receiver->deserializers[i]);
			read(connection_fd, buffer, missing_bytes);
			deserializer_last_message(&receiver->deserializers[i]);
		}
	}
	return 0;
}

int
receiver_next_data(struct Receiver *receiver)
{}

void
receiver_free(struct Receiver *r)
{
	if (!r) {
		return;
	}
	free(r->active_sockets);
	free(r);
}

int
receiver_add(struct Receiver *r, int fd)
{
	r->active_sockets_count++;
	r->active_sockets =
	  realloc(r->active_sockets, sizeof(struct pollfd) * r->active_sockets_count);
	r->active_sockets[r->active_sockets_count].events = 0;
	r->active_sockets[r->active_sockets_count].fd = fd;
	r->active_sockets[r->active_sockets_count].revents = 0;
	r->buffers = realloc(r->buffers, sizeof(void *) * r->active_sockets_count);
	return 0;
}
