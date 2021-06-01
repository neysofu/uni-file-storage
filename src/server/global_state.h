#ifndef SOL_SERVER_GLOBAL_STATE
#define SOL_SERVER_GLOBAL_STATE

#include "config.h"
#include "htable.h"
#include "logc/src/log.h"
#include <pthread.h>

/* Stands for "Guarded Log" (thread-safe logging). */
#define glog_trace(...) glog_guarded(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define glog_debug(...) glog_guarded(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define glog_info(...) glog_guarded(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define glog_warn(...) glog_guarded(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define glog_error(...) glog_guarded(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define glog_fatal(...) glog_guarded(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)
#define glog_guarded(...)                                                                  \
	{                                                                                      \
		pthread_mutex_lock(&log_guard);                                                    \
		log_log(__VA_ARGS__);                                                              \
		pthread_mutex_unlock(&log_guard);                                                  \
	}

extern pthread_mutex_t log_guard;
extern struct Config *global_config;
extern struct HTable *htable;

#endif
