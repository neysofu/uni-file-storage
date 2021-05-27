#ifndef SOL_SERVER_CONFIG
#define SOL_SERVER_CONFIG

/* Server configuration settings. */
struct Config
{
	unsigned max_files;
	unsigned max_storage_in_bytes;
	unsigned num_workers;
	char *socket_filepath;
	char *log_filepath;
};

struct Config *
config_parse_file(char abs_path[], int *err);

/* Frees all memory used by `config` after `config_parse_file`. */
void
config_free(struct Config *config);

#endif
