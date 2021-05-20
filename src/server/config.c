#include "config.h"
#include "tomlc99/toml.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct Config *
config_parse_file(char abs_path[], int *err)
{
	assert(abs_path);
	assert(err);
	*err = 0;
	FILE *f = fopen(abs_path, "r");
	if (!f) {
		*err = 1;
		goto cleanup_after_error;
	}
	toml_table_t *toml = toml_parse_file(f, NULL, 0);
	if (!toml) {
		*err = 1;
		goto cleanup_after_error;
	}
	toml_table_t *toml_table = toml_table_in(toml, "server");
	if (!toml_table) {
		*err = 1;
		goto cleanup_after_error;
	}
	toml_datum_t param_max_files = toml_int_in(toml_table, "max-files");
	toml_datum_t param_max_storage = toml_int_in(toml_table, "max-storage");
	toml_datum_t param_num_workers = toml_int_in(toml_table, "num-workers");
	toml_datum_t param_socket_filepath = toml_string_in(toml_table, "socket-filepath");
	toml_datum_t param_log_filepath = toml_string_in(toml_table, "log-filepath");
	if (!param_max_files.ok || !param_max_storage.ok || !param_num_workers.ok ||
	    !param_socket_filepath.ok || !param_log_filepath.ok) {
		*err = 1;
		goto cleanup_after_error;
	}
	unsigned max_files = param_max_files.u.i;
	unsigned max_storage_in_bytes = param_max_storage.u.i;
	unsigned num_workers = param_num_workers.u.i;
	char *socket_filepath = param_socket_filepath.u.s;
	char *log_filepath = param_log_filepath.u.s;
	struct Config *config = malloc(sizeof(struct Config));
	if (!config) {
		*err = 1;
		goto cleanup_after_error;
	}
	config->max_files = max_files;
	config->max_storage_in_bytes = max_storage_in_bytes;
	config->num_workers = num_workers;
	config->socket_filepath = socket_filepath;
	config->log_filepath = log_filepath;
	toml_free(toml);
	fclose(f);
	return config;
cleanup_after_error:
	toml_free(toml);
	fclose(f);
	return NULL;
}

void
config_delete(struct Config *config)
{
	if (!config) {
		return;
	}
	free(config->socket_filepath);
	free(config->log_filepath);
	free(config);
}