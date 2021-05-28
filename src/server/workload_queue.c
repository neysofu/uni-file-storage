#include "workload_queue.h"
#include "receiver.h"
#include <assert.h>
#include <pthread.h>

static struct WorkloadQueue *workload_queues = NULL;
static unsigned count = 0;

void
workload_queues_init(unsigned n)
{
	workload_queues = malloc(sizeof(struct WorkloadQueue) * n);
	count = n;
	for (size_t i = 0; i < count; i++) {
		workload_queues[i].guard;
		workload_queues[i].next_incoming = NULL;
		workload_queues[i].last_incoming = NULL;
	}
}

void
workload_queue_add(struct Message *msg, unsigned i)
{
	assert(i < count);
	// pthread_mutex_lock(&queue->guard);
	//// TODO
	// pthread_mutex_unlock(&queue->guard);
}

void
workload_queues_free(void)
{
	for (size_t i = 0; i < count; i++) {
		pthread_mutex_destroy(&workload_queues[i].guard);
	}
	free(workload_queues);
}
