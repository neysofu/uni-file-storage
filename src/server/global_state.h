#ifndef SOL_SERVER_GLOBAL_STATE
#define SOL_SERVER_GLOBAL_STATE

#include "config.h"
#include "htable.h"
#include "logc/src/log.h"
#include <pthread.h>
#include <stdbool.h>

/* Stands for "Guarded Log" (thread-safe logging). */
#define glog_trace(...) glog_guarded(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define glog_debug(...) glog_guarded(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define glog_info(...) glog_guarded(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define glog_warn(...) glog_guarded(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define glog_error(...) glog_guarded(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define glog_fatal(...) glog_guarded(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)
#define glog_guarded(...)                                                                  \
	do {                                                                                   \
		int glog_err = pthread_mutex_lock(&log_guard);                                     \
		if (glog_err != 0) {                                                               \
			printf("Mutex error (%d) while logging. This is a fatal error, and most "      \
			       "likely a bug.",                                                        \
			       glog_err);                                                              \
			exit(EXIT_FAILURE);                                                            \
		}                                                                                  \
		log_log(__VA_ARGS__);                                                              \
		glog_err = pthread_mutex_unlock(&log_guard);                                       \
		if (glog_err != 0) {                                                               \
			printf("Mutex error (%d) while logging. This is a fatal error, and most "      \
			       "likely a bug.",                                                        \
			       glog_err);                                                              \
			exit(EXIT_FAILURE);                                                            \
		}                                                                                  \
	} while (0)

extern pthread_mutex_t log_guard;
extern struct Config *global_config;
extern struct HTable *global_htable;

/* Returns the current counter value and then increments the global counter.
 * Thread-safe.
 *
 * Starts from 0. */
unsigned
ts_counter(void);

/* Triggers a "soft" shutdown, i.e. stop listening for new connections. */
void
shutdown_soft(void);

/* Triggers a "hard" shutdown, i.e. stop listening for new connections and close
 * all existing connections. */
void
shutdown_hard(void);

/* Has a soft shutdown occured already? */
bool
detect_shutdown_soft(void);

/* Has a hard shutdown occured already? */
bool
detect_shutdown_hard(void);

#endif
