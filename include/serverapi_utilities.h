#ifndef SOL_SERVERAPI_UTILS
#define SOL_SERVERAPI_UTILS

/* Unique identifiers for all operations available over the public API. */
enum ApiOp
{
	/* We leave the three least significant bits availble for flags. */
	API_OP_OPEN_CONNECTION = 8,
	API_OP_CLOSE_CONNECTION,
	API_OP_OPEN_FILE,
	API_OP_READ_FILE,
	API_OP_READ_N_FILES,
	API_OP_WRITE_FILE,
	API_OP_APPEND_TO_FILE,
	API_OP_LOCK_FILE,
	API_OP_UNLOCK_FILE,
	API_OP_CLOSE_FILE,
	API_OP_REMOVE_FILE,
};

enum ResponseType
{
	RESPONSE_OK,
	RESPONSE_ERR,
};

#endif
