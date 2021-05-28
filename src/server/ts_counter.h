#ifndef SOL_SERVER_TS_COUNTER
#define SOL_SERVER_TS_COUNTER

/* Initializes a global thread-safe counter that starts from 0. */
int
ts_counter_init(void);

/* Returns the current counter value and then increments the global counter.
 * Thread-safe. */
unsigned
ts_counter(void);

#endif
