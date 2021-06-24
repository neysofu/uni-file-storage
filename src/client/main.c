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

void
print_client_err(enum ClientErr err)
{
	if (err == CLIENT_ERR_OK) {
		/* No error at all. */
		return;
	}
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
		case CLIENT_ERR_BAD_OPTION_CAP_D:
			puts("-D can only be used after either -w or -W.");
			break;
		case CLIENT_ERR_BAD_OPTION_D:
			puts("-d can only be used after either -r or -R.");
			break;
		default:
			puts("No further information is available.");
	}
}

void
establish_connection(struct CliArgs *cli_args)
{
	assert(cli_args);
	log_info("Opening connection...");
	struct timespec empty = { 0 };
	int err = openConnection(
	  cli_args->socket_name, cli_args->msec_between_connection_attempts, empty);
	if (err) {
		log_fatal("Couldn't establish a connection with the server. Abort.");
		cli_args_free(cli_args);
		exit(EXIT_FAILURE);
	}
	log_info("Done.");
}

void
close_connection(struct CliArgs *cli_args)
{
	log_info("Closing connection...");
	int err = closeConnection(cli_args->socket_name);
	if (err) {
		log_fatal("An error occured during connection dropping. Abort.");
		cli_args_free(cli_args);
		exit(EXIT_FAILURE);
	}
	log_info("Done.");
}

int
inner_main(struct CliArgs *cli_args)
{
	establish_connection(cli_args);
	for (struct Action *action = cli_args->head; action; action = action->next) {
		run_action(action);
	}
	close_connection(cli_args);
	cli_args_free(cli_args);
	log_info("Goodbye.");
	return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
	/* Disable logs by default. */
	log_set_quiet(true);
	struct CliArgs *cli_args = cli_args_parse(argc, argv);
	if (cli_args->err) {
		print_client_err(cli_args->err);
		cli_args_free(cli_args);
		return EXIT_FAILURE;
	} else if (!cli_args->socket_name) {
		log_fatal("Socket path not specified. Abort.");
		cli_args_free(cli_args);
		return EXIT_FAILURE;
	} else if (cli_args->help_message) {
		print_help();
		cli_args_free(cli_args);
		return EXIT_SUCCESS;
	} else {
		/* Write logs to STDOUT as per specification. */
		if (cli_args->enable_log) {
			log_add_fp(stdout, LOG_TRACE);
		}
		return inner_main(cli_args);
	}
}
