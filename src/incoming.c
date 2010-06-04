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

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include <ev.h>

#include "feng.h"
#include "fnc_log.h"
#include "incoming.h"
#include "network/rtsp.h"
#include "network/netembryo.h"

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
 * @brief libev listeners for incoming connections on opened ports
 *
 * This is an array of ev_io objects allocated with the g_new0()
 * function (which this need to be freed with g_free()).
 *
 * The indexes are the same as for @ref feng::config_storage.
 *
 * @note Part of the cleanup destructors code, not compiled in
 *       production use.
 */
static GSList *listening;

/**
 * @brief Close sockets in the listening_sockets array
 *
 * @param element The Sock object to close
 * @param user_data Unused
 *
 * This function is used to close the opened software during cleanup.
 *
 * @note Part of the cleanup destructors code, not compiled in
 *       production use.
 */
static void feng_bound_socket_close(gpointer element,
                                    ATTR_UNUSED gpointer user_data)
{
    Feng_Listener *listener = element;

    close(listener->fd);
    free(listener->local_host);

    g_slice_free1(listener->sa_len, listener->local_sa);

    g_slice_free(Feng_Listener, listener);
}

/**
 * @brief Cleanup function for the @ref listening array
 *
 * This function is used to free the @ref listening array that was
 * allocated in @ref feng_bind_ports; note that this function is
 * called automatically as a destructur when the compiler supports it,
 * and debug is not disabled.
 *
 * This function is unnecessary on production code, since the memory
 * would be freed only at the end of execution, when the resources
 * would be freed anyway.
 *
 * @note Part of the cleanup destructors code, not compiled in
 *       production use.
 */
static void CLEANUP_DESTRUCTOR feng_ports_cleanup()
{
    if ( listening == NULL ) return;

    g_slist_foreach(listening, feng_bound_socket_close, NULL);
    g_slist_free(listening);
}
#endif

/**
 * @brief Bind a socket to a given address information
 *
 * @param ai Address to bind the socket to
 * @param s The specific vhost configuration to bind for
 */
static gboolean feng_bind_addr(struct addrinfo *ai,
                               specific_config *s)
{
    int sock;
    static const int on = 1;
    Feng_Listener *listener = NULL;
    struct sockaddr_storage sa;
    socklen_t sa_len = sizeof(struct sockaddr_storage);
    ev_io *io;

#if ENABLE_SCTP
    const int ipproto = s->is_sctp ? IPPROTO_SCTP : IPPROTO_TCP;
#else
    static const int ipproto = IPPROTO_TCP;
#endif

    if ( (sock = socket(ai->ai_family, SOCK_STREAM, ipproto)) < 0 ) {
        fnc_perror("opening socket");
        return false;
    }

#if ENABLE_SCTP
    if ( s->is_sctp ) {
        const struct sctp_initmsg initparams = {
            .sinit_max_instreams = s->sctp_max_streams,
            .sinit_num_ostreams = s->sctp_max_streams,
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

    /* For IPv6 sockets, we make sure to only bind the v6 address, and
       we'll then be binding the v4 later on. */

    if ( ai->ai_family == AF_INET6 &&
         setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
                    &on, sizeof(on)) < 0 ) {
        fnc_perror("setsockopt(IPV6_V6ONLY)");
        goto open_error;
    }

    if ( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                    &on, sizeof(on)) < 0 ) {
        fnc_perror("setsockopt(SO_REUSEADDR)");
        goto open_error;
    }

    if ( bind(sock, ai->ai_addr, ai->ai_addrlen) < 0 ) {
        fnc_perror("bind");
        goto open_error;
    }

    if ( listen(sock, SOMAXCONN ) < 0 ) {
        fnc_perror("listen");
        goto open_error;
    }

    if ( getsockname(sock, (struct sockaddr *)(&sa), &sa_len) < 0 ) {
        fnc_perror("getsockname");
        goto open_error;
    }

    listener = g_slice_new0(Feng_Listener);

    listener->fd = sock;
    listener->specific = s;

    listener->sa_len = sa_len;
    listener->local_sa = g_slice_copy(sa_len, &sa);
    listener->local_host = neb_sa_get_host(listener->local_sa);
    if ( listener->local_host == NULL )
        listener->local_host = strdup(ai->ai_family == AF_INET6 ? "::" : "0.0.0.0");

    io = &listener->io;
    io->data = listener;
    ev_io_init(io, rtsp_client_incoming_cb, sock, EV_READ);
    ev_io_start(feng_srv.loop, io);

#ifdef CLEANUP_DESTRUCTOR
    listening = g_slist_prepend(listening, listener);
#endif

    return true;

 open_error:
    close(sock);
    return false;
}

