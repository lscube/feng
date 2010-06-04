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
#include "network/rtsp.h"

#include <stdio.h>
#include <glib.h>

#if HAVE_SYSLOG_H
# include <syslog.h>
#endif

gboolean accesslog_init()
{
    size_t i;

    for(i = 0; i < feng_srv.config_context->used; i++) {
        char *access_log_filename = feng_srv.config_storage[i].access_log_file;

        if (feng_srv.config_storage[i].access_log_syslog) {
#if HAVE_SYSLOG_H
            /* ignore the next checks */
            continue;
#else
            fnc_log(FNC_LOG_ERR, "Unable to use syslog for access log");
            return false;
#endif
        }

        if ( access_log_filename == NULL )
            continue;

        if (NULL == (feng_srv.config_storage[i].access_log_fp = fopen(access_log_filename, "a"))) {
            fnc_perror("fopen");
            return false;
        }
    }
    return true;
}

void accesslog_uninit()
{
    size_t i;

    if ( feng_srv.config_storage )
        for(i = 0; i < feng_srv.config_context->used; i++)
            if ( !feng_srv.config_storage[i].access_log_syslog &&
                 feng_srv.config_storage[i].access_log_fp != NULL )
                fclose(feng_srv.config_storage[i].access_log_fp);
}

#define PRINT_STRING \
    "%s - - [%s], \"%s %s %s\" %d %s %s %s\n",\
        client->remote_host,\
        rfc822_headers_lookup(response->headers, RFC822_Header_Date),\
        response->request->method_str, response->request->object,\
        response->request->protocol_str,\
        response->status, response_length ? response_length : "-",\
        referer ? referer : "-",\
        useragent ? useragent : "-"

void accesslog_log(struct RTSP_Client *client, struct RFC822_Response *response)
{
    const char *referer = rfc822_headers_lookup(response->request->headers, RFC822_Header_Referer);
    const char *useragent = rfc822_headers_lookup(response->request->headers, RFC822_Header_User_Agent);
    char *response_length = response->body ?
        g_strdup_printf("%zd", response->body->len) : NULL;

    /** @TODO Support VHOST */
#if HAVE_SYSLOG_H
    if (feng_srv.config_storage[0].access_log_syslog)
        syslog(LOG_INFO, "Access " PRINT_STRING);
    else
#endif
    {
        FILE *fp = feng_srv.config_storage[0].access_log_fp;
        fprintf(fp, PRINT_STRING);
        fflush(fp);
    }
    g_free(response_length);
}
