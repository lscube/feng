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
#include <stdbool.h>
#include <netinet/in.h>

#include "cfgparser/cfgparser.h"

extern const char feng_signature[];

typedef struct feng_socket_listener {
    int fd;
    ev_io io;
} feng_socket_listener;

extern cfg_options_t feng_srv;
extern GList *configured_sockets;
extern GList *configured_vhosts;

#define feng_default_vhost ((cfg_vhost_t*)(configured_vhosts->data))

extern struct ev_loop *feng_loop;

void config_file_parse(const char *file, bool lint);

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
