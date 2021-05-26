#define _POSIX_C_SOURCE 200809L

#include "cli.h"
#include "logc/src/log.h"
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
	puts("    Reads a certain amount (or all, if N is unspecified or 0) files");
	puts("    the server.");
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
}

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
	openConnection(cli_args->socket_name, 0, empty);
	for (struct Action *action = cli_args->head; action; action = action->next) {
		switch (action->type) {
			case ACTION_REMOVE:
				break;
			case ACTION_SET_EVICTED_DIR:
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
	log_info("Starting client.");
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
	return inner_main(cli_args);
}
