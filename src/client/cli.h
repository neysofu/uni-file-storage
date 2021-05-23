#ifndef SOL_CLIENT_CLI
#define SOL_CLIENT_CLI

#include "err.h"
#include <stdbool.h>

enum ActionType
{
	ACTION_SEND_DIR_CONTENTS,
	ACTION_SEND,
	ACTION_WRITE_EVICTED,
	ACTION_READ,
	ACTION_READ_RANDOM,
	ACTION_SET_DIR_DESTINATION,
	ACTION_WAIT,
	ACTION_LOCK,
	ACTION_UNLOCK,
	ACTION_REMOVE,
};

struct ActionArgs
{
	char **str;
	int n;
};

struct Action
{
	enum ActionType type;
	char *arg_s;
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
 * returns a pointer to it. This function returns NULL on allocation failure and
 * sets the `err` field for bad command-line arguments. */
struct CliArgs *
cli_args_parse(int argc, char **argv);

#endif
