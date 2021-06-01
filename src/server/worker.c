#include "worker.h"
#include "htable.h"
#include "global_state.h"
#include "logc/src/log.h"
#include "serverapi_utils.h"
#include "ts_counter.h"
#include "utils.h"
#include "workload_queue.h"
#include <assert.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

/* Worker ID tracker. */
struct Worker
{
	unsigned id;
};

void
worker_handle_read(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	log_info("[Worker n. %u] New API request `readFile`.", worker->id);
	struct File *file = htable_fetch_file(htable, buffer);
	char response[1+8] = { RESPONSE_OK };
	u64_to_big_endian(file->length_in_bytes, &response[1]);
	writen(fd, response, 9);
	writen(fd, file->contents, file->length_in_bytes);
	htable_release(htable, buffer);
}

void
worker_handle_write_file(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	log_info("[Worker n. %u] New API request `writeFile`.", worker->id);
	char response[1] = { RESPONSE_OK };
	writen(fd, response, 1);
}

void
worker_handle_lock(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	log_info("[Worker n. %u] New API request `lockFile`.", worker->id);
	htable_lock_file(htable, buffer);
	char response[1] = { RESPONSE_OK };
	int err = write(fd, response, 1);
}

void
worker_handle_unlock(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	log_info("[Worker n. %u] New API request `unlockFile`.", worker->id);
	htable_unlock_file(htable, buffer);
	char response[1] = { RESPONSE_OK };
	int err = write(fd, response, 1);
}

void
worker_handle_remove(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	log_info("[Worker n. %u] New API request `removeFile`.", worker->id);
	htable_remove_file(htable, buffer);
	char response[1] = { RESPONSE_OK };
	int err = write(fd, response, 1);
}

void
worker_handle_message(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	/* Validate the header, which contains the length of the payload in bytes. */
	assert(big_endian_to_u64(buffer) == len_in_bytes - 8);
	if (len_in_bytes == 8) {
		return;
	}
	log_debug("[Worker n. %u] The latest message is %zu bytes long.",
	          worker->id,
	          len_in_bytes);
	switch (((char *)(buffer))[8]) {
		case API_OP_READ_FILE:
			worker_handle_read(worker, fd, buffer, len_in_bytes);
			break;
		case API_OP_WRITE_FILE:
			worker_handle_write_file(worker, fd, buffer, len_in_bytes);
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
	unsigned id = ts_counter();
	struct Worker worker;
	worker.id = id;
	sem_t *sem = &workload_queue_get(id)->sem;
	while (1) {
		sem_wait(sem);
		log_debug("[Worker n. %u] New message incoming. Pulling...", id);
		struct Message *msg = workload_queue_pull(id);
		log_debug("[Worker n. %u] Done.", id);
		worker_handle_message(&worker, msg->fd, msg->buffer.raw, msg->buffer.size_in_bytes);
	}
	return NULL;
}
