#ifndef SOL_SERVERAPI
#define SOL_SERVERAPI

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/* Unique identifiers for all operations available over the public API. */
enum ApiOp
{
	API_OP_OPEN_CONNECTION,
	API_OP_CLOSE_CONNECTION,
	API_OP_OPEN_FILE,
	API_OP_OPEN_FILE_CREATE,
	API_OP_OPEN_FILE_LOCK,
	API_OP_OPEN_FILE_CREATE_LOCK,
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

enum FileFlag
{
	O_CREATE = 1,
	O_LOCK = 2,
};

/* Opens an `AF_UNIX` socket connection over `sockname`. On failure, connection
 * is attempted again and again every `msec` milliseconds until either success or
 * abstime has elapsed.
 *
 * It returns 0 on success and -1 on failure (read `errno` for more information). */
int
openConnection(const char *sockname, int msec, const struct timespec abstime);

/* Closes the `AF_UNIX` connection over `sockname` that was previously
 * established with `openConnection`, if any. `sockname` must match, otherwise
 * an error condition is triggered.
 *
 * It returns 0 on success and -1 on failure (read `errno` for more information). */
int
closeConnection(const char *sockname);

/* Asks the storage server to either open or create the file located at
 * `pathname`. `flags` changes the specific semantics of this operation.
 * There's two flags available:
 *  - O_CREATE specifies that the file should be created rather than opened. An
 *  error condition in case the file exists already or in case the file doesn't
 *  exit and this flag wasn't specified.
 *  - O_LOCK locks the file after this operation. Only the current process is
 *  allowed to do any further operations or to unlock the file.
 *
 * It returns 0 on success and -1 on failure (read `errno` for more information). */
int
openFile(const char *pathname, int flags);

/* Fetches the contents of the file located at `pathname` from the storage server and makes
 * them available at `*buf`. After this call, `buf` will be an address to a heap memory
 * region of size `*size`.
 *
 * It returns 0 on success and -1 on failure (read `errno` for more information). */
int
readFile(const char *pathname, void **buf, size_t *size);

/* Asks the storage server for `n` random files and stores them all in `dirname`.
 * In case
 *  - `n` is less than or equal to zero, or
 *  - the storage server is currently tracking less than `n` files,
 * The storage server will simply respond with all the files that it is
 * currently tracking.
 *
 * It returns 0 on success and -1 on failure (read `errno` for more information). */
int
readNFiles(int N, const char *dirname);

/* Reads the file located at `pathname` and asks the storage server to start
 * tracking it. Any evicted file due to this request is then written to
 * `dirname` if not NULL.
 *
 * It returns 0 on success and -1 on failure (read `errno` for more information). */
int
writeFile(const char *pathname, const char *dirname);

/* Atomically appends some content to the file located at `pathname` currently
 * tracked by the storage server. `buf` must point to a readable memory region
 * of size `size`, which contains the appended data. Any evicted file due to
 * this request is then written to `dirname` if not NULL.
 *
 * It returns 0 on success and -1 on failure (read `errno` for more information). */
int
appendToFile(const char *pathname, void *buf, size_t size, const char *dirname);

/* Enables the O_LOCK flag on the file located at `pathname` within the storage
 * server.
 *
 * It returns 0 on success and -1 on failure (read `errno` for more information). */
int
lockFile(const char *pathname);

/* Resets the `O_LOCK` flag on the file located at `pathname`. The current
 * process must be the one that locked the file in the first place.
 *
 * It returns 0 on success and -1 on failure (read `errno` for more information). */
int
unlockFile(const char *pathname);

/* Asks the storage server to close the file located at `pathname`. All futher
 * read operations on `pathname` will fail unless it's opened again.
 *
 * It returns 0 on success and -1 on failure (read `errno` for more information). */
int
closeFile(const char *pathname);

/* Asks the storage server to completely erase the file located at `pathname`
 * from memory. This operation is also known as "eviction".
 *
 * It returns 0 on success and -1 on failure (read `errno` for more information). */
int
removeFile(const char *pathname);

#endif
