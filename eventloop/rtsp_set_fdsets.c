/* * 
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
#include <string.h>

#include <fenice/eventloop.h>
#include <fenice/utils.h>
#include <fenice/rtsp.h>
#include <fenice/fnc_log.h>

void rtsp_set_fdsets(RTSP_buffer * rtsp, int * max_fd , fd_set * rset, 
	fd_set * wset, fd_set * xset) {
	RTSP_session *q = NULL;
	RTP_session *p = NULL;
	RTSP_interleaved *intlvd;

	if (rtsp == NULL) {
		return;
	}
	// FD used for RTSP connection
	FD_SET(Sock_fd(rtsp->sock), rset);
	*max_fd = max(*max_fd, Sock_fd(rtsp->sock));
	if (rtsp->out_size > 0) {
		FD_SET(Sock_fd(rtsp->sock), wset);
	}
	// Local FDS for interleaved trasmission
	for (intlvd=rtsp->interleaved; intlvd; intlvd=intlvd->next) {
		if (intlvd->rtp_local) {
			FD_SET(Sock_fd(intlvd->rtp_local), rset);
			*max_fd = max(*max_fd, Sock_fd(intlvd->rtp_local));
		}
		if (intlvd->rtcp_local) {
			FD_SET(Sock_fd(intlvd->rtcp_local), rset);
			*max_fd = max(*max_fd, Sock_fd(intlvd->rtcp_local));
		}
		
	}
	// RTCP input
	for (q = rtsp->session_list, p = q ? q->rtp_session : NULL; p; p = p->next) {

		if (!p->started) {
			q->cur_state = READY_STATE;	// �play finished, go to ready state
			/* TODO: RTP struct to be freed */
		} else if (p->transport.rtcp_sock) {
			FD_SET(Sock_fd(p->transport.rtcp_sock), rset);
			*max_fd = max(*max_fd, Sock_fd(p->transport.rtcp_sock));
		}
	}
}
