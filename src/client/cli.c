#define _POSIX_C_SOURCE 200809L

#include "cli.h"
#include "err.h"
#include "logc/src/log.h"
#include "serverapi_actions.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define OPTSTRING "hf:w:n:W::D:r::Rd:t:l::u::c::p"

void
cli_args_add_action(struct CliArgs *cli_args, struct Action action)
{
	action.next = NULL;
	struct Action *next = malloc(sizeof(struct Action));
	if (!next) {
		cli_args->err = CLIENT_ERR_ALLOC;
	}
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
	if (cli_args->socket_name) {
		cli_args->err = CLIENT_ERR_REPEATED_F;
	} else {
		cli_args->socket_name = arg;
	}
}

void
cli_args_add_action_write_dir(struct CliArgs *cli_args, char *arg)
{
	char *comma = strrchr(arg, ',');
	char *equal_sign = strrchr(arg, '=');
	struct Action action;
	action.type = ACTION_WRITE_EVICTED;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	if (comma && equal_sign && equal_sign == comma + 2 && strncmp(comma + 1, "n", 1)) {
		comma[0] = '\0';
		action.arg_i = equal_sign + 1;
	}
	cli_args_add_action(cli_args, action);
}

void
cli_args_set_evicted_dir(struct CliArgs *cli_args, char *arg)
{
	struct Action action;
	action.type = ACTION_SET_EVICTED_DIR;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	cli_args_add_action(cli_args, action);
}

void
cli_args_add_action_write_files(struct CliArgs *cli_args, char *arg)
{
	struct Action action;
	action.type = ACTION_SEND;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	cli_args_add_action(cli_args, action);
}

void
cli_args_add_action_read(struct CliArgs *cli_args, char *arg)
{
	struct Action action;
	action.type = ACTION_READ;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	cli_args_add_action(cli_args, action);
}

void
cli_args_add_action_read_random(struct CliArgs *cli_args, char *arg)
{
	struct Action action;
	action.type = ACTION_READ_RANDOM;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	cli_args_add_action(cli_args, action);
}

void
cli_args_set_read_dir(struct CliArgs *cli_args, char *arg)
{
	struct Action action;
	action.type = ACTION_SET_READ_DIR;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	cli_args_add_action(cli_args, action);
}

void
cli_args_add_action_wait(struct CliArgs *cli_args, char *arg)
{
	struct Action action;
	action.type = ACTION_WAIT;
	action.arg_s = NULL;
	action.arg_i = atoi(arg);
	action.next = NULL;
	cli_args_add_action(cli_args, action);
}

void
cli_args_add_action_lock(struct CliArgs *cli_args, char *arg)
{
	struct Action action;
	action.type = ACTION_LOCK;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	cli_args_add_action(cli_args, action);
}

void
cli_args_add_action_unlock(struct CliArgs *cli_args, char *arg)
{
	struct Action action;
	action.type = ACTION_UNLOCK;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	cli_args_add_action(cli_args, action);
}

void
cli_args_add_action_remove(struct CliArgs *cli_args, char *arg)
{
	struct Action action;
	action.type = ACTION_REMOVE;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	cli_args_add_action(cli_args, action);
}

void
cli_args_enable_log(struct CliArgs *cli_args)
{
	if (cli_args->enable_log) {
		cli_args->err = CLIENT_ERR_REPEATED_P;
	} else {
		cli_args->enable_log = true;
	}
}

void
cli_args_enable_help_message(struct CliArgs *cli_args)
{
	if (cli_args->help_message) {
		cli_args->err = CLIENT_ERR_REPEATED_H;
	} else {
		cli_args->help_message = true;
	}
}

void
cli_args_parse_option(struct CliArgs *cli_args, const char option, char *arg)
{
	switch (option) {
		case 'h':
			cli_args_enable_help_message(cli_args);
			break;
		case 'f':
			cli_args_set_socket_name(cli_args, arg);
			break;
		case 'w':
			cli_args_add_action_write_dir(cli_args, arg);
			break;
		case 'W':
			cli_args_add_action_write_files(cli_args, arg);
			break;
		case 'D':
			cli_args_set_evicted_dir(cli_args, arg);
			break;
		case 'r':
			cli_args_add_action_read(cli_args, arg);
			break;
		case 'R':
			cli_args_add_action_read_random(cli_args, arg);
			break;
		case 'd':
			cli_args_set_read_dir(cli_args, arg);
			break;
		case 't':
			cli_args_add_action_wait(cli_args, arg);
			break;
		case 'l':
			cli_args_add_action_lock(cli_args, arg);
			break;
		case 'u':
			cli_args_add_action_unlock(cli_args, arg);
			break;
		case 'c':
			cli_args_add_action_remove(cli_args, arg);
			break;
		case 'p':
			cli_args_enable_log(cli_args);
			break;
		default:
			cli_args->err = CLIENT_ERR_UNKNOWN_OPTION;
	}
}

struct CliArgs *
cli_args_default(void)
{
	struct CliArgs *cli_args = malloc(sizeof(struct CliArgs));
	if (!cli_args) {
		return NULL;
	}
	cli_args->err = CLIENT_ERR_OK;
	cli_args->socket_name = NULL;
	cli_args->enable_log = false;
	cli_args->help_message = false;
	cli_args->head = NULL;
	cli_args->last = NULL;
	return cli_args;
}

struct CliArgs *
cli_args_parse(int argc, char **argv)
{
	struct CliArgs *cli_args = cli_args_default();
	if (!cli_args) {
		return NULL;
	}
	char c = '\0';
	while ((c = getopt(argc, argv, OPTSTRING)) != -1) {
		log_debug("Parsing command-line option '%c' with value '%s'",
		          c,
		          optarg ? optarg : "[NONE]");
		cli_args_parse_option(cli_args, c, optarg);
		if (cli_args->err) {
			log_error(
			  "Failure during command-line parsing (err. code: %d). Stopping immediately.",
			  cli_args->err);
			break;
		}
	}
	log_debug("Finished parsing command-line arguments.");
	return cli_args;
}
