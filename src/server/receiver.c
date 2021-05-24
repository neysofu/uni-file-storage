#include "receiver.h"
#include "deserializer.h"
#include <poll.h>
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
	struct Receiver *receiver = malloc(sizeof(struct Receiver));
	if (!receiver) {
		return NULL;
	}
	receiver->max_fd = socket_descriptor;
	receiver->active_sockets_count = 1;
	receiver->active_sockets = malloc(sizeof(struct pollfd));
	if (!receiver->active_sockets) {
		free(receiver);
		return NULL;
	}
	receiver->active_sockets[0].events = 0;
	receiver->active_sockets[0].fd = socket_descriptor;
	receiver->active_sockets[0].revents = 0;
	receiver->buffers = NULL;
	return receiver;
}

int
receiver_poll(struct Receiver *receiver, int **fds)
{
	int fds_count = poll(receiver->active_sockets, receiver->active_sockets_count, -1);
	if (fds_count == -1) {
		return -1;
	}
	if (receiver->active_sockets[0].revents & POLLIN) {
		return 0;
	}
	*fds = malloc(sizeof(int) * fds_count);
	if (*fds) {
		return -1;
	}
	size_t fd_counter = 0;
	for (size_t i = 1; i < receiver->active_sockets_count; i++) {
		if (receiver->active_sockets[i].revents & POLLIN) {
			fds[fd_counter++] = i;
			int connection_fd = receiver->active_sockets[i].fd;
			void *buffer = deserializer_buffer(receiver->deserializers[i]);
			size_t missing_bytes = deserializer_missing(receiver->deserializers[i]);
			read(connection_fd, buffer, missing_bytes);
		}
	}
	return 0;
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
	free(receiver->active_sockets);
	free(receiver);
}

int
receiver_add(struct Receiver *receiver, int fd)
{
	receiver->active_sockets_count++;
	receiver->active_sockets = realloc(
	  receiver->active_sockets, sizeof(struct pollfd) * receiver->active_sockets_count);
	receiver->active_sockets[receiver->active_sockets_count].events = 0;
	receiver->active_sockets[receiver->active_sockets_count].fd = fd;
	receiver->active_sockets[receiver->active_sockets_count].revents = 0;
	receiver->buffers =
	  realloc(receiver->buffers, sizeof(void *) * receiver->active_sockets_count);
	return 0;
}
