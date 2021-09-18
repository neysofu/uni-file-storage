#include "fifo_cache.h"
#include "utilities.h"
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

/* Items are maintained as nodes in a doubly-linked list. */
struct FifoItem
{
	char *key;
	size_t size_in_bytes;
	struct FifoItem *next;
	struct FifoItem *prev;
};

struct FifoCache
{
	pthread_mutex_t guard;
	/* Where new items are inserted. */
	struct FifoItem *head;
	/* Where items are evicted. */
	struct FifoItem *last;
	size_t count;
	size_t count_max;
	size_t size_in_bytes;
	size_t size_in_bytes_max;
};

struct FifoCache *
fifo_cache_create(size_t max, size_t max_size_in_bytes)
{
	struct FifoCache *fcache = xmalloc(sizeof(struct FifoCache));
	if (pthread_mutex_init(&fcache->guard, NULL)) {
		/* Simply return NULL on initialization failure. */
		free(fcache);
		return NULL;
	}
	fcache->head = NULL;
	fcache->last = NULL;
	fcache->count = 0;
	fcache->count_max = max;
	fcache->size_in_bytes = 0;
	fcache->size_in_bytes_max = max_size_in_bytes;
	return fcache;
}

void
fifo_cache_add_new_head(struct FifoCache *fcache, char *key, size_t size_in_bytes)
{
	struct FifoItem *new_head = xmalloc(sizeof(struct FifoItem));
	new_head->key = key;
	new_head->size_in_bytes = size_in_bytes;
	new_head->next = fcache->head;
	new_head->prev = NULL;
	fcache->head->prev = new_head;
	fcache->head = new_head;
}

unsigned
fifo_cache_count_how_many_evicted(const struct FifoCache *fcache)
{
	unsigned count = 0;
	struct FifoItem *last = fcache->last;
	while (fcache->count > fcache->count_max ||
	       fcache->size_in_bytes > fcache->size_in_bytes_max) {
		assert(last);
		count++;
		last = last->prev;
	}
	return count;
}

void
fifo_cache_evict(struct FifoCache *fcache, unsigned count, char **evicted)
{
	for (unsigned i = 0; i < count; i++) {
		struct FifoItem *last = fcache->last;
		assert(last);
		evicted[i] = last->key;
		last = last->prev;
		free(last->next);
		last->next = NULL;
		fcache->last = last;
	}
}

int
fifo_cache_add(struct FifoCache *fcache,
               char *key,
               size_t size_in_bytes,
               char **evicted,
               unsigned *evicted_count)
{
	assert(fcache);
	int err = 0;
	err = pthread_mutex_lock(&fcache->guard);
	if (err) {
		return err;
	}
	/* Check some representation invariants. */
	assert(fcache->count <= fcache->count_max);
	assert(!fcache->head->prev);
	fifo_cache_add_new_head(fcache, key, size_in_bytes);
	*evicted_count = fifo_cache_count_how_many_evicted(fcache);
	evicted = xmalloc(sizeof(char *) * *evicted_count);
	fifo_cache_evict(fcache, *evicted_count, evicted);
	err = pthread_mutex_unlock(&fcache->guard);
	if (err) {
		return err;
	}
	return 0;
}

void
fifo_cache_free(struct FifoCache *fcache)
{
	if (!fcache) {
		return;
	}
	struct FifoItem *head = fcache->head;
	while (head) {
		struct FifoItem *next = head->next;
		free(head->key);
		free(head);
		head = next;
	}
	free(fcache);
}
