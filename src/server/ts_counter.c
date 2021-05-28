#include "ts_counter.h"
#include <pthread.h>

static pthread_mutex_t *thread_id_guard;
static unsigned thread_id_counter = 0;

int
ts_counter_init(void)
{
	pthread_mutex_init(thread_id_guard, NULL);
}

unsigned
ts_counter(void)
{
	pthread_mutex_lock(thread_id_guard);
	unsigned id = thread_id_counter;
	thread_id_counter++;
	pthread_mutex_unlock(thread_id_guard);
	return id;
}