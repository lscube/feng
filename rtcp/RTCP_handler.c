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

#include <fenice/eventloop.h>
#include <fenice/rtcp.h>
#include <fenice/rtp.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

int RTCP_handler(RTP_session * session)
{
	fd_set wset;
	struct timeval t;

	if (session->rtcp_stats[i_server].pkt_count % 20 == 0) {
		if (session->rtcp_stats[i_server].pkt_count == 0)
			RTCP_send_packet(session, RR);
		else
			RTCP_send_packet(session, SR);
		RTCP_send_packet(session, SDES);
		/*---------------SEND PKT-------------------------*/
		/*---------------SEE eventloop/rtsp_server.c-------*/
		FD_ZERO(&wset);
		t.tv_sec = 0;
		t.tv_usec = 100000;

		if (session->rtcp_outsize > 0)
			FD_SET(Sock_fd(session->transport.rtcp_sock), &wset);
		if (select(Sock_fd(session->transport.rtcp_sock) + 1, 0, &wset, 0, &t) < 0) {
			fnc_log(FNC_LOG_ERR, "select error\n");
			/*send_reply(500, NULL, rtsp); */
			return ERR_GENERIC;	//errore interno al server
		}

		if (FD_ISSET(Sock_fd(session->transport.rtcp_sock), &wset)) {
			if (Sock_write(session->transport.rtcp_sock, session->rtcp_outbuffer,
			     session->rtcp_outsize, NULL) < 0)
				fnc_log(FNC_LOG_VERBOSE, "RTCP Packet Lost\n");
			session->rtcp_outsize = 0;
			fnc_log(FNC_LOG_VERBOSE, "OUT RTCP\n");
		}
		/*------------------------------------------------*/
	}
	return ERR_NOERROR;
}
