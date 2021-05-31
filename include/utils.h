#ifndef SOL_UTILS
#define SOL_UTILS

#include <stdint.h>
#include <stdlib.h>

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

/** Evita letture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la lettura da fd leggo EOF
 *   \retval size se termina con successo
 */
int
readn(long fd, void *buf, size_t size);

/** Evita scritture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la scrittura la write ritorna 0
 *   \retval  1   se la scrittura termina con successo
 */
int
writen(long fd, void *buf, size_t size);

uint64_t
big_endian_to_u64(char bytes[8]);

void
u64_to_big_endian(uint64_t data, char bytes[8]);

#endif
