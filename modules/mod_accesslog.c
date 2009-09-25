/* *
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 *
 * feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * feng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with feng; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * */

/**
 * @file accesslog.c
 * Apache/lighttpd-like accesslog facility
 */

#include "config.h"

#include <stdlib.h>
#include <errno.h>
#include <grp.h>
#include <string.h>
#include <stdbool.h>

#include "conf/buffer.h"
#include "conf/array.h"
#include "conf/configfile.h"

#include "feng.h"
#include "fnc_log.h"
#include "feng_utils.h"
#include "network/rtsp.h"
#include "plugin.h"

#include <fcntl.h>
#include <glib.h>
#include <gmodule.h>

typedef struct {
    unsigned short use_syslog;
    conf_buffer *access_logfile;
    int    log_access_fd;
} accesslog_config;

typedef struct {
    accesslog_config **config_storage;  //!< specific configuration
    accesslog_config conf;              //!< global configuration
} accesslog_data;

void *accesslog_init()
{
    return g_new0(accesslog_data, 1);
}

int accesslog_set_defaults(feng *srv, void *data)
{
    accesslog_data *d = data;
    size_t i = 0;

    config_values_t cv[] = {
        { "accesslog.filename",             NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION },
        { "accesslog.use-syslog",           NULL, T_CONFIG_BOOLEAN, T_CONFIG_SCOPE_CONNECTION },
//        { "accesslog.format",               NULL, T_CONFIG_STRING, T_CONFIG_SCOPE_CONNECTION },
        { NULL,                             NULL, T_CONFIG_UNSET, T_CONFIG_SCOPE_UNSET }
    };

    d->config_storage = g_new0(accesslog_config*, srv->config_context->used);

    for (i = 0; i < srv->config_context->used; i++) {
        accesslog_config *s = g_new0(accesslog_config, 1);
        s->access_logfile = buffer_init();
        s->log_access_fd = -1;

        cv[0].destination = s->access_logfile;
        cv[1].destination = &(s->use_syslog);
//        cv[2].destination = s->format;

        d->config_storage[i] = s;

        if (config_insert_values_global(srv,
                ((data_config *)srv->config_context->data[i])->value, cv)) {
            return ERR_FATAL;
        }

        if (s->use_syslog) {
            /* ignore the next checks */
            continue;
        }

        if (s->access_logfile->used < 2) continue;

        if (-1 == (s->log_access_fd = open(s->access_logfile->ptr,
                                           O_APPEND| O_CREAT, S_IRWXG)))
            return ERR_FATAL;

    }
    return ERR_NOERROR;
}

int accesslog_uninit(feng *srv, void *data)
{
    return 0;
}

int accesslog_log(feng *srv, RTSP_Response *resp, void *data)
{
    return 0;
}

int init (struct plugin *p)
{
    p->init = accesslog_init;
    p->set_defaults = accesslog_set_defaults;
    p->uninit = accesslog_uninit;
    p->response_send = accesslog_log;
    return 1;
}
