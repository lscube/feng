#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "array.h"

static data_unset *data_config_copy(const data_unset *s) {
	data_config *src = (data_config *)s;
	data_config *ds = data_config_init();

    g_string_assign(ds->key, src->key->str);
    g_string_assign(ds->comp_key, src->comp_key->str);
	array_free(ds->value);
	ds->value = array_init_array(src->value);
	return (data_unset *)ds;
}

static void data_config_free(data_unset *d) {
	data_config *ds = (data_config *)d;

    g_string_free(ds->key, true);
    g_string_free(ds->op, true);
    g_string_free(ds->comp_key, true);
    g_string_free(ds->string, true);

	array_free(ds->value);
	array_free(ds->childs);

	free(d);
}

static void data_config_reset(data_unset *d) {
	data_config *ds = (data_config *)d;

	/* reused array elements */
    g_string_assign(ds->key, "");
    g_string_assign(ds->comp_key, "");
	array_reset(ds->value);
}

static int data_config_insert_dup(ATTR_UNUSED data_unset *dst, data_unset *src) {
	src->free(src);

	return 0;
}

data_config *data_config_init(void) {
	data_config *ds;

	ds = calloc(1, sizeof(*ds));

	ds->key = g_string_new("");
	ds->op = g_string_new("");
	ds->comp_key = g_string_new("");
	ds->value = array_init();
	ds->childs = array_init();
	ds->childs->is_weakref = 1;

	ds->copy = data_config_copy;
	ds->free = data_config_free;
	ds->reset = data_config_reset;
	ds->insert_dup = data_config_insert_dup;
	ds->type = TYPE_CONFIG;

	return ds;
}
