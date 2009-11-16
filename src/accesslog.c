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

#include "feng.h"
#include "fnc_log.h"
#include "feng_utils.h"
#include "network/rtsp.h"

#include <liberis/headers.h>
#include <stdio.h>
#include <glib.h>

#if HAVE_SYSLOG_H
# include <syslog.h>
#endif

gboolean accesslog_init(feng *srv)
{
    size_t i;

    for(i = 0; i < srv->config_context->used; i++) {
        if (srv->config_storage[i].access_log_syslog) {
#if HAVE_SYSLOG_H
            /* ignore the next checks */
            continue;
#else
            fnc_log(FNC_LOG_ERR, "Unable to use syslog for access log");
            return false;
#endif
        }

        if (srv->config_storage[i].access_log_file->used < 2) continue;

        if (NULL == (srv->config_storage[i].access_log_fp = fopen(srv->config_storage[i].access_log_file->ptr, "a"))) {
            fnc_log(FNC_LOG_ERR, "Unable to open access log %s", srv->config_storage[i].access_log_file->ptr);
            return false;
        }
    }
    return true;
}

void accesslog_uninit(feng *srv)
{
    size_t i;

    for(i = 0; i < srv->config_context->used; i++)
        if ( !srv->config_storage[i].access_log_syslog )
            fclose(srv->config_storage[i].access_log_fp);
}

#define PRINT_STRING \
    "%s - - [%s], \"%s %s %s\" %d %s %s %s\n",\
        client->sock->remote_host,\
        (const char*)g_hash_table_lookup(response->headers, eris_hdr_date),\
        response->request->method, response->request->object,\
        response->request->version,\
        response->status, response_length ? response_length : "-",\
        referer ? referer : "-",\
        useragent ? useragent : "-"

void accesslog_log(struct RTSP_Client *client, struct RTSP_Response *response)
{
    const char *referer =
        g_hash_table_lookup(response->request->headers, eris_hdr_referer);
    const char *useragent =
        g_hash_table_lookup(response->request->headers, eris_hdr_user_agent);
    char *response_length = response->body ?
        g_strdup_printf("%zd", response->body->len) : NULL;

    /** @TODO Support VHOST */
#if HAVE_SYSLOG_H
    if (client->srv->config_storage[0].access_log_syslog)
        syslog(LOG_INFO, PRINT_STRING);
    else
#endif
    {
        FILE *fp = client->srv->config_storage[0].access_log_fp;
        fprintf(fp, PRINT_STRING);
        fflush(fp);
    }
    g_free(response_length);
}
