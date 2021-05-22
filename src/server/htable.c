#include "htable.h"
#include "xxHash/xxhash.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define XXHASH_SEED 0
#define GROWTH_FACTOR 2

struct HTableLlNode
{
	struct HTableItem item;
	struct HTableLlNode *next;
};

struct HTableBucket
{
	struct HTableLlNode *head;
	struct HTableLlNode *last;
};

struct HTable
{
	size_t items_count;
	size_t buckets_count;
	struct HTableBucket *buckets;
};

struct HTable *
htable_new(size_t buckets)
{
	assert(buckets > 0);
	struct HTable *htable = malloc(sizeof(struct HTable));
	if (!htable) {
		return NULL;
	}
	htable->items_count = 0;
	htable->buckets_count = buckets;
	htable->buckets = malloc(sizeof(struct HTableLlNode) * buckets);
	if (!htable->buckets) {
		free(htable);
		return NULL;
	}
	for (size_t i = 0; i < buckets; i++) {
		htable->buckets[i].head = NULL;
		htable->buckets[i].last = NULL;
	}
	return htable;
}

void
htable_free(struct HTable *htable)
{
	if (!htable) {
		return;
	}
	free(htable->buckets);
	free(htable);
}

struct HTableItem *
htable_get(const struct HTable *htable, const char *key)
{
	uint64_t hash = XXH64(key, strlen(key), XXHASH_SEED);
	size_t bucket_i = hash % htable->buckets_count;
	struct HTableLlNode *node = htable->buckets[bucket_i].head;
	while (node) {
		if (node->item.key == key) {
			return &node->item;
		}
		node = node->next;
	}
	return NULL;
}

struct HTableItem *
htable_insert(struct HTable *htable, const char *key)
{
	struct HTableItem *found = htable_get(htable, key);
	if (found) {
		return found;
	}
	uint64_t hash = XXH64(key, strlen(key), XXHASH_SEED);
	size_t bucket_i = hash % htable->buckets_count;
	struct HTableBucket *bucket = &htable->buckets[bucket_i];
	struct HTableLlNode *node = malloc(sizeof(struct HTableLlNode));
	if (!node) {
		return NULL;
	}
	htable->items_count++;
	node->next = NULL;
	bucket->last = node;
	if (!bucket->head) {
		bucket->head = node;
	}
	return &(node->item);
}

struct HTable *
htable_clone(const struct HTable *htable)
{
	// Let's keep the load fuctor under 1.0. We technically can tolerate more as
	// we're using chaining rather than open addressing, but it's a good
	// default.
	if (htable->items_count <= htable->buckets_count) {
		return NULL;
	}
	struct HTable *new = htable_new(htable->buckets_count * GROWTH_FACTOR);
	if (!new) {
		return NULL;
	}
	for (size_t i = 0; i < htable->buckets_count; i++) {
		struct HTableBucket *bucket = &htable->buckets[i];
		struct HTableLlNode *node = bucket->head;
		while (node) {
			struct HTableItem *item = htable_insert(new, node->item.key);
			item->key = node->item.key;
			item->value = node->item.value;
			node = node->next;
		}
	}
	return new;
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
