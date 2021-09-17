#ifndef SOL_SERVER_FIFO_CACHE
#define SOL_SERVER_FIFO_CACHE

#include <stdlib.h>

struct FifoCache;

/* Creates a new, empty FIFO cache that can contain up to `max` items and
 * `max_size_in_bytes` space. Returns NULL on initialization failure.
 */
struct FifoCache *
fifo_cache_create(size_t max, size_t max_size_in_bytes);

/* Tries to add a file path to `fcache`; evicts as many files as necessary to fit
 * the new file. This function call is thread-safe. */
int
fifo_cache_add(struct FifoCache *fcache,
               char *key,
               size_t size_in_bytes,
               char **evicted,
               unsigned *evicted_count);

/* Frees all memory internally used by `fcache`, if not NULL. */
void
fifo_cache_free(struct FifoCache *fcache);

#endif
