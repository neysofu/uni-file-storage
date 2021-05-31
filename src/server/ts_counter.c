#include "ts_counter.h"
#include <pthread.h>

static pthread_mutex_t thread_id_guard = PTHREAD_MUTEX_INITIALIZER;
static unsigned thread_id_counter = 0;

unsigned
ts_counter(void)
{
	// TODO: error checking.
	int err = 0;
	err = pthread_mutex_lock(&thread_id_guard);
	unsigned id = thread_id_counter;
	thread_id_counter++;
	err = pthread_mutex_unlock(&thread_id_guard);
	return id;
}