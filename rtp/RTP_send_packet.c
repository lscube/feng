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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>

#include <fenice/rtp.h>
#include <fenice/utils.h>
#include <fenice/bufferpool.h>

int RTP_send_packet(RTP_session *session)
{
	unsigned char *packet;
	unsigned int hdr_size;
	RTP_header r;      // 12 bytes
	int res;
	double s_time;
	OMSSlot *slot;
	
//	s_time = session->mtime - session->mstart + session->mstart_offset;
	if(!(slot = OMSbuff_read(session->cons))){
		//This operation runs only if producer writes the slot
		session->current_media->mtime += session->current_media->description.delta_mtime; //emma  
		
		//session->current_media->mtime+=session->current_media->description.pkt_len;     // old scheduler
		
		
		s_time = session->current_media->mtime - session->current_media->mstart + session->current_media->mstart_offset;
		if ((res=get_frame(session->current_media,&s_time))!=ERR_NOERROR){
			/* if(res==ERR_EOF)
				fprintf(stderr,"Just Finished!");
			*/
			return res;
		}
		slot=OMSbuff_read(session->cons);
	} else /*This runs if the consumer reads slot written in another RTP session*/
		s_time=slot->timestamp;
		
	while ((slot) && (slot->timestamp == s_time)) {
		//fprintf(stderr,"s_time=%f\n",s_time);
		if (strcmp(session->current_media->description.encoding_name,"MP2T")!=0) {
			//session->current_media->mtime = slot->timestamp + session->current_media->mstart - session->current_media->mstart_offset;
		}
		
		fprintf(stderr,"slot->timestamp=%f play_offset=%f %s \n",slot->timestamp,session->current_media->play_offset,(strcmp(session->current_media->description.encoding_name,"MPA")==0)?"audio":"video");
		
    		hdr_size=sizeof(r);	
		r.version = 2;
    		r.padding = 0;
		r.extension = 0;
   		r.csrc_len = 0;
		r.marker=slot->marker;
    		r.payload = session->current_media->description.payload_type;
		r.seq_no = htons(session->seq++ + session->start_seq);
   		r.timestamp=htonl(session->start_rtptime+msec2tick(slot->timestamp,session->current_media));
   		// r.timestamp=htonl(session->start_rtptime+msec2tick(s_time,session->current_media));
		r.ssrc = htonl(session->ssrc);
#if HAVE_ALLOCA
		packet=(unsigned char*)alloca(slot->data_size+hdr_size);
#else
    		packet=(unsigned char*)calloc(1,slot->data_size+hdr_size);
#endif
    		if (packet==NULL) {
    			return ERR_ALLOC;    	
    		}
		memcpy(packet,&r,hdr_size);
		memcpy(packet+hdr_size,slot->data,slot->data_size);
	
		if (sendto(session->rtp_fd,packet,slot->data_size+hdr_size,0,&(session->rtp_peer),sizeof(session->rtp_peer))<0){
#ifdef DEBUG		
			fprintf(stderr,"RTP Packet Lost\n");
#endif
		}	
		else {
			session->rtcp_stats[i_server].pkt_count++;
			session->rtcp_stats[i_server].octet_count+=slot->data_size;
		}
#if !HAVE_ALLOCA
		free(packet);
#endif
		// free(data);
		slot = OMSbuff_read(session->cons);

	}
	
	return ERR_NOERROR;
}

