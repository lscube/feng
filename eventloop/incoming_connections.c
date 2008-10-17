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
    RTSP_buffer *new = g_new0(RTSP_buffer, 1);

    new->srv = srv;
    new->sock = client_sock;

    new->session = g_new0(RTSP_session, 1);
    new->session->session_id = -1;
    new->session->srv = srv;

    srv->clients = g_slist_prepend(srv->clients, new);

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
    srv->max_fd = MAX(Sock_fd(sock), srv->max_fd);
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
 * @brief Search function for connections in the clients list
 * @param value Current entry
 * @param compare Socket to find in the list
 */
static gboolean connections_compare_socket(gconstpointer value, gconstpointer compare)
{
  RTSP_buffer *p = (RTSP_buffer*)value;

  return !!Sock_compare((Sock *)compare, p->sock);
}

/**
 * Accepts the new connection if possible.
 */

static void incoming_connection(gpointer data, gpointer user_data)
{
    Sock *sock = data;
    feng *srv = user_data;
    Sock *client_sock = NULL;
    GSList *p = NULL;

    if (FD_ISSET(Sock_fd(sock), &srv->rset)) {
        client_sock = Sock_accept(sock, NULL);
    }

    // Handle a new connection
    if (!client_sock)
      return;

    p = g_slist_find_custom(srv->clients, client_sock,
                            connections_compare_socket);

    if ( p == NULL ) {
      if (srv->conn_count < ONE_FORK_MAX_CONNECTION) {
	++srv->conn_count;
	// ADD A CLIENT
	add_client(srv, client_sock);
      } else {
	Sock_close(client_sock);
      }
      srv->num_conn++;
      fnc_log(FNC_LOG_INFO, "Connection reached: %d\n", srv->num_conn);

      return;
    }

    fnc_log(FNC_LOG_INFO, "Connection found: %d\n",
	    Sock_fd(client_sock));
}

/**
 * Manage new connections, it iterates over the listen sockets
 */

void incoming_connections(feng *srv)
{
    g_ptr_array_foreach(srv->listen_socks, incoming_connection, srv);
}
