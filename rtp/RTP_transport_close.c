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

 #include <fenice/rtp.h>
 
 int RTP_transport_close(RTP_session * session)
 {
	switch (session->transport.type) {
	case RTP_rtp_avp:
		if (session->transport.u.udp.is_multicast) {
			struct ip_mreq mreq;
			mreq.imr_multiaddr.s_addr =
					inet_addr(session->sd_descr->multicast);
			mreq.imr_interface.s_addr = INADDR_ANY;
			setsockopt(session->transport.rtp_fd, IPPROTO_IP,
				   IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
			session->sd_descr->flags &= ~SD_FL_MULTICAST_PORT;	/*Release SD_FL_MULTICAST_PORT */
		}
		// release ports
		RTP_release_port_pair(&(session->transport.u.udp.ser_ports));
		// don't break
	case RTP_rtp_avp_tcp:
		if (session->transport.rtp_fd >= 0) {
			close(session->transport.rtp_fd);
		}
		if (session->transport.rtcp_fd_in >= 0) {
			close(session->transport.rtcp_fd_in);
		}
		if (session->transport.rtcp_fd_out >= 0) {
			close(session->transport.rtcp_fd_out);
		}
		// don't break
	case RTP_rtp_avp_sctp:
		session->transport.rtp_fd = -1;
		session->transport.rtcp_fd_in = -1;
		session->transport.rtcp_fd_out = -1;
		break;
	default:
		break;
	}
 	
 	return 0;
 }
