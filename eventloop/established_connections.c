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

int rtsp_server(RTSP_buffer * rtsp, fd_set * rset, fd_set * wset)
{
    int size;
    char buffer[RTSP_BUFFERSIZE + 1];    /* +1 to control the final '\0' */
    int i, n;
    int res;
    RTSP_session *q = NULL;
    RTP_session *p = NULL;
    RTSP_interleaved *intlvd;
#ifdef HAVE_LIBSCTP
    struct sctp_sndrcvinfo sctp_info;
    int m = 0;
#endif

    if (rtsp == NULL) {
        return ERR_NOERROR;
    }
    if (FD_ISSET(Sock_fd(rtsp->sock), wset)) { // first of all: there is some data to send?
        //char is_interlvd = rtsp->interleaved_size ? 1 : 0; 
        // There are RTSP packets to send
        if ( (n = RTSP_send(rtsp)) < 0) {
            send_reply(500, NULL, rtsp);
            return ERR_GENERIC;// internal server error
        }
#ifdef VERBOSE
         else if (*rtsp->out_buffer != '$') {
            fnc_log(FNC_LOG_VERBOSE, "OUTPUT_BUFFER was:\n");
            dump_buffer(rtsp->out_buffer);
        }
#endif
    }
    if (FD_ISSET(Sock_fd(rtsp->sock), rset)) {
        // There are RTSP or RTCP packets to read in
        memset(buffer, 0, sizeof(buffer));
        size = sizeof(buffer) - 1;
#ifdef HAVE_LIBSCTP
        if (Sock_type(rtsp->sock) == SCTP) {
            memset(&sctp_info, 0, sizeof(sctp_info));
            n = Sock_read(rtsp->sock, buffer, size, &sctp_info, 0);
            m = sctp_info.sinfo_stream;
            fnc_log(FNC_LOG_DEBUG,
                "Sock_read() received %d bytes from sctp stream %d\n", n, m);
        } else {    // RTSP protocol is TCP
#endif    // HAVE_LIBSCTP
            n = Sock_read(rtsp->sock, buffer, size, NULL, 0);
#ifdef HAVE_LIBSCTP
        }
#endif    // HAVE_LIBSCTP
        if (n == 0) {
            return ERR_CONNECTION_CLOSE;
        }
        if (n < 0) {
            fnc_log(FNC_LOG_DEBUG,
                "Sock_read() error in rtsp_server()\n");
            send_reply(500, NULL, rtsp);
            return ERR_GENERIC;    //errore interno al server                           
        }
        if (Sock_type(rtsp->sock) == TCP
#ifdef HAVE_LIBSCTP
            || (Sock_type(rtsp->sock) == SCTP&& m == 0)
#endif    // HAVE_LIBSCTP 
                        ) {
            if (rtsp->in_size + n > RTSP_BUFFERSIZE) {
                fnc_log(FNC_LOG_DEBUG,
                    "RTSP buffer overflow (input RTSP message is most likely invalid).\n");
                send_reply(500, NULL, rtsp);
                return ERR_GENERIC;    //errore da comunicare
            }
#ifdef VERBOSE
            fnc_log(FNC_LOG_VERBOSE, "INPUT_BUFFER was:\n");
            dump_buffer(buffer);
#endif
            memcpy(&(rtsp->in_buffer[rtsp->in_size]), buffer, n);
            rtsp->in_size += n;
            if ((res = RTSP_handler(rtsp)) == ERR_GENERIC) {
                fnc_log(FNC_LOG_ERR,
                    "Invalid input message.\n");
                return ERR_GENERIC;
            }
        } else {    /* if (rtsp->proto == SCTP && m != 0) */
#ifdef HAVE_LIBSCTP
            for (intlvd = rtsp->interleaved;
                 intlvd && !((intlvd->proto.sctp.rtp.sinfo_stream == m)
                || (intlvd->proto.sctp.rtcp.sinfo_stream == m));
                 intlvd = intlvd->next);
            if (intlvd) {
                if (m == intlvd->proto.sctp.rtcp.sinfo_stream) {
                    Sock_write(intlvd->rtcp_local, buffer, n, NULL, 0);
                } else {    // RTP pkt arrived: do nothing...
                    fnc_log(FNC_LOG_DEBUG,
                        "Interleaved RTP packet arrived from stream %d.\n",
                        m);
                }
            } else {
                fnc_log(FNC_LOG_DEBUG,
                    "Packet arrived from unknown stream (%d)... ignoring.\n",
                    m);
            }
#endif    // HAVE_LIBSCTP
        }
    }
    for (intlvd=rtsp->interleaved; intlvd; intlvd=intlvd->next) {
        if ( FD_ISSET(Sock_fd(intlvd->rtcp_local), rset) ) {
            if ( (n = Sock_read(intlvd->rtcp_local, buffer, RTSP_BUFFERSIZE, NULL, 0)) < 0) {
                fnc_log(FNC_LOG_ERR, "Error reading from local socket\n");
                continue;
            }
            switch (Sock_type(rtsp->sock)){
            case TCP:
                if ((i = rtsp->out_size) + n < RTSP_BUFFERSIZE - RTSP_RESERVED) {
                    rtsp->out_buffer[i] = '$';
                    rtsp->out_buffer[i + 1] = (unsigned char) intlvd->proto.tcp.rtcp_ch;
                    *((uint16_t *) & rtsp->out_buffer[i + 2]) = 
                        htons((uint16_t) n);
                    rtsp->out_size += n + 4;
                    memcpy(rtsp->out_buffer + i + 4, buffer, n);
                    if ( (n = RTSP_send(rtsp)) < 0) {
                        send_reply(500, NULL, rtsp);
                        return ERR_GENERIC;// internal server error
                    }
                }
                break;
#ifdef HAVE_LIBSCTP
            case SCTP:
                memcpy(&sctp_info, &(intlvd->proto.sctp.rtcp), sizeof(struct sctp_sndrcvinfo));
                Sock_write(rtsp->sock, buffer, n, &sctp_info, MSG_DONTWAIT | MSG_EOR);
                break;
#endif
            default:
                break;
            }
        }
        if ( FD_ISSET(Sock_fd(intlvd->rtp_local), rset) ) {
            if ( (n = Sock_read(intlvd->rtp_local, buffer, RTSP_BUFFERSIZE, NULL, 0)) < 0) {
            // if ( (n = read(intlvd->rtp_fd, intlvd->out_buffer, sizeof(intlvd->out_buffer))) < 0) {
                fnc_log(FNC_LOG_ERR, "Error reading from local socket\n");
                continue;
            }
            switch (Sock_type(rtsp->sock)){
            case TCP:
                if ((i = rtsp->out_size) + n < RTSP_BUFFERSIZE - RTSP_RESERVED) {
                    rtsp->out_buffer[i] = '$';
                    rtsp->out_buffer[i + 1] = (unsigned char) intlvd->proto.tcp.rtp_ch;
                    *((uint16_t *) & rtsp->out_buffer[i + 2]) =
                        htons((uint16_t) n);
                    rtsp->out_size += n + 4;
                    memcpy(rtsp->out_buffer + i + 4, buffer, n);
                    if ( (n = RTSP_send(rtsp)) < 0) {
                        send_reply(500, NULL, rtsp);
                        return ERR_GENERIC;// internal server error
                    }
                }
            break;
#ifdef HAVE_LIBSCTP
            case SCTP:
                memcpy(&sctp_info, &(intlvd->proto.sctp.rtp), sizeof(struct sctp_sndrcvinfo));
                Sock_write(rtsp->sock, buffer, n, &sctp_info, MSG_DONTWAIT | MSG_EOR);
                break;
#endif
            default:
                break;
            }
        }
    }
    for (q = rtsp->session_list, p = q ? q->rtp_session : NULL; p; p = p->next) {
        if ( (p->transport.rtcp_sock) && 
                FD_ISSET(Sock_fd(p->transport.rtcp_sock), rset)) {
            // There are RTCP packets to read in
            if (RTP_recv(p, rtcp_proto) < 0) {
                fnc_log(FNC_LOG_VERBOSE,
                    "Input RTCP packet Lost\n");
            } else {
                RTCP_recv_packet(p);
            }
            fnc_log(FNC_LOG_VERBOSE, "IN RTCP\n");
        }
        /*---------SEE rtcp/RTCP_handler.c-----------------*/
        /* if (FD_ISSET(p->rtcp_fd_out,wset)) {
         *     // There are RTCP packets to send
         *     if ((psize_sent=sendto(p->rtcp_fd_out,p->rtcp_outbuffer,p->rtcp_outsize,0,&(p->rtcp_out_peer),sizeof(p->rtcp_out_peer)))<0) {
         *         fnc_log(FNC_LOG_VERBOSE,"RTCP Packet Lost\n");
         *     }
         *     p->rtcp_outsize=0;
         *     fnc_log(FNC_LOG_VERBOSE,"OUT RTCP\n");
         * } */
    }
    
    return ERR_NOERROR;
}

