#define _POSIX_C_SOURCE 200809L

#include "worker.h"
#include "global_state.h"
#include "htable.h"
#include "logc/src/log.h"
#include "serverapi.h"
#include "utilities.h"
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

static unsigned workers_count = 0;
static pthread_t *workers = NULL;

/* Worker ID tracker. */
struct Worker
{
	unsigned id;
};

#define LOG_IO_ERR(worker, err)                                                            \
	glog_error(                                                                            \
	  "[Worker n.%u] Bad I/O during request handling (error: %d, errno: %d). Ignoring.",   \
	  worker->id,                                                                          \
	  err,                                                                                 \
	  errno)

static void
free_files(struct File *files, unsigned int size)
{
	for (size_t i = 0; i < size; i++) {
		free(files[i].key);
		free(files[i].contents);
	}
}

static void
write_response_byte(struct Worker *worker, int fd, int result)
{
	char response[1];
	if (result == 0) {
		response[0] = RESPONSE_OK;
	} else {
		response[0] = RESPONSE_ERR;
	}
	int err = write(fd, response, 1);
	if (err < 0) {
		LOG_IO_ERR(worker, err);
	}
}

static void
worker_handle_read_file(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	glog_info("[Worker n.%u] New API request `readFile`.", worker->id);
	char *path = buf_to_str(buffer, len_in_bytes);

	struct File *file = htable_fetch_file(global_htable, path);
	if (!file) {
		write_response_byte(worker, fd, -1);
		return;
	}

	glog_debug("[Worker n.%u] This read operation consists of %zu bytes.",
	           worker->id,
	           file->length_in_bytes);

	uint8_t response[9] = { RESPONSE_OK };
	u64_to_big_endian(file->length_in_bytes, &response[1]);
	int err = 0;
	err |= write_bytes(fd, response, 9);
	err |= write_bytes(fd, file->contents, file->length_in_bytes);
	if (err < 0) {
		LOG_IO_ERR(worker, err);
	}
	htable_release_file(global_htable, path);
	free(path);
}

static void
worker_handle_read_n_files(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	int err = 0;
	glog_info("[Worker n.%u] New API request `readNFiles`.", worker->id);
	if (len_in_bytes != 8) {
		glog_error("[Worker n.%u] Bad message format.", worker->id);
		return;
	}
	uint64_t n = big_endian_to_u64(buffer);
	struct HTableVisitor *visitor = htable_visit(global_htable, n);
	struct File *file = NULL;
	while ((file = htable_visitor_next(visitor))) {
		glog_debug("[Worker n.%u] Sending '%s' to client.", worker->id, file->key);
		glog_debug("[Worker n.%u] This read operation consists of %zu bytes.",
		           worker->id,
		           file->length_in_bytes);

		uint8_t buf1[8] = { 0 };
		uint8_t buf2[8] = { 0 };
		u64_to_big_endian(strlen(file->key), buf1);
		u64_to_big_endian(file->length_in_bytes, buf2);
		err |= write_bytes(fd, buf1, 8);
		err |= write_bytes(fd, buf2, 8);
		err |= write_bytes(fd, file->key, strlen(file->key));
		err |= write_bytes(fd, file->contents, file->length_in_bytes);
		if (err < 0) {
			LOG_IO_ERR(worker, err);
			break;
		}
	}
	uint8_t buf[8] = { 0 };
	err |= write_bytes(fd, buf, 8);
	if (err < 0) {
		LOG_IO_ERR(worker, err);
	}
	htable_visitor_free(visitor);
}

