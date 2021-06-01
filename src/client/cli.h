#ifndef SOL_CLIENT_CLI
#define SOL_CLIENT_CLI

#include "err.h"
#include <stdbool.h>

/* Identifiers for all actions, i.e. sequences of API calls as specified by a
 * single command-line option. */
enum ActionType {
    ACTION_WRITE_DIR,
    ACTION_WRITE_FILES,
    ACTION_READ_FILES,
    ACTION_READ_RANDOM_FILES,
    ACTION_WAIT_MILLISECONDS,
    ACTION_LOCK_FILES,
    ACTION_UNLOCK_FILES,
    ACTION_REMOVE_FILES,
};

/* A recipe for a sequence of API calls. */
struct Action
{
	enum ActionType type;
	char *arg_s1;
	char *arg_s2;
	int arg_i;
	struct Action *next;
};

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
 * returns a pointer to it. This function
 * sets the `err` field for bad command-line arguments. */
struct CliArgs *
cli_args_parse(int argc, char **argv);

/* Frees all memory internally used by `cli_args`. */
void
cli_args_free(struct CliArgs *cli_args);

#endif
