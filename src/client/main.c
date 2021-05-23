#define _POSIX_C_SOURCE 200809L

#include "cli.h"
#include "serverapi.h"
#include "utils.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void
print_help(void)
{

	puts("Client for file storage server. Options:");
	puts("");
	puts("-h            Prints this message and exits.");
	puts("-f filename   Specifies the AF_UNIX socket name.");
	puts("-w dirname    Sends all files in `dirname`.");
	puts("-W files      Sends a list of files to the server.");
	puts("-D dirname    Writes evicted files to `dirname`.");
	puts("-r files      Reads a list of files from the server.");
	puts("-R            Reads a certain amount (or all) files in the server.");
}

void
print_client_err(enum ClientErr err)
{
	switch (err) {
		case CLIENT_ERR_OK:
			return;
		case CLIENT_ERR_ALLOC:
			puts("Out-of-memory error.");
			break;
		case CLIENT_ERR_REPEATED_F:
			puts("Multiple -f options. Only one is allowed.");
			break;
		case CLIENT_ERR_REPEATED_H:
			puts("Multiple -h options. Only one is allowed.");
			break;
		case CLIENT_ERR_REPEATED_P:
			puts("Multiple -p options. Only one is allowed.");
			break;
		default:
			puts("Generic error.");
	}
}

int
run_actions(const struct CliArgs *cli_args)
{
	struct timespec empty = { 0 };
	openConnection(cli_args->socket_name, 0, empty);
	for (struct Action *action = cli_args->head; action; action = action->next) {
		switch (action->type) {
			case ACTION_REMOVE:
				break;
			case ACTION_SEND_DIR_CONTENTS:
				break;
			case ACTION_SEND:
				break;
			case ACTION_WAIT:
				wait_msec(action->arg_i);
				break;
			default:
				assert(false);
		}
	}
	closeConnection(cli_args->socket_name);
	return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
	struct CliArgs *cli_args = cli_args_parse(argc, argv);
	if (!cli_args) {
		print_client_err(CLIENT_ERR_ALLOC);
		return EXIT_FAILURE;
	} else if (cli_args->err) {
		print_client_err(cli_args->err);
		return EXIT_FAILURE;
	} else if (cli_args->help_message) {
		print_help();
		return EXIT_SUCCESS;
	}
	return run_actions(cli_args);
}
