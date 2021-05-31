#include "receiver.h"
#include "deserializer.h"
#include "logc/src/log.h"
#include "workload_queue.h"
#include <assert.h>
#include <poll.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

struct Receiver
{
	unsigned num_workers;
	int max_fd;
	unsigned active_sockets_count;
	struct pollfd *active_sockets;
	struct Deserializer **deserializers;
	bool accept_new_connections;
};

struct Receiver *
receiver_create(int socket_descriptor, unsigned num_workers)
{
	struct Receiver *r = malloc(sizeof(struct Receiver));
	if (!r) {
		return NULL;
	}
	r->max_fd = socket_descriptor;
	r->active_sockets_count = 1;
	r->num_workers = num_workers;
	r->accept_new_connections = true;
	r->active_sockets = malloc(sizeof(struct pollfd));
	if (!r->active_sockets) {
		free(r);
		return NULL;
	}
	r->active_sockets[0].events = POLLIN;
	r->active_sockets[0].fd = socket_descriptor;
	r->active_sockets[0].revents = 0;
	r->deserializers = malloc(sizeof(struct Deserializer *));
	if (!r->deserializers) {
		free(r->active_sockets);
		free(r);
		return NULL;
	}
	r->deserializers[0] = deserializer_create();
	if (!r->deserializers[0]) {
		free(r->deserializers);
		free(r->active_sockets);
		free(r);
		return NULL;
	}
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
	return (r->active_sockets[0].revents & POLLIN) > 0;
}

void
receiver_add_new_connection(struct Receiver *r)
{
	assert(r);
	log_info("Adding a new connection to the server's pool.");
	int fd = accept(r->active_sockets[0].fd, NULL, NULL);
	/* Faulty new connection. Let's keep going and don't stop the whole server. */
	if (fd < 0) {
		log_info("Ignoring faulty connection.");
		return;
	}
	r->active_sockets_count++;
	r->active_sockets =
	  realloc(r->active_sockets, sizeof(struct pollfd) * r->active_sockets_count);
	r->active_sockets[r->active_sockets_count - 1].fd = fd;
	r->active_sockets[r->active_sockets_count - 1].events = POLLIN;
	r->active_sockets[r->active_sockets_count - 1].revents = 0;
	r->deserializers =
	  realloc(r->deserializers, sizeof(struct Deserializer *) * r->active_sockets_count);
	r->deserializers[r->active_sockets_count - 1] = deserializer_create();
	/* Clear state. */
	read(fd, NULL, 0);
	r->active_sockets[0].revents = 0;
}

/* Removes dead connetions from this `struct Receiver`. */
void
receiver_cleanup(struct Receiver *r)
{
	size_t last_i = r->active_sockets_count - 1;
	for (size_t i = 1; i <= last_i; i++) {
		if (r->active_sockets[i].fd < 0) {
			r->active_sockets[i] = r->active_sockets[last_i];
			r->deserializers[i] = r->deserializers[last_i];
			last_i--;
			r->active_sockets_count--;
		}
	}
	struct pollfd *new_active_sockets =
	  realloc(r->active_sockets, sizeof(struct pollfd) * r->active_sockets_count);
	struct Deserializer **new_deserializers =
	  realloc(r->deserializers, sizeof(struct Deserializer *) * r->active_sockets_count);
	r->active_sockets = new_active_sockets;
	r->deserializers = new_deserializers;
}

void
hand_over_buf_to_worker(struct Receiver *r, struct Buffer *buf)
{
	unsigned thread_i = rand() % r->num_workers;
	log_debug("Handing over message to worker n. %u/%u.", thread_i + 1, r->num_workers);
	r->deserializers[thread_i] = deserializer_create();
	struct WorkloadQueue *queue = workload_queue_get(thread_i);
	assert(queue);
	workload_queue_add(buf, thread_i);
	sem_post(&queue->sem);
}

struct Message *
receiver_poll(struct Receiver *r)
{
	receiver_cleanup(r);
	/* The first socket is not an active connection, remember. */
	log_debug("New iteration in the polling loop with %zu connection(s).",
	          r->active_sockets_count - 1);
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
	struct Message *head = NULL;
	/* We start counting from 1 because the first element contains the root
	 * socket connection and we're not interested in that. */
	for (size_t i = 1; i < r->active_sockets_count; i++) {
		if ((r->active_sockets[i].revents & POLLIN) > 0) {
			int fd = r->active_sockets[i].fd;
			void *buffer = deserializer_buffer(r->deserializers[i]);
			size_t missing_bytes = deserializer_missing(r->deserializers[i]);
			/* Incomplete messages always need a positive number of bytes! */
			assert(missing_bytes > 0);
			ssize_t num_bytes = read(fd, buffer, missing_bytes);
			/* We drop connections on two situations:
			 *  - EOF.
			 *  - Errors during read. */
			if (num_bytes == 0) {
				log_info("Dropping connection n. %zu due to EOF.", i);
				r->active_sockets[i].fd = -1;
			} else if (num_bytes < 0) {
				log_warn("Dropping connection n. %zu due to socket error.", i);
				r->active_sockets[i].fd = -1;
			} else {
				log_trace("Read %zd bytes from connection n. %zu.", num_bytes, i);
				struct Buffer *buf = deserializer_detach(r->deserializers[i], num_bytes);
				if (buf) {
					log_debug("Got a full message of %zu bytes from connection n. %zu.",
					          buf->size_in_bytes,
					          i);
					hand_over_buf_to_worker(r, buf);
				}
			}
		}
		/* Clear all events. */
		r->active_sockets[i].revents = 0;
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
