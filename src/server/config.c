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
	toml_table_t *toml = NULL;
	FILE *f = fopen(abs_path, "r");
	struct Config *config = malloc(sizeof(struct Config));
	if (!config || !f) {
		*err = 1;
		goto cleanup_after_error;
	}
	toml = toml_parse_file(f, NULL, 0);
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
	config->max_files = param_max_files.u.i;
	config->max_storage_in_bytes = param_max_storage.u.i;
	config->num_workers = param_num_workers.u.i;
	config->socket_filepath = param_socket_filepath.u.s;
	config->log_filepath = param_log_filepath.u.s;
	toml_free(toml);
	fclose(f);
	return config;
cleanup_after_error:
	free(config);
	toml_free(toml);
	if (f) {
		fclose(f);
	}
	return NULL;
}

void
config_free(struct Config *config)
{
	if (!config) {
		return;
	}
	free(config->socket_filepath);
	free(config->log_filepath);
	free(config);
}
