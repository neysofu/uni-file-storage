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

struct Worker
{
	unsigned worker_id;
};

void
worker_handle_read(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	log_info("New API request `readFile`.");
	if (len_in_bytes < 8) {
		return;
	}
}

void
worker_handle_write_files(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	log_info("New API request `readFile`.");
	if (len_in_bytes < 8) {
		return;
	}
	htable_insert(htable, "");
}

void
worker_handle_lock(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	log_info("New API request `lockFile`.");
	if (len_in_bytes < 8) {
		return;
	}
}

void
worker_handle_unlock(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	log_info("New API request `unlockFile`.");
	if (len_in_bytes < 8) {
		return;
	}
}

void
worker_handle_remove(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	log_info("New API request `removeFile`.");
	if (len_in_bytes < 8) {
		return;
	}
}

void
worker_handle_message(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	/* Please note that workers receive messages WITH their length-prefix
	 * header (8 bytes). */
	if (len_in_bytes == 8) {
		return;
	}
	log_debug("[Worker n. %u] The latest message is %zu bytes long.",
	          worker->worker_id,
	          len_in_bytes);
	switch (((char *)(buffer))[8]) {
		case API_OP_READ_FILE:
			worker_handle_read(worker, fd, buffer, len_in_bytes);
			break;
		case API_OP_WRITE_FILE:
			worker_handle_write_files(worker, fd, buffer, len_in_bytes);
			break;
		case API_OP_UNLOCK_FILE:
			worker_handle_unlock(worker, fd, buffer, len_in_bytes);
			break;
		case API_OP_LOCK_FILE:
			worker_handle_lock(worker, fd, buffer, len_in_bytes);
			break;
		case API_OP_REMOVE_FILE:
			worker_handle_remove(worker, fd, buffer, len_in_bytes);
			break;
		default:
			log_error("Unrecognized request from client.");
	}
}

void *
worker_entry_point(void *args)
{
	UNUSED(args);
	unsigned worker_id = ts_counter();
	struct Worker worker;
	worker.worker_id = worker_id;
	sem_t *sem = &workload_queue_get(worker_id)->sem;
	while (1) {
		sem_wait(sem);
		log_debug("[Worker n. %u] New message incoming. Pulling...", worker_id);
		struct Message *msg = workload_queue_pull(worker_id);
		log_debug("[Worker n. %u] Done.", worker_id);
		worker_handle_message(&worker, msg->fd, msg->buffer.raw, msg->buffer.size_in_bytes);
	}
	return NULL;
}
