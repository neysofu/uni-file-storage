#ifndef SOL_SERVER_WORKER
#define SOL_SERVER_WORKER

#include <pthread.h>

/* Creates `num` listening workers. */
void
workers_spawn(unsigned num);

/* Waits for all workers to exit, then frees any data structures allocated during
 * `workers_spawn` to manage worker threads. */
void
workers_join(void);

#endif
