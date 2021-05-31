#include "worker.h"
#include "global_state.h"
#include "logc/src/log.h"
#include "serverapi_utils.h"
#include "ts_counter.h"
#include "utils.h"
#include "workload_queue.h"
#include <semaphore.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

void
worker_handle_read(void *buffer, size_t len_in_bytes)
{
	log_info("New API request `readFile`.");
	if (len_in_bytes < 8) {
		return;
	}
}

void
worker_handle_write_files(void *buffer, size_t len_in_bytes)
{
	log_info("New API request `readFile`.");
	if (len_in_bytes < 8) {
		return;
	}
}

void
worker_handle_lock(void *buffer, size_t len_in_bytes)
{
	log_info("New API request `lockFile`.");
	if (len_in_bytes < 8) {
		return;
	}
}

void
worker_handle_unlock(void *buffer, size_t len_in_bytes)
{
	log_info("New API request `unlockFile`.");
	if (len_in_bytes < 8) {
		return;
	}
}

void
worker_handle_remove(void *buffer, size_t len_in_bytes)
{
	log_info("New API request `removeFile`.");
	if (len_in_bytes < 8) {
		return;
	}
}

void
worker_parse_message(void *buffer, size_t len_in_bytes)
{
	if (len_in_bytes == 0) {
		return;
	}
	switch (((char *)(buffer))[0]) {
		case API_OP_READ_FILE:
			worker_handle_read(buffer, len_in_bytes);
		case API_OP_WRITE_FILE:
			worker_handle_write_files(buffer, len_in_bytes);
		case API_OP_UNLOCK_FILE:
			worker_handle_unlock(buffer, len_in_bytes);
		case API_OP_LOCK_FILE:
			worker_handle_lock(buffer, len_in_bytes);
		case API_OP_REMOVE_FILE:
			worker_handle_remove(buffer, len_in_bytes);
		default:
			log_error("Unrecognized request from client.");
	}
}

void *
worker_entry_point(void *args)
{
	UNUSED(args);
	unsigned worker_id = ts_counter();
	sem_t *sem = &workload_queue_get(worker_id)->sem;
	sem_wait(sem);
	return NULL;
}
