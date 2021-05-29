#ifndef SOL_SERVER_WORKER
#define SOL_SERVER_WORKER

#include <pthread.h>

/* Unique entry point for all worker threads. */
void *
worker_entry_point(void *args);

inline static void
spawn_workers(unsigned num)
{
	for (unsigned i = 0; i < num; i++) {
		pthread_t worker;
		pthread_create(&worker, NULL, worker_entry_point, NULL);
	}
}

#endif
