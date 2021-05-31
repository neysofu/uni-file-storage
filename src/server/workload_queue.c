#include "workload_queue.h"
#include "receiver.h"
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>

static struct WorkloadQueue *workload_queues = NULL;
static unsigned count = 0;

int
workload_queues_init(unsigned n)
{
	workload_queues = malloc(sizeof(struct WorkloadQueue) * n);
	/* Check allocation failure. */
	if (!workload_queues) {
		return -1;
	}
	int err = 0;
	count = n;
	for (size_t i = 0; i < count; i++) {
		err |= sem_init(&workload_queues[i].sem, 0, 0);
		workload_queues[i].guard;
		workload_queues[i].next_incoming = NULL;
		workload_queues[i].last_incoming = NULL;
	}
	if (err != 0) {
		/* Free all memory on semaphore initialization failure. */
		free(workload_queues);
		return -1;
	} else {
		return 0;
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
	struct WorkloadQueue *queue = workload_queue_get(i);
	assert(queue);
	pthread_mutex_lock(&queue->guard);
	struct Message *msg = queue->next_incoming;
	pthread_mutex_unlock(&queue->guard);
	return msg;
}

void
workload_queue_add(struct Message *msg, unsigned i)
{
	assert(i < count);
	struct WorkloadQueue *queue = workload_queue_get(i);
	pthread_mutex_lock(&queue->guard);
	queue->next_incoming = msg;
	// FIXME
	sem_post(&queue->sem);
	pthread_mutex_unlock(&queue->guard);
}

void
workload_queues_free(void)
{
	for (size_t i = 0; i < count; i++) {
		pthread_mutex_destroy(&workload_queues[i].guard);
	}
	free(workload_queues);
}
