#ifndef SOL_SERVER_SHUTDOWN
#define SOL_SERVER_SHUTDOWN

#include <stdbool.h>

int
init_shutdown_system(void);

int
shutdown_soft(void);

int
shutdown_hard(void);

bool
detect_shutdown_soft(void);

bool
detect_shutdown_hard(void);

#endif
