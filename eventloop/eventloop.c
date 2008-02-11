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
#include <sys/types.h>
#include <unistd.h>

#include <fenice/eventloop.h>
#include <fenice/utils.h>

#include <fenice/schedule.h>

void eventloop(feng *srv)
{
    Sock *main_sock = srv->main_sock;
    Sock *sctp_main_sock = srv->sctp_main_sock;

    int max_fd;
    RTSP_buffer *p = NULL;
    int fd_found;
    fd_set rset,wset;
    Sock *client_sock = NULL;

    //Init of scheduler
    FD_ZERO(&rset);
    FD_ZERO(&wset);

    if (srv->conn_count != -1) {
        /* This process is accepting new clients */
        FD_SET(Sock_fd(main_sock), &rset);
        max_fd = Sock_fd(main_sock);
#ifdef HAVE_LIBSCTP
        if (sctp_main_sock) {
            FD_SET(Sock_fd(sctp_main_sock), &rset);
            max_fd = max(max_fd, Sock_fd(sctp_main_sock));
        }
#endif
    }

    /* Add all sockets of all sessions to fd_sets */
    for (p = srv->rtsp_list; p; p = p->next) {
        rtsp_set_fdsets(p, &max_fd, &rset, &wset);
    }
    /* Stay here and wait for something happens */
    if (select(max_fd + 1, &rset, &wset, NULL, NULL) < 0) {
        fnc_log(FNC_LOG_ERR, "select error in eventloop(). %s\n",
                strerror(errno));
        /* Maybe we have to force exit here*/
        return;
    }
    /* transfer data for any RTSP sessions */
    schedule_connections(srv, &rset, &wset);
    /* handle new connections */
    if (srv->conn_count != -1) {
#ifdef HAVE_LIBSCTP
        if (sctp_main_sock && FD_ISSET(Sock_fd(sctp_main_sock), &rset)) {
            client_sock = Sock_accept(sctp_main_sock);
        } else
#endif
        if (FD_ISSET(Sock_fd(main_sock), &rset)) {
            client_sock = Sock_accept(main_sock);
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
                #if 0
                    // Pending complete rewrite
                    if (fork() == 0) {
                        // I'm the child
                        ++child_count;
                        RTP_port_pool_init (ONE_FORK_MAX_CONNECTION *
                                            child_count * 2 +
                             *((int *) get_pref(PREFS_FIRST_UDP_PORT)));
                        if (schedule_init() == ERR_FATAL) {
                            fnc_log(FNC_LOG_FATAL, "Can't start scheduler. "
                                    "Server is aborting.\n");
                            return;
                        }
                        conn_count = 1;
                        rtsp_list = NULL;
                        add_client(srv, &rtsp_list, client_sock);
                    } else {
                        // I'm the father
                        conn_count = -1;
                #endif
                        Sock_close(client_sock);
                        Sock_close(main_sock);
#ifdef HAVE_LIBSCTP
                        if (sctp_main_sock)
                            Sock_close(sctp_main_sock);
#endif
                }
                srv->num_conn++;
                fnc_log(FNC_LOG_INFO, "Connection reached: %d\n",
                    srv->num_conn);
            } else
                fnc_log(FNC_LOG_INFO, "Connection found: %d\n",
                    Sock_fd(client_sock));
        }
    } // shawill: and... if not?  END OF "HANDLE NEW CONNECTIONS"
}
