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

#include <glib.h>

#include <sys/stat.h>

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "feng.h"
//#include "log.h"

#include "stream.h"
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

    static const config_values_t global_cv[] = {
        { "server.bind", &feng_srv.srvconf.bindhost, T_CONFIG_STRING },      /* 0 */
        { "server.errorlog", &feng_srv.srvconf.errorlog_file, T_CONFIG_STRING },      /* 1 */
        { "server.username", &feng_srv.srvconf.username, T_CONFIG_STRING },      /* 2 */
        { "server.groupname", &feng_srv.srvconf.groupname, T_CONFIG_STRING },      /* 3 */
        { "server.port", &feng_srv.srvconf.bindport, T_CONFIG_STRING },      /* 4 */
        { "server.errorlog-use-syslog", &feng_srv.srvconf.errorlog_use_syslog, T_CONFIG_BOOLEAN },     /* 7 */
        { "server.max-connections", &feng_srv.srvconf.max_conns, T_CONFIG_SHORT },       /* 8 */
        { "server.buffered_frames", &feng_srv.srvconf.buffered_frames, T_CONFIG_SHORT },
        { "server.loglevel", &feng_srv.srvconf.loglevel, T_CONFIG_SHORT },
        { "server.twin", &feng_srv.srvconf.twin, T_CONFIG_STRING },
        { NULL,                          NULL, T_CONFIG_UNSET }
    };

    feng_srv.config_storage = calloc(feng_srv.config_context->used, sizeof(specific_config));

    assert(feng_srv.config_storage);

    if (config_insert_values_internal(((data_config *)feng_srv.config_context->data[0])->value, global_cv))
        return -1;

    if (feng_srv.srvconf.max_conns == 0)
        feng_srv.srvconf.max_conns = 100;

    if (feng_srv.srvconf.buffered_frames == 0)
        feng_srv.srvconf.buffered_frames = BUFFERED_FRAMES_DEFAULT;

    for (i = 0; i < feng_srv.config_context->used; i++) {
        specific_config *s = &feng_srv.config_storage[i];

        const config_values_t vhost_cv[] = {
            { "server.use-ipv6", &s->use_ipv6, T_CONFIG_BOOLEAN }, /* 5 */

            { "server.document-root", &s->document_root, T_CONFIG_STRING },  /* 6 */
            { "sctp.protocol", &s->is_sctp, T_CONFIG_BOOLEAN },
            { "sctp.max_streams", &s->sctp_max_streams, T_CONFIG_SHORT },

            { "accesslog.filename", &s->access_log_file, T_CONFIG_STRING }, /* 11 */
            { "accesslog.use-syslog", &s->access_log_syslog, T_CONFIG_BOOLEAN },
            { NULL,                          NULL, T_CONFIG_UNSET }
        };

        s->use_ipv6      = 0;
        s->is_sctp       = 0;
        s->sctp_max_streams = 16;
        s->access_log_syslog = 1;

        if (config_insert_values_internal(((data_config *)feng_srv.config_context->data[i])->value, vhost_cv))
            return -1;

        if ( s->document_root == NULL )
            return -1;
    }

    return 0;
}

typedef struct {
    int foo;
    int bar;

    const conf_buffer *source;
    const char *input;
    size_t offset;
    size_t size;

    int line_pos;
    int line;

    int in_key;
    int in_brace;
    int in_cond;
} tokenizer_t;

/**
 * skip any newline (unix, windows, mac style)
 */

static int config_skip_newline(tokenizer_t *t) {
    int skipped = 1;
    assert(t->input[t->offset] == '\r' || t->input[t->offset] == '\n');
    if (t->input[t->offset] == '\r' && t->input[t->offset + 1] == '\n') {
        skipped ++;
        t->offset ++;
    }
    t->offset ++;
    return skipped;
}

/**
 * Skip comments
 * a comment starts with an # and last till a newline
 */

