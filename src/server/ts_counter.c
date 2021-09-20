#include "ts_counter.h"
#include "global_state.h"
#include <pthread.h>
#include <stdlib.h>

static pthread_mutex_t thread_id_guard = PTHREAD_MUTEX_INITIALIZER;
static unsigned thread_id_counter = 0;

static void
on_mutex_err(void)
{
	glog_fatal(
	  "Unexpected mutex error. This is most probably a bug and it should be fixed.");
	abort();
}

unsigned
ts_counter(void)
{
	int err = 0;
	err = pthread_mutex_lock(&thread_id_guard);
	if (err) {
		on_mutex_err();
	}
	unsigned id = thread_id_counter;
	thread_id_counter++;
	err = pthread_mutex_unlock(&thread_id_guard);
	if (err) {
		on_mutex_err();
	}
	return id;
}
