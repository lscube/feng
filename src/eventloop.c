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
 * @file eventloop.c
 * Network main loop
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <ev.h>

#include "fnc_log.h"
#include "eventloop.h"
#include "client_events.h"
#include "network/rtsp.h"
#include <fenice/utils.h>

static GPtrArray *io_watchers; //!< keep track of ev_io allocated

static void add_client(feng *srv, Sock *client_sock)
{
    RTSP_Client *rtsp = rtsp_client_new(srv, client_sock);

    srv->clients = g_slist_prepend(srv->clients, rtsp);
    client_sock->data = srv;

    rtsp->ev_io_read.data = rtsp;
    g_ptr_array_add(io_watchers, &rtsp->ev_io_read);
    ev_io_init(&rtsp->ev_io_read, rtsp_read_cb, Sock_fd(client_sock), EV_READ);
    ev_io_start(srv->loop, &rtsp->ev_io_read);

    /* to be started/stopped when necessary */
    rtsp->ev_io_write.data = rtsp;
    ev_io_init(&rtsp->ev_io_write, rtsp_write_cb, Sock_fd(client_sock), EV_WRITE);
    fnc_log(FNC_LOG_INFO, "Incoming RTSP connection accepted on socket: %d\n",
            Sock_fd(client_sock));

    client_events_register(rtsp);
}

/**
 * @brief Search function for connections in the clients list
 * @param value Current entry
 * @param compare Socket to find in the list
 */
static gboolean
connections_compare_socket(gconstpointer value, gconstpointer compare)
{
    RTSP_Client *p = (RTSP_Client*)value;
    return !!Sock_compare((Sock *)compare, p->sock);
}

/**
 * Accepts the new connection if possible.
 */

static void
incoming_connection_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    Sock *sock = w->data;
    feng *srv = sock->data;
    Sock *client_sock = NULL;
    GSList *p = NULL;

    client_sock = Sock_accept(sock, NULL);

    // Handle a new connection
    if (!client_sock)
        return;

    p = g_slist_find_custom(srv->clients, client_sock,
                            connections_compare_socket);

    if ( p == NULL ) {
        if (srv->conn_count < ONE_FORK_MAX_CONNECTION) {
            ++srv->conn_count;
            add_client(srv, client_sock);
        } else {
            Sock_close(client_sock);
        }
        srv->num_conn++;
        fnc_log(FNC_LOG_INFO, "Connection reached: %d\n", srv->num_conn);

        return;
    }

    Sock_close(client_sock);

    fnc_log(FNC_LOG_INFO, "Connection found: %d\n", Sock_fd(client_sock));
}

/**
 * Bind to the defined listening port
 */

int feng_bind_port(feng *srv, char *host, char *port, specific_config *s)
{
    int is_sctp = s->is_sctp;
    Sock *sock;
    ev_io *ev_io_listen = g_new(ev_io, 1);

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
    ev_io_listen->data = sock;
    g_ptr_array_add(io_watchers, ev_io_listen);
    ev_io_init(ev_io_listen, incoming_connection_cb,
               Sock_fd(sock), EV_READ);
    ev_io_start(srv->loop, ev_io_listen);
    return 0;
}

/**
 * @brief Initialise data structure needed for eventloop
 */
void eventloop_init(feng *srv)
{
    io_watchers = g_ptr_array_new();
    srv->loop = ev_default_loop(0);
    srv->clients = NULL;
}

/**
 * Main loop waiting for clients
 * @param srv server instance variable.
 */

void eventloop(feng *srv)
{
    ev_loop (srv->loop, 0);
}

/**
 * @brief Cleanup data structure needed by the eventloop
 */
void eventloop_cleanup(feng *srv) {
    g_ptr_array_free(io_watchers, TRUE);
}
