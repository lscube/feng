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

#ifndef FN_CONFIG_PARSER_H
#define FN_CONFIG_PARSER_H

#include "array.h"
#include "buffer.h"
#include "feng.h"

typedef struct {
    server *srv;
    int     ok;
    array  *all_configs;
    array  *configs_stack; /* to parse nested block */
    data_config *current; /* current started with { */
    conf_buffer *basedir;
} config_t;

void *configparserAlloc(void *(*mallocProc)(size_t));
void configparserFree(void *p, void (*freeProc)(void*));
void configparser(void *yyp, int yymajor, conf_buffer *yyminor, config_t *ctx);
int config_parse_file(server *srv, config_t *context, const char *fn);
int config_parse_cmd(server *srv, config_t *context, const char *cmd);
data_unset *configparser_merge_data(data_unset *op1, const data_unset *op2);
int config_read(server *srv, const char *fn);
int config_set_defaults(server *srv);
int config_insert_values_global(server *srv, array *ca,
                                const config_values_t cv[]);
//void config_cond_cache_reset(server *srv, connection *con);
//void config_cond_cache_reset_item(server *srv, connection *con, comp_key_t item);

#define config_cond_cache_reset_all_items(srv, con) \
    config_cond_cache_reset_item(srv, con, COMP_LAST_ELEMENT);

#endif
