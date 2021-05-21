#include "serverapi.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define OPTSTRING "hf:w:n:W::D:r::Rd:t:l::u::c::p"

enum ClientErr
{
	CLIENT_ERR_OK = 0,
	CLIENT_ERR_REPEATED_P,
	CLIENT_ERR_REPEATED_H,
	CLIENT_ERR_REPEATED_F,
};

enum ActionType
{
	ACTION_SEND_DIR_CONTENTS,
	ACTION_SEND_FILES,
	ACTION_WRITE_EVICTED,
	ACTION_REMOVE,
};

struct ActionArgs
{
	char **str;
	int n;
};

struct Action
{
	enum ActionType type;
	char *arg1;
	char *arg2;
	struct Action *next;
};

struct Config
{
	char *socket_name;
	bool enable_log;
	struct Action *head;
	struct Action *last;
};

int
config_add_action(struct Config *config, struct Action action)
{
	struct Action *next = malloc(sizeof(struct Action));
	next->type = action.type;
	next->arg1 = action.arg1;
	if (config->last) {
		config->last->next = next;
	} else {
		config->head = next;
	}
	config->last = next;
}

int
config_push_w(struct Config *config, char *arg)
{
	char *comma = strrchr(arg, ',');
	char *equal_sign = strrchr(arg, '=');
	struct Action action;
	action.type = ACTION_WRITE_EVICTED;
	action.arg1 = arg;
	action.arg2 = NULL;
	action.next = NULL;
	if (comma && equal_sign && equal_sign == comma + 2 && strncmp(comma + 1, "n", 1)) {
		int n = 0;
		comma[0] = '\0';
		action.arg2 = equal_sign + 1;
	}
	return config_add_action(config, action);
}

int
config_push_capd(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_SEND_DIR_CONTENTS;
	action.arg1 = arg;
	action.arg2 = NULL;
	action.next = NULL;
	return config_add_action(config, action);
}

int
config_push_capw(struct Config *config, char *arg)
{}

int
config_push_f(struct Config *config, char *arg)
{
	if (config->socket_name) {
		return CLIENT_ERR_REPEATED_F;
	}
	config->socket_name = arg;
	return CLIENT_ERR_OK;
}

int
config_push_r(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_SEND_DIR_CONTENTS;
	action.arg1 = arg;
	action.arg2 = NULL;
	action.next = NULL;
	return config_add_action(config, action);
}

int
config_push_capr(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_SEND_DIR_CONTENTS;
	action.arg1 = arg;
	action.arg2 = NULL;
	action.next = NULL;
	return config_add_action(config, action);
}

int
config_push_d(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_SEND_DIR_CONTENTS;
	action.arg1 = arg;
	action.arg2 = NULL;
	action.next = NULL;
	return config_add_action(config, action);
}

int
config_push_t(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_SEND_DIR_CONTENTS;
	action.arg1 = arg;
	action.arg2 = NULL;
	action.next = NULL;
	return config_add_action(config, action);
}

int
config_push_l(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_SEND_DIR_CONTENTS;
	action.arg1 = arg;
	action.arg2 = NULL;
	action.next = NULL;
	return config_add_action(config, action);
}

int
config_push_u(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_SEND_DIR_CONTENTS;
	action.arg1 = arg;
	action.arg2 = NULL;
	action.next = NULL;
	return config_add_action(config, action);
}

int
config_push_c(struct Config *config, char *arg)
{
	struct Action action;
	action.type = ACTION_REMOVE;
	action.arg1 = arg;
	action.arg2 = NULL;
	action.next = NULL;
	return config_add_action(config, action);
}

int
config_push_p(struct Config *config)
{
	if (config->enable_log) {
		return CLIENT_ERR_REPEATED_P;
	}
	config->enable_log = true;
	return CLIENT_ERR_OK;
}

int
config_push_arg(struct Config *config, const char option, char *arg)
{
	switch (option) {
		case 'f':
			return config_push_f(config, arg);
		case 'w':
			return config_push_w(config, arg);
		case 'n':
			return 1;
		case 'W':
			return config_push_capw(config, arg);
		case 'D':
			return config_push_capd(config, arg);
		case 'r':
			return config_push_r(config, arg);
		case 'R':
			return config_push_capr(config, arg);
		case 'd':
			return config_push_d(config, arg);
		case 't':
			return config_push_t(config, arg);
		case 'l':
			return config_push_l(config, arg);
		case 'u':
			return config_push_u(config, arg);
		case 'c':
			return config_push_c(config, arg);
		case 'p':
			return config_push_p(config);
		default:
			abort();
	}
	return 0;
}

void
print_help(void)
{

	puts("Client for file storage server. Options:");
	puts("");
	puts("-h            Prints this message and exits.");
	puts("-f filename   Specifies the AF_UNIX socket name.");
	puts("-w dirname    Sends all files in `dirname`.");
	puts("-W files      Sends a list of files to the server.");
	puts("-D dirname    Writes evicted files to `dirname`.");
	puts("-r files      Reads a list of files from the server.");
	puts("-R            Reads a certain amount (or all) files in the server.");
}

int
main(int argc, char **argv)
{
	int aflag;
	int c = 0;
	struct Action *actions = malloc(sizeof(struct Action) * argc);
	const char *socket_name = NULL;
	bool arg_f = false;
	unsigned action_i = 0;
	struct Config config;
	while ((c = getopt(argc, argv, OPTSTRING)) != -1) {
		if (c == 'h') {
			print_help();
		} else {
			config_push_arg(&config, c, optarg);
		}
	}
	for (struct Action *action = config.head; action; action = action->next) {
		switch (action->type) {
			case ACTION_REMOVE:
				break;
			case ACTION_SEND_DIR_CONTENTS:
				break;
			case ACTION_SEND_FILES:
				break;
			default:
				assert(false);
		}
	}
	return EXIT_SUCCESS;
}