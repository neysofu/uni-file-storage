#ifndef SOL_SERVER_HTABLE
#define SOL_SERVER_HTABLE

#include "config.h"
#include <stdbool.h>
#include <stdlib.h>

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

/* Returns the amount of files currently stored by `htable`. */
unsigned long
htable_num_items(const struct HTable *htable);

/* Returns the maximum amount of files ever stored by `htable`. */
unsigned long
htable_max_files_stored(const struct HTable *htable);

/* Returns the total number of file evictions ever performed by `htable`. */
long unsigned
htable_num_evictions(const struct HTable *htable);

/* Returns the maximum amount of bytes ever stored by `htable`. */
unsigned long
htable_max_size(const struct HTable *htable);

/* Searches a file with path `key` within `htable` and returns a pointer to its
 * contents if found, NULL otherwise. */
struct File *
htable_fetch_file(struct HTable *htable, const char *key);

int
htable_write_file_contents(struct HTable *htable,
                           const char *key,
                           const void *contents,
                           size_t size_in_bytes,
                           struct File **evicted,
                           unsigned *evicted_count);

int
htable_append_to_file_contents(struct HTable *htable,
                               const char *key,
                               const void *contents,
                               size_t size_in_bytes,
                               struct File **evicted,
                               unsigned *evicted_count);

/* Drops a unique handle on the file with path `key` within `htable` and releases
 * an internal lock that blocks access to it from other threads. Returns 0 on
 * success, -1 on failure. */
int
htable_release(struct HTable *htable, const char *key);

/* Locks the file with path `key` within `htable` and returns 0 if the operation
 * was successful, -1 otherwise. */
int
htable_lock_file(struct HTable *htable, const char *key, int fd);

/* Unlocks the file with path `key` within `htable` and returns 0 if the operation
 * was successful, -1 otherwise. */
int
htable_unlock_file(struct HTable *htable, const char *key, int fd);

/* Opens the file with path `key` within `htable` and returns 0 if the operation
 * was successful, -1 otherwise. */
int
htable_open_file(struct HTable *htable, const char *key, int fd);

/* Removes the file with path `key` within `htable` and returns 0 if the operation
 * was successful, -1 otherwise. */
int
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
