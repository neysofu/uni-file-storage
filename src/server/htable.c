#include "htable.h"
#include "config.h"
#include "fifo_cache.h"
#include "utilities.h"
#include "xxHash/xxhash.h"
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define return_if_err(x)                                                                   \
	do {                                                                                   \
		int err = (x);                                                                     \
		if (err != 0) {                                                                    \
			return err;                                                                    \
		}                                                                                  \
	} while (0)

#define XXHASH_SEED 0

struct HTableItem
{
	struct File file;
	struct HTableItem *next;
	struct HTableItem *prev;
};

struct HTableBucket
{
	pthread_mutex_t guard;
	struct HTableItem *head;
	struct HTableItem *last;
};

struct HTable
{
	struct FifoCache *fcache;
	/* Settings. */
	size_t max_items_count;
	size_t max_space_in_bytes;
	/* Internal data. */
	pthread_mutex_t stats_guard;
	struct HTableStats stats;
	size_t buckets_count;
	struct HTableBucket *buckets;
};

struct HTable *
htable_create(size_t buckets, const struct Config *config)
{
	assert(buckets > 0);
	struct HTable *htable = xmalloc(sizeof(struct HTable));
	htable->fcache = fifo_cache_create(config->max_files, config->max_storage_in_bytes);
	if (!htable->fcache) {
		free(htable);
		return NULL;
	}
	int err = pthread_mutex_init(&htable->stats_guard, NULL);
	if (err) {
		free(htable);
		free(htable->fcache);
		return NULL;
	}
	htable->max_items_count = config->max_files;
	htable->max_space_in_bytes = config->max_storage_in_bytes;
	htable->stats.items_count = 0;
	htable->stats.open_count = 0;
	htable->stats.total_space_in_bytes = 0;
	htable->stats.historical_max_items_count = 0;
	htable->stats.historical_max_space_in_bytes = 0;
	htable->stats.historical_num_evictions = 0;
	htable->buckets_count = buckets;
	htable->buckets = xmalloc(sizeof(struct HTableBucket) * buckets);
	for (size_t i = 0; i < buckets; i++) {
		htable->buckets[i].head = NULL;
		htable->buckets[i].last = NULL;
		err = pthread_mutex_init(&htable->buckets[i].guard, NULL);
		if (err != 0) {
			pthread_mutex_destroy(&htable->stats_guard);
			free(htable->buckets);
			free(htable);
			return NULL;
		}
	}
	return htable;
}

void
htable_free(struct HTable *htable)
{
	if (!htable) {
		return;
	}
	for (size_t i = 0; i < htable->buckets_count; i++) {
		/* Ignore any errors: everything will be out of scope very soon, we don't
		 * really care. */
		pthread_mutex_destroy(&htable->buckets[i].guard);
		struct HTableItem *item = htable->buckets[i].head;
		while (item) {
			free(item->file.contents);
			free(item->file.key);
			struct Subscriber *sub = item->file.subs;
			while (sub) {
				struct Subscriber *next = sub->next;
				free(sub);
				sub = next;
			}
			item = item->next;
		}
	}
	fifo_cache_free(htable->fcache);
	pthread_mutex_destroy(&htable->stats_guard);
	free(htable->buckets);
	free(htable);
}

/************ INTERNAL HTABLE UTILITIES ***********/

static enum HTableError
htable_lock_stats(struct HTable *htable)
{
	if (pthread_mutex_lock(&htable->stats_guard) == 0) {
		return HTABLE_ERR_OK;
	} else {
		return HTABLE_ERR_MUTEX;
	}
}

static enum HTableError
htable_unlock_stats(struct HTable *htable)
{
	if (pthread_mutex_unlock(&htable->stats_guard) == 0) {
		return HTABLE_ERR_OK;
	} else {
		return HTABLE_ERR_MUTEX;
	}
}

/* Returns a pointer to the bucket within `htable` that contains `key`, if
 * present at all. */
static struct HTableBucket *
htable_bucket_ptr(struct HTable *htable, const char *key)
{
	uint64_t hash = XXH64(key, strlen(key), XXHASH_SEED);
	return &htable->buckets[hash % htable->buckets_count];
}

/* Locks the bucket within `htable` that contains `key` and returns a pointer to
 * its associated item, if present. Returns NULL for unsuccesful searches. The
 * bucket must be unlocked after this call. */
