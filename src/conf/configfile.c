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
 * @file configfile.c
 * Main configuration parsing functions
 */

#include <config.h>

#include <glib.h>

#include <sys/stat.h>

#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "feng.h"
#include "fnc_log.h"
//#include "log.h"

//#include "plugin.h"
#include "network/rtp.h" // defaults
#include "configparser.h"
#include "configfile.h"

typedef enum { T_CONFIG_UNSET,
                T_CONFIG_STRING,
                T_CONFIG_SHORT,
                T_CONFIG_BOOLEAN
} config_values_type_t;

typedef struct {
        const char *key;
        void *destination;

        config_values_type_t type;
} config_values_t;

static int config_insert_values_internal(array *ca, const config_values_t cv[]);

/**
 * Insert the main configuration keys and their defaults
 */

static int config_insert() {
    size_t i;
    char *bindhost = NULL;
    char *bindport = NULL;

    const config_values_t global_cv[] = {
        { "server.bind", &bindhost, T_CONFIG_STRING },      /* 0 */
        { "server.errorlog", &feng_srv.errorlog_file, T_CONFIG_STRING },      /* 1 */
        { "server.username", &feng_srv.username, T_CONFIG_STRING },      /* 2 */
        { "server.groupname", &feng_srv.groupname, T_CONFIG_STRING },      /* 3 */
        { "server.port", &bindport, T_CONFIG_STRING },      /* 4 */
        { "server.errorlog-use-syslog", &feng_srv.errorlog_use_syslog, T_CONFIG_BOOLEAN },     /* 7 */
        { "server.max-connections", &feng_srv.max_conns, T_CONFIG_SHORT },       /* 8 */
        { "server.buffered_frames", &feng_srv.buffered_frames, T_CONFIG_SHORT },
        { "server.loglevel", &feng_srv.loglevel, T_CONFIG_SHORT },
        { "server.twin", &feng_srv.twin, T_CONFIG_STRING },
        { "server.document-root", &feng_srv.document_root, T_CONFIG_STRING },
        { NULL,                          NULL, T_CONFIG_UNSET }
    };

    if (config_insert_values_internal(((data_config *)feng_srv.config_context->data[0])->value, global_cv))
        return -1;

    if (feng_srv.document_root == NULL)
        return -1;

    if (feng_srv.max_conns == 0)
        feng_srv.max_conns = 100;

    if (feng_srv.buffered_frames == 0)
        feng_srv.buffered_frames = BUFFERED_FRAMES_DEFAULT;

    for (i = 0; i < feng_srv.config_context->used; i++) {
        specific_config s = {
#if ENABLE_SCTP
            .sctp_max_streams = 16,
#endif
            .access_log_syslog = 1
        };

        size_t hostlen;

        const config_values_t vhost_cv[] = {
            { "server.use-ipv6", &s.use_ipv6, T_CONFIG_BOOLEAN }, /* 5 */

#if ENABLE_SCTP
            { "sctp.protocol", &s.is_sctp, T_CONFIG_BOOLEAN },
            { "sctp.max_streams", &s.sctp_max_streams, T_CONFIG_SHORT },
#endif
            { "accesslog.filename", &s.access_log_file, T_CONFIG_STRING }, /* 11 */
#if HAVE_SYSLOG_H
            { "accesslog.use-syslog", &s.access_log_syslog, T_CONFIG_BOOLEAN },
#endif
            { NULL,                          NULL, T_CONFIG_UNSET }
        };

        if (config_insert_values_internal(((data_config *)feng_srv.config_context->data[i])->value, vhost_cv))
            return -1;

        /* The first entry is the global scope, so use the temporary values read before */
        if ( i == 0 ) {
            s.host = bindhost;
            s.port = bindport ? bindport : g_strdup(FENG_DEFAULT_PORT);
        } else {
            char *socketstr = g_strdup(((data_config *)feng_srv.config_context->data[i])->string->str);
            char *port = strrchr(socketstr, ':');
            if ( port == NULL ) {
                s.host = g_strdup(socketstr);
                s.port = g_strdup(FENG_DEFAULT_PORT);
            } else {
                s.host = g_strndup(socketstr, port-socketstr);
                s.port = g_strdup(port+1);
            }
        }

        if ( s.host ) {
            if ( s.host[0] == '\0' ) {
                g_free(s.host);
                s.host = NULL;
            } else if ( strcmp(s.host, "0.0.0.0") == 0 ) {
                g_free(s.host);
                s.host = NULL;
            } else if ( strcmp(s.host, "[::]") == 0 ) {
                g_free(s.host);
                s.host = NULL;
                s.use_ipv6 = 1;
            } else {
                hostlen = strlen(s.host);
                if ( s.host[0] == '[' &&
                     s.host[hostlen-1] == ']' ) {
                    s.use_ipv6 = 1;
                    memmove(s.host, s.host+1, hostlen-2);
                    s.host[hostlen-2] = '\0';
                }
            }
        }

        feng_srv.sockets = g_slist_prepend(feng_srv.sockets, g_slice_dup(specific_config, &s));
    }

    return 0;
}

