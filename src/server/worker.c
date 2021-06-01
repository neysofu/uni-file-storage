#define _POSIX_C_SOURCE 200809L

#include "worker.h"
#include "global_state.h"
#include "htable.h"
#include "logc/src/log.h"
#include "serverapi_utils.h"
#include "shutdown.h"
#include "ts_counter.h"
#include "utils.h"
#include "workload_queue.h"
#include <assert.h>
#include <errno.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* Worker ID tracker. */
struct Worker
{
	unsigned id;
};

void
worker_handle_read(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	glog_info("[Worker n.%u] New API request `readFile`.", worker->id);
	struct File *file = htable_fetch_file(htable, buffer);
	char response[1 + 8] = { RESPONSE_OK };
	u64_to_big_endian(file->length_in_bytes, &response[1]);
	writen(fd, response, 9);
	writen(fd, file->contents, file->length_in_bytes);
	htable_release(htable, buffer);
}

void
worker_handle_write_file(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	glog_info("[Worker n.%u] New API request `writeFile`.", worker->id);
	char response[1] = { RESPONSE_OK };
	int err = writen(fd, response, 1);
	UNUSED(err);
}

void
worker_handle_lock(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	glog_info("[Worker n.%u] New API request `lockFile`.", worker->id);
	char *path = buf_to_str(buffer, len_in_bytes);
	htable_lock_file(htable, path);
	char response[1] = { RESPONSE_OK };
	int err = write(fd, response, 1);
	free(path);
	UNUSED(err);
}

void
worker_handle_unlock(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	glog_info("[Worker n.%u] New API request `unlockFile`.", worker->id);
	char *path = buf_to_str(buffer, len_in_bytes);
	char response[1];
	int result = htable_unlock_file(htable, path);
	if (result < 0) {
		response[0] = RESPONSE_ERR;
	} else {
		response[0] = RESPONSE_OK;
	}
	int err = write(fd, response, 1);
	free(path);
	UNUSED(err);
}

void
worker_handle_remove(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	glog_info("[Worker n.%u] New API request `removeFile`.", worker->id);
	char *path = buf_to_str(buffer, len_in_bytes);
	char response[1];
	int result = htable_remove_file(htable, path);
	if (result < 0) {
		response[0] = RESPONSE_ERR;
	} else {
		response[0] = RESPONSE_OK;
	}
	int err = write(fd, response, 1);
	free(path);
	UNUSED(err);
}

void
worker_handle_message(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	/* Validate the header, which contains the length of the payload in bytes. */
	assert(big_endian_to_u64(buffer) == len_in_bytes - 8);
	if (len_in_bytes == 8) {
		return;
	}
	glog_debug(
	  "[Worker n.%u] The latest message is %zu bytes long.", worker->id, len_in_bytes);
	/* Remove header details from the buffer. */
	char op = ((char *)(buffer))[8];
	buffer = &((char *)(buffer))[9];
	len_in_bytes = len_in_bytes - 9;
	switch (op) {
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
			glog_error("Unrecognized request from client.");
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
	while (!detect_shutdown_hard()) {
		struct timespec deadline;
		clock_gettime(CLOCK_REALTIME, &deadline);
		deadline.tv_sec += 1;
		sem_timedwait(sem, &deadline);
		if (errno == ETIMEDOUT) {
			continue;
		}
		glog_debug("[Worker n.%u] New message incoming. Pulling...", id);
		struct Message *msg = workload_queue_pull(id);
		glog_debug("[Worker n.%u] Done.", id);
		worker_handle_message(&worker, msg->fd, msg->buffer.raw, msg->buffer.size_in_bytes);
	}
	return NULL;
}
