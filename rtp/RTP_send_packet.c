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

#if HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include <fenice/rtp.h>
#include <fenice/utils.h>

int RTP_send_packet(RTP_session *session)
{
	unsigned char *data=NULL,*packet;
	unsigned int data_size=0,hdr_size;
	RTP_header r;      // 12 bytes
	int res;
	double s_time;
	int recallme=1;
        while (recallme) 	{
		s_time = session->mtime - session->mstart + session->mstart_offset;
			//	printf("s_time e' mtime-mstart+mstart_offset = %lf\n",s_time);
		if ((res=get_frame(session->current_media,&data,&data_size,&s_time,&recallme))!=ERR_NOERROR) {
			return res;
		}
		if (strcmp(session->current_media->description.encoding_name,"MP2T")!=0) {
			session->mtime = s_time + session->mstart - session->mstart_offset;
		}
    		hdr_size=sizeof(r);	
 		//printf("\npacket sizesize=%d",data_size+hdr_size);
    		r.version = 2;
    		r.padding = 0;
    		r.extension = 0;
    		r.csrc_len = 0;
    		if ( (session->current_media->description.mtype==video) && (recallme==0) && (strcmp(session->current_media->description.encoding_name,"MP2T")!=0))	{
      			r.marker = 1;
      		} else 	{
      			r.marker = 0;
      		}
    		r.payload = session->current_media->description.payload_type;
    		r.seq_no = htons(session->seq++ + session->start_seq);
    		r.timestamp=htonl(session->start_rtptime+msec2tick(s_time,session->current_media));
    		r.ssrc = htonl(session->ssrc);
#if HAVE_ALLOCA
    		packet=(unsigned char*)alloca(data_size+hdr_size);
#else
    		packet=(unsigned char*)calloc(1,data_size+hdr_size);
#endif
    		if (packet==NULL) {
    			return ERR_ALLOC;    	
    		}
		memcpy(packet,&r,hdr_size);
		memcpy(packet+hdr_size,data,data_size);

		if (sendto(session->rtp_fd,packet,data_size+hdr_size,0,&(session->rtp_peer),sizeof(session->rtp_peer))<0) {
			#ifdef DEBUG		
				printf("RTP Packet Lost\n");
			#endif
		}
		else {
			session->rtcp_stats[i_server].pkt_count++;
			session->rtcp_stats[i_server].octet_count+=data_size;
		}
#if !HAVE_ALLOCA
		free(packet);
#endif
		free(data);
	}
	return ERR_NOERROR;
}

