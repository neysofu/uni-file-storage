#ifndef SOL_HTABLE
#define SOL_HTABLE

#include <stdlib.h>

struct HTable;

struct HTableItem {
    char *key;
    char *value;
};

struct HTable *
htable_new(size_t buckets);

void
htable_free(struct HTable *htable);

struct HTableItem *
htable_get(const struct HTable *htable, const char *key);

struct HTableItem *
htable_insert(struct HTable *htable, const char *key);

int
htable_remove(struct HTable *htable, const char *key);

#endif