static void
worker_handle_write_file(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	glog_debug("[Worker n.%u] New API request `writeFile`.", worker->id);
	if (len_in_bytes < 8 + 8) {
		glog_error("[Worker n.%u] Bad message format.", worker->id);
		return;
	}
	uint64_t arg1_size = big_endian_to_u64(buffer);
	uint64_t arg2_size = big_endian_to_u64((uint8_t *)buffer + 8);
	if (len_in_bytes != 8 * 2 + arg1_size + arg2_size) {
		glog_error("[Worker n.%u] Bad message format.", worker->id);
		return;
	}
	char *path = buf_to_str((uint8_t *)(buffer) + 16, arg1_size);
	glog_debug("[Worker n.%u] The path is '%s', file size is %lu bytes.",
	           worker->id,
	           path,
	           arg2_size);
	glog_debug(
	  "[Worker n.%u] This write operation consists of %zu bytes.", worker->id, arg2_size);
	struct File *evicted = NULL;
	unsigned evicted_count = 0;
	glog_debug("[Worker n.%u] Successfully parsed the latest message.", worker->id);
	void *arg2_buffer = (char *)buffer + 8 * 2 + arg1_size;
	int err = htable_replace_file_contents(
	  global_htable, path, arg2_buffer, arg2_size, &evicted, &evicted_count);
	if (err != HTABLE_ERR_OK) {
		glog_error(
		  "[Worker n.%u] Last operation failed with err code %d.", worker->id, err);
	}
	glog_debug("[Worker n.%u] Last operation evicted %u files.", worker->id, evicted_count);
	uint8_t buf_response_code[1] = { RESPONSE_OK };
	uint8_t buf[8] = { 0 };
	u64_to_big_endian(evicted_count, buf);
	err |= write_bytes(fd, buf_response_code, 1);
	err |= write_bytes(fd, buf, 8);
	if (err < 0) {
		free(path);
		free_files(evicted, evicted_count);
		LOG_IO_ERR(worker, err);
		return;
	}
	glog_trace("[Worker n.%u] Sending over %u evicted files.", worker->id, evicted_count);
	for (size_t i = 0; i < evicted_count; i++) {
		glog_trace(
		  "[Worker n.%u] Sending over the evicted file '%s'.", worker->id, evicted[i].key);
		uint8_t buf_arg1_size[8];
		uint8_t buf_arg2_size[8];
		u64_to_big_endian(strlen(evicted[i].key), buf_arg1_size);
		u64_to_big_endian(evicted[i].length_in_bytes, buf_arg2_size);
		err |= write_bytes(fd, buf_arg1_size, 8);
		err |= write_bytes(fd, buf_arg2_size, 8);
		err |= write_bytes(fd, evicted[i].key, strlen(evicted[i].key));
		err |= write_bytes(fd, evicted[i].contents, evicted[i].length_in_bytes);
		if (err < 0) {
			free(path);
			free_files(evicted, evicted_count);
			LOG_IO_ERR(worker, err);
			return;
		}
	}
	free(path);
	free_files(evicted, evicted_count);
}

static void
worker_handle_open_file(struct Worker *worker,
                        int fd,
                        void *buffer,
                        size_t len_in_bytes,
                        bool create,
                        bool lock)
{
	glog_debug("[Worker n.%u] New API request `openFile`, create = %d, lock = %d.",
	           worker->id,
	           create,
	           lock);
	char *path = buf_to_str(buffer, len_in_bytes);
	glog_debug("[Worker n.%u] The path is '%s'.", worker->id, path);
	int result = htable_open_or_create_file(global_htable, path, fd, create, lock);
	free(path);
	write_response_byte(worker, fd, result);
}

static void
worker_handle_lock_file(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	glog_debug("[Worker n.%u] New API request `lockFile`.", worker->id);
	char *path = buf_to_str(buffer, len_in_bytes);
	char response[1];
	int result = htable_lock_file(global_htable, path, fd);
	if (result < 0) {
		response[0] = RESPONSE_ERR;
	} else {
		response[0] = RESPONSE_OK;
	}
	int err = write(fd, response, 1);
	if (err < 0) {
		LOG_IO_ERR(worker, err);
	}
	free(path);
}

static void
worker_handle_unlock_file(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	glog_debug("[Worker n.%u] New API request `unlockFile`.", worker->id);
	char *path = buf_to_str(buffer, len_in_bytes);
	int result = htable_unlock_file(global_htable, path, fd);
	free(path);
	write_response_byte(worker, fd, result);
}

static void
worker_handle_close_file(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	glog_debug("[Worker n.%u] New API request `closeFile`.", worker->id);
	char *path = buf_to_str(buffer, len_in_bytes);
	int result = htable_close_file(global_htable, path, fd);
	free(path);
	write_response_byte(worker, fd, result);
}

static void
worker_handle_remove_file(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	glog_debug("[Worker n.%u] New API request `removeFile`.", worker->id);
	char *path = buf_to_str(buffer, len_in_bytes);
	int result = htable_unlock_file(global_htable, path, fd);
	free(path);
	write_response_byte(worker, fd, result);
}

