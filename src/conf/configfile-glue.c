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
            case TYPE_STRING: {
                return -1;
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
        case T_CONFIG_LOCAL:
        case T_CONFIG_UNSET:
            break;
        case T_CONFIG_UNSUPPORTED:
            break;
        case T_CONFIG_DEPRECATED:
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
#if 0
unsigned short sock_addr_get_port(sock_addr *addr) {
#ifdef HAVE_IPV6
    return ntohs(addr->plain.sa_family ? addr->ipv6.sin6_port : addr->ipv4.sin_port);
#else
    return ntohs(addr->ipv4.sin_port);
#endif
}

static cond_result_t config_check_cond_cached(server *srv, connection *con, data_config *dc);

static cond_result_t config_check_cond_nocache(server *srv, connection *con, data_config *dc) {
    conf_buffer *l;
    server_socket *srv_sock = con->srv_socket;

    /* check parent first */
    if (dc->parent && dc->parent->context_ndx) {
        switch (config_check_cond_cached(srv, con, dc->parent)) {
        case COND_RESULT_FALSE:
            return COND_RESULT_FALSE;
        case COND_RESULT_UNSET:
            return COND_RESULT_UNSET;
        default:
            break;
        }
    }

    if (dc->prev) {
        /* make sure prev is checked first */
        config_check_cond_cached(srv, con, dc->prev);

        /* one of prev set me to FALSE */
        switch (con->cond_cache[dc->context_ndx].result) {
        case COND_RESULT_FALSE:
            return con->cond_cache[dc->context_ndx].result;
        default:
            break;
        }
    }

    if (!con->conditional_is_valid[dc->comp]) {
        return COND_RESULT_UNSET;
    }

    /* pass the rules */

    switch (dc->comp) {
    case COMP_HTTP_HOST: {
        char *ck_colon = NULL, *val_colon = NULL;

        if (!buffer_is_empty(con->uri.authority)) {

            /*
             * append server-port to the HTTP_POST if necessary
             */

            l = con->uri.authority;

            switch(dc->cond) {
            case CONFIG_COND_NE:
            case CONFIG_COND_EQ:
                ck_colon = strchr(dc->string->ptr, ':');
                val_colon = strchr(l->ptr, ':');

                if (ck_colon == val_colon) {
                    /* nothing to do with it */
                    break;
                }
                if (ck_colon) {
                    /* condition "host:port" but client send "host" */
                    buffer_copy_string_buffer(srv->cond_check_buf, l);
                    BUFFER_APPEND_STRING_CONST(srv->cond_check_buf, ":");
                    buffer_append_long(srv->cond_check_buf, sock_addr_get_port(&(srv_sock->addr)));
                    l = srv->cond_check_buf;
                } else if (!ck_colon) {
                    /* condition "host" but client send "host:port" */
                    buffer_copy_string_len(srv->cond_check_buf, l->ptr, val_colon - l->ptr);
                    l = srv->cond_check_buf;
                }
                break;
            default:
                break;
            }
        } else {
            l = srv->empty_string;
        }
        break;
    }
    case COMP_HTTP_REMOTEIP: {
        char *nm_slash;
        /* handle remoteip limitations
         *
         * "10.0.0.1" is provided for all comparisions
         *
         * only for == and != we support
         *
         * "10.0.0.1/24"
         */

        if ((dc->cond == CONFIG_COND_EQ ||
             dc->cond == CONFIG_COND_NE) &&
            (con->dst_addr.plain.sa_family == AF_INET) &&
            (NULL != (nm_slash = strchr(dc->string->ptr, '/')))) {
            int nm_bits;
            long nm;
            char *err;
            struct in_addr val_inp;

            if (*(nm_slash+1) == '\0')
                return COND_RESULT_FALSE;

            nm_bits = strtol(nm_slash + 1, &err, 10);

            if (*err)

            /* take IP convert to the native */
            buffer_copy_string_len(srv->cond_check_buf, dc->string->ptr, nm_slash - dc->string->ptr);
#ifdef __WIN32
            if (INADDR_NONE == (val_inp.s_addr = inet_addr(srv->cond_check_buf->ptr)))
                return COND_RESULT_FALSE;

#else
            if (0 == inet_aton(srv->cond_check_buf->ptr, &val_inp))
                return COND_RESULT_FALSE;
#endif

            /* build netmask */
            nm = htonl(~((1 << (32 - nm_bits)) - 1));

            if ((val_inp.s_addr & nm) == (con->dst_addr.ipv4.sin_addr.s_addr & nm)) {
                return (dc->cond == CONFIG_COND_EQ) ? COND_RESULT_TRUE : COND_RESULT_FALSE;
            } else {
                return (dc->cond == CONFIG_COND_EQ) ? COND_RESULT_FALSE : COND_RESULT_TRUE;
            }
        } else {
            l = con->dst_addr_buf;
        }
        break;
    }
    case COMP_HTTP_URL:
        l = con->uri.path;
        break;

    case COMP_HTTP_QUERYSTRING:
        l = con->uri.query;
        break;

    case COMP_SERVER_SOCKET:
        l = srv_sock->srv_token;
        break;

    case COMP_HTTP_REFERER: {
        data_string *ds;

        if (NULL != (ds = (data_string *)array_get_element(con->request.headers, "Referer"))) {
            l = ds->value;
        } else {
            l = srv->empty_string;
        }
        break;
    }
    case COMP_HTTP_COOKIE: {
        data_string *ds;
        if (NULL != (ds = (data_string *)array_get_element(con->request.headers, "Cookie"))) {
            l = ds->value;
        } else {
            l = srv->empty_string;
        }
        break;
    }
    case COMP_HTTP_USERAGENT: {
        data_string *ds;
        if (NULL != (ds = (data_string *)array_get_element(con->request.headers, "User-Agent"))) {
            l = ds->value;
        } else {
            l = srv->empty_string;
        }
        break;
    }

    default:
        return COND_RESULT_FALSE;
    }

    if (NULL == l) {
        return COND_RESULT_FALSE;
    }

    switch(dc->cond) {
    case CONFIG_COND_NE:
    case CONFIG_COND_EQ:
        if (buffer_is_equal(l, dc->string)) {
            return (dc->cond == CONFIG_COND_EQ) ? COND_RESULT_TRUE : COND_RESULT_FALSE;
        } else {
            return (dc->cond == CONFIG_COND_EQ) ? COND_RESULT_FALSE : COND_RESULT_TRUE;
        }
        break;
#ifdef HAVE_PCRE_H
    case CONFIG_COND_NOMATCH:
    case CONFIG_COND_MATCH: {
        cond_cache_t *cache = &con->cond_cache[dc->context_ndx];
        int n;

#ifndef elementsof
#define elementsof(x) (sizeof(x) / sizeof(x[0]))
#endif
        n = pcre_exec(dc->regex, dc->regex_study, l->ptr, l->used - 1, 0, 0,
                cache->matches, elementsof(cache->matches));

        cache->patterncount = n;
        if (n > 0) {
            cache->comp_value = l;
            cache->comp_type  = dc->comp;
            return (dc->cond == CONFIG_COND_MATCH) ? COND_RESULT_TRUE : COND_RESULT_FALSE;
        } else {
            /* cache is already cleared */
            return (dc->cond == CONFIG_COND_MATCH) ? COND_RESULT_FALSE : COND_RESULT_TRUE;
        }
        break;
    }
#endif
    default:
        /* no way */
        break;
    }

    return COND_RESULT_FALSE;
}

static cond_result_t config_check_cond_cached(server *srv, connection *con, data_config *dc) {
    cond_cache_t *caches = con->cond_cache;

    if (COND_RESULT_UNSET == caches[dc->context_ndx].result) {
        if (COND_RESULT_TRUE == (caches[dc->context_ndx].result = config_check_cond_nocache(srv, con, dc))) {
            if (dc->next) {
                data_config *c;
                for (c = dc->next; c; c = c->next) {
                    caches[c->context_ndx].result = COND_RESULT_FALSE;
                }
            }
        }
        caches[dc->context_ndx].comp_type = dc->comp;
    }
    return caches[dc->context_ndx].result;
}

/**
 * reset the config-cache for a named item
 *
 * if the item is COND_LAST_ELEMENT we reset all items
 */
void config_cond_cache_reset_item(server *srv, connection *con, comp_key_t item) {
    size_t i;

    for (i = 0; i < srv->config_context->used; i++) {
        if (item == COMP_LAST_ELEMENT ||
            con->cond_cache[i].comp_type == item) {
            con->cond_cache[i].result = COND_RESULT_UNSET;
            con->cond_cache[i].patterncount = 0;
            con->cond_cache[i].comp_value = NULL;
        }
    }
}

/**
 * reset the config cache to its initial state at connection start
 */
void config_cond_cache_reset(server *srv, connection *con) {
    size_t i;

    config_cond_cache_reset_all_items(srv, con);

    for (i = 0; i < COMP_LAST_ELEMENT; i++) {
        con->conditional_is_valid[i] = 0;
    }
}

int config_check_cond(server *srv, connection *con, data_config *dc) {
    return (config_check_cond_cached(srv, con, dc) == COND_RESULT_TRUE);
}

int config_append_cond_match_buffer(connection *con, data_config *dc, conf_buffer *buf, int n)
{
    cond_cache_t *cache = &con->cond_cache[dc->context_ndx];
    if (n > cache->patterncount) {
        return 0;
    }

    n <<= 1; /* n *= 2 */
    buffer_append_string_len(buf,
            cache->comp_value->ptr + cache->matches[n],
            cache->matches[n + 1] - cache->matches[n]);
    return 1;
}

#endif
