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

#ifndef FN_CONF_H
#define FN_CONF_H

#include <config.h>

#include <stdio.h>

typedef enum { T_CONFIG_UNSET,
                T_CONFIG_STRING,
                T_CONFIG_SHORT,
                T_CONFIG_BOOLEAN,
                T_CONFIG_ARRAY,
                T_CONFIG_LOCAL,
                T_CONFIG_DEPRECATED,
                T_CONFIG_UNSUPPORTED
} config_values_type_t;

typedef enum { T_CONFIG_SCOPE_UNSET,
                T_CONFIG_SCOPE_SERVER,
                T_CONFIG_SCOPE_CONNECTION
} config_scope_type_t;

typedef struct {
        const char *key;
        void *destination;

        config_values_type_t type;
        config_scope_type_t scope;
} config_values_t;

typedef struct {
    short port;

    short buffered_frames;
    short loglevel;

    int errorlog_use_syslog;

    unsigned short max_conns;

    conf_buffer *bindhost;

    conf_buffer *errorlog_file;

    conf_buffer *twin;

    conf_buffer *username;
    conf_buffer *groupname;

    array *upload_tempdirs;
} server_config;

typedef struct specific_config {
    gboolean use_ipv6;
    gboolean access_log_syslog;

#if ENABLE_SCTP
    gboolean is_sctp;
    unsigned short sctp_max_streams;
#endif

    /* virtual-servers */
    conf_buffer *document_root;

    conf_buffer *access_log_file;
    FILE *access_log_fp;
} specific_config;

#endif // FN_CONF_H
