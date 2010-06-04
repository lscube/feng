/**
 * Copyright (c) 2004, Jan Kneschke, incremental
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the 'incremental' nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file configfile-glue.c
 * @brief glue code for configuration
 * like all glue code this file contains functions which
 * are the external interface of lighttpd. The functions
 * are used by the server itself and the plugins.
 *
 * The main-goal is to have a small library in the end
 * which is linked against both and which will define
 * the interface itself in the end.
 *
 */

#include <glib.h>

#include <string.h>

//#include "base.h"
#include "buffer.h"
#include "array.h"
// #include "log.h"
// #include "plugin.h"

#include "configfile.h"

/* handle global options */

/** parse config array */
static int config_insert_values_internal(array *ca, const config_values_t cv[]) {
    size_t i;
    data_unset *du;

    for (i = 0; cv[i].key; i++) {

        if (NULL == (du = array_get_element(ca, cv[i].key))) {
            /* no found */

            continue;
        }

        switch (cv[i].type) {
        case T_CONFIG_ARRAY:
            if (du->type == TYPE_ARRAY) {
                size_t j;
                data_array *da = (data_array *)du;

                for (j = 0; j < da->value->used; j++) {
                    if (da->value->data[j]->type == TYPE_STRING) {
                        data_string *ds = data_string_init();

                        buffer_copy_string_buffer(ds->value, ((data_string *)(da->value->data[j]))->value);
                        if (!da->is_index_key) {
                            /* the id's were generated automaticly, as we copy now we might have to renumber them
                             * this is used to prepend server.modules by mod_indexfiles as it has to be loaded
                             * before mod_fastcgi and friends */
                            buffer_copy_string_buffer(ds->key, ((data_string *)(da->value->data[j]))->key);
                        }

                        array_insert_unique(cv[i].destination, (data_unset *)ds);
                    } else {
                        return -1;
                    }
                }
            } else {
                return -1;
            }
            break;
        case T_CONFIG_STRING:
            if (du->type == TYPE_STRING) {
                data_string *ds = (data_string *)du;

                buffer_copy_string_buffer(cv[i].destination, ds->value);
            } else {
                return -1;
            }
            break;
        case T_CONFIG_SHORT:
            switch(du->type) {
            case TYPE_INTEGER: {
                data_integer *di = (data_integer *)du;

                *((unsigned short *)(cv[i].destination)) = di->value;
                break;
            }
            default:
                return -1;
            }
            break;
        case T_CONFIG_BOOLEAN:
            if (du->type == TYPE_STRING) {
                data_string *ds = (data_string *)du;

                if (buffer_is_equal_string(ds->value, CONST_STR_LEN("enable"))) {
                    *((unsigned short *)(cv[i].destination)) = 1;
                } else if (buffer_is_equal_string(ds->value, CONST_STR_LEN("disable"))) {
                    *((unsigned short *)(cv[i].destination)) = 0;
                } else {
                    return -1;
                }
            } else {
                return -1;
            }
            break;
        }
    }

    return 0;
}

/**
 * Insert configuration value into the global array
 */

int config_insert_values_global(server *srv, array *ca, const config_values_t cv[]) {
    size_t i;
    data_unset *du;

    for (i = 0; cv[i].key; i++) {
        data_string *touched;

        if (NULL == (du = array_get_element(ca, cv[i].key))) {
            /* no found */

            continue;
        }

        /* touched */
        touched = data_string_init();

        buffer_copy_string(touched->value, "");
        buffer_copy_string_buffer(touched->key, du->key);

        array_insert_unique(srv->config_touched, (data_unset *)touched);
    }

    return config_insert_values_internal(ca, cv);
}
