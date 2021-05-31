#ifndef SOL_SERVERAPI_UTILS
#define SOL_SERVERAPI_UTILS

/* Unique identifiers for all operations available over the public API, besides
 * `openConnection` and `closeConnection`. */
enum ApiOp
{
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

/* The anatomy of the server's response to any request:
 * - Header: 8 bytes (length of payload in bytes).
 * Payload:
 * - Response type: 1 byte.
 * - N. of evicted files (if relevant): 8 bytes.
 * - Evicted files' details...
 */
enum ResponseType {
	RESPONSE_OK,
	RESPONSE_ERR,
};

#endif
