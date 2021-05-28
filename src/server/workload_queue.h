#ifndef SOL_SERVER_WORKLOAD_QUEUE
#define SOL_SERVER_WORKLOAD_QUEUE

#include "receiver.h"
#include <pthread.h>

struct WorkloadQueue
{
	pthread_mutex_t guard;
	struct Message *next_incoming;
	struct Message *last_incoming;
};

/* Initializes a global array of workload queues, as many as specified by
 * `count`. */
void
workload_queues_init(unsigned count);

/* Returns a pointer to the workload queue number `i`. Returns NULL in case `i`
 * is invalid. */
struct WorkloadQueue *
workload_queue_get(unsigned i);

/* Appends `msg` to the workload queue number `i`. */
void
workload_queue_add(struct Message *msg, unsigned i);

/* Deletes all workload queues and frees all used memory. */
void
workload_queues_free(void);

#endif