struct HTableItem *
htable_fetch_item(struct HTable *htable, const char *key)
{
	assert(htable);
	assert(key);
	struct HTableBucket *bucket = htable_bucket_ptr(htable, key);
	assert(bucket);
	int err = 0;
	err = pthread_mutex_lock(&bucket->guard);
	if (err) {
		return NULL;
	}
	struct HTableItem *item = bucket->head;
	while (item) {
		if (strcmp(item->file.key, key) == 0) {
			return item;
		}
		item = item->next;
	}
	return NULL;
}

struct File *
htable_fetch_file(struct HTable *htable, const char *key)
{
	assert(htable);
	assert(key);
	struct HTableItem *node = htable_fetch_item(htable, key);
	if (!node) {
		struct HTableBucket *bucket = htable_bucket_ptr(htable, key);
		pthread_mutex_unlock(&bucket->guard);
		return NULL;
	}
	return &node->file;
}

/* Frees the bucket within `htable` that contains `key`. */
enum HTableError
htable_release(struct HTable *htable, const char *key)
{
	struct HTableBucket *bucket = htable_bucket_ptr(htable, key);
	assert(bucket);

	if (pthread_mutex_unlock(&bucket->guard) == 0) {
		return HTABLE_ERR_OK;
	} else {
		return HTABLE_ERR_MUTEX;
	}
}

struct File *
htable_evict_one_file_locked(struct HTable *htable)
{
	size_t attempts = 3;
	while (attempts > 0) {
		size_t i = rand() % htable->buckets_count;
		struct HTableBucket *bucket = &htable->buckets[i];
		struct HTableItem *last = bucket->last;
		if (!bucket->last) {
			attempts--;
			continue;
		}
		bucket->last = last->prev;
		assert(last);
		return &last->file;
	}
	return NULL;
}

enum HTableError
htable_evict_files_locked(struct HTable *htable,
                          struct File **evicted,
                          unsigned *evicted_count)
{
	*evicted = NULL;
	*evicted_count = 0;
	while (htable->stats.items_count > htable->max_items_count ||
	       htable->stats.total_space_in_bytes > htable->max_space_in_bytes) {
		evicted_count++;
		*evicted = xrealloc(*evicted, sizeof(struct File *) * *evicted_count);
		struct File **file_ptr = &(*evicted)[*evicted_count - 1];
		*file_ptr = htable_evict_one_file_locked(htable);
		if (*file_ptr) {
			htable->stats.items_count--;
			htable->stats.total_space_in_bytes -= (*file_ptr)->length_in_bytes;
		} else {
			return 0;
		}
	}
	return HTABLE_ERR_OK;
}

/* Runs the cache replacement policy algorithm on `htable` after some operation
 * that might trigger evictions. `new_file` is `true` iff a new file has just
 * been created, `new_space_in_bytes` is the new amount of space in bytes that
 * must be taken into account. `evicted` and `evicted_count` will -after this
 * call- hold data about evicted files. */
int
htable_evict_files(struct HTable *htable,
                   bool new_file,
                   size_t new_space_in_bytes,
                   struct File **evicted,
                   unsigned *evicted_count)
{
	int err = 0;
	err |= pthread_mutex_lock(&htable->stats_guard);
	htable->stats.total_space_in_bytes += new_space_in_bytes;
	if (new_file) {
		htable->stats.items_count++;
	}
	htable_evict_files_locked(htable, evicted, evicted_count);
	err |= pthread_mutex_unlock(&htable->stats_guard);
	if (err) {
		return -1;
	} else {
		return 0;
	}
}

/************ HIGH LEVEL APIS ***********/

const struct HTableStats *
htable_stats(const struct HTable *htable)
{
	return &htable->stats;
}

enum HTableError
htable_lock_file(struct HTable *htable, const char *key, int fd)
{
	struct File *file = htable_fetch_file(htable, key);
	if (!file) {
		return HTABLE_ERR_FILE_NOT_FOUND;
	} else if (file->is_locked) {
		return_if_err(htable_release(htable, key));
		return HTABLE_ERR_ALREADY_LOCKED;
	} else {
		file->is_locked = true;
		file->fd_owner = fd;
		return htable_release(htable, key);
	}
}

enum HTableError
htable_unlock_file(struct HTable *htable, const char *key, int fd)
{
	struct File *file = htable_fetch_file(htable, key);
	if (!file) {
		return HTABLE_ERR_FILE_NOT_FOUND;
	} else if (!file->is_locked) {
		return_if_err(htable_release(htable, key));
		return HTABLE_ERR_ALREADY_UNLOCKED;
	} else if (file->fd_owner != fd) {
		return_if_err(htable_release(htable, key));
		return HTABLE_ERR_CANT_UNLOCK;
	} else {
		file->is_locked = false;
		return htable_release(htable, key);
	}
}

