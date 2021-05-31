#include "htable.h"
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
htable_new(size_t buckets, enum CacheReplacementPolicy policy)
{
	assert(buckets > 0);
	struct HTable *htable = malloc(sizeof(struct HTable));
	if (!htable) {
		return NULL;
	}
	// FIXME: max limits
	htable->policy = policy;
	int err = pthread_mutex_init(&htable->stats_guard, NULL);
	if (err) {
		free(htable);
		return NULL;
	}
	htable->items_count = 0;
	htable->max_items_count = 0;
	htable->total_space_in_bytes = 0;
	htable->max_space_in_bytes = 0;
	htable->buckets_count = buckets;
	htable->buckets = malloc(sizeof(struct HTableLlNode) * buckets);
	if (!htable->buckets) {
		free(htable);
		return NULL;
	}
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
htable_fetch_node(struct HTable *htable, const char *key)
{
	assert(htable);
	assert(key);
	int err = 0;
	uint64_t hash = XXH64(key, strlen(key), XXHASH_SEED);
	size_t bucket_i = hash % htable->buckets_count;
	struct HTableBucket *bucket = &htable->buckets[bucket_i];
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
	struct HTableLlNode *node = htable_fetch_node(htable, key);
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
	if (new_file) {
		htable->items_count++;
	}
	htable->total_space_in_bytes += new_space_in_bytes;
	// TODO: evict files...
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
	struct HTableLlNode *node = htable_fetch_node(htable, key);
	if (!node) {
		return -1;
	}
	/* Chain together the previous and next nodes within the bucket's linked
	 * list. */
	if (node->prev) {
		node->prev->next = node->next;
	}
	if (node->next) {
		node->next->prev = node->prev;
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
	file->contents = malloc(size_in_bytes);
	// TODO check malloc
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
	void *new_buffer = realloc(file->contents, file->length_in_bytes);
	// TODO malloc check
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

int
htable_remove(struct HTable *htable, const char *key)
{
	uint64_t hash = XXH64(key, strlen(key), XXHASH_SEED);
	size_t bucket_i = hash % htable->buckets_count;
	struct HTableBucket *bucket = &htable->buckets[bucket_i];
	struct HTableLlNode *node = bucket->head;
	struct HTableLlNode *prev_node = NULL;
	while (node) {
		if (node->item.key == key) {
			break;
		}
		prev_node = node;
		node = node->next;
	}
	htable->items_count--;
	prev_node->next = node->next;
	return 0;
}
