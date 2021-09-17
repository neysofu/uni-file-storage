#define _POSIX_C_SOURCE 200809L

#include "cli.h"
#include "logc/src/log.h"
#include "serverapi_utils.h"
#include "utils.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define OPTSTRING "hf:w:n:W:D:r:Rd:t:l:u:c:pZ:z:"

void
cli_args_add_action(struct CliArgs *cli_args, struct Action action)
{
	assert(cli_args);
	action.next = NULL;
	struct Action *next = xmalloc(sizeof(struct Action));
	*next = action;
	if (cli_args->last) {
		cli_args->last->next = next;
	} else {
		cli_args->head = next;
	}
	cli_args->last = next;
}

void
cli_args_set_socket_name(struct CliArgs *cli_args, char *arg)
{
	assert(cli_args);
	if (cli_args->socket_name) {
		cli_args->err = CLIENT_ERR_REPEATED_F;
	} else {
		cli_args->socket_name = arg;
	}
}

void
cli_args_add_action_write_dir(struct CliArgs *cli_args, char *arg)
{
	assert(cli_args);
	char *comma = strrchr(arg, ',');
	char *equal_sign = strrchr(arg, '=');
	struct Action action;
	action.type = 'W';
	action.arg_s1 = arg;
	action.arg_s2 = NULL;
	action.arg_i = 0;
	action.next = NULL;
	if (comma && equal_sign && equal_sign == comma + 2 && strncmp(comma + 1, "n", 1)) {
		comma[0] = '\0';
		action.arg_i = atoi(equal_sign + 1);
	}
	cli_args_add_action(cli_args, action);
}

void
cli_args_set_evicted_dir(struct CliArgs *cli_args, char *arg)
{
	assert(cli_args);
	if (!arg) {
		cli_args->err = CLIENT_ERR_MISSING_ARG;
		return;
	}
	cli_args->last->arg_s2 = arg;
}

void
cli_args_add_action_write_files(struct CliArgs *cli_args, char *arg)
{
	assert(cli_args);
	if (!arg) {
		cli_args->err = CLIENT_ERR_MISSING_ARG;
		return;
	}
	struct Action action;
	action.type = 'w';
	action.arg_s1 = arg;
	action.arg_s2 = NULL;
	action.arg_i = 0;
	action.next = NULL;
	cli_args_add_action(cli_args, action);
}

void
cli_args_add_action_read(struct CliArgs *cli_args, char *arg)
{
	assert(cli_args);
	if (!arg) {
		cli_args->err = CLIENT_ERR_MISSING_ARG;
		return;
	}
	struct Action action;
	action.type = 'r';
	action.arg_s1 = arg;
	action.arg_s2 = NULL;
	action.arg_i = 0;
	action.next = NULL;
	cli_args_add_action(cli_args, action);
}

void
cli_args_add_action_read_random(struct CliArgs *cli_args, char *arg)
{
	assert(cli_args);
	struct Action action;
	action.type = 'R';
	action.arg_s1 = NULL;
	action.arg_s2 = NULL;
	action.arg_i = 0;
	action.next = NULL;
	if (arg) {
		if (strncmp(arg, "n=", 2) == 0) {
			action.arg_i = atoi(arg + 2);
		} else {
			cli_args->err = CLIENT_ERR_BAD_OPTION_CAP_R;
		}
	}
	cli_args_add_action(cli_args, action);
}

void
cli_args_set_read_dir(struct CliArgs *cli_args, char *arg)
{
	assert(cli_args);
	if (!arg) {
		cli_args->err = CLIENT_ERR_MISSING_ARG;
		return;
	}
	cli_args->last->arg_s2 = arg;
}

void
cli_args_add_action_wait(struct CliArgs *cli_args, char *arg)
{
	assert(cli_args);
	if (!arg) {
		cli_args->err = CLIENT_ERR_MISSING_ARG;
		return;
	}
	struct Action action;
	action.type = 't';
	action.arg_s1 = NULL;
	action.arg_s2 = NULL;
	action.arg_i = atoi(arg);
	action.next = NULL;
	cli_args_add_action(cli_args, action);
}

void
cli_args_add_action_lock(struct CliArgs *cli_args, char *arg)
{
	assert(cli_args);
	if (!arg) {
		cli_args->err = CLIENT_ERR_MISSING_ARG;
		return;
	}
	struct Action action;
	action.type = 'l';
	action.arg_s1 = arg;
	action.arg_s2 = NULL;
	action.arg_i = 0;
	action.next = NULL;
	cli_args_add_action(cli_args, action);
}

void
cli_args_add_action_unlock(struct CliArgs *cli_args, char *arg)
{
	assert(cli_args);
	if (!arg) {
		cli_args->err = CLIENT_ERR_MISSING_ARG;
		return;
	}
	struct Action action;
	action.type = 'u';
	action.arg_s1 = arg;
	action.arg_s2 = NULL;
	action.arg_i = 0;
	action.next = NULL;
	cli_args_add_action(cli_args, action);
}

