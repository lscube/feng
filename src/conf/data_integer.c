#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"

static data_unset *data_integer_copy(const data_unset *s) {
	data_integer *src = (data_integer *)s;
	data_integer *ds = data_integer_init();

    g_string_assign(ds->key, src->key->str);
	ds->is_index_key = src->is_index_key;
	ds->value = src->value;
	return (data_unset *)ds;
}

static void data_integer_free(data_unset *d) {
	data_integer *ds = (data_integer *)d;

    g_string_free(ds->key, true);

	free(d);
}

static void data_integer_reset(data_unset *d) {
	data_integer *ds = (data_integer *)d;

	/* reused integer elements */
    g_string_assign(ds->key, "");
	ds->value = 0;
}

static int data_integer_insert_dup(ATTR_UNUSED data_unset *dst, data_unset *src) {
	src->free(src);

	return 0;
}

data_integer *data_integer_init(void) {
	data_integer *ds;

	ds = calloc(1, sizeof(*ds));

	ds->key = g_string_new("");
	ds->value = 0;

	ds->copy = data_integer_copy;
	ds->free = data_integer_free;
	ds->reset = data_integer_reset;
	ds->insert_dup = data_integer_insert_dup;
	ds->type = TYPE_INTEGER;

	return ds;
}
