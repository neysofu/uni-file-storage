#ifndef SOL_CLIENT_CLI
#define SOL_CLIENT_CLI

#include "err.h"
#include "serverapi_actions.h"
#include <stdbool.h>

struct CliArgs
{
	enum ClientErr err;
	char *socket_name;
	bool help_message;
	bool enable_log;
	struct Action *head;
	struct Action *last;
};

/* Reads client command-line arguments into a `struct CliArgs` on the heap and
 * returns a pointer to it. This function returns NULL on allocation failure and
 * sets the `err` field for bad command-line arguments. */
struct CliArgs *
cli_args_parse(int argc, char **argv);

#endif
