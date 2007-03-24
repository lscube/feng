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
 *      - Dario Gallucci        <dario.gallucci@gmail.com>
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
#include <string.h>

#include <fenice/eventloop.h>
#include <fenice/utils.h>




int rtsp_server(RTSP_buffer * rtsp, fd_set * rset, fd_set * wset, fd_set * xset)
{
    int size;
    char buffer[RTSP_BUFFERSIZE + 1];    /* +1 to control the final '\0' */
    int i, n;
    int res;
    RTSP_session *q = NULL;
    RTP_session *p = NULL;
    RTSP_interleaved *intlvd;
#ifdef HAVE_SCTP_FENICE
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
#ifdef HAVE_SCTP_FENICE
        if (Sock_type(rtsp->sock) == SCTP) {
            memset(&sctp_info, 0, sizeof(sctp_info));
            n = Sock_read(rtsp->sock, buffer, size, &sctp_info, 0);
            m = sctp_info.sinfo_stream;
            fnc_log(FNC_LOG_DEBUG,
                "Sock_read() received %d bytes from sctp stream %d\n", n, m);
        } else {    // RTSP protocol is TCP
#endif    // HAVE_SCTP_FENICE
            n = Sock_read(rtsp->sock, buffer, size, NULL, 0);
#ifdef HAVE_SCTP_FENICE
        }
#endif    // HAVE_SCTP_FENICE
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
#ifdef HAVE_SCTP_FENICE
            || (Sock_type(rtsp->sock) == SCTP&& m == 0)
#endif    // HAVE_SCTP_FENICE 
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
                return ERR_NOERROR;
            }
        } else {    /* if (rtsp->proto == SCTP && m != 0) */
#ifdef HAVE_SCTP_FENICE
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
#endif    // HAVE_SCTP_FENICE
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
                    *((uint16 *) & rtsp->out_buffer[i + 2]) = htons((uint16) n);
                    rtsp->out_size += n + 4;
                    memcpy(rtsp->out_buffer + i + 4, buffer, n);
                    if ( (n = RTSP_send(rtsp)) < 0) {
                        send_reply(500, NULL, rtsp);
                        return ERR_GENERIC;// internal server error
                    }
                }
                break;
#ifdef HAVE_SCTP_FENICE
            case SCTP:
                memcpy(&sctp_info, &(intlvd->proto.sctp.rtcp), sizeof(struct sctp_sndrcvinfo));
                Sock_write(rtsp->sock, buffer, n, &sctp_info, MSG_DONTWAIT | MSG_EOR | MSG_NOSIGNAL);
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
                    *((uint16 *) & rtsp->out_buffer[i + 2]) = htons((uint16) n);
                    rtsp->out_size += n + 4;
                    memcpy(rtsp->out_buffer + i + 4, buffer, n);
                    if ( (n = RTSP_send(rtsp)) < 0) {
                        send_reply(500, NULL, rtsp);
                        return ERR_GENERIC;// internal server error
                    }
                }
            break;
#ifdef HAVE_SCTP_FENICE
            case SCTP:
                memcpy(&sctp_info, &(intlvd->proto.sctp.rtp), sizeof(struct sctp_sndrcvinfo));
                Sock_write(rtsp->sock, buffer, n, &sctp_info, MSG_DONTWAIT | MSG_EOR | MSG_NOSIGNAL);
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
    
#ifdef POLLED
    schedule_do(0);
#endif
    return ERR_NOERROR;
}