/**
 * Bind to the defined listening port
 *
 * @param host The hostname to bind ports on
 * @param port The port to bind
 * @param s The specific configuration from @ref feng::config_storage
 *
 * @retval true Binding complete
 * @retval false Error during binding
 */
static gboolean feng_bind_port(const char *host, const char *port,
                               specific_config *s)
{
    gboolean is_sctp = !!s->is_sctp;
    int n;

    struct addrinfo *res, *it;
    static const struct addrinfo hints_ipv4 = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE
    };
    static const struct addrinfo hints_ipv6 = {
        .ai_family = AF_INET6,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE
    };

    /* if host is empty, make it NULL */
    if ( host != NULL && *host == '\0' )
        host = NULL;

    fnc_log(FNC_LOG_INFO, "Listening to port %s (%s/%s) on %s",
            port,
            (is_sctp? "SCTP" : "TCP"),
            (s->use_ipv6? "ipv6" : "ipv4"),
            ((host == NULL)? "all interfaces" : host));

    if ( s->use_ipv6 ) {
        if ( (n = getaddrinfo(host, port, &hints_ipv6, &res)) < 0 ) {
            fnc_log(FNC_LOG_ERR, "unable to resolve %s:%s (%s)",
                    host, port, gai_strerror(n));
            return false;
        }

        it = res;
        do
            if ( !feng_bind_addr(it, s) )
                goto error;
        while ( (it = it->ai_next) != NULL );

        freeaddrinfo(res);
    }

    if ( (n = getaddrinfo(host, port, &hints_ipv4, &res)) < 0 ) {
        fnc_log(FNC_LOG_ERR, "unable to resolve %s:%s (%s)",
                host, port, gai_strerror(n));
        return false;
    }

    it = res;
    do
        if ( !feng_bind_addr(it, s) )
            goto error;
    while ( (it = it->ai_next) != NULL );

    freeaddrinfo(res);

    return true;

 error:
    freeaddrinfo(res);
    return false;
}

gboolean feng_bind_ports()
{
    size_t i;
    char *host = feng_srv.srvconf.bindhost;
    char port[6] = { 0, };

    snprintf(port, sizeof(port), "%d", feng_srv.srvconf.port);

    if (!feng_bind_port(host, port,
                        &feng_srv.config_storage[0]))
        return false;

   /* check for $SERVER["socket"] */
    for (i = 1; i < feng_srv.config_context->used; i++) {
        data_config *dc = (data_config *)feng_srv.config_context->data[i];
        specific_config *s = &feng_srv.config_storage[i];
        char *port, *host;

        /* not our stage */
        if (COMP_SERVER_SOCKET != dc->comp) continue;

        if (dc->cond != CONFIG_COND_EQ) {
            fnc_log(FNC_LOG_ERR,"only == is allowed for $SERVER[\"socket\"].");
            return false;
        }
        /* check if we already know this socket,
         * if yes, don't init it */

        /* split the host:port line */
        host = dc->string->ptr;
        port = strrchr(host, ':');
        if (!port) {
            fnc_log(FNC_LOG_ERR,"Cannot parse \"%s\" as host:port",
                                dc->string->ptr);
            return false;
        }

        port[0] = '\0';

        if (host[0] == '[' && port[-1] == ']') {
            port[-1] = '\0';
            host++;
            s->use_ipv6 = 1; //XXX
        }

        port++;

        if (!feng_bind_port(host, port, s))
            return false;
    }

    return true;
}
