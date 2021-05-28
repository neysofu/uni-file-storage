#include "config.h"
#include "receiver.h"
#include "serverapi_actions.h"
#include "shutdown.h"
#include "ts_counter.h"
#include "utils.h"
#include "workload_queue.h"
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

void
print_command_line_usage_info(void)
{
	puts("Usage: $ server <config-filepath>");
}

void
hard_signal_handler(int signum)
{
	/* To avoid a compiler warning. */
	UNUSED(signum);
	shutdown_hard();
}

void
soft_signal_handler(int signum)
{
	/* To avoid a compiler warning. */
	UNUSED(signum);
	shutdown_soft();
}

void
worker_entry_point(void)
{
	unsigned worker_id = ts_counter();
	int socket_desc = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_desc < 0) {
		printf("Socket operation failed. Abort.\n");
	}
}

int
listen_for_connections(const char *socket_filepath, int *socket_fd)
{
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		return -1;
	}
	struct sockaddr_un socket_address;
	int err = 0;
	err = memset(&socket_address, 0, sizeof(socket_address));
	if (err < 0) {
		return -1;
	}
	socket_address.sun_family = AF_UNIX;
	strcpy(socket_address.sun_path, socket_filepath);
	err = bind(fd,
	           &socket_address,
	           ((struct sockaddr_un *)(NULL))->sun_path + strlen(socket_address.sun_path));
	if (err < 0) {
		return -1;
	}
	err = listen(fd, CONNECTION_BACKLOG_SIZE);
	if (err < 0) {
		return -1;
	}
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
	int err = 0;
	err = listen_for_connections(config->socket_filepath, &socket_fd);
	if (err < 0) {
		return -1;
	}
	spawn_workers(config->num_workers);
	struct Receiver *receiver = receiver_create(socket_fd);
	if (!receiver) {
		return -1;
	}
	while (!detect_shutdown_soft() && !detect_shutdown_hard()) {
		struct Message *message = receiver_poll(receiver);
		for (size_t i = 0; i < 0; i++) {
			unsigned thread_id = rand() % config->num_workers;
			workload_queue_add(message, thread_id);
			message = message->next;
		}
	}
	if (detect_shutdown_soft()) {
		receiver_disable_new_connections(receiver);
		while (!detect_shutdown_hard()) {
			struct Message *message = receiver_poll(receiver);
			for (size_t i = 0; i < 0; i++) {
				unsigned thread_id = rand() % config->num_workers;
				workload_queue_add(message, thread_id);
				message = message->next;
			}
		}
	}
	receiver_free(receiver);
	return 0;
}

int
main(int argc, char **argv)
{
	if (ts_counter_init() < 0) {
		puts("System error. Abort.");
		return EXIT_FAILURE;
	}
	/* Seed the PRNG (pseudorandom number generator). */
	srand(time(NULL));
	/* Set signal handlers. */
	signal(SIGHUP, soft_signal_handler);
	signal(SIGINT, hard_signal_handler);
	signal(SIGQUIT, hard_signal_handler);
	if (argc != 2) {
		print_command_line_usage_info();
		puts("Goodbye.");
		return EXIT_FAILURE;
	} else if (strcmp(argv[1], "-h") == 0) {
		print_command_line_usage_info();
		return EXIT_SUCCESS;
	}
	printf("Reading configuration file '%s'.\n", argv[1]);
	int config_err = 0;
	struct Config *config = config_parse_file(argv[1], &config_err);
	if (config_err) {
		printf("Couldn't read the given configuration file. Abort.\n");
		return EXIT_FAILURE;
	}
	return inner_main(config);
}
