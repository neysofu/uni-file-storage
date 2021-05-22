#define _POSIX_C_SOURCE 200809L

#include "utils.h"
#include <time.h>

void
wait_msec(int msec)
{
	struct timespec interval;
	interval.tv_nsec = (msec % 1000) * 1000000;
	interval.tv_sec = msec / 1000;
	int result = 1;
	do {
		result = nanosleep(&interval, &interval);
	} while (result);
}
