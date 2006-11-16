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

#include <string.h>

#include <fenice/rtsp.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

int RTSP_handler(RTSP_buffer * rtsp)
{
	unsigned short status;
	char msg[100];
	int m, op;
	int full_msg;
	RTSP_interleaved *intlvd;
	int hlen, blen;

	while (rtsp->in_size) {
		switch ((full_msg = RTSP_full_msg_rcvd(rtsp, &hlen, &blen))) {
		case RTSP_method_rcvd:
			op = RTSP_valid_response_msg(&status, msg, rtsp);
			if (op == 0) {
				// There is NOT an input RTSP message, therefore it's a request
				m = RTSP_validate_method(rtsp);
				if (m < 0) {
					// Bad request: non-existing method
					fnc_log(FNC_LOG_INFO, "Bad Request ");
					send_reply(400, NULL, rtsp);
				} else
					RTSP_state_machine(rtsp, m);
			} else {
				// There's a RTSP answer in input.
				if (op == ERR_GENERIC) {
					// Invalid answer
				}
			}
			RTSP_discard_msg(rtsp);
			break;
		case RTSP_interlvd_rcvd:
			m = rtsp->in_buffer[1];
			for (intlvd = rtsp->interleaved;
			     intlvd && !((intlvd->proto.tcp.rtp_ch == m)
				|| (intlvd->proto.tcp.rtcp_ch == m));
			     intlvd = intlvd->next);
			if (!intlvd) {	// session not found
				fnc_log(FNC_LOG_DEBUG,
					"Interleaved RTP or RTCP packet arrived for unknown channel (%d)... discarding.\n",
					m);
				RTSP_discard_msg(rtsp);
				break;
			}
			if (m == intlvd->proto.tcp.rtcp_ch) {	// RTCP pkt arrived
				fnc_log(FNC_LOG_DEBUG,
					"Interleaved RTCP packet arrived for channel %d (len: %d).\n",
					m, blen);
				Sock_write(intlvd->rtcp_local, &rtsp->in_buffer[hlen],
					  blen, NULL, 0);
			} else	// RTP pkt arrived: do nothing...
				fnc_log(FNC_LOG_DEBUG,
					"Interleaved RTP packet arrived for channel %d.\n",
					m);
			RTSP_discard_msg(rtsp);
			break;
		default:
			return full_msg;
			break;
		}
	}

	return ERR_NOERROR;
}
