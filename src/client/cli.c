#define _POSIX_C_SOURCE 200809L

#include "cli.h"
#include "err.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define OPTSTRING "hf:w:n:W::D:r::Rd:t:l::u::c::p"

struct Config *
config_default(void)
{
	struct Config *config = malloc(sizeof(struct Config));
	if (!config) {
		return NULL;
	}
	config->err = CLIENT_ERR_OK;
	config->socket_name = NULL;
	config->enable_log = false;
	config->help_message = false;
	config->head = NULL;
	config->last = NULL;
	return config;
}

void
config_add_action(struct Config *config, struct Action action)
{
	action.next = NULL;
	struct Action *next = malloc(sizeof(struct Action));
	if (!next) {
		config->err = CLIENT_ERR_ALLOC;
	}
	*next = action;
	if (config->last) {
		config->last->next = next;
	} else {
		config->head = next;
	}
	config->last = next;
}

void
config_push_w(struct Config *config, char *arg)
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
	config_add_action(config, action);
}

void
config_push_capd(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_SEND_DIR_CONTENTS;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	config_add_action(config, action);
}

void
config_push_capw(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_SEND;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	config_add_action(config, action);
}

void
config_push_f(struct Config *config, char *arg)
{
	if (config->socket_name) {
		config->err = CLIENT_ERR_REPEATED_F;
	} else {
		config->socket_name = arg;
	}
}

void
config_push_read(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_READ;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	config_add_action(config, action);
}

void
config_push_read_random(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_READ_RANDOM;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	config_add_action(config, action);
}

void
config_push_set_dir_destination(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_SET_DIR_DESTINATION;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	config_add_action(config, action);
}

void
config_push_wait(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_WAIT;
	action.arg_s = NULL;
	action.arg_i = atoi(arg);
	action.next = NULL;
	config_add_action(config, action);
}

void
config_push_lock(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_SEND_DIR_CONTENTS;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	config_add_action(config, action);
}

void
config_push_unlock(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_SEND_DIR_CONTENTS;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	config_add_action(config, action);
}

void
config_push_remove(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_REMOVE;
	action.arg_s = arg;
	action.arg_i = 0;
	action.next = NULL;
	config_add_action(config, action);
}

void
config_enable_log(struct Config *config)
{
	if (config->enable_log) {
		config->err = CLIENT_ERR_REPEATED_P;
	} else {
		config->enable_log = true;
	}
}

void
config_enable_help_message(struct Config *config)
{
	if (config->help_message) {
		config->err = CLIENT_ERR_REPEATED_H;
	} else {
		config->help_message = true;
	}
}

void
config_push_arg(struct Config *config, const char option, char *arg)
{
	switch (option) {
		case 'h':
			config_enable_help_message(config);
			break;
		case 'f':
			config_push_f(config, arg);
			break;
		case 'w':
			config_push_w(config, arg);
			break;
		case 'n':
			1;
		case 'W':
			config_push_capw(config, arg);
			break;
		case 'D':
			config_push_capd(config, arg);
			break;
		case 'r':
			config_push_read(config, arg);
			break;
		case 'R':
			config_push_read_random(config, arg);
			break;
		case 'd':
			config_push_set_dir_destination(config, arg);
			break;
		case 't':
			config_push_wait(config, arg);
			break;
		case 'l':
			config_push_lock(config, arg);
			break;
		case 'u':
			config_push_unlock(config, arg);
			break;
		case 'c':
			config_push_remove(config, arg);
			break;
		case 'p':
			config_enable_log(config);
			break;
		default:
			abort();
	}
}

struct Config *
config_from_cli_args(int argc, char **argv)
{
	struct Config *config = config_default();
	if (!config) {
		return NULL;
	}
	char c = '\0';
	while ((c = getopt(argc, argv, OPTSTRING)) != -1) {
		config_push_arg(config, c, optarg);
		if (config->err) {
			break;
		}
	}
	return config;
}
