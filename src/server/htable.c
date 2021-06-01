#include "htable.h"
#include "utils.h"
#include "xxHash/xxhash.h"
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define XXHASH_SEED 0

struct HTableLlNode
{
	struct File item;
	struct HTableLlNode *next;
	struct HTableLlNode *prev;
};

struct HTableBucket
{
	pthread_mutex_t guard;
	struct HTableLlNode *head;
	struct HTableLlNode *last;
};

struct HTable
{
	enum CacheReplacementPolicy policy;
	pthread_mutex_t stats_guard;
	size_t items_count;
	size_t max_items_count;
	size_t buckets_count;
	size_t total_space_in_bytes;
	size_t max_space_in_bytes;
	struct HTableBucket *buckets;
};

struct HTable *
htable_create(size_t buckets, enum CacheReplacementPolicy policy)
{
	assert(buckets > 0);
	struct HTable *htable = xmalloc(sizeof(struct HTable));
	htable->policy = policy;
	int err = pthread_mutex_init(&htable->stats_guard, NULL);
	if (err) {
		free(htable);
		return NULL;
	}
	// FIXME: max limits
	htable->items_count = 0;
	htable->max_items_count = 0;
	htable->total_space_in_bytes = 0;
	htable->max_space_in_bytes = 0;
	htable->buckets_count = buckets;
	htable->buckets = xmalloc(sizeof(struct HTableBucket) * buckets);
	for (size_t i = 0; i < buckets; i++) {
		htable->buckets[i].head = NULL;
		htable->buckets[i].last = NULL;
		err = pthread_mutex_init(&htable->buckets[i].guard, NULL);
		if (err != 0) {
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
		 * care. */
		pthread_mutex_destroy(&htable->buckets[i].guard);
		// TODO: free other stuff
	}
	free(htable->buckets);
	free(htable);
}

struct HTableLlNode *
htable_fetch_node(struct HTable *htable, const char *key, struct HTableBucket **b)
{
	assert(htable);
	assert(key);
	int err = 0;
	uint64_t hash = XXH64(key, strlen(key), XXHASH_SEED);
	size_t bucket_i = hash % htable->buckets_count;
	struct HTableBucket *bucket = &htable->buckets[bucket_i];
	*b = bucket;
	err = pthread_mutex_lock(&bucket->guard);
	if (err) {
		return NULL;
	}
	struct HTableLlNode *node = bucket->head;
	while (node) {
		if (node->item.key == key) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

struct File *
htable_fetch_file(struct HTable *htable, const char *key)
{
	struct HTableBucket *bucket = NULL;
	struct HTableLlNode *node = htable_fetch_node(htable, key, &bucket);
	if (!node) {
		return NULL;
	}
	return &node->item;
}

int
htable_release(struct HTable *htable, const char *key)
{
	uint64_t hash = XXH64(key, strlen(key), XXHASH_SEED);
	size_t bucket_i = hash % htable->buckets_count;
	struct HTableBucket *bucket = &htable->buckets[bucket_i];
	return pthread_mutex_unlock(&bucket->guard);
}

int
htable_lock_file(struct HTable *htable, const char *key)
{
	struct File *file = htable_fetch_file(htable, key);
	if (!file) {
		return -1;
	}
	file->is_locked = true;
	return htable_release(htable, key);
}

int
htable_unlock_file(struct HTable *htable, const char *key)
{
	struct File *file = htable_fetch_file(htable, key);
	if (!file) {
		return -1;
	}
	file->is_locked = false;
	return htable_release(htable, key);
}

int
htable_open_file(struct HTable *htable, const char *key)
{
	struct File *file = htable_fetch_file(htable, key);
	if (!file) {
		return -1;
	}
	file->is_open = true;
	return htable_release(htable, key);
}

struct File *
htable_evict_one_file_locked(struct HTable *htable)
{
	size_t attempts = 3;
	while (attempts > 0) {
		size_t i = rand() % htable->buckets_count;
		struct HTableBucket *bucket = &htable->buckets[i];
		struct HTableLlNode *last = bucket->last;
		if (!bucket->last) {
			attempts--;
			continue;
		}
		bucket->last = last->prev;
		return last;
	}
	return NULL;
}

int
htable_evict_files_locked(struct HTable *htable,
                          struct File **evicted,
                          unsigned *evicted_count)
{
	*evicted = NULL;
	*evicted_count = 0;
	while (htable->items_count > htable->max_items_count ||
	       htable->total_space_in_bytes > htable->max_space_in_bytes) {
		evicted_count++;
		*evicted = xrealloc(*evicted, sizeof(struct File *) * *evicted_count);
		struct File **file_ptr = &(*evicted)[*evicted_count - 1];
		*file_ptr = htable_evict_one_file_locked(htable);
		if (*file_ptr) {
			htable->items_count--;
			htable->total_space_in_bytes -= (*file_ptr)->length_in_bytes;
		} else {
			return 0;
		}
	}
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
	htable->total_space_in_bytes += new_space_in_bytes;
	if (new_file) {
		htable->items_count++;
	}
	htable_evict_files_locked(htable, evicted, evicted_count);
	err |= pthread_mutex_unlock(&htable->stats_guard);
	if (err) {
		return -1;
	} else {
		return 0;
	}
}

int
htable_remove_file(struct HTable *htable, const char *key)
{
	struct HTableBucket *bucket = NULL;
	struct HTableLlNode *node = htable_fetch_node(htable, key, &bucket);
	if (!node) {
		return -1;
	}
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
	/* Free stuff. */
	free(node->item.key);
	free(node->item.contents);
	free(node);
	return htable_release(htable, key);
}

int
htable_write_file_contents(struct HTable *htable,
                           const char *key,
                           const void *contents,
                           size_t size_in_bytes,
                           struct File **evicted,
                           unsigned *evicted_count)
{
	struct File *file = htable_fetch_file(htable, key);
	if (!file) {
		return -1;
	}
	file->length_in_bytes = size_in_bytes;
	free(file->contents);
	file->contents = xmalloc(size_in_bytes);
	memcpy(file->contents, contents, size_in_bytes);
	file->length_in_bytes = size_in_bytes;
	int err = htable_release(htable, key);
	if (err) {
		return -1;
	}
	return htable_evict_files(htable, true, size_in_bytes, evicted, evicted_count);
}

int
htable_append_to_file_contents(struct HTable *htable,
                               const char *key,
                               const void *contents,
                               size_t size_in_bytes,
                               struct File **evicted,
                               unsigned *evicted_count)
{
	struct File *file = htable_fetch_file(htable, key);
	if (!file) {
		return -1;
	}
	file->length_in_bytes += size_in_bytes;
	void *new_buffer = xrealloc(file->contents, file->length_in_bytes);
	file->contents = new_buffer;
	memcpy((char *)(new_buffer) + file->length_in_bytes - size_in_bytes,
	       contents,
	       size_in_bytes);
	int err = htable_release(htable, key);
	if (err) {
		return -1;
	}
	return htable_evict_files(htable, false, size_in_bytes, evicted, evicted_count);
}

struct HTableVisitor
{
	struct HTable *htable;
	unsigned max_visits;
	unsigned visits;
	size_t bucket_i;
	struct HTableLlNode *last_visited;
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
	err |= pthread_mutex_lock(&bucket->guard);
	if (err < 0) {
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
	struct HTableLlNode *next_in_bucket = visitor->last_visited->next;
	/* The current bucket doesn't have any more items, so let's unlock this one
	 * and lock the next one, until we find one with at least one item. */
	while (!next_in_bucket) {
		int err = pthread_mutex_unlock(&bucket->guard);
		visitor->bucket_i++;
		if (visitor->bucket_i == htable->buckets_count) {
			/* No more buckets! */
			return NULL;
		}
		bucket++;
		err |= pthread_mutex_lock(&bucket->guard);
		if (err < 0) {
			return NULL;
		}
		next_in_bucket = bucket->head;
	}
	visitor->last_visited = next_in_bucket;
}
