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
#include <ev.h>
#include <pwd.h>
#include <stdio.h> /* for FILE* */
#include <netinet/in.h>

#include "conf/array.h"

typedef struct server_config {
    short port;

    short buffered_frames;
    short loglevel;

    int errorlog_use_syslog;

    unsigned short max_conns;

    char *bindhost;

    char *errorlog_file;

    char *twin;

    char *username;
    char *groupname;
} server_config;

typedef struct specific_config {
    gboolean use_ipv6;
    gboolean access_log_syslog;

#if ENABLE_SCTP
    gboolean is_sctp;
    unsigned short sctp_max_streams;
#endif

    /* virtual-servers */
    char *document_root;

    char *access_log_file;
    FILE *access_log_fp;
} specific_config;

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
#endif
} feng;

typedef struct Feng_Listener {
    int fd;
    socklen_t sa_len;
    struct sockaddr *local_sa;
    char *local_host;
    specific_config *specific;
    ev_io io;
} Feng_Listener;

typedef feng server;

/**
 * @brief Global instance for feng settings
 */
extern struct feng *feng_srv;

#define MAX_PROCESS    1    /*! number of fork */
#define MAX_CONNECTION    feng_srv->srvconf.max_conns   /*! rtsp connection */
#define ONE_FORK_MAX_CONNECTION ((int)(MAX_CONNECTION/MAX_PROCESS)) /*! rtsp connection for one fork */

struct RTSP_Client;
struct RFC822_Response;

gboolean accesslog_init();
void accesslog_uninit();
void accesslog_log(struct RTSP_Client *client, struct RFC822_Response *response);

#endif // FN_SERVER_H
