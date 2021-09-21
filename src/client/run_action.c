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
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int
run_some_action_over_list_of_files(struct Action *action,
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

static int
run_action_write_file(const char *relative_filepath)
{
	char *filepath = realpath(relative_filepath, NULL);
	if (!filepath) {
		return -1;
	}

	log_info("Calling API function `openFile`.");

	int err = 0;
	err = openFile(filepath, O_CREATE | O_LOCK);
	if (err < 0) {
		log_error("`openFile` failed.");
		return err;
	}

	log_info("Calling API function `writeFile`.");
	log_debug("Target file: '%s'.", filepath);
	if (false) {
		log_debug("Destination directory: '%s'.", NULL);
	} else {
		log_debug("No destination directory was specified.");
	}

	err = writeFile(filepath, NULL);
	if (err < 0) {
		log_error("`writeFile` failed.");
		return err;
	}

	log_info("Calling API function `closeFile`.");
	err = closeFile(filepath);
	if (err < 0) {
		log_error("`closeFile` failed.");
		return err;
	}

	return 0;
}

static int
run_action_write_list_of_files(struct Action *action)
{
	/* This might cause issue if the filepaths contain commas, but there's not
	 * much we can do about that. */
	char *path = strtok(action->arg_s1, ",");
	while (path) {
		int err = run_action_write_file(path);
		if (err) {
			return err;
		}

		path = strtok(NULL, ",");
	}
	return 0;
}

static int
write_dir(const char *absdir, int *limit)
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

static int
run_action_write_files_in_dir(struct Action *action)
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

static char *
basename(char const *filepath)
{
	char *s = strrchr(filepath, '/');
	if (!s) {
		return strdup(filepath);
	} else {
		return strdup(s + 1);
	}
}

static int
write_file_to_dir(const void *buffer,
                  size_t buffer_size,
                  const char *dir,
                  const char *original_filepath)
{
	char *base = basename(original_filepath);
	if (!base) {
		return -1;
	}
	char *filepath = xmalloc(strlen(dir) + 1 + strlen(base) + 1);
	strcpy(filepath, dir);
	strcpy(filepath + strlen(dir), "/");
	strcpy(filepath + strlen(dir) + 1, base);

	FILE *f = fopen(filepath, "wb");
	if (!f) {
		free(filepath);
		return -1;
	}
	fwrite(buffer, buffer_size, 1, f);
	fclose(f);
	free(filepath);
	return 0;
}

static int
run_action_read_list_of_files(struct Action *action)
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
		void *buffer = NULL;
		size_t buffer_size = 0;
		int err = readFile(filepath, &buffer, &buffer_size);
		if (err) {
			return err;
		}
		write_file_to_dir(buffer, buffer_size, dir_name, filepath);
		rel_filepath = strtok(NULL, ",");
	}
	if (d) {
		closedir(d);
	}
	return 0;
}

static int
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
			return run_action_write_files_in_dir(action);
		case 'W':
			return run_action_write_list_of_files(action);
		case 'r':
			return run_action_read_list_of_files(action);
		case 'R':
			return run_action_read_random_files(action);
		case 'l':
			return run_some_action_over_list_of_files(action, lockFile, "lockFile");
		case 'u':
			return run_some_action_over_list_of_files(action, unlockFile, "unlockFile");
		case 'c':
			return run_some_action_over_list_of_files(action, removeFile, "removeFile");
		case 't':
			log_debug("Waiting %d milliseconds.", action->arg_i);
			wait_msec(action->arg_i);
			return 0;
		default:
			log_fatal("Unrecognized client action symbol. This is a bug.");
			assert(false);
	}
	return 0;
}
