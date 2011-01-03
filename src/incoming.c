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

#include <config.h>

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include <ev.h>

#include "feng.h"
#include "fnc_log.h"
#include "network/rtsp.h"
#include "network/netembryo.h"
#include "cfgparser/cfgparser.h"

#ifdef ENABLE_SCTP
# include <netinet/sctp.h>
/* FreeBSD and Mac OS X don't have SOL_SCTP and re-use IPPROTO_SCTP
   for setsockopt() */
# if !defined(SOL_SCTP)
#  define SOL_SCTP IPPROTO_SCTP
# endif
#endif

#ifdef CLEANUP_DESTRUCTOR
/**
 * @brief List of created @ref feng_socket_listener objects to be cleaned up
 *
 * This is an array of feng_socket_listener objects allocated
 * with the g_slice_new() function (which this need to be freed with
 * g_slice_free()).
 *
 * @note Part of the cleanup destructors code, not compiled in
 *       production use.
 */
static GSList *listeners;

/**
 * @brief Free socket configurations in the @ref feng_srv::sockets array
 *
 * @param listener_p The feng_socket_listener object to close
 * @param user_data Unused
 *
 * This function is used to close the opened software during cleanup.
 *
 * @note Part of the cleanup destructors code, not compiled in
 *       production use.
 */
static void listeners_cleanup(gpointer listener_p, ATTR_UNUSED gpointer user_data)
{
    feng_socket_listener *listener = listener_p;

    close(listener->fd);
    g_slice_free(feng_socket_listener, listener);
}

/**
 * @brief Cleanup function for the @ref listener list
 *
 * This function is used to free the @ref listener array that was
 * allocated in @feng_bind_socket; note that this function is called
 * automatically as a destructor when the compiler supports it, and
 * debug is not disabled.
 *
 * This function is unnecessary on production code, since the memory
 * would be freed only at the end of execution, when the resources
 * would be freed anyway.
 *
 * @note Part of the cleanup destructors code, not compiled in
 *       production use.
 */
static void CLEANUP_DESTRUCTOR feng_sockets_cleanup()
{
    if ( listeners == NULL )
        return;

    g_slist_foreach(listeners, listeners_cleanup, NULL);
    g_slist_free(listeners);
}
#endif

/**
 * @brief Bind a socket to a given address information
 *
 * @param ai Address to bind the socket to
 * @param s The specific vhost configuration to bind for
 */
static gboolean feng_bind_addr(struct addrinfo *ai,
                               cfg_socket_t *s,
                               int ipproto)
{
    int sock;
    static const int on = 1;
    feng_socket_listener *listener = NULL;
    ev_io *io;

    if ( (sock = socket(ai->ai_family, SOCK_STREAM, ipproto)) < 0 ) {
        fnc_perror("opening socket");
        return false;
    }

#if ENABLE_SCTP
    if ( ipproto == IPPROTO_SCTP ) {
        const struct sctp_initmsg initparams = {
            .sinit_max_instreams = s->sctp_streams,
            .sinit_num_ostreams = s->sctp_streams,
        };
        static const struct sctp_event_subscribe subscribe = {
            .sctp_data_io_event = 1
        };

        // Enable the propagation of packets headers
        if (setsockopt(sock, SOL_SCTP, SCTP_EVENTS, &subscribe,
                       sizeof(subscribe)) < 0) {
            fnc_perror("setsockopt(SCTP_EVENTS)");
            goto open_error;
        }

        // Setup number of streams to be used for SCTP connection
        if (setsockopt(sock, SOL_SCTP, SCTP_INITMSG, &initparams,
                       sizeof(initparams)) < 0) {
            fnc_perror("setsockopt(SCTP_INITMSG)");
            goto open_error;
        }
    }
#endif

    if ( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                    &on, sizeof(on)) < 0 ) {
        fnc_perror("setsockopt(SO_REUSEADDR)");
        goto open_error;
    }

#if defined(IPV6_V6ONLY) && defined(IPPROTO_IPV6)
    if (ai->ai_addr->sa_family == AF_INET6) {
        if ( setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
                        &on, sizeof(on)) < 0 ) {
            fnc_perror("setsockopt(IPV6_V6ONLY)");
            goto open_error;
        }
    }
#endif

    if ( bind(sock, ai->ai_addr, ai->ai_addrlen) < 0 ) {
        fnc_perror("bind");
        goto open_error;
    }

    if ( listen(sock, SOMAXCONN ) < 0 ) {
        fnc_perror("listen");
        goto open_error;
    }

    listener = g_slice_new0(feng_socket_listener);
    listener->fd = sock;
    io = &listener->io;

    io->data = listener;
    ev_io_init(io, rtsp_client_incoming_cb, sock, EV_READ);
    ev_io_start(feng_loop, io);

#ifdef CLEANUP_DESTRUCTOR
    listeners = g_slist_prepend(listeners, listener);
#endif

    return true;

 open_error:
    close(sock);
    return false;
}

/**
 * Bind to a socket
 *
 * @param socket_p Generic pointer to the specific_config structure
 * @param user_data Unused
 */
void feng_bind_socket(gpointer socket_p, ATTR_UNUSED gpointer user_data)
{
    cfg_socket_t *socket = socket_p;
    int n;

    struct addrinfo *res, *it;
    static const struct addrinfo hints_ipv4 = {
        .ai_family = AF_INET, /* IPv4 ONLY */
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE
    };
    static const struct addrinfo hints_ipv6 = {
        .ai_family = AF_UNSPEC, /* all address families */
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE
    };

    fnc_log(FNC_LOG_INFO, "Listening to port %s (%sTCP/%sipv4) on %s",
            socket->port,
            (socket->sctp? "SCTP+" : ""),
            (socket->ipv6? "ipv6+" : ""),
            ((socket->listen_on == NULL)? "all interfaces" : socket->listen_on));

    if ( socket->ipv6 ) {
        if ( (n = getaddrinfo(socket->listen_on, socket->port, &hints_ipv6, &res)) < 0 ) {
            fnc_log(FNC_LOG_ERR, "unable to resolve %s:%s (%s)",
                    socket->listen_on, socket->port, gai_strerror(n));
            exit(1);
        }
    } else {
        if ( (n = getaddrinfo(socket->listen_on, socket->port, &hints_ipv4, &res)) < 0 ) {
            fnc_log(FNC_LOG_ERR, "unable to resolve %s:%s (%s)",
                    socket->listen_on, socket->port, gai_strerror(n));
            exit(1);
        }
    }

    it = res;
    do {
        if ( !feng_bind_addr(it, socket, IPPROTO_TCP) )
            goto error;
#if ENABLE_SCTP
        /* Only enable SCTP following TCP, otherwise Linux can become
           messed up and tries to feed TCP connections via SCTP */
        if ( socket->sctp && !feng_bind_addr(it, socket, IPPROTO_SCTP) )
            goto error;
#endif
    } while ( (it = it->ai_next) != NULL );

    freeaddrinfo(res);

    return;

 error:
    freeaddrinfo(res);
    exit(1);
}
