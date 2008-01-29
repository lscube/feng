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

#include <stdio.h>
#include <string.h>

#include <fenice/eventloop.h>


// XXX FIXME FIXME I should return something meaningfull!

void add_client(feng *srv, RTSP_buffer ** rtsp_list, Sock *client_sock)
{
    RTSP_buffer *p = NULL, *pp = NULL, *new = NULL;

    if (!(new = calloc(1, sizeof(RTSP_buffer)))) {
        fnc_log(FNC_LOG_FATAL, "Could not alloc memory in add_client()\n");
        return;
    }

    new->srv = srv;

    // Add a client
    if (*rtsp_list == NULL) {
        *rtsp_list = new;
    } else {
        for (p = *rtsp_list; p != NULL; p = p->next) {
            pp = p;
        }
        if (pp != NULL) {
            pp->next = new;
            new->next = NULL;
        }
    }

    new->sock = client_sock;
    if (!(new->session_list = calloc(1, sizeof(RTSP_session)))) {
        fnc_log(FNC_LOG_FATAL, "Could not alloc memory in add_client()\n");
        return;
    }

    new->session_list->session_id = -1;
    new->session_list->srv = srv;

    fnc_log(FNC_LOG_INFO,
        "Incoming RTSP connection accepted on socket: %d\n",
        Sock_fd(client_sock));
}