#ifdef CLEANUP_DESTRUCTOR
static void sockets_cleanup(gpointer socket_p, ATTR_UNUSED gpointer user_data)
{
    specific_config *socket = socket_p;

    if ( socket->access_log_fp != NULL )
        fclose(socket->access_log_fp);

    g_free(socket->host);
    g_free(socket->port);
    g_free(socket->access_log_file);

    g_slice_free(specific_config, socket);
}

static void CLEANUP_DESTRUCTOR config_cleanup()
{
    g_slist_foreach(feng_srv.sockets, sockets_cleanup, NULL);
    g_slist_free(feng_srv.sockets);
}
#endif

typedef struct {
    int foo;
    int bar;

    gchar *input;
    size_t offset;
    size_t size;

    int line_pos;
    int line;

    int in_key;
    int in_brace;
    int in_cond;
} tokenizer_t;

/**
 * Skip comments
 * a comment starts with an # and last till a newline
 */

static void config_skip_comment(tokenizer_t *t) {
    char *nextline = strchr(t->input + t->offset, '\n');
    if ( nextline )
        t->offset = nextline - t->input + 1;
    else
        t->offset = t->size;
}

/**
 * Break the configuration in tokens
 */

static int config_tokenizer(tokenizer_t *t, int *token_id, GString *token) {
    int tid = 0;
    size_t i;

    for (tid = 0; tid == 0 && t->offset < t->size && t->input[t->offset] ; ) {
        char c = t->input[t->offset];
        const char *start = NULL;

        switch (c) {
        case '=':
            if (t->in_brace) {
                if (t->input[t->offset + 1] == '>') {
                    t->offset += 2;

                    g_string_assign(token, "=>");

                    tid = TK_ARRAY_ASSIGN;
                } else {
                    return -1;
                }
            } else if (t->in_cond) {
                if (t->input[t->offset + 1] == '=') {
                    t->offset += 2;

                    g_string_assign(token, "==");

                    tid = TK_EQ;
                } else {
                    return -1;
                }
                t->in_key = 1;
                t->in_cond = 0;
            } else if (t->in_key) {
                tid = TK_ASSIGN;

                g_string_append_len(token, t->input + t->offset, 1);

                t->offset++;
                t->line_pos++;
            } else {
                return -1;
            }

            break;
        case '!':
            if (t->in_cond) {
                if (t->input[t->offset + 1] == '=') {
                    t->offset += 2;

                    g_string_assign(token, "!=");

                    tid = TK_NE;
                } else {
                    return -1;
                }
                t->in_key = 1;
                t->in_cond = 0;
            } else {
                return -1;
            }

            break;
        case '\t':
        case '\r':
        case ' ':
            t->offset++;
            t->line_pos++;
            break;
        case '\n':
            if (t->in_brace == 0) {
                int done = 0;
                while (!done && t->offset < t->size) {
                    switch (t->input[t->offset]) {
                    case '\n':
                        t->offset++;
                        t->line_pos = 1;
                        t->line++;
                        break;

                    case '#':
                        config_skip_comment(t);
                        t->line_pos = 1;
                        t->line++;
                        break;

                    case '\t':
                    case '\r':
                    case ' ':
                        t->offset++;
                        t->line_pos++;
                        break;

                    default:
                        done = 1;
                    }
                }
                t->in_key = 1;
                tid = TK_EOL;
                g_string_assign(token, "(EOL)");
            } else {
                t->offset++;
                t->line_pos = 1;
                t->line++;
            }
            break;
        case ',':
            if (t->in_brace > 0) {
                tid = TK_COMMA;

                g_string_assign(token, "(COMMA)");
            }

            t->offset++;
            t->line_pos++;
            break;
        case '"':
            /* search for the terminating " */
            start = t->input + t->offset + 1;
            g_string_assign(token, "");

            for (i = 1; t->input[t->offset + i]; i++) {
                if (t->input[t->offset + i] == '\\' &&
                    t->input[t->offset + i + 1] == '"') {

                    g_string_append_len(token, start, t->input + t->offset + i - start);

                    start = t->input + t->offset + i + 1;

                    /* skip the " */
                    i++;
                    continue;
                }


                if (t->input[t->offset + i] == '"') {
                    tid = TK_STRING;

                    g_string_append_len(token, start, t->input + t->offset + i - start);

                    break;
                }
            }

            if (t->input[t->offset + i] == '\0') {
                /* ERROR */

                return -1;
            }

            t->offset += i + 1;
            t->line_pos += i + 1;

            break;
        case '(':
            t->offset++;
            t->in_brace++;

            tid = TK_LPARAN;

            g_string_assign(token, "(");
            break;
        case ')':
            t->offset++;
            t->in_brace--;

            tid = TK_RPARAN;

            g_string_assign(token, ")");
            break;
        case '$':
            t->offset++;

            tid = TK_DOLLAR;
            t->in_cond = 1;
            t->in_key = 0;

            g_string_assign(token, "$");

            break;

        case '+':
            if (t->input[t->offset + 1] == '=') {
                t->offset += 2;
                g_string_assign(token, "+=");
                tid = TK_APPEND;
            } else {
                t->offset++;
                tid = TK_PLUS;
                g_string_assign(token, "+");
            }
            break;

        case '{':
            t->offset++;

            tid = TK_LCURLY;

            g_string_assign(token, "{");

            break;

        case '}':
            t->offset++;

            tid = TK_RCURLY;

            g_string_assign(token, "}");

            break;

        case '[':
            t->offset++;

            tid = TK_LBRACKET;

            g_string_assign(token, "[");

            break;

        case ']':
            t->offset++;

            tid = TK_RBRACKET;

            g_string_assign(token, "]");

            break;
        case '#':
            config_skip_comment(t);
            t->line_pos = 1;
            t->line++;

            break;
        default:
            if (t->in_cond) {
                for (i = 0; t->input[t->offset + i] &&
                     (isalpha((unsigned char)t->input[t->offset + i])
                      ); i++);

                if (i && t->input[t->offset + i]) {
                    tid = TK_SRVVARNAME;
                    g_string_append_len(token, t->input + t->offset, i);

                    t->offset += i;
                    t->line_pos += i;
                } else {
                    /* ERROR */
                    return -1;
                }
            } else if (isdigit((unsigned char)c)) {
                /* take all digits */
                for (i = 0; t->input[t->offset + i] && isdigit((unsigned char)t->input[t->offset + i]);  i++);

                /* was there it least a digit ? */
                if (i) {
                    tid = TK_INTEGER;

                    g_string_append_len(token, t->input + t->offset, i);

                    t->offset += i;
                    t->line_pos += i;
                }
            } else {
                /* the key might consist of [-.0-9a-z] */
                for (i = 0; t->input[t->offset + i] &&
                     (isalnum((unsigned char)t->input[t->offset + i]) ||
                      t->input[t->offset + i] == '.' ||
                      t->input[t->offset + i] == '_' || /* for env.* */
                      t->input[t->offset + i] == '-'
                      ); i++);

                if (i && t->input[t->offset + i]) {
                    g_string_append_len(token, t->input + t->offset, i);

                    if (strcmp(token->str, "include") == 0) {
                        tid = TK_INCLUDE;
                    } else if (strcmp(token->str, "global") == 0) {
                        tid = TK_GLOBAL;
                    } else if (strcmp(token->str, "else") == 0) {
                        tid = TK_ELSE;
                    } else {
                        tid = TK_LKEY;
                    }

                    t->offset += i;
                    t->line_pos += i;
                } else {
                    /* ERROR */
                    return -1;
                }
            }
            break;
        }
    }

    if (tid) {
        *token_id = tid;

        return 1;
    } else if (t->offset < t->size) {
        fprintf(stderr, "%s.%d: %d, %s\n",
            __FILE__, __LINE__,
            tid, token->str);
    }
    return 0;
}

