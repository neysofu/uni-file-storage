#include "config.h"
#include "receiver.h"
#include "serverapi_actions.h"
#include "utils.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#define CONNECTION_BACKLOG_SIZE 8

struct Config *global_config = NULL;

struct WorkerPublicState *worker_states = NULL;

pthread_mutex_t *thread_id_guard;
unsigned thread_id_counter = 0;

struct WorkloadQueue
{
	pthread_mutex_t guard;
	struct Action *next_action;
	struct Action *last_action;
};

void
print_command_line_usage_info(void)
{
	puts("Usage: $ server <config-filepath>");
}

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
listen_for_connections(const char *socket_filepath, int *socket_fd)
{
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	struct sockaddr_un socket_address;
	memset(&socket_address, 0, sizeof(socket_address));
	socket_address.sun_family = AF_UNIX;
	strcpy(socket_address.sun_path, socket_filepath);
	bind(fd,
	     &socket_address,
	     ((struct sockaddr_un *)(NULL))->sun_path + strlen(socket_address.sun_path));
	listen(fd, CONNECTION_BACKLOG_SIZE);
	*socket_fd = fd;
	return 0;
}

void
spawn_workers(unsigned num)
{
	for (unsigned i = 0; i < num; i++) {
		pthread_t worker;
		pthread_create(&worker, NULL, worker_entry_point, NULL);
	}
}

int
inner_main(struct Config *config)
{
	global_config = config;
	int socket_fd = 0;
	listen_for_connections(config->socket_filepath, &socket_fd);
	spawn_workers(config->num_workers);
	struct Receiver *receiver = receiver_create(socket_fd);
	while (1) {
		int *fds;
		int err = receiver_poll(receiver, &fds);
		for (size_t i = 0; i < 0; i++) {
		}
	}
	receiver_free(receiver);
	return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
	/* Seed the PRNG (pseudorandom number generator). */
	srand(time(NULL));
	/* Set signal handlers. */
	signal(SIGHUP, disable_new_connections);
	signal(SIGINT, close_all_connections);
	signal(SIGQUIT, close_all_connections);
	if (argc != 2) {
		print_command_line_usage_info();
		puts("Goodbye.");
		return EXIT_FAILURE;
	} else if (argv[1] == "-h") {
		print_command_line_usage_info();
		return EXIT_SUCCESS;
	}
	printf("Reading configuration file '%s'.\n", argv[1]);
	int config_err = 0;
	const struct Config *config = config_parse_file(argv[1], &config_err);
	if (config_err) {
		printf("Couldn't read the given configuration file. Abort.\n");
		return EXIT_FAILURE;
	}
	return inner_main(config);
}
