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
 * Main loop waiting for clients
 * @param srv server instance variable.
 */

void eventloop(feng *srv)
{
    //Init of scheduler
    FD_ZERO(&srv->rset);
    FD_ZERO(&srv->wset);

    if (srv->conn_count != -1) {
        incoming_fd(srv);
    }

    /* Add all sockets of all sessions to rset and wset */
    g_slist_foreach(srv->clients, established_each_fd, NULL);

    /* Wait for connections */
    if (select(srv->max_fd + 1, &srv->rset, &srv->wset, NULL, NULL) < 0) {
        fnc_log(FNC_LOG_ERR, "select error in eventloop(). %s\n",
                strerror(errno));
        /* Maybe we have to force exit here*/
        return;
    }
    /* transfer data for any RTSP sessions */
    g_slist_foreach(srv->clients, established_each_connection, NULL);

    /* handle new connections */
    if (srv->conn_count != -1) {
        incoming_connections(srv);
    }
}
