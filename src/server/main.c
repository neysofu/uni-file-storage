#include "config.h"
#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{
	int config_err = 0;
	printf("Reading configuration file '%s'.\n", argv[1]);
	struct Config *config = config_parse_file(argv[1], &config_err);
	if (config_err) {
		printf("Couldn't read the given configuration file. Abort.\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
