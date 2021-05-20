#ifndef SOL_SERVER_CONFIG
#define SOL_SERVER_CONFIG

struct Config
{
	unsigned max_files;
	unsigned max_storage_in_bytes;
};

struct Config *
config_parse_file(char abs_path[], int *err);

void
config_delete(struct Config *config);

#endif
