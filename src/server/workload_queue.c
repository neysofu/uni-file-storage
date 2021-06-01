#include "workload_queue.h"
#include "global_state.h"
#include "receiver.h"
#include "utils.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

static struct WorkloadQueue *workload_queues = NULL;
static unsigned count = 0;

void
workload_queues_init(unsigned n)
{
	workload_queues = xmalloc(sizeof(struct WorkloadQueue) * n);
	int err = 0;
	count = n;
	for (size_t i = 0; i < count; i++) {
		err |= sem_init(&workload_queues[i].sem, 0, 0);
		err |= pthread_mutex_init(&workload_queues[i].guard, NULL);
		/* We're not supposed to have any error whatsover here. If we do, that's
		 * a bug within the initialization logic. */
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
	glog_fatal("Poisoned mutex on workload queue. This is a non-recoverable error.");
	exit(EXIT_FAILURE);
}

struct Message *
workload_queue_pull(unsigned i)
{
	struct WorkloadQueue *queue = workload_queue_get(i);
	assert(queue);
	int err = 0;
	err |= pthread_mutex_lock(&queue->guard);
	if (err) {
		on_mutex_err();
	}
	struct Message *msg = queue->next_incoming;
	if (msg) {
		queue->next_incoming = msg->next;
		msg->next = NULL;
	}
	err |= pthread_mutex_unlock(&queue->guard);
	if (err) {
		on_mutex_err();
	}
	return msg;
}

void
workload_queue_add(struct Message *msg, unsigned i)
{
	assert(i < count);
	assert(!msg->next);
	struct WorkloadQueue *queue = workload_queue_get(i);
	int err = 0;
	err |= pthread_mutex_lock(&queue->guard);
	queue->next_incoming = msg;
	struct Message *last = queue->last_incoming;
	if (last) {
		last->next = msg;
		queue->last_incoming = msg;
	} else {
		queue->next_incoming = msg;
		queue->last_incoming = msg;
	}
	err |= sem_post(&queue->sem);
	err |= pthread_mutex_unlock(&queue->guard);
	/* Errors would only arise in the case of race conditions of concurrency bugs
	 * (deadlocks, bad semaphore waits, etc.). They are thus non-recoverable. */
	assert(err == 0);
}

void
workload_queues_free(void)
{
	for (size_t i = 0; i < count; i++) {
		int err = pthread_mutex_destroy(&workload_queues[i].guard);
		assert(err == 0);
	}
	free(workload_queues);
}
