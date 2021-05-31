#ifndef SOL_SERVER_HTABLE
#define SOL_SERVER_HTABLE

#include <stdlib.h>

/* A specialized, concurrent hash table for the file storage server. */
struct HTable;

/* File data. */
struct File
{
	void *contents;
	size_t contents_length_in_bytes;
	int flags;
};

/* The contents of an item within a `struct HTable`. */
struct HTableItem
{
	char *key;
	struct File value;
};

/* Creates an empty `struct HTable`. */
struct HTable *
htable_new(size_t buckets);

/* Deletes `htable` and all its contents from memory (files, flags, etc.). */
void
htable_free(struct HTable *htable);

struct HTableItem *
htable_get(const struct HTable *htable, const char *key);

struct HTableItem *
htable_insert(struct HTable *htable, const char *key);

int
htable_remove(struct HTable *htable, const char *key);

#endif
