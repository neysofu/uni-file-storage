#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

int
main(int argc, char **argv)
{
	int config_err = 0;
	if (argc != 2) {
		printf("Usage: $ server <config-filepath>\n"
		       "\n"
		       "Goodbye.\n");
		return EXIT_FAILURE;
	}
	printf("Reading configuration file '%s'.\n", argv[1]);
	struct Config *config = config_parse_file(argv[1], &config_err);
	if (config_err) {
		printf("Couldn't read the given configuration file. Abort.\n");
		return EXIT_FAILURE;
	}
	int socket_desc = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_desc < 0) {
		free(config);
		printf("Socket operation failed. Abort.\n");
		return EXIT_FAILURE;
	}
	struct sockaddr_un socket_address;
	memset(&socket_address, 0, sizeof(socket_address));
	socket_address.sun_family = AF_UNIX;
	return EXIT_SUCCESS;
}
