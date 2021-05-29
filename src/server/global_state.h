#ifndef SOL_SERVER_GLOBAL_STATE
#define SOL_SERVER_GLOBAL_STATE

#include "config.h"
#include "htable.h"
#include <stdlib.h>

static struct Config *global_config = NULL;

static struct HTable *htable = NULL;

#endif
