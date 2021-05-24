#ifndef SOL_SERVER_REPORT
#define SOL_SERVER_REPORT

#include <stdlib.h>

struct Report
{
	unsigned long num_reads;
	size_t avg_read_size_in_bytes;
	unsigned long num_writes;
	size_t avg_write_size_in_bytes;
	unsigned long num_locks;
	unsigned long num_unlocks;
	unsigned long num_closes;
	size_t max_storage_size_in_bytes;
	unsigned long max_files_count;
	unsigned long num_evictions;
	unsigned long *num_requests_by_workers;
	unsigned num_workers;
	unsigned max_concurrent_connections;
};

void
report_print(struct Report *report);

#endif
