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
#include <sys/time.h>		// shawill: for gettimeofday
#include <stdlib.h>		// shawill: for rand, srand
#include <unistd.h>		// shawill: for dup
// shawill: for inet_aton
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fenice/rtsp.h>
#include <fenice/socket.h>
#include <fenice/utils.h>
#include <fenice/prefs.h>
#include <fenice/multicast.h>
#include <fenice/fnc_log.h>

// #ifdef SOCK_SEQPACKET
// #define SOCKPAIRTYPE SOCK_SEQPACKET
// #else
#define SOCKPAIRTYPE SOCK_DGRAM
// #endif

/*
 	****************************************************************
 	*			SETUP METHOD HANDLING
 	****************************************************************
*/


int RTSP_setup(RTSP_buffer * rtsp, RTSP_session ** new_session)
{
	char address[16];
	char object[255], server[255];
	char url[255];
	unsigned short port;
	RTSP_session *rtsp_s;
	RTP_session *rtp_s, *rtp_s_prec;
	int SessionID = 0;
//      port_pair cli_ports;
//      port_pair ser_ports;
	struct timeval now_tmp;
	char *p /* = NULL */ ;
	unsigned int start_seq, start_rtptime;
	char transport_str[255];
	media_entry *list, *matching_me, req;
	struct sockaddr_storage rtsp_peer;
	socklen_t namelen = sizeof(rtsp_peer);
	unsigned long ssrc;
	SD_descr *matching_descr;
	unsigned char is_multicast_dad = 1;	//unicast and the first multicast
	RTP_transport transport;
	char *saved_ptr, *transport_tkn;
	int max_interlvd;
	int sdpair[2];
	RTSP_interleaved *intlvd;

	// init
	memset(&req, 0, sizeof(req));
	memset(&transport, 0, sizeof(transport));

	// Parse the input message

	/* Get the URL */
	if (!sscanf(rtsp->in_buffer, " %*s %254s ", url)) {
		send_reply(400, 0, rtsp);	/* bad request */
		return ERR_NOERROR;
	}
	/* Validate the URL */
	switch (parse_url(url, server, sizeof(server), &port, object, sizeof(object))) {	//object is requested file's name
	case 1:		// bad request
		send_reply(400, 0, rtsp);
		return ERR_NOERROR;
	case -1:		// interanl server error
		send_reply(500, 0, rtsp);
		return ERR_NOERROR;
		break;
	default:
		break;
	}
	if (strcmp(server, prefs_get_hostname()) != 0) {	/* Currently this feature is disabled. */
		/* wrong server name */
		//      send_reply(404, 0 , rtsp); /* Not Found */
		//      return ERR_NOERROR;
	}
	if (strstr(object, "../")) {
		/* disallow relative paths outside of current directory. */
		send_reply(403, 0, rtsp);	/* Forbidden */
		return ERR_NOERROR;
	}
	if (strstr(object, "./")) {
		/* Disallow the ./ */
		send_reply(403, 0, rtsp);	/* Forbidden */
		return ERR_NOERROR;
	}

	if (!(p = strrchr(object, '.'))) {	// if filename is without extension
		send_reply(415, 0, rtsp);	/* Unsupported media type */
		return ERR_NOERROR;
	} else if (!is_supported_url(p)) {	//if filename's extension is not valid
		send_reply(415, 0, rtsp);	/* Unsupported media type */
		return ERR_NOERROR;
	}
	if (!(p = strchr(object, '!'))) {	//if '!' is not present then a file has not been specified
		send_reply(500, 0, rtsp);	/* Internal server error */
		return ERR_NOERROR;
	} else {
		// SETUP name.sd!stream
		strcpy(req.filename, p + 1);
		req.flags |= ME_FILENAME;

		*p = '\0';
	}

// ------------ START PATCH
	{
		char temp[255];
		char *pd = NULL;

		strcpy(temp, object);
#if 0
		printf("%s\n", object);
		// BEGIN 
		// if ( (p = strstr(temp, "/")) ) {
		if ((p = strchr(temp, '/'))) {
			strcpy(object, p + 1);	// CRITIC. 
		}
		printf("%s\n", temp);
#endif
		// pd = strstr(p, ".sd");       // this part is usefull in order to
		pd = strstr(temp, ".sd");
		if ((p = strstr(pd + 1, ".sd"))) {	// have compatibility with RealOne
			strcpy(object, pd + 4);	// CRITIC. 
		}		//Note: It's a critic part
		// END 
	}
// ------------ END PATCH
	if (enum_media(object, &matching_descr) != ERR_NOERROR) {
		send_reply(500, 0, rtsp);	/* Internal server error */
		return ERR_NOERROR;
	}
	list = matching_descr->me_list;

	if (get_media_entry(&req, list, &matching_me) == ERR_NOT_FOUND) {
		send_reply(404, 0, rtsp);	/* Not found */
		return ERR_NOERROR;
	}
	// Get the CSeq 
	if ((p = strstr(rtsp->in_buffer, HDR_CSEQ)) == NULL) {
		send_reply(400, 0, rtsp);	/* Bad Request */
		return ERR_NOERROR;
	} else {
		if (sscanf(p, "%*s %d", &(rtsp->rtsp_cseq)) != 1) {
			send_reply(400, 0, rtsp);	/* Bad Request */
			return ERR_NOERROR;
		}
	}
	/*if ((p = strstr(rtsp->in_buffer, "ssrc")) != NULL) {
	   p = strchr(p, '=');
	   sscanf(p + 1, "%lu", &ssrc);
	   } else { */
	ssrc = random32(0);
	//}

	// Start parsing the Transport header
	if ((p = strstr(rtsp->in_buffer, HDR_TRANSPORT)) == NULL) {
		send_reply(406, "Require: Transport settings" /* of rtp/udp;port=nnnn. " */ , rtsp);	/* Not Acceptable */
		return ERR_NOERROR;
	}
	if (sscanf(p, "%*10s%255s", transport_str) != 1) {
		fnc_log(FNC_LOG_ERR,
			"SETUP request malformed: Transport string is empty\n");
		send_reply(400, 0, rtsp);	/* Bad Request */
		return ERR_NOERROR;
	}
	// printf("transport: %s\n", transport_str); // XXX tmp.

	// tokenize the coma seaparated list of transport settings:
	if (!(transport_tkn = strtok_r(transport_str, ",", &saved_ptr))) {
		fnc_log(FNC_LOG_ERR,
			"Malformed Transport string from client\n");
		send_reply(400, 0, rtsp);	/* Bad Request */
		return ERR_NOERROR;
	}

	if (getpeername(rtsp->fd, (struct sockaddr *) &rtsp_peer, &namelen) !=
	    0) {
		send_reply(415, 0, rtsp);	// Internal server error
		return ERR_GENERIC;
	}

	transport.type = RTP_no_transport;
	do {			// search a good transport string
		if ((p = strstr(transport_tkn, RTSP_RTP_AVP))) {	// Transport: RTP/AVP
			p += strlen(RTSP_RTP_AVP);
			if (!*p || (*p == ';') || (*p == ' ')) {
#if 0 // default transport value for RTP/AVP is multicast, so we should check if "unicast" word is present, not "multicast"
				// if ((p = strstr(rtsp->in_buffer, "client_port")) == NULL && strstr(rtsp->in_buffer, "multicast") == NULL) {
				if ((p =
				     strstr(transport_tkn,
					    "client_port")) == NULL
				    && strstr(transport_tkn,
					      "multicast") == NULL) {
					send_reply(406, "Require: Transport settings of rtp/udp;port=nnnn. ", rtsp);	/* Not Acceptable */
					return ERR_NOERROR;
				}
#endif				// #if 0
				if (strstr(transport_tkn, "unicast")) {
					if ((p =
					     strstr(transport_tkn,
						    "client_port"))) {
						p = strstr(p, "=");
						sscanf(p + 1, "%d",
						       &(transport.u.udp.
							 cli_ports.RTP));
						p = strstr(p, "-");
						sscanf(p + 1, "%d",
						       &(transport.u.udp.
							 cli_ports.RTCP));
					}
					if (RTP_get_port_pair
					    (&transport.u.udp.ser_ports) !=
					    ERR_NOERROR) {
						send_reply(500, 0, rtsp);	/* Internal server error */
						return ERR_GENERIC;
					}
					// strcpy(address, get_address());
					udp_open(transport.u.udp.ser_ports.RTP, &transport.u.udp.rtp_peer, &transport.rtp_fd);	//bind
					udp_open(transport.u.udp.ser_ports.RTCP, &transport.u.udp.rtcp_in_peer, &transport.rtcp_fd_in);	//bind
					//UDP connection for outgoing RTP packets
					udp_connect(transport.u.udp.cli_ports.
						    RTP,
						    &transport.u.udp.rtp_peer,
						    (*
						     ((struct sockaddr_in
						       *) (&rtsp_peer))).
						    sin_addr.s_addr,
						    &transport.rtp_fd);
					//UDP connection for outgoing RTCP packets
					transport.rtcp_fd_out = transport.rtcp_fd_in; 
					udp_connect(transport.u.udp.cli_ports.
						    RTCP,
						    &transport.u.udp.
						    rtcp_out_peer,
						    (*
						     ((struct sockaddr_in
						       *) (&rtsp_peer))).
						    sin_addr.s_addr,
						    &transport.rtcp_fd_out);

					transport.u.udp.is_multicast = 0;
				} else if (matching_descr->flags & SD_FL_MULTICAST) {	/*multicast */
					// TODO: make the difference between only multicast allowed or unicast fallback allowed.
					transport.u.udp.cli_ports.RTP =
					    transport.u.udp.ser_ports.RTP =
					    matching_me->rtp_multicast_port;
					transport.u.udp.cli_ports.RTCP =
					    transport.u.udp.ser_ports.RTCP =
					    matching_me->rtp_multicast_port + 1;

					is_multicast_dad = 0;
					if (!
					    (matching_descr->
					     flags & SD_FL_MULTICAST_PORT)) {
						struct in_addr inp;
						unsigned char ttl = DEFAULT_TTL;
						struct ip_mreq mreq;

						mreq.imr_multiaddr.s_addr =
						    inet_addr(matching_descr->
							      multicast);
						mreq.imr_interface.s_addr =
						    INADDR_ANY;
						setsockopt(transport.rtp_fd,
							   IPPROTO_IP,
							   IP_ADD_MEMBERSHIP,
							   &mreq, sizeof(mreq));
						setsockopt(transport.rtp_fd,
							   IPPROTO_IP,
							   IP_MULTICAST_TTL,
							   &ttl, sizeof(ttl));

						is_multicast_dad = 1;
						strcpy(address,
						       matching_descr->
						       multicast);
						//RTP outgoing packets
						inet_aton(address, &inp);
						udp_connect(transport.u.udp.
							    ser_ports.RTP,
							    &transport.u.udp.
							    rtp_peer,
							    inp.s_addr,
							    &transport.rtp_fd);
						//RTCP outgoing packets
						inet_aton(address, &inp);
						udp_connect(transport.u.udp.
							    ser_ports.RTCP,
							    &transport.u.udp.
							    rtcp_out_peer,
							    inp.s_addr,
							    &transport.
							    rtcp_fd_out);
						//udp_open(transport.u.udp.ser_ports.RTCP, &(sp2->rtcp_in_peer), &(sp2->rtcp_fd_in));   //bind 

						if (matching_me->next == NULL)
							matching_descr->flags |=
							    SD_FL_MULTICAST_PORT;

						matching_me->
						    rtp_multicast_port =
						    transport.u.udp.ser_ports.
						    RTP;
						transport.u.udp.is_multicast =
						    1;
						fnc_log(FNC_LOG_DEBUG,
							"\nSet up socket for multicast ok\n");
					}
				} else
					continue;

				transport.type = RTP_rtp_avp;
				break;	// found a valid transport
			} else if (!strncmp(p, "/TCP", 4)) {	// Transport: RTP/AVP/TCP;interleaved=x-y // XXX still not finished
				
				if ( !(intlvd = calloc(1, sizeof(RTSP_interleaved))) )
					return ERR_GENERIC;
				
				if ((p = strstr(transport_tkn, "interleaved"))) {
					p = strstr(p, "=");
					sscanf(p + 1, "%hu",
					       &(transport.u.tcp.interleaved.
						 RTP));
					if ((p = strstr(p, "-")))
						sscanf(p + 1, "%hu",
						       &(transport.u.tcp.
							 interleaved.RTCP));
					else
						transport.u.tcp.interleaved.
						    RTCP =
						    transport.u.tcp.interleaved.
						    RTP + 1;
						    
					if ( (transport.u.tcp.interleaved.RTP > 255) || (transport.u.tcp.interleaved.RTCP > 255) ) {
						fnc_log(FNC_LOG_ERR, "Interleaved channel number suggested from client too high\n");
						send_reply(400, "Interleaved channel number suggested from client too high", rtsp);
						return ERR_GENERIC;
					}
				} else {	// search for max used interleved channel.
					max_interlvd = -1;
					for (rtp_s =
					     (rtsp->session_list) ? rtsp->
					     session_list->rtp_session : NULL;
					     rtp_s; rtp_s = rtp_s->next)
						max_interlvd =
						    max(max_interlvd,
							(rtp_s->transport.
							 type ==
							 RTP_rtp_avp_tcp) ?
							rtp_s->transport.u.tcp.
							interleaved.RTCP : -1);
					transport.u.tcp.interleaved.RTP =
					    max_interlvd + 1;
					transport.u.tcp.interleaved.RTCP =
					    max_interlvd + 2;
					    
					if ( (transport.u.tcp.interleaved.RTP > 255) || (transport.u.tcp.interleaved.RTCP > 255) ) {
						fnc_log(FNC_LOG_ERR, "Interleaved channel number already reached max\n");
						send_reply(500, "Interleaved channel number already reached max", rtsp);
						return ERR_GENERIC;
					}
				}
				
				if ( !(intlvd = calloc(1, sizeof(RTSP_interleaved))) )
					return ERR_ALLOC;
				
				intlvd->proto.tcp.rtp_ch = transport.u.tcp.interleaved.RTP;
				intlvd->proto.tcp.rtcp_ch = transport.u.tcp.interleaved.RTCP;
				
				// RTP local sockpair
				if ( socketpair(AF_LOCAL, SOCKPAIRTYPE, 0, sdpair) < 0) {
					fnc_log(FNC_LOG_ERR, "Cannot create AF_LOCAL socketpair for rtp\n");
					send_reply(500, 0, rtsp);
					free(intlvd);
					return ERR_GENERIC;
				}
				transport.rtp_fd = sdpair[0];
				intlvd->rtp_fd = sdpair[1];
				// RTCP local sockpair
				if ( socketpair(AF_LOCAL, SOCKPAIRTYPE, 0, sdpair) < 0) {
					fnc_log(FNC_LOG_ERR, "Cannot create AF_LOCAL socketpair for rtcp\n");
					send_reply(500, 0, rtsp);
					close(transport.rtp_fd);
					close(intlvd->rtp_fd);
					free(intlvd);
					return ERR_GENERIC;
				}
				
				transport.rtcp_fd_out = sdpair[0];
				intlvd->rtcp_fd = sdpair[1];
				
				transport.rtcp_fd_in = -1;
				
				// insert new interleaved stream in the list
				intlvd->next = rtsp->interleaved;
				rtsp->interleaved = intlvd;

				transport.type = RTP_rtp_avp_tcp;
				break;	// found a valid transport
#ifdef HAVE_SCTP_FENICE
			} else if (rtsp->proto == SCTP && !strncmp(p, "/SCTP", 5)) {	// Transport: RTP/AVP/SCTP;streams=x-y
				if ((p = strstr(transport_tkn, "streams"))) {
					p = strstr(p, "=");
					sscanf(p + 1, "%hu",
					       &(transport.u.sctp.streams.RTP));
					if ((p = strstr(p, "-")))
						sscanf(p + 1, "%hu",
						       &(transport.u.sctp.
							 streams.RTCP));
					else
						transport.u.sctp.streams.RTCP =
						    transport.u.sctp.streams.
						    RTP + 1;
				} else {	// search for max used stream.
					max_interlvd = -1;
					for (rtp_s =
					     (rtsp->session_list) ? rtsp->
					     session_list->rtp_session : NULL;
					     rtp_s; rtp_s = rtp_s->next)
						max_interlvd =
						    max(max_interlvd,
							(rtp_s->transport.
							 type ==
							 RTP_rtp_avp_sctp) ?
							rtp_s->transport.u.sctp.
							streams.RTCP : -1);
					transport.u.sctp.streams.RTP =
					    max_interlvd + 1;
					transport.u.sctp.streams.RTCP =
					    max_interlvd + 2;
				}

/*				// RTP local sockpair
				if ( socketpair(AF_LOCAL, SOCKPAIRTYPE, 0, sdpair) < 0) {
					fnc_log(FNC_LOG_ERR, "Cannot create AF_LOCAL socketpair for rtp\n");
					send_reply(500, 0, rtsp);
					return ERR_GENERIC;
				}
				transport.rtp_fd = sdpair[0];
				// RTCP local sockpair
				if ( socketpair(AF_LOCAL, SOCKPAIRTYPE, 0, sdpair) < 0) {
					fnc_log(FNC_LOG_ERR, "Cannot create AF_LOCAL socketpair for rtcp\n");
					send_reply(500, 0, rtsp);
					return ERR_GENERIC;
				}*/
				transport.rtp_fd = rtsp->fd;

				transport.rtcp_fd_out = rtsp->fd;
				
				transport.rtcp_fd_in = -1;
				
				// TODO: FINISH!!!

				transport.type = RTP_rtp_avp_sctp;
				break;	// found a valid transport
#endif
			}
		}
	} while ((transport_tkn = strtok_r(NULL, ",", &saved_ptr)));
	// printf("rtp transport: %d\n", transport.type);

	if (transport.type == RTP_no_transport) {
		// fnc_log(FNC_LOG_ERR,"Unsupported Transport\n");
		send_reply(461, "Unsupported Transport", rtsp);	/* Bad Request */
		return ERR_NOERROR;
	}
	// If there's a Session header we have an aggregate control
	if ((p = strstr(rtsp->in_buffer, HDR_SESSION)) != NULL) {
		if (sscanf(p, "%*s %d", &SessionID) != 1) {
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

	// Add an RTSP session if necessary
	if (!rtsp->session_list) {
		rtsp->session_list =
		    (RTSP_session *) calloc(1, sizeof(RTSP_session));
	}
	rtsp_s = rtsp->session_list;

	// Setup the RTP session
	if (rtsp->session_list->rtp_session == NULL) {
		rtsp->session_list->rtp_session =
		    (RTP_session *) calloc(1, sizeof(RTP_session));
		rtp_s = rtsp->session_list->rtp_session;
	} else {
		for (rtp_s = rtsp_s->rtp_session; rtp_s != NULL;
		     rtp_s = rtp_s->next) {
			rtp_s_prec = rtp_s;
		}
		rtp_s_prec->next =
		    (RTP_session *) calloc(1, sizeof(RTP_session));
		rtp_s = rtp_s_prec->next;
	}


#ifdef WIN32
	start_seq = rand();
	start_rtptime = rand();
#else
	// start_seq = 1 + (int) (10.0 * rand() / (100000 + 1.0));
	// start_rtptime = 1 + (int) (10.0 * rand() / (100000 + 1.0));
#if 0
	start_seq =
	    1 +
	    (unsigned int) ((float) (0xFFFF) *
			    ((float) rand() / (float) RAND_MAX));
	start_rtptime =
	    1 +
	    (unsigned int) ((float) (0xFFFFFFFF) *
			    ((float) rand() / (float) RAND_MAX));
#else
	start_seq = 1 + (unsigned int) (rand() % (0xFFFF));
	start_rtptime = 1 + (unsigned int) (rand() % (0xFFFFFFFF));
#endif
#endif
	if (start_seq == 0) {
		start_seq++;
	}
	if (start_rtptime == 0) {
		start_rtptime++;
	}
	rtp_s->pause = 1;
	strcpy(rtp_s->sd_filename, object);
	/*xxx */
	rtp_s->current_media = (media_entry *) calloc(1, sizeof(media_entry));

	// if(!(matching_descr->flags & SD_FL_MULTICAST_PORT)){
	if (is_multicast_dad) {
		if (mediacpy(&rtp_s->current_media, &matching_me)) {
			send_reply(500, 0, rtsp);	/* Internal server error */
			return ERR_GENERIC;
		}
	}

	gettimeofday(&now_tmp, 0);
	srand((now_tmp.tv_sec * 1000) + (now_tmp.tv_usec / 1000));
	rtp_s->start_rtptime = start_rtptime;
	rtp_s->start_seq = start_seq;
	memcpy(&rtp_s->transport, &transport, sizeof(transport));
	rtp_s->is_multicast_dad = is_multicast_dad;

	/*xxx */
	rtp_s->sd_descr = matching_descr;

	rtp_s->sched_id = schedule_add(rtp_s);


	rtp_s->ssrc = ssrc;
	// Setup the RTSP session       
	rtsp_s->session_id = SessionID;
	*new_session = rtsp_s;

	fnc_log(FNC_LOG_INFO, "SETUP %s RTSP/1.0 ", url);
	send_setup_reply(rtsp, rtsp_s, matching_descr, rtp_s);
	// See User-Agent 
	if ((p = strstr(rtsp->in_buffer, HDR_USER_AGENT)) != NULL) {
		char cut[strlen(p)];
		strcpy(cut, p);
		p = strstr(cut, "\n");
		cut[strlen(cut) - strlen(p) - 1] = '\0';
		fnc_log(FNC_LOG_CLIENT, "%s\n", cut);
	} else
		fnc_log(FNC_LOG_CLIENT, "- \n");

	return ERR_NOERROR;
}
