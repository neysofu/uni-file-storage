#include "workload_queue.h"
#include "global_state.h"
#include "receiver.h"
#include "utils.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>

static struct WorkloadQueue *workload_queues = NULL;
static unsigned count = 0;

void
workload_queues_init(unsigned n)
{
	workload_queues = xmalloc(sizeof(struct WorkloadQueue) * n);
	count = n;
	for (size_t i = 0; i < count; i++) {
		int err = 0;
		err |= pthread_cond_init(&workload_queues[i].cond, NULL);
		err |= pthread_mutex_init(&workload_queues[i].mutex, NULL);
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

void
on_mutex_err(void)
{
	/* Errors would only arise in the case of race conditions of concurrency bugs
	 * (deadlocks, bad semaphore waits, etc.). They are thus non-recoverable. */
	glog_fatal("Poisoned mutex on workload queue. This is a non-recoverable error.");
	exit(EXIT_FAILURE);
}

struct Message *
workload_queue_pull(unsigned i)
{
	struct WorkloadQueue *queue = &workload_queues[i];
	assert(queue);
	while (true) {
		int err = 0;
		err = pthread_mutex_lock(&queue->mutex);
		if (err) {
			on_mutex_err();
		}
		while (!queue->next_incoming) {
			pthread_cond_wait(&queue->cond, &queue->mutex);
		}
		struct Message *msg = queue->next_incoming;
		if (msg) {
			queue->next_incoming = msg->next;
			msg->next = NULL;
		}
		err = pthread_mutex_unlock(&queue->mutex);
		if (err) {
			on_mutex_err();
		}
		return msg;
	}
}

void
workload_queue_add(struct Message *msg, unsigned i)
{
	assert(i < count);
	assert(!msg->next);
	struct WorkloadQueue *queue = &workload_queues[i];
	assert(queue);
	int err = 0;
	err = pthread_mutex_lock(&queue->mutex);
	if (err) {
		on_mutex_err();
	}
	queue->next_incoming = msg;
	struct Message *last = queue->last_incoming;
	if (last) {
		last->next = msg;
		queue->last_incoming = msg;
	} else {
		queue->next_incoming = msg;
		queue->last_incoming = msg;
	}
	err = pthread_cond_signal(&queue->cond);
	if (err) {
		on_mutex_err();
	}
	err = pthread_mutex_unlock(&queue->mutex);
	if (err) {
		on_mutex_err();
	}
}

void
workload_queues_free(void)
{
	for (size_t i = 0; i < count; i++) {
		pthread_cond_destroy(&workload_queues[i].cond);
		pthread_mutex_destroy(&workload_queues[i].mutex);
	}
	free(workload_queues);
}
