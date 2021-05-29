#include "config.h"
#include "logc/src/log.h"
#include "tomlc99/toml.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct Config *
config_parse_file(char abs_path[])
{
	assert(abs_path);
	toml_table_t *toml = NULL;
	struct Config *config = malloc(sizeof(struct Config));
	if (!config) {
		return NULL;
	}
	FILE *f = fopen(abs_path, "r");
	if (!f) {
		log_fatal("Configuration file open failed.");
		goto err;
	}
	log_trace("Opened configuration file.");
	toml = toml_parse_file(f, NULL, 0);
	if (!toml) {
		log_fatal("Bad configuration TOML.");
		goto err;
	}
	toml_table_t *toml_table = toml_table_in(toml, "server");
	if (!toml_table) {
		log_fatal("Malformed TOML [server] in configuration file.");
		goto err;
	}
	toml_datum_t param_max_files = toml_int_in(toml_table, "max-files");
	toml_datum_t param_max_storage = toml_int_in(toml_table, "max-storage");
	toml_datum_t param_num_workers = toml_int_in(toml_table, "num-workers");
	toml_datum_t param_socket_filepath = toml_string_in(toml_table, "socket-filepath");
	toml_datum_t param_log_filepath = toml_string_in(toml_table, "log-filepath");
	if (!param_max_files.ok || !param_max_storage.ok || !param_num_workers.ok ||
	    !param_socket_filepath.ok || !param_log_filepath.ok) {
		log_fatal("Malformed TOML attributes in configuration file.");
		goto err;
	}
	config->max_files = param_max_files.u.i;
	config->max_storage_in_bytes = param_max_storage.u.i;
	config->num_workers = param_num_workers.u.i;
	config->socket_filepath = param_socket_filepath.u.s;
	config->log_filepath = param_log_filepath.u.s;
	config->err = 0;
	toml_free(toml);
	fclose(f);
	return config;
err:
	toml_free(toml);
	if (f) {
		fclose(f);
	}
	return config;
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
