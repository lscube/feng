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

#include <config.h>

/*#if HAVE_ALLOCA_H
#include <alloca.h>
#endif*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fenice/rtp.h>
#include <fenice/utils.h>
#include <fenice/bufferpool.h>

RTP_session *RTP_session_destroy(RTP_session *session)
{
	RTP_session *next = session->next;
	OMSBuffer *buff = session->current_media->pkt_buffer;
	//struct stat fdstat;

	//Release SD_flag using in multicast and unjoing the multicast group
	if(session->sd_descr->flags & SD_FL_MULTICAST){
		struct ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = inet_addr(session->sd_descr->multicast);
		mreq.imr_interface.s_addr = INADDR_ANY;
		setsockopt(session->rtp_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
		session->sd_descr->flags &= ~SD_FL_MULTICAST_PORT; /*Release SD_FL_MULTICAST_PORT*/
	}

	close(session->rtp_fd);
	close(session->rtcp_fd_in);
	close(session->rtcp_fd_out);
	// Release ports
	RTP_release_port_pair(&(session->ser_ports));
	// destroy consumer
	OMSbuff_unref(session->cons);
	if (session->current_media->pkt_buffer->refs==0) {
			session->current_media->pkt_buffer=NULL;
			OMSbuff_free(buff);
			// close file if it's not a pipe
			//fstat(session->current_media->fd, &fdstat);
			//if ( !S_ISFIFO(fdstat.st_mode) )
				mediaclose(session->current_media);
	}
	// Deallocate memory
	free(session);

	return next;
}

