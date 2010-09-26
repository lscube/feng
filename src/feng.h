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

extern const char feng_signature[];

typedef struct server_config {
    short buffered_frames;
    short loglevel;

    int errorlog_use_syslog;

    unsigned short max_conns;

    char *errorlog_file;

    char *twin;

    char *username;
    char *groupname;

    char *document_root;
} server_config;

typedef struct specific_config {
    char *host;
    char *port;

    gboolean use_ipv6;
    gboolean access_log_syslog;

#if ENABLE_SCTP
    gboolean is_sctp;
    unsigned short sctp_max_streams;
#endif

    char *access_log_file;
    FILE *access_log_fp;
} specific_config;

typedef struct feng_stats {
    size_t total_sent;
    size_t total_received;
    time_t start_time;
    //add_more
} feng_stats;

typedef struct feng {
    array *config_context;

    GSList *sockets;

    server_config srvconf;

    /**
     * @brief Number of active connections
     *
     * Once it reaches the maximum the server is supposed to redirect
     * to a twin if available
     */
    size_t connection_count;
} feng;

extern struct feng feng_srv;
extern struct ev_loop *feng_loop;

typedef struct Feng_Listener {
    int fd;
    specific_config *specific;
    ev_io io;
} Feng_Listener;

#define MAX_PROCESS    1    /*! number of fork */
#define MAX_CONNECTION    feng_srv.srvconf.max_conns   /*! rtsp connection */
#define ONE_FORK_MAX_CONNECTION ((int)(MAX_CONNECTION/MAX_PROCESS)) /*! rtsp connection for one fork */

void feng_bind_socket(gpointer socket_p, gpointer user_data);

struct RTSP_Client;
struct RFC822_Response;

void accesslog_init(gpointer socket_p, gpointer user_data);
void accesslog_log(struct RTSP_Client *client, struct RFC822_Response *response);

#if HAVE_JSON
void stats_init();
#else
#define stats_init()
#endif

/**
 * @defgroup utils Utility functions
 *
 * @{
 */
gboolean feng_str_is_unreserved(const char *string);

#ifdef DOXYGEN
void feng_assert_or_goto(gboolean condition, label label_name);
void feng_assert_or_return(gboolean condition);
void feng_assert_or_retval(gboolean condition, int retval);
#elif defined(NDEBUG)
# define feng_assert_or_goto(assertion, label) \
    do {                                       \
        if ( !(assertion) )                    \
            goto label;                        \
    } while(0)
# define feng_assert_or_return(assertion)      \
    do {                                       \
        if ( !(assertion) )                    \
            return;                            \
    } while(0)
# define feng_assert_or_retval(assertion, retval)   \
    do {                                            \
        if ( !(assertion) )                         \
            return retval;                          \
    } while(0)
#else
# define feng_assert_or_goto(assertion, label) g_assert(assertion)
# define feng_assert_or_return(assertion, label) g_assert(assertion)
# define feng_assert_or_retval(assertion, label) g_assert(assertion)
#endif
/**
 * @}
 */

#endif // FN_SERVER_H
