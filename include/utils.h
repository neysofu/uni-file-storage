#ifndef SOL_UTILS
#define SOL_UTILS

/* Suppresses "unused variable" warnings by the compiler. */
#define UNUSED(x) (void)(x)

/* Suspends the execution of the current thread for approximately `msec`
 * milliseconds. */
void
wait_msec(int msec);

#endif
