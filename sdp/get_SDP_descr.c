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
#include <fenice/debug.h>
#include <fenice/sdp.h>
#include <fenice/mediainfo.h>
#include <fenice/mpeg4es.h>
#include <fenice/utils.h>
#include <fenice/rtp.h>
#include <fenice/multicast.h>

int get_SDP_descr(media_entry *media,char *descr,int extended,char *url)
{	
	char s[30],t[255],app[80];
	port_pair pair;
	media_entry *p,*list,req;
	SD_descr *matching_descr;
	char ttl[4];
		
	int res;
	
	if((res=enum_media(url,&matching_descr))!=ERR_NOERROR)
		return res;
	list=matching_descr->me_list;
	memset(&req,0,sizeof(req));
	req.description.flags|=MED_PRIORITY;
	req.description.priority=1;
	p=search_media(&req,list);
	
	strcpy(descr, "v=0"SDP_EL);		
   	strcat(descr, "o=");
   	strcat(descr, get_SDP_user_name(s));
   	strcat(descr," ");
   	strcat(descr, get_SDP_session_id(s));
   	strcat(descr," ");
   	strcat(descr, get_SDP_version(s));
   	strcat(descr, SDP_EL);
   	strcat(descr, "c=");
   	strcat(descr, "IN ");		/* Network type: Internet. */
   	strcat(descr, "IP4 ");		/* Address type: IP4. */

	if(matching_descr->flags & SD_FL_MULTICAST){
   		strcat(descr, matching_descr->multicast);
		strcat(descr,"/");
		sprintf(ttl, "%d", (int)DEFAULT_TTL);
		strcat(descr, ttl); /*TODO: the possibility to change ttl. See multicast.h, RTSP_setup.c, send_setup_reply.c*/
	}
	else
   		strcat(descr, get_address());
   	
	strcat(descr, SDP_EL);
   	strcat(descr, "s=RTSP Session"SDP_EL);
	// sprintf(descr, "%si=%s %s Streaming Server"SDP_EL, descr, PACKAGE, VERSION);
	sprintf(descr + strlen(descr), "i=%s %s Streaming Server"SDP_EL, PACKAGE, VERSION);

	if (p==NULL) 
		return ERR_PARSE;
	
   	if (p->flags & ME_AGGREGATE) {
   		sprintf(descr + strlen(descr), "a=control:%s!%s"SDP_EL, url, p->aggregate);
   	}
   	sprintf(descr + strlen(descr), "u=%s"SDP_EL, url);
   	strcat(descr, "t=0 0"SDP_EL);	
   	// media specific
	p=list;
	while (p!=NULL) {
		p->reserved=1;
		p->flags|=ME_RESERVED;
		p=p->next;
	}
   	if (extended) {
		p=list;
   	}
	else {
	   	p=media;
	}
   	do {   	
   		if (p->description.priority == 1) {
			strcat(descr,"m=");
		}
		else {
			if (extended) {
				strcat(descr,"a=med:");
			}
			else {
				strcat(descr,"m=");
			}
		}
	   	switch (p->description.mtype) {
	   		case audio: {
	   			strcat(descr,"audio ");
	   			break;
	   		}
	   		case video: {
	   			strcat(descr,"video ");
	   			break;
	   		}   	
		   	case application: {
				strcat(descr,"application ");
				break;
		   	}	   		
		   	case data: {
				strcat(descr,"data ");
				break;
		   	}	   		
		   	case control: {
				strcat(descr,"control ");
				break;
		   	}	   			   	
	   	}
		   	
		if(matching_descr->flags & SD_FL_MULTICAST_PORT)
			pair.RTP=p->rtp_multicast_port;
		else
			pair.RTP=0;

	   	sprintf(t, "%d", pair.RTP);
	   	strcat(descr,t);
		
		if(matching_descr->flags & SD_FL_MULTICAST_PORT)
	   		pair.RTP+=2;

		strcat(descr," RTP/AVP "); // Use UDP
	   	sprintf(t, "%d"SDP_EL, p->description.payload_type);
	   	strcat(descr,t);

		/*start CC*/
		if(p->description.flags & MED_LICENSE){
		     strcat(descr,"a=uriLicense:");
		     strcat(descr,p->description.commons_dead);
		     strcat(descr, SDP_EL);
		 }
		 if(p->description.flags & MED_RDF_PAGE){
		     strcat(descr,"a=uriMetadata:");
		     strcat(descr,p->description.rdf_page);
		     strcat(descr, SDP_EL);
		 } 
		if(p->description.flags & MED_TITLE){
		     strcat(descr,"a=title:");
		     sprintf(app, "%s", p->description.title);
		     strcat(descr,app);
		     strcat(descr, SDP_EL);		 
		     } 
		 if(p->description.flags & MED_CREATOR){
		     strcat(descr,"a=creator:");
		     sprintf(app, "%s", p->description.author);
		     strcat(descr,app);
		     strcat(descr, SDP_EL);
		 } 
		/*end CC*/

	   	if (p->description.payload_type>=96) {
	   		// Dynamically defined payload
			strcat(descr,"a=rtpmap:");
			sprintf(t, "%d", p->description.payload_type);
			strcat(descr,t);
			strcat(descr," ");			
			//strcat(descr,p->description.dyn_payload_token);
			strcat(descr,p->description.encoding_name);
			strcat(descr,"/");
			sprintf(t, "%u", p->description.clock_rate);
			strcat(descr,t);
			if (p->description.flags & MED_AUDIO_CHANNELS) {
				strcat(descr,"/");
				sprintf(t, "%d", p->description.audio_channels);
				strcat(descr,t);
			}
			if (strcmp(p->description.encoding_name,"MP4V-ES")==0) {
				//static_MPEG4_video_es *s=(static_MPEG4_video_es *)p->stat;
				strcat(descr, SDP_EL);
				sprintf(t, "a=fmtp:96");
				//sprintf(t, "a=fmtp:96 profile-level-id=%d config=%s"/*SDP_EL*/, s->profile_id , s->config);
				strcat(descr,t);
			}
			strcat(descr, SDP_EL);
   		}
	
   		if (p->description.priority == 1) {
		   	strcat(descr,"a=control:");
		}
		else {
			if (extended) {
				strcat(descr,"a=ctrl:");
			}
			else {
				strcat(descr,"a=control:");
			}
		}   		
		strcat(descr,url); 
		strcat(descr,"!");
   		strcat(descr,p->filename);
   		strcat(descr, SDP_EL);
   		if (p->flags & ME_AGGREGATE && extended) {
   			strcat(descr,"a=aggregate:");
   			strcat(descr,url);
   			strcat(descr,"!");
   			strcat(descr,p->aggregate);
   			strcat(descr, SDP_EL);
   		}   		
	/*	
		else
   			strcat(descr, SDP_EL);
	*/
		
   		if (extended!=0) {
   			// We must describe ALL the media
	   		p=p->next;
	   	}
	   	else {
	   		if (p->flags & ME_AGGREGATE) {
	   			// We must describe all and only this aggregate
	   			p->reserved=0;
	   			p->flags^=ME_RESERVED;
				memset(&req,0,sizeof(req));	   				   			
	   			strcpy(req.aggregate,p->aggregate);
	   			req.flags|=ME_AGGREGATE;
	   			req.flags|=ME_RESERVED;
	   			p=search_media(&req,list);
	   		}
	   		else {	   		
	   			// Dobbiamo descrivere solo questo media
		   		p=NULL;
		   	}
	   	}
   	} while (p!=NULL);
   	return ERR_NOERROR;
}

