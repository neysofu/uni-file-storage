#ifndef SOL_SERVER_CONFIG
#define SOL_SERVER_CONFIG

/* Server configuration settings. */
struct Config
{
	unsigned max_files;
	unsigned max_storage_in_bytes;
	unsigned num_workers;
	char *socket_filepath;
	char *glog_filepath;
	int err;
};

/* Allocates a `struct Config` on the heap and it fills it in with values read
 * from the TOML file found at `abs_path`. */
struct Config *
config_parse_file(char abs_path[]);

/* Frees all memory used by `config` after `config_parse_file`. */
void
config_free(struct Config *config);

#endif
