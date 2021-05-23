#include "config.h"
#include "serverapi_actions.h"
#include "utils.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

struct Config *global_config = NULL;

struct WorkerPublicState *worker_states = NULL;

pthread_mutex_t *thread_id_guard;
unsigned thread_id_counter = 0;

struct WorkerQueue
{
	pthread_mutex_t guard;
	struct Action *next_action;
	struct Action *last_action;
};

void
close_all_connections(int signum)
{
	UNUSED(signum);
}

void
disable_new_connections(int signum)
{
	UNUSED(signum);
}

void
worker_entry_point(void)
{
	int socket_desc = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_desc < 0) {
		printf("Socket operation failed. Abort.\n");
	}
}

void
handle_incoming_connection(void)
{}

int
inner_main(struct Config *config)
{
	global_config = config;
	/* Launch worker threads. */
	for (unsigned i = 0; i < config->num_workers; i++) {
		pthread_t worker;
		pthread_create(&worker, NULL, worker_entry_point, NULL);
	}
	/* Listen for incoming connections. */
	int socket_descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
	struct sockaddr_un socket_address;
	memset(&socket_address, 0, sizeof(socket_address));
	socket_address.sun_family = AF_UNIX;
	strcpy(socket_address.sun_path, config->socket_filepath);
	bind(socket_descriptor,
	     &socket_address,
	     ((struct sockaddr_un *)(NULL))->sun_path + strlen(socket_address.sun_path));
	listen(socket_descriptor, 1);
	while (1) {
		int connection_fd = accept(socket_descriptor, NULL, NULL);
	}
	return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
	signal(SIGHUP, disable_new_connections);
	signal(SIGINT, close_all_connections);
	signal(SIGQUIT, close_all_connections);
	int config_err = 0;
	if (argc != 2) {
		printf("Usage: $ server <config-filepath>\n"
		       "Goodbye.\n");
		return EXIT_FAILURE;
	}
	printf("Reading configuration file '%s'.\n", argv[1]);
	const struct CliArgs *config = config_parse_file(argv[1], &config_err);
	if (config_err) {
		printf("Couldn't read the given configuration file. Abort.\n");
		return EXIT_FAILURE;
	}
	return inner_main(config);
}
