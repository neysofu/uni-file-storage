#define _POSIX_C_SOURCE 200809L

#include "cli.h"
#include "logc/src/log.h"
#include "run_action.h"
#include "serverapi.h"
#include "utilities.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
print_help(void)
{
	puts("Client for file storage server. Options:");
	puts("");
	puts("-h");
	puts("    Prints this message and exits.");
	puts("-f filename");
	puts("    Specifies the AF_UNIX socket name.");
	puts("-w dirname,[n=0]");
	puts("    Sends recursively all files in `dirname`, up to n.");
	puts("    If n = 0 or unspecified, there is no limit.");
	puts("-W file1[,file2...]");
	puts("    Sends a list of files to the server.");
	puts("-D dirname");
	puts("    Writes evicted files to `dirname`.");
	puts("-r files");
	puts("    Reads a list of files from the server.");
	puts("-R[n=0]");
	puts("    Reads a certain amount (or all, if N is unspecified or 0) of files");
	puts("    from the server.");
	puts("-d dirname");
	puts("    Specifies the directory where to write files that have been");
	puts("    read.");
	puts("-t msec");
	puts("    Time between subsequent server requests.");
	puts("-l file1[,file2]");
	puts("    Locks some files.");
	puts("-u file1[,file2]");
	puts("    Unlocks some files.");
	puts("-c file1[,file2]");
	puts("    Removes some files from the server.");
	puts("-p");
	puts("    Enables logging to standard output.");
	puts("-Z");
	puts("    Sets the time in milliseconds between connection attempts to the");
	puts("    file storage server. 300 (msec) by default.");
	puts("-z");
	puts("    Sets the time in seconds after which connection attempts to the");
	puts("    file storage server will stop and the client will abort. 5");
	puts("    (seconds) by default. No limit if set to 0.");
}

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
		case CLIENT_ERR_BAD_OPTION_P:
			puts("Invalid value for -p.");
			break;
		default:
			break;
	}
}

int
main(int argc, char **argv)
{
	/* Disable logs by default. */
	log_set_quiet(true);
	log_set_level(LOG_INFO);
	/* Parse and store CLI arguments in a handy, special-purposed data structure
	 * that allows easy retrieval later. */
	struct CliArgs *cli_args = cli_args_parse(argc, argv);
	if (cli_args->err) {
		print_client_err(cli_args->err);
		cli_args_free(cli_args);
		return EXIT_FAILURE;
	}
	/* Enable writing logs to STDOUT as per specification. */
	log_add_fp(stdout, cli_args->log_level);
	if (cli_args->help_message) {
		print_help();
		cli_args_free(cli_args);
		return EXIT_SUCCESS;
	}
	/* CLI arguments MUST specify a socket. Otherwise, it's impossible to connect
	 * to the file storage server. */
	if (!cli_args->socket_name) {
		log_fatal("Socket path not specified. Abort.");
		print_client_err(cli_args->err);
		print_help();
		cli_args_free(cli_args);
		return EXIT_FAILURE;
	}
	/* Finally, everything seems to be in order. Let's proceed. */
	log_info("The client is up and running.");
	log_info("Opening connection with a timeout of %lu seconds...",
	         cli_args->sec_max_attempt_time);
	struct timespec connection_due = { 0 };
	clock_gettime(CLOCK_REALTIME, &connection_due);
	if (cli_args->sec_max_attempt_time) {
		connection_due.tv_sec = connection_due.tv_sec + cli_args->sec_max_attempt_time;
	} else {
		connection_due.tv_sec = connection_due.tv_sec + 10000;
	}
	int err = 0;
	err = openConnection(
	  cli_args->socket_name, cli_args->msec_between_connection_attempts, connection_due);
	if (err) {
		log_fatal("Couldn't establish a connection with the server. Abort.");
		cli_args_free(cli_args);
		exit(EXIT_FAILURE);
	}
	for (struct Action *action = cli_args->head; action; action = action->next) {
		run_action(action);
	}
	log_info("Closing connection...");
	err = closeConnection(cli_args->socket_name);
	if (err) {
		log_fatal("An error occured during connection dropping. Abort.");
		cli_args_free(cli_args);
		exit(EXIT_FAILURE);
	}
	log_info("Done.");
	cli_args_free(cli_args);
	log_info("Goodbye.");
	return EXIT_SUCCESS;
}
