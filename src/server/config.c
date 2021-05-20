#include "config.h"
#include "tomlc99/toml.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct Config *
config_parse_file(char abs_path[], int *err)
{
	assert(err);
	*err = 0;
	FILE *f = fopen(abs_path, "r");
	if (!f) {
		*err = 1;
		return NULL;
	}
	toml_table_t *toml = toml_parse_file(f, NULL, 0);
	if (!toml) {
		fclose(f);
		*err = 1;
		return NULL;
	}
	toml_table_t *toml_table = toml_table_in(toml, "server");
	if (!toml_table) {
		toml_free(toml);
		fclose(f);
		*err = 1;
		return NULL;
	}
	toml_datum_t param_max_files = toml_int_in(toml_table, "max-files");
	toml_datum_t param_max_storage = toml_int_in(toml_table, "max-storage");
	if (!param_max_files.ok || !param_max_storage.ok) {
		toml_free(toml);
		fclose(f);
		*err = 1;
		return NULL;
	}
	int max_files = param_max_files.u.i;
	int max_storage_in_bytes = param_max_storage.u.i;
	struct Config *config = malloc(sizeof(struct Config));
	if (!config) {
		toml_free(toml);
		fclose(f);
		*err = 1;
		return NULL;
	}
	config->max_files = max_files;
	config->max_storage_in_bytes = max_storage_in_bytes;
	toml_free(toml);
	fclose(f);
	return config;
}

void
config_delete(struct Config *config)
{
	free(config);
}