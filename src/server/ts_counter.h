#ifndef SOL_SERVER_TS_COUNTER
#define SOL_SERVER_TS_COUNTER

/* Returns the current counter value and then increments the global counter.
 * Thread-safe.
 *
 * Starts from 0. */
unsigned
ts_counter(void);

#endif
