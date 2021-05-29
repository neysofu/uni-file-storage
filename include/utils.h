#ifndef SOL_UTILS
#define SOL_UTILS

/* Suppresses "unused variable" warnings by the compiler. */
#define UNUSED(x) (void)(x)

/* SUN_LEN isn't standard POSIX, so let's define it ourselves. */
#ifndef SUN_LEN
#define SUN_LEN(ptr)                                                                       \
	((size_t)(((struct sockaddr_un *)0)->sun_path) + strlen((ptr)->sun_path))
#endif

/* Suspends the execution of the current thread for approximately `msec`
 * milliseconds. */
void
wait_msec(int msec);

#endif
