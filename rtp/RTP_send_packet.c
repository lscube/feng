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
#include <fenice/fnc_log.h>

#if ENABLE_DUMP
#include <fenice/debug.h>
#endif

int RTP_send_packet(RTP_session * session)
{
	unsigned char *packet = NULL;
	unsigned int hdr_size = 0;
	RTP_header r;		// 12 bytes
	int res = ERR_GENERIC;
	double s_time;
	double nextts;
	OMSSlot *slot = NULL;
	ssize_t psize_sent = 0;

	if (!(slot = OMSbuff_getreader(session->cons))) {
		//This operation runs only if producer writes the slot
		//session->current_media->mtime += session->current_media->description.delta_mtime; //emma  
		//session->current_media->mtime+=session->current_media->description.pkt_len;     // old scheduler
		s_time =
		    session->current_media->mtime -
		    session->current_media->mstart +
		    session->current_media->mstart_offset;
		if ((res =
		     get_frame(session->current_media,
			       &s_time)) != ERR_NOERROR) {
//                      fprintf(stderr,"Some errors occurred\n");
			return res;
		}
//              session->current_media->mtime += session->current_media->description.delta_mtime; //emma  
		session->cons->frames++;
		slot = OMSbuff_getreader(session->cons);
	} else			/*This runs if the consumer reads slot written in another RTP session */
		s_time = slot->timestamp - session->cons->firstts;
#if 0
	if (s_time > session->current_media->mtime) {	// time for pkt still not arrived
		session->current_media->mtime = s_time;
		return ERR_NOERROR;
	}
#endif

	while (slot) {
		/*if (strcmp(session->current_media->description.encoding_name,"MP2T")!=0) {
		   //session->current_media->mtime = slot->timestamp + session->current_media->mstart - session->current_media->mstart_offset;
		   } */

		hdr_size = sizeof(r);
		r.version = 2;
		r.padding = 0;
		r.extension = 0;
		r.csrc_len = 0;
		r.marker = slot->marker;
		r.payload = session->current_media->description.payload_type;
//              r.seq_no = htons(session->seq++ + session->start_seq);
		r.seq_no = htons(slot->slot_seq + session->start_seq - 1);
		r.timestamp =
		    htonl(session->start_rtptime +
			  msec2tick(slot->timestamp,
				    session->current_media) -
			  session->cons->firstts);
		r.ssrc = htonl(session->ssrc);
#if HAVE_ALLOCA
		packet = (unsigned char *) alloca(slot->data_size + hdr_size);
#else
		packet =
		    (unsigned char *) calloc(1, slot->data_size + hdr_size);
#endif
		if (packet == NULL) {
			return ERR_ALLOC;
		}
		memcpy(packet, &r, hdr_size);
		memcpy(packet + hdr_size, slot->data, slot->data_size);

		// fnc_log(FNC_LOG_DEBUG, "sending pkt... seq: %llu (RTPseq:%u)\n", slot->slot_seq, ntohs(r.seq_no));
		// if ((psize_sent=sendto(session->transport.u.udp.rtp_fd,packet,slot->data_size+hdr_size,0,&(session->transport.u.udp.rtp_peer),sizeof(session->transport.u.udp.rtp_peer)))<0){
		if ((psize_sent =
		     Sock_write(session->transport.rtp_sock, packet,
				slot->data_size + hdr_size, NULL, MSG_DONTWAIT
				| MSG_EOR)) < 0) {

			fnc_log(FNC_LOG_DEBUG, "RTP Packet Lost\n");
		} else {
#if ENABLE_DUMP
			char fname[255];
			char crtp[255];
			memset(fname, 0, sizeof(fname));
			strcpy(fname, "dump_fenice.");
			strcat(fname,
			       session->current_media->description.
			       encoding_name);
			strcat(fname, ".");
			sprintf(crtp, "%d", session->transport.rtp_fd);
			strcat(fname, crtp);
			if (strcmp
			    (session->current_media->description.encoding_name,
			     "MPV") == 0
			    || strcmp(session->current_media->description.
				      encoding_name, "MPA") == 0)
				dump_payload(packet + 16, psize_sent - 16,
					     fname);
			else
				dump_payload(packet + 12, psize_sent - 12,
					     fname);
#endif
			session->rtcp_stats[i_server].pkt_count++;
			session->rtcp_stats[i_server].octet_count +=
			    slot->data_size;
		}
#if !HAVE_ALLOCA
		free(packet);
#endif
		OMSbuff_gotreader(session->cons);

		if ((nextts = OMSbuff_nextts(session->cons)) >= 0)
			nextts -= session->cons->firstts;
		// fnc_log(FNC_LOG_DEBUG, "*** current time=%f - next time=%f\n\n", s_time, nextts);
		if ((nextts == -1) || (nextts != s_time)) {
			// fnc_log(FNC_LOG_DEBUG, "*** time on\n");
			if (session->current_media->description.delta_mtime)
				session->current_media->mtime += session->current_media->description.delta_mtime;	//emma
//                      else if (nextts > 0)
//                              session->current_media->mtime = nextts;
//                              session->current_media->mtime = s_time;
			else
				session->current_media->mtime +=
				    session->current_media->description.pkt_len;
			slot = NULL;
			session->cons->frames--;
		} else
			slot = OMSbuff_getreader(session->cons);

	}

	return ERR_NOERROR;
}
