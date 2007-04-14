/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *      
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
 *    - Eugenio Menegatti     <m.eu@libero.it>
 *    - Stefano Cau
 *    - Giuliano Emma
 *    - Stefano Oldrini
 *    - Dario Gallucci    <dario.gallucci@gmail.com>
 * 
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <fenice/eventloop.h>
#include <fenice/utils.h>

#include <fenice/schedule.h>

int num_conn = 0;

void eventloop(Sock *main_sock, Sock *sctp_main_sock)
{
    static uint32_t child_count = 0;
    static int conn_count = 0;
    int max_fd;
    static RTSP_buffer *rtsp_list = NULL;
    RTSP_buffer *p = NULL;
    uint32 fd_found;
    fd_set rset,wset;
    Sock *client_sock = NULL;

    //Init of scheduler
    FD_ZERO(&rset);
    FD_ZERO(&wset);

    if (conn_count != -1) {
        /* This is the process allowed for accepting new clients */
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
    for (p = rtsp_list; p; p = p->next) {
        rtsp_set_fdsets(p, &max_fd, &rset, &wset, NULL);
    }
    /* Stay here and wait for something happens */
    if (select(max_fd + 1, &rset, &wset, NULL, NULL) < 0) {
        fnc_log(FNC_LOG_ERR, "select error in eventloop(). %s\n", strerror(errno));
        /* Maybe we have to force exit here*/
        return;
    }
    /* transfer data for any RTSP sessions */
    schedule_connections(&rtsp_list, &conn_count, &rset, &wset, NULL);
    /* handle new connections */
    if (conn_count != -1) {
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
            for (fd_found = 0, p = rtsp_list; p != NULL; p = p->next)
                if (!Sock_compare(client_sock, p->sock)) {
                    fd_found = 1;
                    break;
                }
            if (!fd_found) {
                if (conn_count < ONE_FORK_MAX_CONNECTION) {
                ++conn_count;
                // ADD A CLIENT
                add_client(&rtsp_list, client_sock);
                } else {
                    if (fork() == 0) {
                        // I'm the child
                        ++child_count;
                        RTP_port_pool_init
                            (ONE_FORK_MAX_CONNECTION *
                             child_count * 2 +
                             RTP_DEFAULT_PORT);
                        if (schedule_init() == ERR_FATAL) {
                            fnc_log(FNC_LOG_FATAL,
                            "Can't start scheduler. "
                            "Server is aborting.\n");
                            return;
                        }
                        conn_count = 1;
                        rtsp_list = NULL;
                        add_client(&rtsp_list, client_sock);
                    } else {
                        // I'm the father
                        conn_count = -1;
                        Sock_close(client_sock);
                        Sock_close(main_sock);
#ifdef HAVE_LIBSCTP
                        if (sctp_main_sock)
                            Sock_close(sctp_main_sock);
#endif
                    }
                }
                num_conn++;
                fnc_log(FNC_LOG_INFO, "Connection reached: %d\n",
                    num_conn);
            } else
                fnc_log(FNC_LOG_INFO, "Connection found: %d\n",
                    Sock_fd(client_sock));
        }
    } // shawill: and... if not?  END OF "HANDLE NEW CONNECTIONS"
}
