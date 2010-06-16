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
 * @file feng.h
 * Server instance structure
 */

#ifndef FN_SERVER_H
#define FN_SERVER_H

#include <glib.h>
#include <netembryo/wsocket.h>
#include <ev.h>
#include <pwd.h>

#include "conf/array.h"
#include "conf/conf.h"

typedef struct feng_stats {
    size_t total_sent;
    size_t total_received;
    //add_more
} feng_stats;

typedef struct feng {

    array *config;
    array *config_touched;

    array *config_context;
    specific_config *config_storage;

    server_config srvconf;

    struct ev_loop *loop;       //!< event loop

    /**
     * @brief Number of active connections
     *
     * Once it reaches the maximum the server is supposed to redirect
     * to a twin if available
     */
    size_t connection_count;

    GSList *clients; //!< All the currently connected clients
#ifdef HAVE_JSON
    size_t total_sent;
    size_t total_read;
    time_t start_time;
#endif
} feng;

typedef feng server;

#define MAX_PROCESS    1    /*! number of fork */
#define MAX_CONNECTION    srv->srvconf.max_conns   /*! rtsp connection */
#define ONE_FORK_MAX_CONNECTION ((int)(MAX_CONNECTION/MAX_PROCESS)) /*! rtsp connection for one fork */

struct RTSP_Client;
struct RFC822_Response;

gboolean accesslog_init(feng *srv);
void accesslog_uninit(feng *srv);
void accesslog_log(struct RTSP_Client *client, struct RFC822_Response *response);

#endif // FN_SERVER_H