static void
worker_handle_message(struct Worker *worker, int fd, void *buffer, size_t len_in_bytes)
{
	/* Validate the header, which contains the length of the payload in bytes. */
	if (big_endian_to_u64(buffer) == len_in_bytes - 8) {
		glog_fatal("[Worker n.%u] The message header is invalid. Message length is "
		           "expected to be %zu, but is now reported to be %zu.",
		           worker->id,
		           len_in_bytes,
		           big_endian_to_u64(buffer) + 8);
	}
	/* Check if the buffer only has a header, i.e. it is empty. */
	if (len_in_bytes == 8) {
		return;
	}
	/* Read header details from the buffer. */
	char op = ((char *)(buffer))[8];
	glog_trace("[Worker n.%u] The latest message is %zu bytes long, with opcode %d.",
	           worker->id,
	           len_in_bytes,
	           (int)op);
	buffer = &((char *)(buffer))[9];
	len_in_bytes = len_in_bytes - 9;
	switch (op) {
		case API_OP_READ_FILE:
			worker_handle_read_file(worker, fd, buffer, len_in_bytes);
			break;
		case API_OP_READ_N_FILES:
			worker_handle_read_n_files(worker, fd, buffer, len_in_bytes);
			break;
		case API_OP_OPEN_FILE:
			worker_handle_open_file(worker, fd, buffer, len_in_bytes, false, false);
			break;
		case API_OP_OPEN_FILE_CREATE:
			worker_handle_open_file(worker, fd, buffer, len_in_bytes, true, false);
			break;
		case API_OP_OPEN_FILE_LOCK:
			worker_handle_open_file(worker, fd, buffer, len_in_bytes, false, true);
			break;
		case API_OP_OPEN_FILE_CREATE_LOCK:
			worker_handle_open_file(worker, fd, buffer, len_in_bytes, true, true);
			break;
		case API_OP_WRITE_FILE:
			worker_handle_write_file(worker, fd, buffer, len_in_bytes);
			break;
		case API_OP_UNLOCK_FILE:
			worker_handle_unlock_file(worker, fd, buffer, len_in_bytes);
			break;
		case API_OP_LOCK_FILE:
			worker_handle_lock_file(worker, fd, buffer, len_in_bytes);
			break;
		case API_OP_CLOSE_FILE:
			worker_handle_close_file(worker, fd, buffer, len_in_bytes);
			break;
		case API_OP_REMOVE_FILE:
			worker_handle_remove_file(worker, fd, buffer, len_in_bytes);
			break;
		default:
			glog_error("[Worker n.%u] Unrecognized request from client.", worker->id);
	}
	glog_trace("[Worker n.%u] Finished handling request.", worker->id);
}

void *
worker_entry_point(void *args)
{
	UNUSED(args);
	/* Wait a bit so that everything is ready. For more serious, production-ready
	 * software, this shuould be replaced with a synchronization mechanism, but
	 * it's overkill for this. */
	wait_msec(250);
	unsigned id = ts_counter();
	struct Worker worker;
	worker.id = id;
	while (true) {
		struct Message *msg = workload_queue_pull(id);
		if (!msg) {
			/* Shutdown! */
			break;
		}
		glog_trace("[Worker n.%u] New message incoming.", id);
		worker_handle_message(&worker, msg->fd, msg->buffer.raw, msg->buffer.size_in_bytes);
		free(msg->buffer.raw);
		free(msg);
	}
	glog_info("[Worker n.%u] Exiting thread.", id);
	pthread_exit(NULL);
	return NULL;
}

void
workers_spawn(unsigned num)
{
	workers_count = num;
	workers = xmalloc(num * sizeof(pthread_t));
	for (unsigned i = 0; i < num; i++) {
		int err = pthread_create(&workers[i], NULL, worker_entry_point, NULL);
		if (err) {
			glog_fatal("Unexpected `pthread_create` error code %d when spawning workers.",
			           err);
			exit(EXIT_FAILURE);
		}
	}
}

void
workers_join(void)
{
	workload_queues_cond_signal();

	for (unsigned i = 0; i < workers_count; i++) {
		int err = pthread_join(workers[i], NULL);
		if (err) {
			glog_fatal("Unexpected error code %d while shutting down worker n.%u.", err, i);
			exit(EXIT_FAILURE);
		}
	}
	free(workers);
}