/**
 * Parse the configuration
 * @return 0 on success, -1 on failure
 */

static int config_parse(config_t *context, tokenizer_t *t) {
    void *pParser;
    int token_id;
    GString *token, *lasttoken;
    int ret;

    pParser = configparserAlloc();
    lasttoken = g_string_new("");
    token = g_string_new("");
    while((1 == (ret = config_tokenizer(t, &token_id, token))) && context->ok) {
        g_string_assign(lasttoken, token->str);
        configparser(pParser, token_id, token, context);

        token = g_string_new("");
    }
    g_string_free(token, true);

    if (ret != -1 && context->ok) {
        /* add an EOL at EOF, better than say sorry */
        configparser(pParser, TK_EOL, g_string_new("(EOL)"), context);
        if (context->ok) {
            configparser(pParser, 0, NULL, context);
        }
    }
    configparserFree(pParser);

    if (context->ok == 0)
        ret = -1;

    g_string_free(lasttoken, true);

    return ret == -1 ? -1 : 0;
}

/**
 * Parse a configuration file
 * @param srv instance variable
 * @param context configuration context
 * @param fn file name, to be searched in context->basedir if set
 * @return -1 on failure
 */

int config_parse_file(config_t *context, const char *fn) {
    tokenizer_t t = {
        .offset = 0,
        .line = 1,
        .line_pos = 1,

        .in_key = 1,
        .in_brace = 0,
        .in_cond = 0
    };
    int ret = -1;
    GError *err;
    const char *filename;

    if ( g_path_is_absolute(fn) )
        filename = fn;
    else
        filename = g_build_path(context->basedir, fn, NULL);

    if ( !g_file_get_contents(filename,
                              &t.input,
                              &t.size,
                              &err) ) {
        fnc_log(FNC_LOG_FATAL, "unable to parse configuration file: %s",
                err->message);
        goto error;
    }

    ret = config_parse(context, &t);

 error:
    if ( filename != fn )
        g_free((char*)filename);

    g_free(t.input);
    return ret;
}

