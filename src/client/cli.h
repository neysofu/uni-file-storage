#ifndef SOL_CLIENT_CLI
#define SOL_CLIENT_CLI

#include <stdbool.h>

/* Sum type for all possible error causes during a client's execution. */
enum ClientErr
{
	CLIENT_ERR_OK = 0,
	CLIENT_ERR_ALLOC,
	CLIENT_ERR_BAD_OPTION_CAP_D,
	CLIENT_ERR_BAD_OPTION_CAP_R,
	CLIENT_ERR_BAD_OPTION_D,
	CLIENT_ERR_BAD_OPTION_P,
	CLIENT_ERR_UNKNOWN_OPTION,
	CLIENT_ERR_REPEATED_P,
	CLIENT_ERR_REPEATED_H,
	CLIENT_ERR_REPEATED_F,
	CLIENT_ERR_MISSING_ARG,
};

/* A recipe for a sequence of API calls. */
struct Action
{
	/* The character that identifies the CLI flag associated with this action. */
	char type;
	/* The first string argument, if present. */
	char *arg_s1;
	/* The second string argument, if present. */
	char *arg_s2;
	/* The numeric argument, is present. */
	int arg_i;
	struct Action *next;
};

/* A storage model for CLI flags with metadata in a linked list. */
struct CliArgs
{
	enum ClientErr err;
	unsigned msec_between_connection_attempts;
	unsigned msec_between_actions;
	unsigned sec_max_attempt_time;
	char *socket_name;
	bool help_message;
	int log_level;
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
