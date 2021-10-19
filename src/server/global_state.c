#include "global_state.h"
#include "config.h"
#include "htable.h"
#include "server_utilities.h"
#include <pthread.h>

pthread_mutex_t log_guard = PTHREAD_MUTEX_INITIALIZER;
struct Config *global_config = NULL;
struct HTable *htable = NULL;

static pthread_mutex_t thread_id_guard = PTHREAD_MUTEX_INITIALIZER;
static unsigned thread_id_counter = 0;

static pthread_mutex_t shutdown_guard = PTHREAD_MUTEX_INITIALIZER;
static bool soft = false;
static bool hard = false;

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
	ON_SHUTDOWN_ERR(pthread_mutex_lock(&shutdown_guard));
	soft = true;
	ON_SHUTDOWN_ERR(pthread_mutex_unlock(&shutdown_guard));
}

void
shutdown_hard(void)
{
	ON_SHUTDOWN_ERR(pthread_mutex_lock(&shutdown_guard));
	hard = true;
	ON_SHUTDOWN_ERR(pthread_mutex_unlock(&shutdown_guard));
}

bool
detect_shutdown_soft(void)
{
	ON_SHUTDOWN_ERR(pthread_mutex_lock(&shutdown_guard));
	bool shutdown = soft;
	ON_SHUTDOWN_ERR(pthread_mutex_unlock(&shutdown_guard));
	return shutdown;
}

bool
detect_shutdown_hard(void)
{
	ON_SHUTDOWN_ERR(pthread_mutex_lock(&shutdown_guard));
	bool shutdown = hard;
	ON_SHUTDOWN_ERR(pthread_mutex_unlock(&shutdown_guard));
	return shutdown;
}