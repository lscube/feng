#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "array.h"

static data_unset *data_array_copy(const data_unset *s) {
	data_array *src = (data_array *)s;
	data_array *ds = data_array_init();

    g_string_assign(ds->key, src->key->str);
	array_free(ds->value);
	ds->value = array_init_array(src->value);
	ds->is_index_key = src->is_index_key;
	return (data_unset *)ds;
}

static void data_array_free(data_unset *d) {
	data_array *ds = (data_array *)d;

    g_string_free(ds->key, true);
	array_free(ds->value);

	free(d);
}

static void data_array_reset(data_unset *d) {
	data_array *ds = (data_array *)d;

	/* reused array elements */
    g_string_assign(ds->key, "");
	array_reset(ds->value);
}

static int data_array_insert_dup(ATTR_UNUSED data_unset *dst, data_unset *src) {
	src->free(src);

	return 0;
}

data_array *data_array_init(void) {
	data_array *ds;

	ds = calloc(1, sizeof(*ds));

	ds->key = g_string_new("");
	ds->value = array_init();

	ds->copy = data_array_copy;
	ds->free = data_array_free;
	ds->reset = data_array_reset;
	ds->insert_dup = data_array_insert_dup;
	ds->type = TYPE_ARRAY;

	return ds;
}
