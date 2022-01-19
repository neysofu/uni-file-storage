#include "config.h"
#include "global_state.h"
#include "tomlc99/toml.h"
#include "utilities.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Config *
config_parse_file(char abs_path[])
{
	assert(abs_path);
	toml_table_t *toml = NULL;
	struct Config *config = xmalloc(sizeof(struct Config));
	FILE *f = fopen(abs_path, "r");
	if (!f) {
		glog_fatal("Configuration file open failed.");
		goto err;
	}
	glog_trace("Opened configuration file.");
	toml = toml_parse_file(f, NULL, 0);
	if (!toml) {
		glog_fatal("Bad configuration TOML.");
		goto err;
	}
	toml_table_t *toml_table = toml_table_in(toml, "server");
	if (!toml_table) {
		glog_fatal("Malformed TOML [server] in configuration file.");
		goto err;
	}
	toml_datum_t param_max_files = toml_int_in(toml_table, "max-files");
	toml_datum_t param_max_storage = toml_int_in(toml_table, "max-storage");
	toml_datum_t param_num_workers = toml_int_in(toml_table, "num-workers");
	toml_datum_t param_socket_filepath = toml_string_in(toml_table, "socket-filepath");
	toml_datum_t param_cache_eviction_policy =
	  toml_string_in(toml_table, "cache-eviction-policy");
	toml_datum_t param_log_filepath = toml_string_in(toml_table, "log-filepath");
	if (!param_max_files.ok || !param_max_storage.ok || !param_num_workers.ok ||
	    !param_socket_filepath.ok || !param_cache_eviction_policy.ok ||
	    !param_log_filepath.ok || param_max_files.u.i < 1 ||
	    param_max_storage.u.i < 10000 || param_num_workers.u.i < 1 ||
	    param_num_workers.u.i > 32) {
		glog_fatal("Malformed TOML attributes in configuration file.");
		goto err;
	}
	config->max_files = param_max_files.u.i;
	config->max_storage_in_bytes = param_max_storage.u.i;
	config->num_workers = param_num_workers.u.i;
	config->socket_filepath = param_socket_filepath.u.s;
	if (strcmp(param_cache_eviction_policy.u.s, "fifo") == 0) {
		config->cache_eviction_policy = CACHE_EVICTION_POLICY_FIFO;
	} else if (strcmp(param_cache_eviction_policy.u.s, "segmented-fifo") == 0) {
		config->cache_eviction_policy = CACHE_EVICTION_POLICY_SEGMENTED_FIFO;
	} else {
		glog_fatal("Invalid cache eviction policy.");
		goto err;
	}
	config->log_filepath = param_log_filepath.u.s;
	config->log_f = fopen(config->socket_filepath, "a");
	config->err = 0;
	toml_free(toml);
	fclose(f);
	return config;
err:
	toml_free(toml);
	free(config);
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
	if (config->log_f) {
		fclose(config->log_f);
	}
	free(config->socket_filepath);
	free(config->log_filepath);
	free(config);
}