enum HTableError
htable_open_file(struct HTable *htable, const char *key, int fd, bool lock)
{
	struct File *file = htable_fetch_file(htable, key);
	if (!file) {
		return HTABLE_ERR_FILE_NOT_FOUND;
	} else if (file->is_open) {
		return_if_err(htable_release(htable, key));
		return HTABLE_ERR_ALREADY_OPEN;
	} else if (file->is_locked && file->fd_owner != fd) {
		return_if_err(htable_release(htable, key));
		return HTABLE_ERR_CANT_OPEN;
	} else {
		file->is_open = true;
		file->is_locked = lock;
		file->fd_owner = fd;
		return_if_err(htable_release(htable, key));
		return_if_err(htable_lock_stats(htable));
		htable->stats.open_count++;
		return_if_err(htable_unlock_stats(htable));
		return HTABLE_ERR_OK;
	}
}

enum HTableError
htable_create_file(struct HTable *htable, const char *key, int fd, bool lock)
{
	if (htable_fetch_item(htable, key)) {
		return_if_err(htable_release(htable, key));
		return HTABLE_ERR_ALREADY_CREATED;
	}

	struct HTableBucket *bucket = htable_bucket_ptr(htable, key);
	struct HTableItem *item = xmalloc(sizeof(struct HTableItem));
	item->file.fd_owner = fd;
	item->file.is_locked = lock;
	item->file.is_open = true;
	item->file.key = xmalloc(strlen(key) + 1);
	strcpy(item->file.key, key);
	item->file.length_in_bytes = 0;
	item->file.contents = NULL;
	item->file.subs = NULL;
	item->next = bucket->head;
	if (bucket->head) {
		bucket->head->prev = item;
	}
	item->prev = NULL;
	bucket->head = item;
	return_if_err(htable_release(htable, key));
	return_if_err(htable_lock_stats(htable));
	htable->stats.open_count++;
	htable->stats.items_count++;
	return_if_err(htable_unlock_stats(htable));
	return HTABLE_ERR_OK;
}

enum HTableError
htable_open_or_create_file(struct HTable *htable,
                           const char *key,
                           int fd,
                           bool create,
                           bool lock)
{
	if (create) {
		return htable_create_file(htable, key, fd, lock);
	} else {
		return htable_open_file(htable, key, fd, lock);
	}
}

enum HTableError
htable_close_file(struct HTable *htable, const char *key, int fd)
{
	struct File *file = htable_fetch_file(htable, key);
	if (!file) {
		return HTABLE_ERR_FILE_NOT_FOUND;
	} else if (!file->is_open) {
		return_if_err(htable_release(htable, key));
		return HTABLE_ERR_ALREADY_CLOSED;
	} else if (file->is_locked && file->fd_owner != fd) {
		return_if_err(htable_release(htable, key));
		return HTABLE_ERR_CANT_CLOSE;
	} else {
		file->is_open = false;
		return_if_err(htable_release(htable, key));
		return_if_err(htable_lock_stats(htable));
		htable->stats.open_count--;
		return_if_err(htable_unlock_stats(htable));
		return HTABLE_ERR_OK;
	}
}

enum HTableError
htable_remove_file(struct HTable *htable, const char *key, int fd)
{
	struct HTableItem *node = htable_fetch_item(htable, key);
	if (!node) {
		return HTABLE_ERR_FILE_NOT_FOUND;
	} else if (node->file.is_locked && node->file.fd_owner != fd) {
		return_if_err(htable_release(htable, key));
		return HTABLE_ERR_OK;
	}

	struct HTableBucket *bucket = htable_bucket_ptr(htable, key);
	/* Chain together the previous and next nodes within the bucket's linked
	 * list. */
	if (node->prev) {
		node->prev->next = node->next;
	} else {
		bucket->head = node->next;
	}
	if (node->next) {
		node->next->prev = node->prev;
	} else {
		bucket->last = node->prev;
	}

	size_t size_in_bytes = node->file.length_in_bytes;
	/* Free stuff. */
	free(node->file.key);
	free(node->file.contents);
	free(node);

	return_if_err(htable_release(htable, key));
	return_if_err(htable_lock_stats(htable));
	htable->stats.open_count--;
	htable->stats.items_count--;
	htable->stats.total_space_in_bytes -= size_in_bytes;
	return_if_err(htable_unlock_stats(htable));
	return HTABLE_ERR_OK;
}

