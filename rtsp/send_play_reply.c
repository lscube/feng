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

#include <stdio.h>
#include <string.h>

#include <config.h>
#include <fenice/rtsp.h>
#include <fenice/utils.h>

int send_play_reply(RTSP_buffer *rtsp,char *object,RTSP_session *rtsp_session)
{	
    char r[1024];
    char temp[30];
    RTP_session *p=rtsp_session->rtp_session;
	/* build a reply message */
	sprintf(r, "%s %d %s\nCSeq: %d\nServer: %s/%s\n", RTSP_VER, 200, get_stat(200),rtsp->rtsp_cseq,PACKAGE,VERSION);
	add_time_stamp(r, 0);
	strcat(r,"Session: ");
	sprintf(temp,"%d",rtsp_session->session_id);
	strcat(r,temp);
	strcat(r,"\n");
	strcat(r,"RTP-info: url=");
	strcat(r,object);
	strcat(r,";");
	do {
		sprintf(r+strlen(r),"seq=%d;rtptime=%d",p->start_seq,p->start_rtptime);
		if (p->next!=NULL) {
			strcat(r,",");
		}
		else {
			strcat(r,"\r\n\r\n");
		}
		p=p->next;
	} while (p!=NULL);
	bwrite(r, (unsigned short) strlen(r),rtsp);
	
	#ifdef VERBOSE
	printf("PLAY response sent.\n");	
	#endif
	return ERR_NOERROR;
}

