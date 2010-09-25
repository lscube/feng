#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "array.h"

static data_unset *data_string_copy(const data_unset *s) {
	data_string *src = (data_string *)s;
	data_string *ds = data_string_init();

    g_string_assign(ds->key, src->key->str);
    g_string_assign(ds->value, src->value->str);
	ds->is_index_key = src->is_index_key;
	return (data_unset *)ds;
}

static void data_string_free(data_unset *d) {
	data_string *ds = (data_string *)d;

    g_string_free(ds->key, true);
    g_string_free(ds->value, true);

	free(d);
}

static void data_string_reset(data_unset *d) {
	data_string *ds = (data_string *)d;

	/* reused array elements */
    g_string_assign(ds->key, "");
    g_string_assign(ds->value, "");
}

static int data_string_insert_dup(data_unset *dst, data_unset *src) {
	data_string *ds_dst = (data_string *)dst;
	data_string *ds_src = (data_string *)src;

	if (ds_dst->value->len) {
        g_string_append_printf(ds_dst->value, ", %s", ds_src->value->str);
	} else {
        g_string_assign(ds_dst->value, ds_src->value->str);
	}

	src->free(src);

	return 0;
}

static void data_string_print(const data_unset *d, ATTR_UNUSED int depth) {
	data_string *ds = (data_string *)d;

	fprintf(stdout, "\"%s\"", ds->value->str);
}


data_string *data_string_init(void) {
	data_string *ds;

	ds = calloc(1, sizeof(*ds));
	assert(ds);

	ds->key = g_string_new("");
	ds->value = g_string_new("");

	ds->copy = data_string_copy;
	ds->free = data_string_free;
	ds->reset = data_string_reset;
	ds->insert_dup = data_string_insert_dup;
	ds->print = data_string_print;
	ds->type = TYPE_STRING;

	return ds;
}
