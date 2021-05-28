#include "shutdown.h"
#include <pthread.h>
#include <stdbool.h>

static pthread_mutex_t *guard;

static bool soft;
static bool hard;

int
init_shutdown_system(void)
{
	soft = false;
	hard = false;
	return pthread_mutex_init(guard, NULL);
}

int
shutdown_soft(void)
{
	int err = 0;
	err |= pthread_mutex_lock(guard);
	soft = true;
	err |= pthread_mutex_unlock(guard);
	return err == 0 ? 0 : -1;
}

int
shutdown_hard(void)
{
	int err = 0;
	err |= pthread_mutex_lock(guard);
	hard = true;
	err |= pthread_mutex_unlock(guard);
	return err == 0 ? 0 : -1;
}

bool
detect_shutdown_soft(void)
{
	int err = 0;
	bool shutdown = false;
	err |= pthread_mutex_lock(guard);
	shutdown = soft;
	err |= pthread_mutex_unlock(guard);
	return shutdown || err != 0;
}

bool
detect_shutdown_hard(void)
{
	int err = 0;
	bool shutdown = false;
	err |= pthread_mutex_lock(guard);
	shutdown = hard;
	err |= pthread_mutex_unlock(guard);
	return shutdown || err != 0;
}
