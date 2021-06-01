#include "config.h"
#include "global_state.h"
#include "htable.h"
#include <pthread.h>

pthread_mutex_t log_guard = PTHREAD_MUTEX_INITIALIZER;
struct Config *global_config = NULL;
struct HTable *htable = NULL;
