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
#include <fenice/rtsp.h>
#include <fenice/fnc_log.h>
#include <netembryo/wsocket.h>

int rtsp_server(RTSP_buffer * rtsp, fd_set * rset, fd_set * wset, fd_set * xset)
{
	int size;
	char buffer[RTSP_BUFFERSIZE + 1];	/* +1 to control the final '\0' */
	int n;
	int res;
	RTSP_session *q = NULL;
	RTP_session *p = NULL;
	RTSP_interleaved *intlvd;
	uint16 *pkt_size = (uint16 *) & rtsp->out_buffer[2];
#ifdef HAVE_SCTP_FENICE
	struct sctp_sndrcvinfo sctp_info;
	int flags, m = 0;
#endif

	if (rtsp == NULL) {
		return ERR_NOERROR;
	}
	if (FD_ISSET(rtsp->fd, wset)) { // first of all: there is some data to send?
		//char is_interlvd = rtsp->interleaved_size ? 1 : 0; 
		// There are RTSP packets to send
		if ( (n = RTSP_send(rtsp)) < 0) {
			send_reply(500, NULL, rtsp);
			return ERR_GENERIC;// internal server error
		}
/*#ifdef VERBOSE
		 else if (!is_interlvd) {
			fnc_log(FNC_LOG_VERBOSE, "OUTPUT_BUFFER was:\n");
			dump_buffer(rtsp->out_buffer);
		}
#endif*/
	}
	if (FD_ISSET(rtsp->fd, rset)) {
		// There are RTSP or RTCP packets to read in
		memset(buffer, 0, sizeof(buffer));
		size = sizeof(buffer) - 1;
#ifdef HAVE_SCTP_FENICE
		if (rtsp->proto == SCTP) {
			memset(&sctp_info, 0, sizeof(sctp_info));
			flags = 0;
			n = sctp_recvmsg(rtsp->fd, buffer, size, NULL, NULL,
					 &sctp_info, &flags);
			m = sctp_info.sinfo_stream;
		} else {	// RTSP protocol is TCP
#endif	// HAVE_SCTP_FENICE
			n = tcp_read(rtsp->fd, buffer, size);
#ifdef HAVE_SCTP_FENICE
		}
#endif	// HAVE_SCTP_FENICE
		if (n == 0) {
			return ERR_CONNECTION_CLOSE;
		}
		if (n < 0) {
			fnc_log(FNC_LOG_DEBUG,
				"sctp_recvmsg() or tcp_read() error in rtsp_server()\n");
			send_reply(500, NULL, rtsp);
			return ERR_GENERIC;	//errore interno al server                           
		}
#ifdef HAVE_SCTP_FENICE
		if (rtsp->proto == TCP || (rtsp->proto == SCTP && m == 0)) {
#endif	// HAVE_SCTP_FENICE
			if (rtsp->in_size + n > RTSP_BUFFERSIZE) {
				fnc_log(FNC_LOG_DEBUG,
					"RTSP buffer overflow (input RTSP message is most likely invalid).\n");
				send_reply(500, NULL, rtsp);
				return ERR_GENERIC;	//errore da comunicare
			}
#ifdef VERBOSE
			fnc_log(FNC_LOG_VERBOSE, "INPUT_BUFFER was:\n");
			dump_buffer(buffer);
#endif
			memcpy(&(rtsp->in_buffer[rtsp->in_size]), buffer, n);
			rtsp->in_size += n;
			//TODO: SCTP is packet aware! Change behaviour to reflect flags == MSG_EOR or not
			if ((res = RTSP_handler(rtsp)) == ERR_GENERIC) {
				fnc_log(FNC_LOG_ERR,
					"Invalid input message.\n");
				return ERR_NOERROR;
			}
#ifdef HAVE_SCTP_FENICE
		} else {	/* if (rtsp->proto == SCTP && m != 0) */

			for (p =
			     (rtsp->session_list) ? rtsp->session_list->
			     rtp_session : NULL;
			     p && ((p->transport.u.sctp.streams.RTP == m)
				   || (p->transport.u.sctp.streams.RTCP == m));
			     p = p->next)
				if (p) {
					if (m ==
					    p->transport.u.sctp.streams.RTCP) {
						if (sizeof(p->rtcp_inbuffer) >
						    (unsigned)n) {
							memcpy(p->rtcp_inbuffer,
							       buffer, n);
							p->rtcp_insize = n;
						} else
							fnc_log(FNC_LOG_DEBUG,
								"Interleaved RTCP packet too big!.\n");
						RTCP_recv_packet(p);
					} else {	// RTP pkt arrived: do nothing...
						fnc_log(FNC_LOG_DEBUG,
							"Interleaved RTP packet arrived for channel %d.\n",
							m);
					}
				}
		}
#endif	// HAVE_SCTP_FENICE
	}
	for (intlvd=rtsp->interleaved; intlvd && !rtsp->out_size; intlvd=intlvd->next) {
		if ( FD_ISSET(intlvd->rtcp_fd, rset) ) {
			if ( (n = read(intlvd->rtcp_fd, rtsp->out_buffer+4, sizeof(rtsp->out_buffer)-4)) < 0) {
			// if ( (intlvd->out_size = read(intlvd->rtcp_fd, intlvd->out_buffer, sizeof(intlvd->out_buffer))) < 0) {
				fnc_log(FNC_LOG_ERR, "Error reading from local socket\n");
				continue;
			}
			
			rtsp->out_buffer[0] = '$';
			rtsp->out_buffer[1] = (unsigned char) intlvd->proto.tcp.rtcp_ch;
			*pkt_size = htons((uint16) n);
			rtsp->out_size = n+4;
			if ( (n = RTSP_send(rtsp)) < 0) {
				send_reply(500, NULL, rtsp);
				return ERR_GENERIC;// internal server error
			}
		} else if ( FD_ISSET(intlvd->rtp_fd, rset) ) {
			if ( (n = read(intlvd->rtp_fd, rtsp->out_buffer+4, sizeof(rtsp->out_buffer)-4)) < 0) {
			// if ( (n = read(intlvd->rtp_fd, intlvd->out_buffer, sizeof(intlvd->out_buffer))) < 0) {
				fnc_log(FNC_LOG_ERR, "Error reading from local socket\n");
				continue;
			}
			rtsp->out_buffer[0] = '$';
			rtsp->out_buffer[1] = (unsigned char) intlvd->proto.tcp.rtp_ch;
			*pkt_size = htons((uint16) n);
			rtsp->out_size = n+4;
			if ( (n = RTSP_send(rtsp)) < 0) {
				send_reply(500, NULL, rtsp);
				return ERR_GENERIC;// internal server error
			}
		}
	}
	for (q = rtsp->session_list, p = q ? q->rtp_session : NULL; p; p = p->next) {
		if ( (p->transport.rtcp_fd_in >= 0) && 
				FD_ISSET(p->transport.rtcp_fd_in, rset)) {
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
		 * 	// There are RTCP packets to send
		 * 	if ((psize_sent=sendto(p->rtcp_fd_out,p->rtcp_outbuffer,p->rtcp_outsize,0,&(p->rtcp_out_peer),sizeof(p->rtcp_out_peer)))<0) {
		 * 		fnc_log(FNC_LOG_VERBOSE,"RTCP Packet Lost\n");
		 * 	}
		 * 	p->rtcp_outsize=0;
		 * 	fnc_log(FNC_LOG_VERBOSE,"OUT RTCP\n");
		 * } */
	}
	
#ifdef POLLED
	schedule_do(0);
#endif
	return ERR_NOERROR;
}
