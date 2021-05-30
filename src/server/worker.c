#include "worker.h"
#include "global_state.h"
#include "logc/src/log.h"
#include "serverapi_actions.h"
#include "ts_counter.h"
#include "utils.h"
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
worker_handle_send(void *buffer, size_t len_in_bytes)
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
		case ACTION_READ:
			worker_handle_read(buffer, len_in_bytes);
		case ACTION_SEND:
			worker_handle_send(buffer, len_in_bytes);
		case ACTION_UNLOCK:
			worker_handle_unlock(buffer, len_in_bytes);
		case ACTION_LOCK:
			worker_handle_lock(buffer, len_in_bytes);
		case ACTION_REMOVE:
			worker_handle_remove(buffer, len_in_bytes);
		case ACTION_SET_READ_DIR:
		case ACTION_SET_EVICTED_DIR:
		default:
			log_error("Unrecognized request from client.");
	}
}

void *
worker_entry_point(void *args)
{
	UNUSED(args);
	unsigned worker_id = ts_counter();
	int socket_desc = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_desc < 0) {
		printf("Socket operation failed. Abort.\n");
	}
	return NULL;
}
