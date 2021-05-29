#ifndef SOL_SERVER_SHUTDOWN
#define SOL_SERVER_SHUTDOWN

#include <stdbool.h>

/* Trigger a "soft" shutdown, i.e. stop listening for new connections. */
int
shutdown_soft(void);

/* Trigger a "hard" shutdown, i.e. stop listening for new connections and close
 * all existing connections. */
int
shutdown_hard(void);

/* Has a soft shutdown occured already? */
bool
detect_shutdown_soft(void);

/* Has a hard shutdown occured already? */
bool
detect_shutdown_hard(void);

#endif
