#define _POSIX_C_SOURCE 200809L

#include "cli.h"
#include "help.h"
#include "logc/src/log.h"
#include "run_action.h"
#include "serverapi.h"
#include "utils.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void
print_client_err(enum ClientErr err)
{
	assert(err);
	puts("Command-line usage error.");
	switch (err) {
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
		case CLIENT_ERR_UNKNOWN_OPTION:
			puts("Unknown command line options.");
			break;
		default:
			puts("No further information is available.");
	}
}

int
inner_main(const struct CliArgs *cli_args)
{
	struct timespec empty = { 0 };
	if (!cli_args->socket_name) {
		log_fatal("Socket path not specified.");
		return EXIT_FAILURE;
	}
	setLogging(cli_args->enable_log);
	log_info("Opening connection...");
	openConnection(cli_args->socket_name, 0, empty);
	log_info("Done.");
	for (struct Action *action = cli_args->head; action; action = action->next) {
		run_action(action);
	}
	log_info("Closing connection...");
	closeConnection(cli_args->socket_name);
	log_info("Done.");
	return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
	/* Disable logs by default. */
	log_set_quiet(true);
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
	/* Write logs to STDOUT as per specification. */
	if (cli_args->enable_log) {
		log_add_fp(stdout, LOG_TRACE);
	}
	return inner_main(cli_args);
}
