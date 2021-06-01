#ifndef SOL_SERVER_WORKLOAD_QUEUE
#define SOL_SERVER_WORKLOAD_QUEUE

#include "receiver.h"
#include <pthread.h>

struct WorkloadQueue
{
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	struct Message *next_incoming;
	struct Message *last_incoming;
};

/* Initializes a global array of workload queues, as many as specified by
 * `count`. */
void
workload_queues_init(unsigned count);

/* Appends `msg` to the workload queue number `i`. */
void
workload_queue_add(struct Message *msg, unsigned i);

/* Extracts a message from the queue number `i` (waits until one is available). */
struct Message *
workload_queue_pull(unsigned i);

/* Deletes all workload queues and frees all used memory. */
void
workload_queues_free(void);

#endif
