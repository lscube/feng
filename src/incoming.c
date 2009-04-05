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

#include <ev.h>

#include "feng.h"
#include "fnc_log.h"
#include "incoming.h"
#include "client_events.h"
#include "network/rtsp.h"

/**
 * Accepts the new connection if possible.
 */

static void
incoming_connection_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    Sock *sock = w->data;
    feng *srv = sock->data;
    Sock *client_sock = NULL;

    if ( (client_sock = Sock_accept(sock, NULL)) == NULL )
        return;

    if (srv->connection_count >= ONE_FORK_MAX_CONNECTION) {
        Sock_close(client_sock);
        return;
    }

    client_add(srv, client_sock);
    fnc_log(FNC_LOG_INFO, "Connection reached: %d\n", srv->connection_count);
}

/**
 * Bind to the defined listening port
 *
 * @param srv The server instance to bind ports for
 * @param host The hostname to bind ports on
 * @param port The port to bind
 * @param cfg_storage_idx The index in @ref feng::config_storage array
 *                        for the current configuration
 *
 * @retval 1 Error during binding
 * @retval 0 Binding complete
 */
int feng_bind_port(struct feng *srv, const char *host, const char *port,
                   size_t cfg_storage_idx)
{
    specific_config *s = srv->config_storage[cfg_storage_idx];
    gboolean is_sctp = !!s->is_sctp;
    Sock *sock;

    if (is_sctp)
        sock = Sock_bind(host, port, NULL, SCTP, NULL);
    else
        sock = Sock_bind(host, port, NULL, TCP, NULL);
    if(!sock) {
        fnc_log(FNC_LOG_ERR,"Sock_bind() error for port %s.", port);
        fprintf(stderr,
                "[fatal] Sock_bind() error in main() for port %s.\n",
                port);
        return 1;
    }

    fnc_log(FNC_LOG_INFO, "Listening to port %s (%s) on %s",
            port,
            (is_sctp? "SCTP" : "TCP"),
            ((host == NULL)? "all interfaces" : host));

    if(Sock_listen(sock, SOMAXCONN)) {
        fnc_log(FNC_LOG_ERR, "Sock_listen() error for TCP socket.");
        fprintf(stderr, "[fatal] Sock_listen() error for TCP socket.\n");
        return 1;
    }
    sock->data = srv;
    srv->listeners[cfg_storage_idx].data = sock;
    ev_io_init(&srv->listeners[cfg_storage_idx], incoming_connection_cb,
               Sock_fd(sock), EV_READ);
    ev_io_start(srv->loop, &srv->listeners[cfg_storage_idx]);
    return 0;
}