void
cli_args_add_action_remove(struct CliArgs *cli_args, char *arg)
{
	assert(cli_args);
	if (!arg) {
		cli_args->err = CLIENT_ERR_MISSING_ARG;
		return;
	}
	struct Action action;
	action.type = 'c';
	action.arg_s1 = arg;
	action.arg_s2 = NULL;
	action.arg_i = 0;
	action.next = NULL;
	cli_args_add_action(cli_args, action);
}

void
cli_args_enable_log(struct CliArgs *cli_args, char *arg)
{
	assert(cli_args);
	if (!arg || strcmp(arg, "info") == 0) {
		cli_args->log_level = LOG_INFO;
	} else if (strcmp(arg, "debug") == 0) {
		cli_args->log_level = LOG_DEBUG;
	} else if (strcmp(arg, "trace") == 0) {
		cli_args->log_level = LOG_TRACE;
	} else {
		cli_args->err = CLIENT_ERR_BAD_OPTION_P;
	}
}

void
cli_args_enable_help_message(struct CliArgs *cli_args)
{
	assert(cli_args);
	if (cli_args->help_message) {
		cli_args->err = CLIENT_ERR_REPEATED_H;
	} else {
		cli_args->help_message = true;
	}
}

void
cli_args_set_msec_between_connection_attempts(struct CliArgs *cli_args, char *arg)
{
	assert(cli_args);
	cli_args->msec_between_connection_attempts = atoi(arg);
}

void
cli_args_set_max_attempt_time(struct CliArgs *cli_args, char *arg)
{
	assert(cli_args);
	cli_args->sec_max_attempt_time = atoi(arg);
}

struct CliArgs *
cli_args_default(void)
{
	struct CliArgs *cli_args = xmalloc(sizeof(struct CliArgs));
	cli_args->err = CLIENT_ERR_OK;
	cli_args->socket_name = NULL;
	/* Disable logs by default. */
	cli_args->log_level = 1000;
	cli_args->help_message = false;
	cli_args->msec_between_connection_attempts = 300;
	cli_args->sec_max_attempt_time = 5;
	cli_args->head = NULL;
	cli_args->last = NULL;
	return cli_args;
}

struct CliArgs *
cli_args_parse(int argc, char **argv)
{
	struct CliArgs *cli_args = cli_args_default();
	char c = '\0';
	char last_option = '\0';
	while ((c = getopt(argc, argv, OPTSTRING)) != -1) {
		log_debug("Parsing command-line option '%c' with value '%s'",
		          c,
		          optarg ? optarg : "[NONE]");
		switch (c) {
			case 'Z':
				cli_args_set_msec_between_connection_attempts(cli_args, optarg);
				break;
			case 'z':
				cli_args_set_max_attempt_time(cli_args, optarg);
				break;
			case 'h':
				cli_args_enable_help_message(cli_args);
				break;
			case 'f':
				cli_args_set_socket_name(cli_args, optarg);
				break;
			case 'w':
				cli_args_add_action_write_dir(cli_args, optarg);
				break;
			case 'W':
				cli_args_add_action_write_files(cli_args, optarg);
				break;
			case 'D':
				if (last_option == 'w' || last_option == 'W') {
					cli_args_set_evicted_dir(cli_args, optarg);
				} else {
					cli_args->err = CLIENT_ERR_BAD_OPTION_CAP_D;
					break;
				}
				break;
			case 'r':
				cli_args_add_action_read(cli_args, optarg);
				break;
			case 'R':
				cli_args_add_action_read_random(cli_args, optarg);
				break;
			case 'd':
				if (last_option == 'r' || last_option == 'R') {
					cli_args_set_read_dir(cli_args, optarg);
				} else {
					cli_args->err = CLIENT_ERR_BAD_OPTION_D;
					break;
				}
				break;
			case 't':
				cli_args_add_action_wait(cli_args, optarg);
				break;
			case 'l':
				cli_args_add_action_lock(cli_args, optarg);
				break;
			case 'u':
				cli_args_add_action_unlock(cli_args, optarg);
				break;
			case 'c':
				cli_args_add_action_remove(cli_args, optarg);
				break;
			case 'p':
				cli_args_enable_log(cli_args, optarg);
				break;
			default:
				cli_args->err = CLIENT_ERR_UNKNOWN_OPTION;
		}
		last_option = c;
		if (cli_args->err) {
			break;
		}
	}
	log_debug("Finished parsing command-line arguments.");
	return cli_args;
}

void
cli_args_free(struct CliArgs *cli_args)
{
	if (!cli_args) {
		return;
	}
	struct Action *action = cli_args->head;
	while (action) {
		// We don't free the string action argument because it comes from `argv`.
		struct Action *to_free = action;
		action = action->next;
		free(to_free);
	}
	free(cli_args);
}
