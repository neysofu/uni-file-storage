#include "receiver.h"
#include "deserializer.h"
#include "logc/src/log.h"
#include <assert.h>
#include <poll.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

struct Receiver
{
	int max_fd;
	unsigned active_sockets_count;
	struct pollfd *active_sockets;
	struct Deserializer **deserializers;
	bool accept_new_connections;
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
	r->accept_new_connections = true;
	r->active_sockets = malloc(sizeof(struct pollfd));
	if (!r->active_sockets) {
		free(r);
		return NULL;
	}
	r->active_sockets[0].events = 0;
	r->active_sockets[0].fd = socket_descriptor;
	r->active_sockets[0].revents = 0;
	return r;
}

void
receiver_disable_new_connections(struct Receiver *r)
{
	assert(r);
	r->accept_new_connections = false;
}

bool
receiver_has_new_connection(const struct Receiver *r)
{
	assert(r);
	return r->active_sockets[0].revents & POLLIN;
}

void
receiver_add_new_connection(struct Receiver *r)
{
	assert(r);
	int fd = accept(r->active_sockets[0].fd, NULL, NULL);
	/* Faulty new connection. Let's keep going and don't stop the whole server. */
	if (fd < 0) {
		return;
	}
	r->active_sockets_count++;
	r->active_sockets =
	  realloc(r->active_sockets, sizeof(struct pollfd) * r->active_sockets_count);
	r->active_sockets[r->active_sockets_count - 1].fd = fd;
	r->active_sockets[r->active_sockets_count - 1].events = POLLIN;
	r->active_sockets[r->active_sockets_count - 1].revents = 0;
}

struct Message *
receiver_poll(struct Receiver *r)
{
	assert(r);
	/* Block until something happens. */
	int num_reads = poll(r->active_sockets, r->active_sockets_count, -1);
	if (num_reads < 0) {
		return NULL;
	}
	if (receiver_has_new_connection(r)) {
		num_reads--;
		receiver_add_new_connection(r);
	}
	size_t fd_counter = 0;
	struct Message *head = NULL;
	struct Message *last = NULL;
	/* We start counting from 1 because the first element contains the root
	 * socket connection and we're not interested in that. */
	for (size_t i = 1; i < r->active_sockets_count; i++) {
		if (r->active_sockets[i].revents & POLLIN) {
			int fd = r->active_sockets[i].fd;
			void *buffer = deserializer_buffer(r->deserializers[i]);
			size_t missing_bytes = deserializer_missing(r->deserializers[i]);
			size_t num_bytes = read(fd, buffer, missing_bytes);
			struct Buffer *buf = deserializer_detach(r->deserializers[i], num_bytes);
			// TODO
		}
	}
	return head;
}

void
receiver_free(struct Receiver *r)
{
	if (!r) {
		return;
	}
	for (size_t i = 1; i < r->active_sockets_count; i++) {
		close(r->active_sockets[i].fd);
	}
	close(r->active_sockets[0].fd);
	free(r->active_sockets);
	free(r);
}

int
receiver_add(struct Receiver *r, int fd)
{
	struct pollfd *new_active_sockets =
	  realloc(r->active_sockets, sizeof(struct pollfd) * r->active_sockets_count);
	struct Deserializer **new_deserializers =
	  realloc(r->deserializers, sizeof(struct Deserializer *) * r->active_sockets_count);
	struct Deserializer *deserializer = deserializer_create();
	if (!new_active_sockets || !new_deserializers || !deserializer) {
		free(new_active_sockets);
		free(new_deserializers);
		free(deserializer);
		return -1;
	}
	r->active_sockets = new_active_sockets;
	r->deserializers = new_deserializers;
	r->active_sockets[r->active_sockets_count].events = POLLIN;
	r->active_sockets[r->active_sockets_count].fd = fd;
	r->active_sockets[r->active_sockets_count].revents = 0;
	r->deserializers[r->active_sockets_count] = deserializer;
	r->active_sockets_count++;
	return 0;
}