enum HTableError
htable_replace_file_contents(struct HTable *htable,
                             const char *key,
                             const void *contents,
                             size_t size_in_bytes,
                             struct File **evicted,
                             unsigned *evicted_count)
{
	struct File *file = htable_fetch_file(htable, key);
	if (!file) {
		return HTABLE_ERR_FILE_NOT_FOUND;
	}

	size_t old_size_in_bytes = file->length_in_bytes;
	file->length_in_bytes = size_in_bytes;
	free(file->contents);
	file->contents = xmalloc(size_in_bytes);
	memcpy(file->contents, contents, size_in_bytes);
	file->length_in_bytes = size_in_bytes;

	return_if_err(htable_release(htable, key));
	return_if_err(htable_lock_stats(htable));
	htable->stats.total_space_in_bytes =
	  htable->stats.total_space_in_bytes + size_in_bytes - old_size_in_bytes;
	return_if_err(htable_unlock_stats(htable));

	return htable_evict_files(htable, true, size_in_bytes, evicted, evicted_count);
}

enum HTableError
htable_append_to_file_contents(struct HTable *htable,
                               const char *key,
                               const void *contents,
                               size_t size_in_bytes,
                               struct File **evicted,
                               unsigned *evicted_count)
{
	struct File *file = htable_fetch_file(htable, key);
	if (!file) {
		return HTABLE_ERR_FILE_NOT_FOUND;
	}
	file->length_in_bytes += size_in_bytes;
	void *new_buffer = xrealloc(file->contents, file->length_in_bytes);
	file->contents = new_buffer;
	memcpy((char *)(new_buffer) + file->length_in_bytes - size_in_bytes,
	       contents,
	       size_in_bytes);

	return_if_err(htable_release(htable, key));
	return_if_err(htable_lock_stats(htable));
	htable->stats.total_space_in_bytes += size_in_bytes;
	return_if_err(htable_unlock_stats(htable));

	return htable_evict_files(htable, false, size_in_bytes, evicted, evicted_count);
}

/************ VISITOR PATTERN ***********/

struct HTableVisitor
{
	struct HTable *htable;
	unsigned max_visits;
	unsigned visits;
	size_t bucket_i;
	struct HTableItem *last_visited;
};

struct HTableVisitor *
htable_visit(struct HTable *htable, unsigned max_visits)
{
	/* Immediately allocate enough memory on the heap. */
	struct HTableVisitor *visitor = xmalloc(sizeof(struct HTableVisitor *));
	visitor->htable = htable;
	visitor->max_visits = max_visits;
	visitor->visits = 0;
	visitor->bucket_i = 0;
	visitor->last_visited = NULL;
	struct HTableBucket *bucket = &htable->buckets[0];
	int err = 0;
	/* Lock the first bucket. */
	err = pthread_mutex_lock(&bucket->guard);
	if (err < 0) {
		free(visitor);
		return NULL;
	}
	return visitor;
}

struct File *
htable_visitor_next(struct HTableVisitor *visitor)
{
	assert(visitor);
	/* Check if we have visited enough items already. */
	if (visitor->visits >= visitor->max_visits && visitor->max_visits != 0) {
		/* The visit is complete. */
		return NULL;
	}
	struct HTable *htable = visitor->htable;
	struct HTableBucket *bucket = &htable->buckets[visitor->bucket_i];
	struct HTableItem *next_in_bucket = bucket->head;
	if (visitor->last_visited) {
		next_in_bucket = visitor->last_visited->next;
	}
	/* The current bucket doesn't have any more items, so let's unlock this one
	 * and lock the next one, until we find one with at least one item. */
	while (!next_in_bucket) {
		int err = pthread_mutex_unlock(&bucket->guard);
		if (err < 0) {
			return NULL;
		}
		visitor->bucket_i++;
		if (visitor->bucket_i == htable->buckets_count) {
			/* No more buckets! */
			return NULL;
		}
		bucket++;
		err = pthread_mutex_lock(&bucket->guard);
		if (err < 0) {
			return NULL;
		}
		next_in_bucket = bucket->head;
	}
	visitor->last_visited = next_in_bucket;
	if (visitor->last_visited) {
		return &visitor->last_visited->file;
	} else {
		return NULL;
	}
}

void
htable_visitor_free(struct HTableVisitor *visitor)
{
	free(visitor);
}