static int config_skip_comment(tokenizer_t *t) {
    int i;
    assert(t->input[t->offset] == '#');
    for (i = 1; t->input[t->offset + i] &&
         (t->input[t->offset + i] != '\n' && t->input[t->offset + i] != '\r');
         i++);
    t->offset += i;
    return i;
}

/**
 * Break the configuration in tokens
 */

static int config_tokenizer(tokenizer_t *t, int *token_id, conf_buffer *token) {
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

                    buffer_copy_string(token, "=>");

                    tid = TK_ARRAY_ASSIGN;
                } else {
                    return -1;
                }
            } else if (t->in_cond) {
                if (t->input[t->offset + 1] == '=') {
                    t->offset += 2;

                    buffer_copy_string(token, "==");

                    tid = TK_EQ;
                } else {
                    return -1;
                }
                t->in_key = 1;
                t->in_cond = 0;
            } else if (t->in_key) {
                tid = TK_ASSIGN;

                buffer_copy_string_len(token, t->input + t->offset, 1);

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

                    buffer_copy_string(token, "!=");

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
        case ' ':
            t->offset++;
            t->line_pos++;
            break;
        case '\n':
        case '\r':
            if (t->in_brace == 0) {
                int done = 0;
                while (!done && t->offset < t->size) {
                    switch (t->input[t->offset]) {
                    case '\r':
                    case '\n':
                        config_skip_newline(t);
                        t->line_pos = 1;
                        t->line++;
                        break;

                    case '#':
                        t->line_pos += config_skip_comment(t);
                        break;

                    case '\t':
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
                buffer_copy_string(token, "(EOL)");
            } else {
                config_skip_newline(t);
                t->line_pos = 1;
                t->line++;
            }
            break;
        case ',':
            if (t->in_brace > 0) {
                tid = TK_COMMA;

                buffer_copy_string(token, "(COMMA)");
            }

            t->offset++;
            t->line_pos++;
            break;
        case '"':
            /* search for the terminating " */
            start = t->input + t->offset + 1;
            buffer_copy_string(token, "");

            for (i = 1; t->input[t->offset + i]; i++) {
                if (t->input[t->offset + i] == '\\' &&
                    t->input[t->offset + i + 1] == '"') {

                    buffer_append_string_len(token, start, t->input + t->offset + i - start);

                    start = t->input + t->offset + i + 1;

                    /* skip the " */
                    i++;
                    continue;
                }


                if (t->input[t->offset + i] == '"') {
                    tid = TK_STRING;

                    buffer_append_string_len(token, start, t->input + t->offset + i - start);

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

            buffer_copy_string(token, "(");
            break;
        case ')':
            t->offset++;
            t->in_brace--;

            tid = TK_RPARAN;

            buffer_copy_string(token, ")");
            break;
        case '$':
            t->offset++;

            tid = TK_DOLLAR;
            t->in_cond = 1;
            t->in_key = 0;

            buffer_copy_string(token, "$");

            break;

        case '+':
            if (t->input[t->offset + 1] == '=') {
                t->offset += 2;
                buffer_copy_string(token, "+=");
                tid = TK_APPEND;
            } else {
                t->offset++;
                tid = TK_PLUS;
                buffer_copy_string(token, "+");
            }
            break;

        case '{':
            t->offset++;

            tid = TK_LCURLY;

            buffer_copy_string(token, "{");

            break;

        case '}':
            t->offset++;

            tid = TK_RCURLY;

            buffer_copy_string(token, "}");

            break;

        case '[':
            t->offset++;

            tid = TK_LBRACKET;

            buffer_copy_string(token, "[");

            break;

        case ']':
            t->offset++;

            tid = TK_RBRACKET;

            buffer_copy_string(token, "]");

            break;
        case '#':
            t->line_pos += config_skip_comment(t);

            break;
        default:
            if (t->in_cond) {
                for (i = 0; t->input[t->offset + i] &&
                     (isalpha((unsigned char)t->input[t->offset + i])
                      ); i++);

                if (i && t->input[t->offset + i]) {
                    tid = TK_SRVVARNAME;
                    buffer_copy_string_len(token, t->input + t->offset, i);

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

                    buffer_copy_string_len(token, t->input + t->offset, i);

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
                    buffer_copy_string_len(token, t->input + t->offset, i);

                    if (strcmp(token->ptr, "include") == 0) {
                        tid = TK_INCLUDE;
                    } else if (strcmp(token->ptr, "global") == 0) {
                        tid = TK_GLOBAL;
                    } else if (strcmp(token->ptr, "else") == 0) {
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
            tid, token->ptr);
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
    conf_buffer *token, *lasttoken;
    int ret;

    pParser = configparserAlloc();
    lasttoken = buffer_init();
    token = buffer_init();
    while((1 == (ret = config_tokenizer(t, &token_id, token))) && context->ok) {
        buffer_copy_string_buffer(lasttoken, token);
        configparser(pParser, token_id, token, context);

        token = buffer_init();
    }
    buffer_free(token);

    if (ret != -1 && context->ok) {
        /* add an EOL at EOF, better than say sorry */
        configparser(pParser, TK_EOL, buffer_init_string("(EOL)"), context);
        if (context->ok) {
            configparser(pParser, 0, NULL, context);
        }
    }
    configparserFree(pParser);

    if (context->ok == 0)
        ret = -1;

    buffer_free(lasttoken);

    return ret == -1 ? -1 : 0;
}

/**
 * tokenizer initial state
 */

static int tokenizer_init(tokenizer_t *t, const conf_buffer *source, const char *input, size_t size) {

    t->source = source;
    t->input = input;
    t->size = size;
    t->offset = 0;
    t->line = 1;
    t->line_pos = 1;

    t->in_key = 1;
    t->in_brace = 0;
    t->in_cond = 0;
    return 0;
}

/**
 * Parse a configuration file
 * @param srv instance variable
 * @param context configuration context
 * @param fn file name, to be searched in context->basedir if set
 * @return -1 on failure
 */

int config_parse_file(config_t *context, const char *fn) {
    tokenizer_t t;
    stream s;
    int ret;
    conf_buffer *filename;

    if (buffer_is_empty(context->basedir) &&
            (fn[0] == '/' || fn[0] == '\\') &&
            (fn[0] == '.' && (fn[1] == '/' || fn[1] == '\\'))) {
        filename = buffer_init_string(fn);
    } else {
        filename = buffer_init_buffer(context->basedir);
        buffer_append_string(filename, fn);
    }

    if (0 != stream_open(&s, filename->ptr)) {
        if (s.size == 0) {
            /* the file was empty, nothing to parse */
            ret = 0;
        } else {
            ret = -1;
        }
    } else {
        tokenizer_init(&t, filename, s.start, s.size);
        ret = config_parse(context, &t);
    }

    stream_close(&s);
    buffer_free(filename);
    return ret;
}

/**
 * load configuration for global
 * sets the module list.
 */

int config_read(const char *fn) {
    data_config *dc;
    int ret;
    char *pos;

    config_t context = {
        .ok = 1,
        .configs_stack = array_init(),
        .basedir = buffer_init()
    };

    context.configs_stack->is_weakref = 1;

    context.all_configs = feng_srv.config_context;

    pos = strrchr(fn,
#ifdef __WIN32
            '\\'
#else
            '/'
#endif
            );
    if (pos) {
        buffer_copy_string_len(context.basedir, fn, pos - fn + 1);
        fn = pos + 1;
    }

    dc = data_config_init();
    buffer_copy_string(dc->key, "global");

    assert(context.all_configs->used == 0);
    dc->context_ndx = context.all_configs->used;
    array_insert_unique(context.all_configs, (data_unset *)dc);
    context.current = dc;

    ret = config_parse_file(&context, fn);

    /* remains nothing if parser is ok */
    assert(!(0 == ret && context.ok && 0 != context.configs_stack->used));
    array_free(context.configs_stack);
    buffer_free(context.basedir);

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
                if ( ds->value->used < 2 )
                    *dst = NULL;
                else
                    *dst = g_strdup(ds->value->ptr);
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
