/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
 * See AUTHORS for more details 
 *  
 * Feng is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Feng is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *  
 * */

#include <stdio.h>
#include <string.h>

#include <fenice/eventloop.h>


// XXX FIXME FIXME I should return something meaningful!

static void add_client(feng *srv, Sock *client_sock)
{
    RTSP_buffer *p = NULL, *pp = NULL, *new = NULL;

    if (!(new = g_new0(RTSP_buffer, 1))) {
        fnc_log(FNC_LOG_FATAL, "Could not alloc memory in add_client()\n");
        return;
    }

    new->srv = srv;

    // Add a client
    if (srv->rtsp_list == NULL) {
        srv->rtsp_list = new;
    } else {
        for (p = srv->rtsp_list; p != NULL; p = p->next) {
            pp = p;
        }
        if (pp != NULL) {
            pp->next = new;
            new->next = NULL;
        }
    }

    new->sock = client_sock;
    if (!(new->session = g_new0(RTSP_session, 1))) {
        fnc_log(FNC_LOG_FATAL, "Could not alloc memory in add_client()\n");
        return;
    }

    new->session->session_id = -1;
    new->session->srv = srv;

    fnc_log(FNC_LOG_INFO,
        "Incoming RTSP connection accepted on socket: %d\n",
        Sock_fd(client_sock));
}

/**
 * Add the socket fd to the rset
 */

static void add_sock_fd(gpointer data, gpointer user_data)
{
    Sock *sock = data;
    feng *srv = user_data;

    FD_SET(Sock_fd(sock), &srv->rset);
    srv->max_fd = max(Sock_fd(sock), srv->max_fd);
}


/**
 * Add file descriptors to the rset for all the listening sockets.
 * @param srv the server instance
 */

void incoming_fd(feng *srv)
{
    g_ptr_array_foreach(srv->listen_socks, add_sock_fd, srv);
}

/**
 * Accepts the new connection if possible.
 */

static void incoming_connection(gpointer data, gpointer user_data)
{
    Sock *sock = data;
    feng *srv = user_data;
    int fd_found;
    Sock *client_sock = NULL;
    RTSP_buffer *p = NULL;

    if (FD_ISSET(Sock_fd(sock), &srv->rset)) {
        client_sock = Sock_accept(sock, NULL);
    }

    // Handle a new connection
    if (client_sock) {
        for (fd_found = 0, p = srv->rtsp_list; p != NULL; p = p->next)
            if (!Sock_compare(client_sock, p->sock)) {
                fd_found = 1;
                break;
            }
        if (!fd_found) {
            if (srv->conn_count < ONE_FORK_MAX_CONNECTION) {
                ++srv->conn_count;
                // ADD A CLIENT
                add_client(srv, client_sock);
            } else {
                Sock_close(client_sock);
            }
            srv->num_conn++;
            fnc_log(FNC_LOG_INFO, "Connection reached: %d\n", srv->num_conn);
        } else
            fnc_log(FNC_LOG_INFO, "Connection found: %d\n",
                Sock_fd(client_sock));
    }
}

/**
 * Manage new connections, it iterates over the listen sockets
 */

void incoming_connections(feng *srv)
{
    g_ptr_array_foreach(srv->listen_socks, incoming_connection, srv);
}


