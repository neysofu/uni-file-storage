#include "config.h"
#include "global_state.h"
#include "htable.h"
#include "logc/src/log.h"
#include "receiver.h"
#include "serverapi_actions.h"
#include "shutdown.h"
#include "ts_counter.h"
#include "utils.h"
#include "worker.h"
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
#include <unistd.h>

#define CONNECTION_BACKLOG_SIZE 8

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

int
listen_for_connections(const char *socket_filepath, int *socket_fd)
{
	int err = 0;
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		log_fatal("`socket` syscall failed.");
		return -1;
	}
	log_debug("`socket` syscall done.");
	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, socket_filepath);
	err = bind(fd, (struct sockaddr *)&addr, SUN_LEN(&addr));
	if (err < 0) {
		log_fatal("`bind` syscall failed.");
		return -1;
	}
	log_debug("`bind` syscall done.");
	err = listen(fd, CONNECTION_BACKLOG_SIZE);
	if (err < 0) {
		log_fatal("`listen` syscall failed.");
		return -1;
	}
	log_debug("`listen` syscall done.");
	*socket_fd = fd;
	return 0;
}

int
inner_main(struct Config *config)
{
	global_config = config;
	int socket_fd = 0;
	int err = 0;
	log_debug("Now listening for incoming connections.");
	err = listen_for_connections(config->socket_filepath, &socket_fd);
	if (err < 0) {
		log_fatal("Couldn't listen for incoming connections. Abort.");
		return -1;
	}
	log_debug("Now spawning %d worker threads...", config->num_workers);
	spawn_workers(config->num_workers);
	log_debug("Done.");
	struct Receiver *receiver = receiver_create(socket_fd);
	if (!receiver) {
		config_free(config);
		return EXIT_FAILURE;
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
		log_warn("Soft shutdown signal detected! Stopped listening to new connections.");
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
	htable = htable_new(100);
	log_debug("Starting server. Initializing some internal resources.");
	/* Seed the PRNG (pseudorandom number generator). */
	srand(time(NULL));
	/* Set signal handlers. */
	signal(SIGHUP, soft_signal_handler);
	signal(SIGINT, hard_signal_handler);
	signal(SIGQUIT, hard_signal_handler);
	log_debug("Initialization was successful. Now parsing command-line args.");
	if (argc != 2) {
		print_command_line_usage_info();
		puts("Goodbye.");
		return EXIT_FAILURE;
	} else if (strcmp(argv[1], "-h") == 0) {
		print_command_line_usage_info();
		return EXIT_SUCCESS;
	}
	log_debug("Reading configuration file '%s'.", argv[1]);
	struct Config *config = config_parse_file(argv[1]);
	if (!config || config->err) {
		printf("Couldn't read the given configuration file. Abort.\n");
		return EXIT_FAILURE;
	}
	log_debug("Configuration was deemed valid.");
	/* Remove the server socket file, if it exists. We simply ignore any error. */
	unlink(config->socket_filepath);
	return inner_main(config);
}