/**
 * Add to the read set the current rtsp sessions fds.
 * The rtsp/tcp interleaving requires additional care.
 */

void established_fd(feng *srv)
{
    RTSP_buffer *rtsp;
    for (rtsp = srv->rtsp_list; rtsp; rtsp = rtsp->next) {
        RTSP_session *q = NULL;
        RTP_session *p = NULL;
        RTSP_interleaved *intlvd;

        if (rtsp == NULL) {
            return;
        }
        // FD used for RTSP connection
        FD_SET(Sock_fd(rtsp->sock), &srv->rset);
        srv->max_fd = max(srv->max_fd, Sock_fd(rtsp->sock));
        if (rtsp->out_size > 0) {
            FD_SET(Sock_fd(rtsp->sock), &srv->wset);
        }
        // Local FDS for interleaved trasmission
        for (intlvd=rtsp->interleaved; intlvd; intlvd=intlvd->next) {
            if (intlvd->rtp_local) {
                FD_SET(Sock_fd(intlvd->rtp_local), &srv->rset);
                srv->max_fd = max(srv->max_fd, Sock_fd(intlvd->rtp_local));
            }
            if (intlvd->rtcp_local) {
                FD_SET(Sock_fd(intlvd->rtcp_local), &srv->rset);
                srv->max_fd = max(srv->max_fd, Sock_fd(intlvd->rtcp_local));
            }
        }
        // RTCP input
        for (q = rtsp->session_list, p = q ? q->rtp_session : NULL;
             p; p = p->next) {

            if (!p->started) {
            // play finished, go to ready state
                q->cur_state = READY_STATE;
                /* TODO: RTP struct to be freed */
            } else if (p->transport.rtcp_sock) {
                FD_SET(Sock_fd(p->transport.rtcp_sock), &srv->rset);
                srv->max_fd =
                    max(srv->max_fd, Sock_fd(p->transport.rtcp_sock));
            }
        }
    }
}

/**
 * Handle established connections and
 * clean up in case of unexpected disconnections
 */

void established_connections(feng *srv)
{
    int res;
    RTSP_buffer *p = srv->rtsp_list, *pp = NULL;
    RTP_session *r = NULL, *t = NULL;
    fd_set *rset = &srv->rset;
    fd_set *wset = &srv->wset;
    RTSP_interleaved *intlvd;

    while (p != NULL) {
        if ((res = rtsp_server(p, rset, wset)) != ERR_NOERROR) {
            if (res == ERR_CONNECTION_CLOSE || res == ERR_GENERIC) {
                // The connection is closed
                if (res == ERR_CONNECTION_CLOSE)
                    fnc_log(FNC_LOG_INFO,
                        "RTSP connection closed by client.");
                else
                    fnc_log(FNC_LOG_INFO,
                        "RTSP connection closed by server.");

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
                    srv->stop_schedule = 1;
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
