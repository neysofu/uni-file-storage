#ifndef SOL_SERVER_UTILITIES
#define SOL_SERVER_UTILITIES

#include <stdlib.h>

#define ON_ERR(err, ...)                                                                   \
	do {                                                                                   \
		if ((err) != 0) {                                                                  \
			glog_fatal(__FILE__, __LINE__, __VA_ARGS__);                                   \
			exit(EXIT_FAILURE);                                                            \
		}                                                                                  \
	} while (0)

#endif
