#include "global_state.h"
#include "config.h"
#include "htable.h"
#include "server_utilities.h"
#include <pthread.h>
#include <signal.h>

pthread_mutex_t log_guard = PTHREAD_MUTEX_INITIALIZER;
struct Config *global_config = NULL;
struct HTable *global_htable = NULL;

static pthread_mutex_t thread_id_guard = PTHREAD_MUTEX_INITIALIZER;
static unsigned thread_id_counter = 0;

static volatile sig_atomic_t soft = 0;
static volatile sig_atomic_t hard = 0;

#define ON_SHUTDOWN_ERR(err)                                                               \
	ON_ERR((err), "Mutex error during shutdown processing. This is most likely a bug.")

#define ON_TS_COUNTER_ERR(err)                                                             \
	ON_ERR((err), "Unexpected mutex error during global counter processing.")

unsigned
ts_counter(void)
{
	ON_TS_COUNTER_ERR(pthread_mutex_lock(&thread_id_guard));
	unsigned id = thread_id_counter;
	thread_id_counter++;
	ON_TS_COUNTER_ERR(pthread_mutex_unlock(&thread_id_guard));
	return id;
}

void
shutdown_soft(void)
{
	soft = 1;
}

void
shutdown_hard(void)
{
	hard = 1;
}

bool
detect_shutdown_soft(void)
{
	return soft == 1;
}

bool
detect_shutdown_hard(void)
{
	return hard == 1;
}