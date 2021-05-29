#ifndef SOL_SERVER_TS_COUNTER
#define SOL_SERVER_TS_COUNTER

/* Returns the current counter value and then increments the global counter.
 * Thread-safe. */
unsigned
ts_counter(void);

#endif
