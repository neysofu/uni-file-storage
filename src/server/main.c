#include "config.h"
#include "global_state.h"
#include "htable.h"
#include "logc/src/log.h"
#include "receiver.h"
#include "serverapi.h"
#include "utilities.h"
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

#define CONNECTION_BACKglog_SIZE 8

void
print_command_line_usage_info(void)
{
	puts("Usage: $ server <config-filepath>");
}

void
hard_signal_handler(int signum)
{
	UNUSED(signum);
	shutdown_hard();
}

void
soft_signal_handler(int signum)
{
	UNUSED(signum);
	shutdown_soft();
}

int
listen_for_connections(const char *socket_filepath, int *socket_fd)
{
	int err = 0;
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		glog_fatal("`socket` syscall failed.");
		return -1;
	}
	glog_debug("`socket` syscall done.");
	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, socket_filepath);
	err = bind(fd, (struct sockaddr *)&addr, SUN_LEN(&addr));
	if (err < 0) {
		glog_fatal("`bind` syscall failed.");
		return -1;
	}
	glog_debug("`bind` syscall done.");
	err = listen(fd, CONNECTION_BACKglog_SIZE);
	if (err < 0) {
		glog_fatal("`listen` syscall failed.");
		return -1;
	}
	glog_debug("`listen` syscall done.");
	*socket_fd = fd;
	return 0;
}

void
print_summary(void)
{
	const struct HTableStats *stats = htable_stats(htable);
	printf("Max. files stored: %lu\n", stats->historical_max_items_count);
	printf("Max. space in bytes: %lu\n", stats->historical_max_space_in_bytes);
	printf("Num. of evictions: %lu\n", stats->historical_num_evictions);

	printf("Current space in bytes: %lu\n", stats->total_space_in_bytes);
	printf("Current contents of the file storage server: %lu\n", stats->items_count);

	struct HTableVisitor *visitor = htable_visit(htable, 0);
	struct File *current_file = htable_visitor_next(visitor);
	unsigned long i = 1;
	/* Visual separator. */
	printf("================\n");
	while (current_file) {
		printf("- %lu (size): %lu\n", i, current_file->length_in_bytes);
		printf("  %lu (path): %s\n", i, current_file->key);
		current_file = htable_visitor_next(visitor);
		i++;
	}
	htable_visitor_free(visitor);
}

int
inner_main(struct Config *config)
{
	glog_info("Starting server.");
	FILE *f_log = NULL;
	if (config->log_filepath) {
		glog_info("Opening log file '%s'....", config->log_filepath);
		f_log = fopen(config->log_filepath, "wa");
	} else {
		glog_info("Logging only on STDERR.");
	}
	if (f_log) {
		log_add_fp(f_log, LOG_TRACE);
		glog_info("Done. Log file ready.");
	} else if (config->log_filepath) {
		glog_warn("Couldn't open the log file. Ignoring.");
	}
	int socket_fd = 0;
	int err = 0;
	glog_debug("Now listening for incoming connections.");
	err = listen_for_connections(config->socket_filepath, &socket_fd);
	if (err < 0) {
		glog_fatal("Couldn't listen for incoming connections. Abort.");
		if (f_log) {
			fclose(f_log);
		}
		return -1;
	}
	glog_info("Now spawning %d worker threads...", config->num_workers);
	spawn_workers(config->num_workers);
	glog_info("Done.");
	struct Receiver *receiver = receiver_create(socket_fd, config->num_workers);
	while (!detect_shutdown_hard()) {
		if (detect_shutdown_soft()) {
			receiver_disable_new_connections(receiver);
		}
		err = receiver_poll(receiver);
		if (err < 0) {
			glog_warn("Bad I/O during poll.");
			break;
		}
	}
	print_summary();
	receiver_free(receiver);
	if (f_log) {
		fclose(f_log);
	}
	glog_info("Exiting.");
	return 0;
}

int
main(int argc, char **argv)
{
	glog_debug("Initializing some internal resources.");
	/* Seed the PRNG (pseudorandom number generator). */
	srand(time(NULL));
	/* Set signal handlers. */
	signal(SIGHUP, soft_signal_handler);
	signal(SIGINT, hard_signal_handler);
	signal(SIGQUIT, hard_signal_handler);
	glog_debug("Initialization was successful. Now parsing command-line args.");
	if (argc != 2) {
		print_command_line_usage_info();
		return EXIT_FAILURE;
	} else if (strcmp(argv[1], "-h") == 0) {
		print_command_line_usage_info();
		return EXIT_SUCCESS;
	}
	glog_debug("Reading configuration file '%s'.", argv[1]);
	struct Config *config = config_parse_file(argv[1]);
	if (!config || config->err) {
		printf("Couldn't read the given configuration file. Abort.\n");
		return EXIT_FAILURE;
	}
	glog_debug("Configuration was deemed valid.");
	/* Remember to write logs to the preconfigured log file, if present. */
	if (config->log_f) {
		log_add_fp(config->log_f, LOG_TRACE);
	}
	global_config = config;
	/* Remove the server socket file, if it exists. We simply ignore any error
	 * because we don't care about whether on not the link existed in the first
	 * place. */
	unlink(config->socket_filepath);
	workload_queues_init(config->num_workers);
	/* Initialize the global hash table with a reasonable number of buckets.
	 * Ideally this should be set with a goal load factor, but we're splitting
	 * hairs... */
	htable = htable_create(config->max_files, config);
	return inner_main(config);
}
