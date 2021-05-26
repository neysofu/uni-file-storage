#ifndef SOL_SERVERAPI_ACTIONS
#define SOL_SERVERAPI_ACTIONS

enum ActionType
{
	ACTION_SET_EVICTED_DIR,
	ACTION_SEND,
	ACTION_WRITE_EVICTED,
	ACTION_READ,
	ACTION_READ_RANDOM,
	ACTION_SET_READ_DIR,
	ACTION_WAIT,
	ACTION_LOCK,
	ACTION_UNLOCK,
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
	char *arg_s;
	int arg_i;
	struct Action *next;
};

#endif
