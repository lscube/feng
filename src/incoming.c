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
#include <ev.h>

#include "feng.h"
#include "fnc_log.h"
#include "incoming.h"
#include "network/rtsp.h"

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
static ev_io *listeners;

/**
 * @brief List of listening sockets for the process
 *
 * This array is created in @ref feng_bind_ports and used in @ref
 * feng_bind_port if the cleanup destructors are enabled, and will be
 * used by @ref feng_ports_cleanup() to close the sockets and free
 * their memory.
 *
 * @note Part of the cleanup destructors code, not compiled in
 *       production use.
 */
static GPtrArray *listening_sockets;

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
    neb_sock_close((Sock*)element);
}

/**
 * @brief Cleanup function for the @ref listeners array
 *
 * This function is used to free the @ref listeners array that was
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
    if ( listening_sockets == NULL ) return;

    g_ptr_array_foreach(listening_sockets, feng_bound_socket_close, NULL);
    g_ptr_array_free(listening_sockets, true);
    g_free(listeners);
}
#endif

/**
 * Bind to the defined listening port
 *
 * @param srv The server instance to bind ports for
 * @param host The hostname to bind ports on
 * @param port The port to bind
 * @param s The specific configuration from @ref feng::config_storage
 * @param listener The listener pointer from @ref listeners
 *
 * @retval true Binding complete
 * @retval false Error during binding
 */
static gboolean feng_bind_port(feng *srv, const char *host, const char *port,
                               specific_config *s, ev_io *listener)
{
    gboolean is_sctp = !!s->is_sctp;
    Sock *sock;
    int on = 1;

    if (is_sctp)
        sock = neb_sock_bind(host, port, NULL, SCTP);
    else
        sock = neb_sock_bind(host, port, NULL, TCP);
    if(!sock) {
        fnc_log(FNC_LOG_ERR,"neb_sock_bind() error for port %s.", port);
        fprintf(stderr,
                "[fatal] neb_sock_bind() error in main() for port %s.\n",
                port);
        return false;
    }

#ifdef CLEANUP_DESTRUCTOR
    g_ptr_array_add(listening_sockets, sock);
#endif

    fnc_log(FNC_LOG_INFO, "Listening to port %s (%s) on %s",
            port,
            (is_sctp? "SCTP" : "TCP"),
            ((host == NULL)? "all interfaces" : host));

    if(listen(sock->fd, SOMAXCONN)) {
        fnc_log(FNC_LOG_ERR, "Cannot listen on port %s (%s) on %s",
                port,
                (is_sctp? "SCTP" : "TCP"),
                ((host == NULL)? "all interfaces" : host));
        fprintf(stderr, "[fatal] Cannot listen on port %s (%s) on %s",
                port,
                (is_sctp? "SCTP" : "TCP"),
                ((host == NULL)? "all interfaces" : host));
        return false;
    }
    if (setsockopt(sock->fd,
                   SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        fnc_log(FNC_LOG_WARN, "SO_REUSEADDR unavailable");
    }

    sock->data = srv;
    listener->data = sock;
    ev_io_init(listener,
               rtsp_client_incoming_cb,
               sock->fd, EV_READ);
    ev_io_start(srv->loop, listener);

    return true;
}

gboolean feng_bind_ports(feng *srv)
{
    size_t i;
    char *host = srv->srvconf.bindhost->ptr;
    char port[6] = { 0, };
#ifndef CLEANUP_DESTRUCTORS
    /* We make it local if we don't need the cleanup */
    ev_io *listeners;
#endif

    snprintf(port, sizeof(port), "%d", srv->srvconf.port);

    /* This is either static or local, we don't care */
    listeners = g_new0(ev_io, srv->config_context->used);
#ifdef CLEANUP_DESTRUCTOR
    listening_sockets = g_ptr_array_sized_new(srv->config_context->used);
#endif

    if (!feng_bind_port(srv, host, port,
                              &srv->config_storage[0],
                              &listeners[0]))
        return false;

   /* check for $SERVER["socket"] */
    for (i = 1; i < srv->config_context->used; i++) {
        data_config *dc = (data_config *)srv->config_context->data[i];
        specific_config *s = &srv->config_storage[i];
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

        if (!feng_bind_port(srv, host, port,
                            s, &listeners[i]))
            return false;
    }

    return true;
}
