#ifndef SOL_SERVER_HTABLE
#define SOL_SERVER_HTABLE

#include <stdbool.h>
#include <stdlib.h>

struct Subscriber {
    int fd;
    struct Subscriber *next;
};

// TODO: implement these
enum CacheReplacementPolicy {
	POLICY_BUCKET_BASED,
	POLICY_FIFO,
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

/* Creates an empty `struct HTable` with a fixed number of `buckets`. Returns
 * NULL on system errors that make the operation impossible. */
struct HTable *
htable_create(size_t buckets, enum CacheReplacementPolicy policy);

/* Deletes `htable` and all its contents from memory (files, paths, flags, etc.). */
void
htable_free(struct HTable *htable);

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
htable_lock_file(struct HTable *htable, const char *key);

/* Unlocks the file with path `key` within `htable` and returns 0 if the operation
 * was successful, -1 otherwise. */
int
htable_unlock_file(struct HTable *htable, const char *key);

/* Opens the file with path `key` within `htable` and returns 0 if the operation
 * was successful, -1 otherwise. */
int
htable_open_file(struct HTable *htable, const char *key);

/* Removes the file with path `key` within `htable` and returns 0 if the operation
 * was successful, -1 otherwise. */
int
htable_remove_file(struct HTable *htable, const char *key);

struct HTableVisitor;

struct HTableVisitor *
htable_visit(struct HTable *htable, unsigned max_visits);

struct File *
htable_visitor_next(struct HTableVisitor *visitor);

#endif
