/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *  	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Eugenio Menegatti 	<m.eu@libero.it>
 *	- Stefano Cau
 *	- Giuliano Emma
 *	- Stefano Oldrini
 *	- Dario Gallucci	<dario.gallucci@gmail.com>
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
#include <netembryo/wsocket.h>
#include <fenice/socket.h>
#include <fenice/eventloop.h>
#include <fenice/utils.h>
#include <fenice/rtsp.h>
#include <fenice/schedule.h>
#include <fenice/types.h>
#include <fenice/fnc_log.h>

int num_conn = 0;

void eventloop(tsocket main_fd, tsocket main_sctp_fd)
{
	static uint32 child_count = 0;
	static int conn_count = 0;
	tsocket max_fd, fd = -1;
	static RTSP_buffer *rtsp_list = NULL;
	RTSP_buffer *p = NULL;
	RTSP_proto rtsp_proto;
	uint32 fd_found;
	fd_set rset,wset;

	//Init of scheduler
	FD_ZERO(&rset);
	FD_ZERO(&wset);

	if (conn_count != -1) {
		/* This is the process allowed for accepting new clients */
		FD_SET(main_fd, &rset);
		max_fd = main_fd;
#ifdef HAVE_SCTP_FENICE
		if (main_sctp_fd >= 0) {
			FD_SET(main_sctp_fd, &rset);
			max_fd = max(max_fd, main_sctp_fd);
		}
#endif
	}

	/* Add all sockets of all sessions to fd_sets */
	for (p = rtsp_list; p; p = p->next) {
		rtsp_set_fdsets(p, &max_fd, &rset, &wset, NULL);
	}
	/* Stay here and wait for something happens */
	if (select(max_fd+1, &rset, &wset, NULL, NULL) < 0) {
		fnc_log(FNC_LOG_ERR, "select error in eventloop().\n");
		/* Maybe we have to force exit here*/
		return;
	}
	/* transfer data for any RTSP sessions */
	schedule_connections(&rtsp_list, &conn_count, &rset, &wset, NULL);
	/* handle new connections */
	if (conn_count != -1) {
#ifdef HAVE_SCTP_FENICE
		if (main_sctp_fd >= 0 && FD_ISSET(main_sctp_fd, &rset)) {
			fd = sctp_accept(main_sctp_fd);
			rtsp_proto = SCTP;
		} else
#endif
		if (FD_ISSET(main_fd, &rset)) {
			fd = tcp_accept(main_fd);
			rtsp_proto = TCP;
		}
		// Handle a new connection
		if (fd >= 0) {
			for (fd_found = 0, p = rtsp_list; p != NULL; p = p->next)
				if (p->fd == fd) {
					fd_found = 1;
					break;
				}
			if (!fd_found) {
				if (conn_count < ONE_FORK_MAX_CONNECTION) {
				++conn_count;
				// ADD A CLIENT
				add_client(&rtsp_list, fd, rtsp_proto);
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
						add_client(&rtsp_list, fd, rtsp_proto);
					} else {
						// I'm the father
						fd = -1;
						conn_count = -1;
						tcp_close(main_fd);
#ifdef HAVE_SCTP_FENICE
						if (main_sctp_fd >= 0)
							sctp_close(main_sctp_fd);
#endif
					}
				}
				num_conn++;
				fnc_log(FNC_LOG_INFO, "Connection reached: %d\n",
					num_conn);
			}
		}
	} // shawill: and... if not?  END OF "HANDLE NEW CONNECTIONS"
}
