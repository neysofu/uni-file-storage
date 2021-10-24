#ifndef SOL_SERVER_HTABLE
#define SOL_SERVER_HTABLE

#include "config.h"
#include <stdbool.h>
#include <stdlib.h>

enum HTableError
{
	HTABLE_ERR_OK = 0,
	HTABLE_ERR_ALREADY_LOCKED,
	HTABLE_ERR_ALREADY_OPEN,
	HTABLE_ERR_ALREADY_CREATED,
	HTABLE_ERR_ALREADY_CLOSED,
	HTABLE_ERR_ALREADY_UNLOCKED,
	HTABLE_ERR_FILE_NOT_FOUND,
	HTABLE_ERR_CANT_UNLOCK,
	HTABLE_ERR_CANT_OPEN,
	HTABLE_ERR_CANT_CLOSE,
	HTABLE_ERR_SIZE,
};

struct HTableStats
{
	size_t total_space_in_bytes;
	size_t items_count;
	size_t open_count;
	long unsigned historical_max_items_count;
	long unsigned historical_max_space_in_bytes;
	long unsigned historical_num_evictions;
};

struct Subscriber
{
	int fd;
	struct Subscriber *next;
};

/* A specialized, concurrent hash table for the file storage server. */
struct HTable;

/* File data within a `struct HTable`. */
struct File
{
	char *key;
	void *contents;
	size_t length_in_bytes;
	int fd_owner;
	bool is_open;
	bool is_locked;
	struct Subscriber *subs;
};

/* Creates an empty `struct HTable` with a fixed number of `buckets` and settings
 * as mandated by `config`. Returns NULL on system errors that make the
 * operation impossible. */
struct HTable *
htable_create(size_t buckets, const struct Config *config);

/* Deletes `htable` and all its contents from memory (files, paths, flags, etc.). */
void
htable_free(struct HTable *htable);

/* Returns a pointer to the internal `struct HTableStats` of `htable`. */
const struct HTableStats *
htable_stats(const struct HTable *htable);

/* Searches a file with path `key` within `htable` and returns a pointer to its
 * contents if found, NULL otherwise. */
struct File *
htable_fetch_file(struct HTable *htable, const char *key);

enum HTableError
htable_replace_file_contents(struct HTable *htable,
                             const char *key,
                             const void *contents,
                             size_t size_in_bytes,
                             struct File **evicted,
                             unsigned *evicted_count);

enum HTableError
htable_append_to_file_contents(struct HTable *htable,
                               const char *key,
                               const void *contents,
                               size_t size_in_bytes,
                               struct File **evicted,
                               unsigned *evicted_count);

/* Drops a unique handle on the file with path `key` within `htable` and releases
 * an internal lock that blocks access to it from other threads. This MUST
 * always be called after `htable_fetch_file`. */
void
htable_release_file(struct HTable *htable, const char *key);

/* Locks the file with path `key` within `htable`. */
enum HTableError
htable_lock_file(struct HTable *htable, const char *key, int fd);

/* Unlocks the file with path `key` within `htable`. */
enum HTableError
htable_unlock_file(struct HTable *htable, const char *key, int fd);

enum HTableError
htable_close_file(struct HTable *htable, const char *key, int fd);

/* Opens the file with path `key` within `htable`. */
enum HTableError
htable_open_or_create_file(struct HTable *htable,
                           const char *key,
                           int fd,
                           bool create,
                           bool lock);

/* Removes the file with path `key` within `htable` and returns 0 if the operation
 * was successful, -1 otherwise. */
enum HTableError
htable_remove_file(struct HTable *htable, const char *key, int fd);

struct HTableVisitor;

/* Allocates, initializes, and finally returns a new `HTableVisitor` to iterate
 * over the contents of `htable` for up to, but not exceeding, `max_visits`
 * items. */
struct HTableVisitor *
htable_visit(struct HTable *htable, unsigned max_visits);

struct File *
htable_visitor_next(struct HTableVisitor *visitor);

void
htable_visitor_free(struct HTableVisitor *visitor);

#endif