/**
 * load configuration for global
 * sets the module list.
 */

int config_read(const char *fn) {
    data_config *dc;
    int ret;
    const char *filename;

    config_t context = {
        .ok = 1,
        .configs_stack = array_init(),
    };

    context.configs_stack->is_weakref = 1;

    context.all_configs = feng_srv.config_context;

    if ( g_path_is_absolute(fn) )
        filename = fn;
    else {
        char *cwd = g_get_current_dir();
        filename = g_build_path(cwd, fn, NULL);
        g_free(cwd);
    }

    context.basedir = g_path_get_dirname(filename);

    dc = data_config_init();
    g_string_assign(dc->key, "global");

    assert(context.all_configs->used == 0);
    dc->context_ndx = context.all_configs->used;
    array_insert_unique(context.all_configs, (data_unset *)dc);
    context.current = dc;

    ret = config_parse_file(&context, fn);

    /* remains nothing if parser is ok */
    assert(!(0 == ret && context.ok && 0 != context.configs_stack->used));
    array_free(context.configs_stack);

    g_free(context.basedir);
    if ( filename != fn )
        g_free((char*)filename);

    if (0 != ret) {
        return ret;
    }

    return config_insert();
}

static int config_insert_values_internal(array *ca, const config_values_t cv[]) {
    size_t i;
    data_unset *du;

    for (i = 0; cv[i].key; i++) {

        if (NULL == (du = array_get_element(ca, cv[i].key))) {
            /* no found */

            continue;
        }

        switch (cv[i].type) {
        case T_CONFIG_STRING:
            switch(du->type) {
            case TYPE_INTEGER: {
                data_integer *di = (data_integer *)du;
                char **dst = (char**)cv[i].destination;

                *dst = g_strdup_printf("%d", di->value);
                break;
            }
            case TYPE_STRING: {
                data_string *ds = (data_string *)du;
                char **dst = (char**)cv[i].destination;

                /* if the string is empty */
                if ( ds->value->len < 2 )
                    *dst = NULL;
                else
                    *dst = g_strdup(ds->value->str);
                break;
            }
            default:
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

                if (strcmp(ds->value->str, "enable") == 0) {
                    *((unsigned short *)(cv[i].destination)) = 1;
                } else if (strcmp(ds->value->str, "disable") == 0) {
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
