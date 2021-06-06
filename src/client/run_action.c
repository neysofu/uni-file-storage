#define _POSIX_C_SOURCE 200809L

#include "run_action.h"
#include "cli.h"
#include "logc/src/log.h"
#include "serverapi.h"
#include "serverapi_utils.h"
#include "utils.h"
#include <assert.h>
#include <dirent.h>
#include <libgen.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

int
run_action_lock_files(struct Action *action)
{
	char *path = strtok(action->arg_s1, ",");
	while (path) {
		log_info("Calling API function `lockFile`.");
		log_debug("Target file: '%s'.", path);
		int err = lockFile(path);
		if (err < 0) {
			log_error("API call failed.");
			return err;
		} else {
			log_info("API call was successful.");
		}
		path = strtok(NULL, ",");
	}
	return 0;
}

int
run_action_unlock_files(struct Action *action)
{
	char *path = strtok(action->arg_s1, ",");
	while (path) {
		log_info("Calling API function `unlockFile`.");
		log_debug("Target file: '%s'.", path);
		int err = unlockFile(path);
		if (err < 0) {
			log_error("API call failed.");
			return err;
		} else {
			log_info("API call was successful.");
		}
		path = strtok(NULL, ",");
	}
	return 0;
}

int
run_action_write_file(struct Action *action)
{
	char *path = strtok(action->arg_s1, ",");
	while (path) {
		log_info("Calling API function `writeFile`.");
		log_debug("Target file: '%s'.", path);
		if (false) {
			log_debug("Destination directory: '%s'.", NULL);
		} else {
			log_debug("No destination directory was specified.");
		}
		int err = writeFile(path, NULL);
		if (err < 0) {
			log_error("API call failed.");
			return err;
		} else {
			log_info("API call was successful.");
		}
		path = strtok(NULL, ",");
	}
	return 0;
}

int
run_action_write_dir(struct Action *action)
{
	char *dirname = strtok(action->arg_s1, ",");
	DIR *d;
	struct dirent *dir;
	d = opendir(dirname);
	log_debug("Listing directory '%s'.", dirname);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			char *dir_entry_name = dir->d_name;
			int err = writeFile(dir_entry_name, NULL);
		}
		closedir(d);
	}
	return 0;
}

int
run_action_remove_files(struct Action *action)
{
	char *path = strtok(action->arg_s1, ",");
	while (path) {
		log_debug("Target file: '%s'.", path);
		int err = removeFile(path);
		if (err < 0) {
			log_error("API call failed.");
			return err;
		} else {
			log_info("API call was successful.");
		}
		path = strtok(NULL, ",");
	}
	return 0;
}

int
run_action_read_files(struct Action *action)
{
	/* Open directory. */
	char *dirname = action->arg_s2;
	DIR *d;
	struct dirent *dir;
	d = opendir(dirname);
	/* Iterate through files. */
	char *pathname = strtok(action->arg_s1, ",");
	while (pathname) {
		char *filename = basename(pathname);
		FILE *location = fopen(filename, "wb");
		void *buffer = NULL;
		size_t buffer_size = 0;
		int err = readFile(pathname, &buffer, &buffer_size);
		fwrite(buffer, buffer_size, 1, location);
		if (err) {
			return err;
		}
		pathname = strtok(NULL, ",");
		fclose(location);
	}
	return 0;
}

int
run_action_read_random_files(struct Action *action)
{
	return readNFiles(action->arg_i, action->arg_s1);
}

int
run_action(struct Action *action)
{
	switch (action->type) {
		case ACTION_WRITE_FILES:
			return run_action_write_file(action);
		case ACTION_WRITE_DIR:
			return run_action_write_dir(action);
		case ACTION_READ_FILES:
			return run_action_read_files(action);
		case ACTION_READ_RANDOM_FILES:
			return run_action_read_random_files(action);
		case ACTION_WAIT_MILLISECONDS:
			log_debug("Waiting %d milliseconds.", action->arg_i);
			wait_msec(action->arg_i);
			return 0;
		case ACTION_LOCK_FILES:
			return run_action_lock_files(action);
		case ACTION_UNLOCK_FILES:
			return run_action_unlock_files(action);
		case ACTION_REMOVE_FILES:
			return run_action_remove_files(action);
		default:
			assert(false);
	}
	return 0;
}
