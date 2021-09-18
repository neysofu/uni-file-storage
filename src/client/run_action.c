#define _POSIX_C_SOURCE 200809L

/* For `d_type` macros. */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include "run_action.h"
#include "cli.h"
#include "logc/src/log.h"
#include "serverapi.h"
#include "serverapi_utilities.h"
#include "utilities.h"
#include <assert.h>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int
run_generic_action_over_files(struct Action *action,
                              int (*api_f)(const char *pathname),
                              const char *api_f_name)
{
	char *path = strtok(action->arg_s1, ",");
	while (path) {
		log_info("Calling API function `%s`.", api_f_name);
		log_debug("Target file: '%s'.", path);
		int err = api_f(path);
		if (err < 0) {
			log_error("API call failed.");
		} else {
			log_trace("API call was successful.");
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

	int err = 0;
	err = openFile(abspath, O_CREATE | O_LOCK);
	if (err < 0) {
		return err;
	}

	err = writeFile(abspath, NULL);
	if (err < 0) {
		return err;
	}

	return 0;
}

int
run_action_write_file(struct Action *action)
{
	/* This might cause issue if the filepaths contain commas, but there's not
	 * much we can do about that. */
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
run_action_read_files(struct Action *action)
{
	char *dir_name = NULL;
	DIR *d = NULL;
	/* Open the destination directory. */
	if (action->arg_s2) {
		dir_name = action->arg_s2;
		d = opendir(dir_name);
	}
	/* Iterate through files. */
	char *rel_filepath = strtok(action->arg_s1, ",");
	while (rel_filepath) {
		char *filepath = realpath(rel_filepath, NULL);
		if (!filepath) {
			log_error("`realpath` failed");
			break;
		}
		FILE *f = fopen(filepath, "wb");
		void *buffer = NULL;
		size_t buffer_size = 0;
		int err = readFile(filepath, &buffer, &buffer_size);
		if (err) {
			return err;
		}
		fwrite(buffer, buffer_size, 1, f);
		rel_filepath = strtok(NULL, ",");
		free(filepath);
		fclose(f);
	}
	if (d) {
		closedir(d);
	}
	return 0;
}

int
run_action_read_random_files(struct Action *action)
{
	log_info("Calling API function `readNFiles`.");
	const int n = action->arg_i;
	char *destination_dir = NULL;
	if (action->arg_s2) {
		destination_dir = realpath(action->arg_s2, NULL);
		if (!destination_dir) {
			log_error("`realpath` failed");
			return -1;
		}
		log_debug("Destination directory: '%s'.", destination_dir);
	} else {
		log_debug("No destination directory was specified.");
	}
	log_debug("The N parameter is set to %d.", n);
	int result = readNFiles(n, destination_dir);
	free(destination_dir);
	return result;
}

int
run_action(struct Action *action)
{
	assert(action);
	log_trace("Executing action from the '%c' flag.", action->type);
	switch (action->type) {
		case 'w':
			return run_action_write_file(action);
		case 'W':
			return run_action_write_dir(action);
		case 'r':
			return run_action_read_files(action);
		case 'R':
			return run_action_read_random_files(action);
		case 't':
			log_debug("Waiting %d milliseconds.", action->arg_i);
			wait_msec(action->arg_i);
			return 0;
		case 'l':
			return run_generic_action_over_files(action, lockFile, "lockFile");
		case 'u':
			return run_generic_action_over_files(action, unlockFile, "unlockFile");
		case 'c':
			return run_generic_action_over_files(action, removeFile, "removeFile");
		default:
			log_fatal("Unrecognized client action symbol. This is a bug.");
			assert(false);
	}
	return 0;
}
