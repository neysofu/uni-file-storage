#include "shutdown.h"
#include <pthread.h>
#include <stdbool.h>

static pthread_mutex_t guard = PTHREAD_MUTEX_INITIALIZER;

static bool soft = false;
static bool hard = false;

int
shutdown_soft(void)
{
	int err = 0;
	err |= pthread_mutex_lock(&guard);
	soft = true;
	err |= pthread_mutex_unlock(&guard);
	return err == 0 ? 0 : -1;
}

int
shutdown_hard(void)
{
	int err = 0;
	err |= pthread_mutex_lock(&guard);
	hard = true;
	err |= pthread_mutex_unlock(&guard);
	return err == 0 ? 0 : -1;
}

bool
detect_shutdown_soft(void)
{
	int err = 0;
	bool shutdown = false;
	err |= pthread_mutex_lock(&guard);
	shutdown = soft;
	err |= pthread_mutex_unlock(&guard);
	return shutdown || err != 0;
}

bool
detect_shutdown_hard(void)
{
	int err = 0;
	bool shutdown = false;
	err |= pthread_mutex_lock(&guard);
	shutdown = hard;
	err |= pthread_mutex_unlock(&guard);
	return shutdown || err != 0;
}
