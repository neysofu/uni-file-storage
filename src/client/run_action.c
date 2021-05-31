#define _POSIX_C_SOURCE 200809L

#include "run_action.h"
#include "logc/src/log.h"
#include "serverapi.h"
#include "serverapi_actions.h"
#include "utils.h"
#include <assert.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

int
run_action_lock_files(struct Action *action) {
    char *path = strtok(action->arg_s, ",");
    while (path) {
        log_debug("Locking `%s`.", path);
        lockFile(path);
        path = strtok(NULL, ",");
    }
    return 0;
}

int
run_action_unlock_files(struct Action *action) {
    char *path = strtok(action->arg_s, ",");
    while (path) {
        log_debug("Unlocking `%s`.", path);
        unlockFile(path);
        path = strtok(NULL, ",");
    }
    return 0;
}

int
run_action_write(struct Action *action) {
    char *path = strtok(action->arg_s, ",");
    while (path) {
        writeFile(path, NULL);
        path = strtok(NULL, ",");
    }
    return 0;
}

int
run_action(struct Action *action)
{
	switch (action->type) {
		case ACTION_REMOVE:
			break;
		case ACTION_SET_EVICTED_DIR:
			writeFile(action->arg_s, action->arg_s);
			break;
		case ACTION_SEND:
			run_action_write(action);
			break;
		case ACTION_WAIT:
            log_debug("Waiting %d milliseconds.", action->arg_i);
			wait_msec(action->arg_i);
			break;
		case ACTION_LOCK:
            log_debug("", action->arg_i);
			lockFile(action->arg_s);
			break;
		case ACTION_UNLOCK:
			unlockFile(action->arg_s);
			break;
		default:
			assert(false);
	}
	return 0;
}
