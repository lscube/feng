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
#include <sys/time.h> // shawill: for gettimeofday
#include <stdlib.h> // shawill: for rand, srand
#include <unistd.h> // shawill: for close
// shawill: for inet_aton
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fenice/rtsp.h>
#include <fenice/socket.h>
#include <fenice/utils.h>
#include <fenice/prefs.h>
#include <fenice/multicast.h>
#include <fenice/debug.h>

/*
 	****************************************************************
 	*			SETUP METHOD HANDLING
 	****************************************************************
*/

int RTSP_setup(RTSP_buffer * rtsp, RTSP_session ** new_session)
{
	char address[16];
	int valid_url;
	char object[255], server[255], trash[255];
	char url[255];
	unsigned short port;
	RTSP_session *sp;
	RTP_session *sp2, *sp2_prec;
	int SessionID = 0;
	port_pair cli_ports;
	port_pair ser_ports;
	struct timeval now_tmp;
	char *p = NULL, *q = NULL;
	int start_seq, start_rtptime;
	char line[255];
	media_entry *list, *matching_me, req;
	struct sockaddr rtsp_peer;
	int namelen = sizeof(struct sockaddr);
	unsigned long ssrc;
	int multicast=NO_MULTICAST;

	
	fprintf(stderr,"SETUP request received.\n");
	
	memset(&req, 0, sizeof(req));
	// Parse the input message
	// Get the CSeq 


	if ((p = strstr(rtsp->in_buffer, HDR_CSEQ)) == NULL) {
		printf("SETUP request didn't specify a CSeq header.\n");
		send_reply(400, 0, rtsp);	/* Bad Request */
		return ERR_NOERROR;
	} else {
		if (sscanf(p, "%254s %d", trash, &(rtsp->rtsp_cseq)) != 2) {
			printf("SETUP request didn't specify a CSeq number.\n");
			send_reply(400, 0, rtsp);	/* Bad Request */
			return ERR_NOERROR;
		}
	}
	if ((p = strstr(rtsp->in_buffer, "ssrc")) != NULL) {
		p = strchr(p, '=');
		sscanf(p + 1, "%lu", &ssrc);
	} else {
		ssrc = random32(0); 
#if DEBUG
		fprintf(stderr,"%lu",ssrc);
#endif
	}

	if ((p = strstr(rtsp->in_buffer, "client_port")) == NULL && (strstr(rtsp->in_buffer, "multicast")) == NULL) {
		printf("SETUP request didn't specify client ports\n");
		send_reply(406, "Require: Transport settings of rtp/udp;port=nnnn.\n", rtsp);	/* Not Acceptable */
		return ERR_NOERROR;
	}
	// Start parsing the Transport header
	if ((p = strstr(rtsp->in_buffer, HDR_TRANSPORT)) == NULL) {
		printf("SETUP request didn't specify Transport header.\n");
		send_reply(406, "Require: Transport settings of rtp/udp;port=nnnn.\n", rtsp);	/* Not Acceptable */
		return ERR_NOERROR;
	}
	if (sscanf(p, "%10s%255s", trash, line) != 2) {
		printf("SETUP request malformed\n");
		send_reply(400, 0, rtsp);	/* Bad Request */
		return ERR_NOERROR;
	}

      	//BEGIN FEDERICO	
     	if ((strstr(rtsp->in_buffer, "multicast")) != NULL){ 
		multicast=YES_MULTICAST;
		p = strstr(line, "port");
      	} else 
		p = strstr(line, "client_port");
	
		p = strstr(p, "=");
		sscanf(p + 1, "%d", &(cli_ports.RTP));
		p = strstr(p, "-");
		sscanf(p + 1, "%d", &(cli_ports.RTCP));
	 
       	//END FEDERICO
       
	/* Get the URL */
	if (!sscanf(rtsp->in_buffer, " %*s %254s ", url)) {
		printf("SETUP request is missing object (path/file) parameter.\n");
		send_reply(400, 0, rtsp);	/* bad request */
		return ERR_NOERROR;
	}
	/* Validate the URL */
	if (!parse_url(url, server, &port, object)) { //object é il nome del file richiesto
		printf("Mangled URL in SETUP.\n");
		send_reply(400, 0, rtsp);	/* bad request */
		return ERR_NOERROR;
	}
	if (strcmp(server, prefs_get_hostname()) != 0) {	/* Currently this feature is disabled. */
		/* wrong server name */
		//      printf("SETUP request specified an unknown server name.\n");
		//      send_reply(404, 0 , rtsp); /* Not Found */
		//      return ERR_NOERROR;
	}
	if (strstr(object, "../")) {
		/* disallow relative paths outside of current directory. */
		printf
		    ("SETUP request specified an object parameter with a path that is not allowed. '../' not permitted in path.\n");
		send_reply(403, 0, rtsp);	/* Forbidden */
		return ERR_NOERROR;
	}
	if (strstr(object, "./")) {
		/* Disallow the ./ */
		printf
		    ("SETUP request specified an object parameter with a path that is not allowed. './' not permitted in path.\n");
		send_reply(403, 0, rtsp);	/* Forbidden */
		return ERR_NOERROR;
	}
	p = strrchr(object, '.');
	valid_url = 0;
	if (p == NULL) { //se file senza estensione
		printf("SETUP request specified an object (path/file) parameter that is not valid.\n");
		send_reply(415, 0, rtsp);	/* Unsupported media type */
		return ERR_NOERROR;
	} else {
		valid_url = is_supported_url(p); 
	}
	if (!valid_url) {//se l'estensione non é valida
		printf("SETUP request specified an unsupported media type.\n");
		send_reply(415, 0, rtsp);	/* Unsupported media type */
		return ERR_NOERROR;
	}
	q = strchr(object, '!');
	if (q == NULL) { //se non c'é "!" non é stato specificato il file che si vuole in streaming (mp3,mpg...)
		printf("SETUP request specified an object that can't be understood.\n");
		send_reply(500, 0, rtsp);	/* Internal server error */
		return ERR_NOERROR;
	} else {
		// SETUP name.sd!stream
		strcpy(req.filename, q + 1);
		req.flags |= ME_FILENAME;
		
		*q = '\0';
	}
// ------------ START PATCH
	{
		char temp[255];
		char *pd;
		strcpy(temp, object);
		p = strstr(temp, "/");
		// BEGIN FEDERICO
		if (p != NULL) {
			strcpy(object, p + 1);	// CRITIC. 
		}
		pd=strstr(object,".sd"); // this part is usefull in order to
		p=strstr(pd+1,".sd");    // have compatibility with
		if (p != NULL) {         // RealOne
			strcpy(object, pd + 3);	// CRITIC. 
		}                     //Note: It's a critic part
		// END FEDERICO
	}
// ------------ END PATCH
	

	if (enum_media(object, &list) != ERR_NOERROR) {
		printf("SETUP request specified an object file which can be damaged.\n");
		send_reply(500, 0, rtsp);	/* Internal server error */
		return ERR_NOERROR;
	}
	if (get_media_entry(&req, list, &matching_me) == ERR_NOT_FOUND) {
		printf("SETUP request specified an object which can't be found.\n");
		send_reply(404, 0, rtsp);	/* Not found */
		return ERR_NOERROR;
	}

	// If there's a Session header we have an aggregate control
	if ((p = strstr(rtsp->in_buffer, HDR_SESSION)) != NULL) {
		if (sscanf(p, "%254s %d", trash, &SessionID) != 2) {
			printf("SETUP request didn't specify a valid Session number in Session header\n");
			send_reply(454, 0, rtsp);	/* Session Not Found */
			return ERR_NOERROR;
		}
	} else {
		// Generate a random Session number
		gettimeofday(&now_tmp, 0);
		srand((now_tmp.tv_sec * 1000) + (now_tmp.tv_usec / 1000));
#ifdef WIN32
		SessionID = rand();
#else
		SessionID = 1 + (int) (10.0 * rand() / (100000 + 1.0));
#endif
		if (SessionID == 0) {
			SessionID++;
		}
	}


         
	if (RTP_get_port_pair(&ser_ports)!=ERR_NOERROR) {
		printf("SETUP request can't be served. Maximum connection number reached.\n");
		send_reply(500, 0, rtsp);	/* Internal server error */
        	return ERR_GENERIC;
	}
	
	
	// Add an RTSP session if necessary
	if (rtsp->session_list == NULL) {
		rtsp->session_list = (RTSP_session *) calloc(1, sizeof(RTSP_session));
	}
	sp = rtsp->session_list;
	// Add an RTP session if necessary
	if (rtsp->session_list->rtp_session == NULL) {
		rtsp->session_list->rtp_session = (RTP_session *) calloc(1, sizeof(RTP_session));
		sp2 = rtsp->session_list->rtp_session;
	} else {
		for (sp2 = sp->rtp_session; sp2 != NULL; sp2 = sp2->next) {
			sp2_prec = sp2;
		}
		sp2_prec->next = (RTP_session *) calloc(1, sizeof(RTP_session));
		sp2 = sp2_prec->next;
	}
	// Setup the RTP session
#ifdef WIN32
	start_seq = rand();
	start_rtptime = rand();
#else
	start_seq = 1 + (int) (10.0 * rand() / (100000 + 1.0));
	start_rtptime = 1 + (int) (10.0 * rand() / (100000 + 1.0));
#endif
	if (start_seq == 0) {
		start_seq++;
	}
	if (start_rtptime == 0) {
		start_rtptime++;
	}
	sp2->pause = 1;
	strcpy(sp2->sd_filename, object);
	sp2->current_media = (media_entry *) calloc (1, sizeof(media_entry));
	/*TODO control (if alloc doesn't work)*/
	mediacpy(&sp2->current_media,&matching_me);
	gettimeofday(&now_tmp, 0);
	srand((now_tmp.tv_sec * 1000) + (now_tmp.tv_usec / 1000));
	sp2->start_rtptime = start_rtptime;
	sp2->start_seq = start_seq;
	sp2->cli_ports.RTP = cli_ports.RTP;
	sp2->cli_ports.RTCP = cli_ports.RTCP;
	sp2->ser_ports.RTP = ser_ports.RTP;
	
	if (getpeername(rtsp->fd, &rtsp_peer, &namelen) != 0) {
		printf("SETUP request can't be served. Getpeername() failed.\n");
		send_reply(415, 0, rtsp);	// Internal server error
		return ERR_GENERIC;
	}

	
	
	// View if the client is MwServer        
     	if (multicast==YES_MULTICAST) { // MULTICAST SESSION
        	struct in_addr inp;
		unsigned char ttl;
		ttl=32;
		strcpy(address,MULTICAST_ADDRESS);
		
		sp2->isMulticast=YES_MULTICAST;
		//RTP
		inet_aton(MULTICAST_ADDRESS,&inp);
		udp_connect(ser_ports.RTP,&(sp2->rtp_peer),inp.s_addr, &(sp2->rtp_fd));

		//RTCP
		inet_aton(MULTICAST_ADDRESS,&inp);
	        udp_connect(ser_ports.RTCP,&(sp2->rtcp_out_peer),inp.s_addr, &(sp2->rtcp_fd_out));	
                close(sp2->rtcp_fd_out); // why close? Federico: Because I have to bind in order to listen
	        udp_open(ser_ports.RTCP,&(sp2->rtcp_in_peer),&(sp2->rtcp_fd_in));  //bind
		
	        setsockopt(sp2->rtp_fd,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl));	
	        
		printf("\nSet up socket for multicast ok\n");
      	} else {
		//begin unicast
		strcpy(address,get_address());
		sp2->isMulticast=NO_MULTICAST;
		//UDP connection for outgoing RTP packets
		//sp2->rtp_fd=0;
		udp_open(ser_ports.RTP,&(sp2->rtp_peer), &(sp2->rtp_fd));
    		udp_connect(cli_ports.RTP, &(sp2->rtp_peer), (*((struct sockaddr_in *) (&rtsp_peer))).sin_addr.s_addr, &(sp2->rtp_fd));
	        		
		
		//UDP connection for outgoing RTCP packets
		udp_connect(cli_ports.RTCP,&(sp2->rtcp_out_peer),(*((struct sockaddr_in*)(&rtsp_peer))).sin_addr.s_addr, &(sp2->rtcp_fd_out));	
	
	    	// Now I have to assign the ser_ports.RTCP  
	      	close(sp2->rtcp_fd_out); // why close? Federico: Because I have to bind in order to listen
		//sp2->ser_ports.RTCP=ser_ports.RTCP;
		udp_open(ser_ports.RTCP,&(sp2->rtcp_in_peer), &(sp2->rtcp_fd_in));  //bind 
	        
	    }//end else unicast

	sp2->ssrc = ssrc;
	sp2->sched_id = schedule_add(sp2);

	// Setup the RTSP session       
	sp->session_id = SessionID;
	*new_session = sp;
	
	send_setup_reply(rtsp, sp, address,sp2); //marcosbiro: in sp c'é il numero incriminato

	return ERR_NOERROR;
}

