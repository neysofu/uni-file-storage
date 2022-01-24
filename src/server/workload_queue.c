#include "workload_queue.h"
#include "global_state.h"
#include "receiver.h"
#include "server_utilities.h"
#include "utilities.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>

static struct WorkloadQueue *workload_queues = NULL;
static unsigned count = 0;

#define ON_MUTEX_ERR(err)                                                                  \
	ON_ERR((err),                                                                          \
	       "Mutex error during workload queue manipulation. This is most likely a bug.")

void
workload_queues_init(unsigned n)
{
	workload_queues = xmalloc(sizeof(struct WorkloadQueue) * n);
	count = n;
	for (size_t i = 0; i < count; i++) {
		int err = 0;
		ON_MUTEX_ERR(pthread_cond_init(&workload_queues[i].cond, NULL));
		ON_MUTEX_ERR(pthread_mutex_init(&workload_queues[i].mutex, NULL));
		/* We're not supposed to have any error whatsover here. If we do, that's
		 * a bug within the initialization logic and/or workers are already
		 * reading data. */
		assert(err == 0);
		workload_queues[i].next_incoming = NULL;
		workload_queues[i].last_incoming = NULL;
	}
}

struct WorkloadQueue *
workload_queue_get(unsigned i)
{
	return &workload_queues[i];
}

struct Message *
workload_queue_pull(unsigned i)
{
	struct WorkloadQueue *queue = &workload_queues[i];
	assert(queue);

	ON_MUTEX_ERR(pthread_mutex_lock(&queue->mutex));
	while (!queue->next_incoming) {
		glog_trace("[Worker n.%u] Waiting for a message.", i);
		if (detect_shutdown_hard()) {
			glog_debug(
			  "[Worker n.%u] Shutdown detected while waiting for workload queue contents.",
			  i);
			break;
		}
		ON_MUTEX_ERR(pthread_cond_wait(&queue->cond, &queue->mutex));
	}
	struct Message *msg = queue->next_incoming;
	if (msg) {
		queue->next_incoming = msg->next;
		msg->next = NULL;
	}
	ON_MUTEX_ERR(pthread_mutex_unlock(&queue->mutex));
	return msg;
}

void
workload_queue_add(struct Message *msg, unsigned i)
{
	assert(i < count);
	assert(!msg->next);
	struct WorkloadQueue *queue = &workload_queues[i];
	assert(queue);
	ON_MUTEX_ERR(pthread_mutex_lock(&queue->mutex));
	queue->next_incoming = msg;
	struct Message *last = queue->last_incoming;
	if (last) {
		last->next = msg;
		queue->last_incoming = msg;
	} else {
		queue->next_incoming = msg;
		queue->last_incoming = msg;
	}
	ON_MUTEX_ERR(pthread_cond_signal(&queue->cond));
	ON_MUTEX_ERR(pthread_mutex_unlock(&queue->mutex));
}

void
workload_queues_cond_signal(void)
{
	for (size_t i = 0; i < count; i++) {
		struct WorkloadQueue *queue = &workload_queues[i];
		ON_MUTEX_ERR(pthread_mutex_lock(&queue->mutex));
		ON_MUTEX_ERR(pthread_cond_signal(&queue->cond));
		ON_MUTEX_ERR(pthread_mutex_unlock(&queue->mutex));
	}
}

void
workload_queues_free(void)
{
	for (size_t i = 0; i < count; i++) {
		ON_MUTEX_ERR(pthread_cond_destroy(&workload_queues[i].cond));
		ON_MUTEX_ERR(pthread_mutex_destroy(&workload_queues[i].mutex));
		workload_queue_free(&workload_queues[i]);
	}
	free(workload_queues);
}

void
workload_queue_free(struct WorkloadQueue *queue)
{
	while (queue->next_incoming) {
		struct Message *msg = queue->next_incoming;
		queue->next_incoming = msg->next;
		free(msg->buffer.raw);
		free(msg);
	}
}
