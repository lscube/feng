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
#include <stdlib.h>
#include <unistd.h>

#include <fenice/eventloop.h>
#include <fenice/utils.h>
#include <fenice/schedule.h>
#include <bufferpool/bufferpool.h>

int stop_schedule = 0;

void schedule_connections(feng *srv, fd_set * rset, fd_set * wset,
                          fd_set * xset)
{
    int res;
    RTSP_buffer *p = srv->rtsp_list, *pp = NULL;
    RTP_session *r = NULL, *t = NULL;
    RTSP_interleaved *intlvd;

    while (p != NULL) {
        if ((res = rtsp_server(p, rset, wset, xset)) != ERR_NOERROR) {
            if (res == ERR_CONNECTION_CLOSE || res == ERR_GENERIC) {
                // The connection is closed
                if (res == ERR_CONNECTION_CLOSE)
                    fnc_log(FNC_LOG_INFO,
                        "RTSP connection closed by client.");
                else
                    fnc_log(FNC_LOG_INFO,
                        "RTSP connection closed by server.");
        //if client truncated RTSP connection before sending TEARDOWN: error

                if (p->session_list != NULL) {
#if 0 // Do not use it, is just for testing...
                    if (p->session_list->resource->info->multicast[0]) {
                        fnc_log(FNC_LOG_INFO,
                            "RTSP connection closed by client during"
                            " a multicast session, ignoring...");
                        continue;
                    }
#endif
                    r = p->session_list->rtp_session;

                    // Release all RTP sessions
                    while (r != NULL) {
                        // if (r->current_media->pkt_buffer);
                        // Release the scheduler entry
                        t = r->next;
                        schedule_remove(r);
                        r = t;
                    }
                    // Close connection                     
                    //close(p->session_list->fd);
                    // Release the mediathread resource
                    mt_resource_close(p->session_list->resource);
                    // Release the RTSP session
                    free(p->session_list);
                    p->session_list = NULL;
                    fnc_log(FNC_LOG_WARN,
                        "WARNING! RTSP connection truncated before ending operations.\n");
                }
                // close localfds
                for (intlvd=p->interleaved; intlvd; intlvd = intlvd->next) {
                    Sock_close(intlvd->rtp_local);
                    Sock_close(intlvd->rtcp_local);
                }
                // wait for 
                Sock_close(p->sock);
                --srv->conn_count;
                srv->num_conn--;
                // Release the RTSP_buffer
                if (p == srv->rtsp_list) {
                    srv->rtsp_list = p->next;
                    free(p);
                    p = srv->rtsp_list;
                } else {
                    pp->next = p->next;
                    free(p);
                    p = pp->next;
                }
                // Release the scheduler if necessary
                if (p == NULL && srv->conn_count < 0) {
                    fnc_log(FNC_LOG_DEBUG,
                        "Thread stopped\n");
                    stop_schedule = 1;
                }
            } else {
                p = p->next;
            }
        } else {
            pp = p;
            p = p->next;
        }
    }
}
