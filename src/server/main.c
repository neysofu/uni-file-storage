#include "config.h"
#include "utils.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

struct Config *global_config = NULL;
pthread_mutex_t *thread_id_guard;
unsigned thread_id_counter = 0;

struct WorkerPublicState
{
	pthread_mutex_t guard;
	unsigned num_connections;
};

struct WorkerPublicState *worker_states = NULL;

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
	pthread_mutex_lock(thread_id_guard);
	unsigned thread_id = thread_id_counter++;
	pthread_mutex_unlock(thread_id_guard);
	if (socket_desc < 0) {
		printf("Socket operation failed. Abort.\n");
		return EXIT_FAILURE;
	}
	struct sockaddr_un socket_address;
	memset(&socket_address, 0, sizeof(socket_address));
	socket_address.sun_family = AF_UNIX;
}

int
inner_main(const struct Config *config)
{
	global_config = config;
	for (unsigned i = 0; i < config->num_workers; i++) {
		pthread_t worker;
		int n = pthread_create(&worker, NULL, worker_entry_point, NULL);
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
	const struct Config *config = config_parse_file(argv[1], &config_err);
	if (config_err) {
		printf("Couldn't read the given configuration file. Abort.\n");
		return EXIT_FAILURE;
	}
	return inner_main(config);
}
