#include "receiver.h"
#include "deserializer.h"
#include "global_state.h"
#include "utilities.h"
#include "workload_queue.h"
#include <assert.h>
#include <errno.h>
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
	struct Receiver *r = xmalloc(sizeof(struct Receiver));
	r->max_fd = socket_descriptor;
	r->active_sockets_count = 1;
	r->num_workers = num_workers;
	r->accept_new_connections = true;
	r->active_sockets = xmalloc(sizeof(struct pollfd));
	r->active_sockets[0].events = POLLIN;
	r->active_sockets[0].fd = socket_descriptor;
	r->active_sockets[0].revents = 0;
	r->deserializers = xmalloc(sizeof(struct Deserializer *));
	r->deserializers[0] = deserializer_create();
	return r;
}

void
receiver_disable_new_connections(struct Receiver *r)
{
	assert(r);
	r->accept_new_connections = false;
}

/* Returns `true` if and only if a new readable event is detected on the main
 * socket. */
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
	glog_info("Adding a new connection to the server's pool.");
	int fd = accept(r->active_sockets[0].fd, NULL, NULL);
	/* Faulty new connection. Let's keep going and don't stop the whole server. */
	if (fd < 0) {
		glog_warn("Ignoring faulty connection (errno = %d).", errno);
		return;
	}
	r->active_sockets_count++;
	r->active_sockets =
	  xrealloc(r->active_sockets, sizeof(struct pollfd) * r->active_sockets_count);
	r->active_sockets[r->active_sockets_count - 1].fd = fd;
	r->active_sockets[r->active_sockets_count - 1].events = POLLIN;
	r->active_sockets[r->active_sockets_count - 1].revents = 0;
	r->deserializers =
	  xrealloc(r->deserializers, sizeof(struct Deserializer *) * r->active_sockets_count);
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
			close(-r->active_sockets[i].fd);
			deserializer_free(r->deserializers[i]);
			r->active_sockets[i] = r->active_sockets[last_i];
			r->deserializers[i] = r->deserializers[last_i];
			last_i--;
			r->active_sockets_count--;
		}
	}
	struct pollfd *new_active_sockets =
	  xrealloc(r->active_sockets, sizeof(struct pollfd) * r->active_sockets_count);
	struct Deserializer **new_deserializers =
	  xrealloc(r->deserializers, sizeof(struct Deserializer *) * r->active_sockets_count);
	r->active_sockets = new_active_sockets;
	r->deserializers = new_deserializers;
}

void
hand_over_buf_to_worker(struct Receiver *r,
                        void *buffer,
                        size_t size,
                        unsigned conn_id,
                        int fd)
{
	unsigned thread_i = rand() % r->num_workers;
	glog_debug("Handing over connection n.%u to worker n.%u.", conn_id, thread_i);
	struct Message *msg = xmalloc(sizeof(struct Message));
	msg->buffer.raw = buffer;
	msg->buffer.size_in_bytes = size;
	msg->fd = fd;
	msg->next = NULL;
	workload_queue_add(msg, thread_i);
}

int
receiver_poll(struct Receiver *r)
{
	assert(r);
	/* Remove all dead connections before polling. */
	receiver_cleanup(r);
	/* The first socket is not a client-to-server connection, remember! It's the
	 * main socket, which accepts new incoming connections. */
	glog_debug("New iteration in the polling loop with %zu connection(s).",
	           r->active_sockets_count - 1);
	/* Block until something happens. */
	int num_reads = poll(r->active_sockets, r->active_sockets_count, -1);
	if (num_reads < 0) {
		errno = EIO;
		return -1;
	} else if (num_reads == 0) {
		return 0;
	}
	if (receiver_has_new_connection(r)) {
		num_reads--;
		receiver_add_new_connection(r);
	}
	/* We start counting from 1 because the first element contains the root
	 * socket connection and we're not interested in that. */
	for (size_t i = 1; i < r->active_sockets_count; i++) {
		if ((r->active_sockets[i].revents & POLLIN) > 0) {
			glog_trace("Polled a relevant event on connection n.%zu.", i);
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
				glog_info("Dropping connection n.%zu due to EOF.", i);
				r->active_sockets[i].fd = -1;
			} else if (num_bytes < 0) {
				glog_warn("Dropping connection n.%zu due to socket error.", i);
				r->active_sockets[i].fd = -1;
			} else {
				glog_trace("Read %zd bytes from connection n.%zu.", num_bytes, i);
				struct Buffer *buf = deserializer_detach(r->deserializers[i], num_bytes);
				if (buf) {
					glog_debug("Got a full message of %zu bytes from connection n.%zu.",
					           buf->size_in_bytes,
					           i);
					hand_over_buf_to_worker(r, buf->raw, buf->size_in_bytes, i, fd);
					free(buf);
				}
			}
		}
		/* Clear all events. */
		r->active_sockets[i].revents = 0;
	}
	return 0;
}

void
receiver_free(struct Receiver *r)
{
	if (!r) {
		return;
	}
	for (size_t i = 0; i < r->active_sockets_count; i++) {
		close(r->active_sockets[i].fd);
	}
	free(r->active_sockets);
	for (size_t i = 0; i < r->active_sockets_count; i++) {
		deserializer_free(r->deserializers[i]);
	}
	free(r->deserializers);
	free(r);
}

int
receiver_add(struct Receiver *r, int fd)
{
	struct pollfd *new_active_sockets =
	  xrealloc(r->active_sockets, sizeof(struct pollfd) * r->active_sockets_count);
	struct Deserializer **new_deserializers =
	  xrealloc(r->deserializers, sizeof(struct Deserializer *) * r->active_sockets_count);
	struct Deserializer *deserializer = deserializer_create();
	r->active_sockets = new_active_sockets;
	r->deserializers = new_deserializers;
	r->active_sockets[r->active_sockets_count].events = POLLIN;
	r->active_sockets[r->active_sockets_count].fd = fd;
	r->active_sockets[r->active_sockets_count].revents = 0;
	r->deserializers[r->active_sockets_count] = deserializer;
	r->active_sockets_count++;
	return 0;
}
