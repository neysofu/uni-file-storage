#define _POSIX_C_SOURCE 200809L
/* For `d_type` macros. */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include "run_action.h"
#include "cli.h"
#include "logc/src/log.h"
#include "serverapi.h"
#include "serverapi_utils.h"
#include "utils.h"
#include <assert.h>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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

int static write_file(const char *abspath)
{
	log_info("Calling API function `writeFile`.");
	log_debug("Target file: '%s'.", abspath);
	if (false) {
		log_debug("Destination directory: '%s'.", NULL);
	} else {
		log_debug("No destination directory was specified.");
	}
	int err = writeFile(abspath, NULL);
	if (err < 0) {
		log_error("API call failed.");
		return err;
	} else {
		log_info("API call was successful.");
	}
	return 0;
}

int
run_action_write_file(struct Action *action)
{
	char *path = strtok(action->arg_s1, ",");
	while (path) {
		write_file(path);
		path = strtok(NULL, ",");
	}
	return 0;
}

int static write_dir(const char *absdir, int *limit)
{
	DIR *d;
	struct dirent *dir;
	d = opendir(absdir);
	log_debug("Listing directory '%s'.", absdir);
	if (!d) {
		return -1;
	}
	while ((dir = readdir(d)) != NULL && (!limit || *limit > 0)) {
		log_trace("Scanning directory entry...");
		char *fullpath = xmalloc(strlen(absdir) + strlen("/") + strlen(dir->d_name) + 1);
		sprintf(fullpath, "%s/%s", absdir, dir->d_name);
		/* Check for the current and parent directory. See also
		 * <https://stackoverflow.com/a/20265398/5148606>. */
		if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
			continue;
		}
		if (dir->d_type == DT_DIR) {
			write_dir(fullpath, limit);
		} else if (dir->d_type == DT_REG) {
			/* Obviously, we must concatenate directory entries to the base path. */
			int err = writeFile(fullpath, NULL);
			if (err) {
				return -1;
			}
			if (limit) {
				*limit -= 1;
			}
		} else {
			return -1;
		}
	}
	log_trace("Closing directory '%s'.", absdir);
	closedir(d);
	return 0;
}

int
run_action_write_dir(struct Action *action)
{
	char *dirname = NULL;
	/* We must differentiate between relative and absolute directory path. */
	if (action->arg_s1[0] == '/') {
		dirname = strtok(action->arg_s1, ",");
	} else {
		dirname = xmalloc(PATH_MAX);
		if (!getcwd(dirname, PATH_MAX)) {
			return -1;
		}
		strcat(dirname, "/");
		strcat(dirname, action->arg_s1);
	}
	/* The max. number of files may or may not have a hard limit, depending on
	 * CLI arguments. */
	if (action->arg_i > 0) {
		return write_dir(dirname, &action->arg_i);
	} else {
		return write_dir(dirname, NULL);
	}
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
	closedir(d);
	return 0;
}

int
run_action_read_random_files(struct Action *action)
{
	const int n = action->arg_i;
	const char *destination_dir = action->arg_s2;
	return readNFiles(n, destination_dir);
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
			log_fatal("Unrecognized client action code. This is a bug.");
			assert(false);
	}
	return 0;
}
