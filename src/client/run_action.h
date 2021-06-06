#ifndef SOL_CLIENT_RUN_ACTION
#define SOL_CLIENT_RUN_ACTION

#include "cli.h"

/* Executes an RPC (Remote Procedure Call) to the file storage server. The
 * details of the RPC operation, as well as arguments and other details, are
 * specified in `action`.
 *
 * This function is allowed to modify `action`'s internal data and arguments.
 *
 * Returns 0 on success, -1 on failure, and sets `errno` appropriately. */
int
run_action(struct Action *action);

#endif
