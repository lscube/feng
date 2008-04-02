/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
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

/**
 * @file eventloop.c
 * Network main loop
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <fenice/eventloop.h>
#include <fenice/utils.h>

#include <fenice/schedule.h>

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

static void listen_fd(feng *srv)
{
    g_ptr_array_foreach(srv->listen_socks, add_sock_fd, srv);
}

/**
 * Accepts the new connection if possible.
 */

static void new_connection(gpointer data, gpointer user_data)
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

static void new_connections(feng *srv)
{
    g_ptr_array_foreach(srv->listen_socks, new_connection, srv);
}


/**
 * Main loop waiting for clients
 * @param srv server instance variable.
 */

void eventloop(feng *srv)
{
    RTSP_buffer *p = NULL;

    //Init of scheduler
    FD_ZERO(&srv->rset);
    FD_ZERO(&srv->wset);

    if (srv->conn_count != -1) {
        listen_fd(srv);
    }

    /* Add all sockets of all sessions to fd_sets */
    for (p = srv->rtsp_list; p; p = p->next) {
        rtsp_set_fdsets(p, &srv->max_fd, &srv->rset, &srv->wset);
    }
    /* Wait for connections */
    if (select(srv->max_fd + 1, &srv->rset, &srv->wset, NULL, NULL) < 0) {
        fnc_log(FNC_LOG_ERR, "select error in eventloop(). %s\n",
                strerror(errno));
        /* Maybe we have to force exit here*/
        return;
    }
    /* transfer data for any RTSP sessions */
    schedule_connections(srv);
    /* handle new connections */

    if (srv->conn_count != -1) {
        new_connections(srv);
    }
}
