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

/**
 * @brief Initialise access log for a socket
 *
 * @param socket_p Generic pointer to @ref specific_config object
 * @param user_data Unused
 *
 * @TODO this should actually be moved to vhosts, not sockets
 */
void accesslog_init(gpointer socket_p, ATTR_UNUSED gpointer user_data)
{
    specific_config *socket = socket_p;

    if (socket->access_log_syslog)
        return;

    if (socket->access_log_file == NULL )
        return;

    if ((socket->access_log_fp = fopen(socket->access_log_file, "a")) == NULL) {
        fnc_perror("fopen");
        exit(1);
    }
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

#if HAVE_SYSLOG_H
    if (client->specific->access_log_syslog)
        syslog(LOG_INFO, "Access " PRINT_STRING);
    else
#endif
    {
        FILE *fp = client->specific->access_log_fp;
        fprintf(fp, PRINT_STRING);
        fflush(fp);
    }
    g_free(response_length);
}
