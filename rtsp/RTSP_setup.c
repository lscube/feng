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
#include <glib.h>
#include <glib/gprintf.h>

#include <fenice/rtsp.h>
#include <netembryo/wsocket.h>
#include <fenice/utils.h>
#include <fenice/prefs.h>
#include <fenice/multicast.h>
#include <fenice/fnc_log.h>

/*
 	****************************************************************
 	*			SETUP METHOD HANDLING
 	****************************************************************
*/


int RTSP_setup(RTSP_buffer * rtsp, RTSP_session ** new_session)
{
//	char address[16];
	char object[255], server[255];
	char url[255];
	unsigned short port;
	RTSP_session *rtsp_s;
	RTP_session *rtp_s, *rtp_s_prec;
	int SessionID = 0;
	port_pair cli_ports;
	port_pair ser_ports;
	struct timeval now_tmp;
	char *p  /* = NULL */ ;
	unsigned int start_seq, start_rtptime;
	char transport_str[255];
#if ENABLE_MEDIATHREAD
	char trackname[255];
	//mediathread pointers
	Selector *track_sel;
	Track *req_track;
#else
// TODO: delete mediainfo legacy
	media_entry *list, *matching_me, req;
#endif
//	struct sockaddr_storage rtsp_peer;
	unsigned long ssrc;
	SD_descr *matching_descr;
	unsigned char is_multicast_dad = 1;	//unicast and the first multicast
	RTP_transport transport;
	char *saved_ptr, *transport_tkn, *tmp;
	int max_interlvd;
	Sock *sock_pair[2];
	RTSP_interleaved *intlvd, *ilvd_s;

	// init
#if !ENABLE_MEDIATHREAD
// TODO: delete mediainfo legacy
	memset(&req, 0, sizeof(req));
#endif
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
#if ENABLE_MEDIATHREAD
	} else {
		// SETUP resource!trackname
		*p = '\0';
		strcpy (trackname, p + 1);
	}
#else
// TODO: delete mediainfo legacy
	} else {
		// SETUP name.sd!stream
		strcpy(req.filename, p + 1);
		req.flags |= ME_FILENAME;

		*p = '\0';
	}
#endif
	

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

#if ENABLE_MEDIATHREAD
	if (!rtsp->resource) {
		if (!(rtsp->resource = mt_resource_open(prefs_get_serv_root(), object))) {
			send_reply(404, 0, rtsp);	//TODO: Not found or Internal server error?
			return ERR_NOERROR;
		}
	}

	if (!(track_sel = r_open_tracks(rtsp->resource, trackname, NULL))) {
		send_reply(404, 0, rtsp);	// Not found
		return ERR_NOERROR;
	}

	if (!(req_track = r_selected_track(track_sel))) {
		send_reply(500, 0, rtsp);	// Internal server error
		return ERR_NOERROR;
	}

	if (mt_add_track(req_track)) {
		send_reply(500, 0, rtsp);	// Internal server error
		return ERR_NOERROR;
	}
#else
// TODO: delete mediainfo legacy
	if (enum_media(object, &matching_descr) != ERR_NOERROR) {
		send_reply(500, 0, rtsp);	// Internal server error
		return ERR_NOERROR;
	}
	list = matching_descr->me_list;

	if (get_media_entry(&req, list, &matching_me) == ERR_NOT_FOUND) {
		send_reply(404, 0, rtsp);	// Not found
		return ERR_NOERROR;
	}
#endif
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

/*	if (getpeername(rtsp->fd, (struct sockaddr *) &rtsp_peer, &namelen) !=
	    0) {
		send_reply(415, 0, rtsp);	// Internal server error
		return ERR_GENERIC;
	}*/

//	transport.type = RTP_no_transport;
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
						       &(cli_ports.RTP));
						p = strstr(p, "-");
						sscanf(p + 1, "%d",
						       &(cli_ports.RTCP));
					}
					if (RTP_get_port_pair(&ser_ports) !=
					    ERR_NOERROR) {
						send_reply(500, 0, rtsp);	/* Internal server error */
						return ERR_GENERIC;
					}
					//UDP bind for outgoing RTP packets
					tmp = g_strdup_printf("%d", ser_ports.RTP);
					transport.rtp_sock = Sock_bind(NULL, tmp, UDP, 0);
					g_free(tmp);
					//UDP bind for outgoing RTCP packets
					tmp = g_strdup_printf("%d", ser_ports.RTCP);
					transport.rtcp_sock = Sock_bind(NULL, tmp, UDP, 0);
					g_free(tmp);
					//UDP connection for outgoing RTP packets
					tmp = g_strdup_printf("%d", cli_ports.RTP);
					Sock_connect (get_remote_host(rtsp->sock), tmp, transport.rtp_sock, UDP, 0);
					g_free(tmp);
					//UDP connection for outgoing RTCP packets
					tmp = g_strdup_printf("%d", cli_ports.RTCP);
					Sock_connect (get_remote_host(rtsp->sock), tmp, transport.rtcp_sock, UDP, 0);
					g_free(tmp);
				}
#if !ENABLE_MEDIATHREAD
// TODO: multicast with mediathread
				else if (matching_descr->flags & SD_FL_MULTICAST) {	//multicast 
					// TODO: make the difference between only multicast allowed or unicast fallback allowed.
					cli_ports.RTP = ser_ports.RTP =
					    matching_me->rtp_multicast_port;
					cli_ports.RTCP = ser_ports.RTCP =
					    matching_me->rtp_multicast_port + 1;

					is_multicast_dad = 0;
					if (!
					    (matching_descr->
					     flags & SD_FL_MULTICAST_PORT)) {
						is_multicast_dad = 1;
						//RTP outgoing packets
						tmp = g_strdup_printf("%d", cli_ports.RTP);
						transport.rtp_sock = Sock_connect(matching_descr->multicast, tmp, NULL, UDP, 0);
						g_free(tmp);
						//RTCP incoming packets
						//transport.rtcp_sock = Sock_bind(NULL, tmp, UDP, 0); //TODO: check if needed
						//RTCP outgoing packets
						tmp = g_strdup_printf("%d", cli_ports.RTCP);
						transport.rtcp_sock = Sock_connect(matching_descr->multicast, tmp, transport.rtcp_sock, UDP, 0);
						g_free(tmp);

						if (matching_me->next == NULL)
							matching_descr->flags |=
							    SD_FL_MULTICAST_PORT;

						matching_me->
						    rtp_multicast_port =
						    cli_ports.RTP;

						fnc_log(FNC_LOG_DEBUG,
							"\nSet up socket for multicast ok\n");
					}
				} else 
					continue;
#endif
				break;	// found a valid transport
			} else if (Sock_type(rtsp->sock) == TCP && !strncmp(p, "/TCP", 4)) {	// Transport: RTP/AVP/TCP;interleaved=x-y
				
				if ( !(intlvd = calloc(1, sizeof(RTSP_interleaved))) )
					return ERR_GENERIC;
				
				if ((p = strstr(transport_tkn, "interleaved"))) {
					p = strstr(p, "=");
					sscanf(p + 1, "%hu",
					       &(intlvd->proto.tcp.rtp_ch));
					if ((p = strstr(p, "-")))
						sscanf(p + 1, "%hu",
						       &(intlvd->proto.tcp.rtcp_ch));
					else
						intlvd->proto.tcp.rtcp_ch =
						    intlvd->proto.tcp.rtp_ch + 1;
				} else {	// search for max used interleved channel.
					max_interlvd = -1;
					for (ilvd_s = (rtsp->interleaved);
					     ilvd_s; ilvd_s = ilvd_s->next)
						max_interlvd =
						    max(max_interlvd,
							ilvd_s->proto.tcp.rtcp_ch);
					intlvd->proto.tcp.rtp_ch =
					    max_interlvd + 1;
					intlvd->proto.tcp.rtcp_ch =
					    max_interlvd + 2;
				}
				if ( (intlvd->proto.tcp.rtp_ch > 255) || (intlvd->proto.tcp.rtcp_ch > 255) ) {
						fnc_log(FNC_LOG_ERR, "Interleaved channel number already reached max\n");
						send_reply(500, "Interleaved channel number already reached max", rtsp);
						free(intlvd);
						return ERR_GENERIC;
				}
				// RTP local sockpair
				if ( Sock_socketpair(sock_pair) < 0) {
					fnc_log(FNC_LOG_ERR, "Cannot create AF_LOCAL socketpair for rtp\n");
					send_reply(500, 0, rtsp);
					free(intlvd);
					return ERR_GENERIC;
				}
				transport.rtp_sock = sock_pair[0];
				intlvd->rtp_local = sock_pair[1];
				// RTCP local sockpair
				if ( Sock_socketpair(sock_pair) < 0) {
					fnc_log(FNC_LOG_ERR, "Cannot create AF_LOCAL socketpair for rtcp\n");
					send_reply(500, 0, rtsp);
					Sock_close(transport.rtp_sock);
					Sock_close(intlvd->rtp_local);
					free(intlvd);
					return ERR_GENERIC;
				}
				
				transport.rtcp_sock = sock_pair[0];
				intlvd->rtcp_local = sock_pair[1];

				// copy stream number in rtp_transport struct
				transport.rtp_ch = intlvd->proto.tcp.rtp_ch;
				transport.rtcp_ch = intlvd->proto.tcp.rtcp_ch;
				
				// insert new interleaved stream in the list
				intlvd->next = rtsp->interleaved;
				rtsp->interleaved = intlvd;

				break;	// found a valid transport
#ifdef HAVE_SCTP_FENICE
			} else if (Sock_type(rtsp->sock) == SCTP && !strncmp(p, "/SCTP", 5)) {	// Transport: RTP/AVP/SCTP;streams=x-y
				
				if ( !(intlvd = calloc(1, sizeof(RTSP_interleaved))) )
					return ERR_GENERIC;

				if ((p = strstr(transport_tkn, "streams"))) {
					p = strstr(p, "=");
					sscanf(p + 1, "%hu",
					       &(intlvd->proto.sctp.rtp.sinfo_stream));
					if ((p = strstr(p, "-")))
						sscanf(p + 1, "%hu",
						       &(intlvd->proto.sctp.rtcp.sinfo_stream));
					else
						intlvd->proto.sctp.rtcp.sinfo_stream =
						    intlvd->proto.sctp.rtp.sinfo_stream + 1;
				} else {	// search for max used stream.
					max_interlvd = -1;
					for (ilvd_s = (rtsp->interleaved);
					     ilvd_s; ilvd_s = ilvd_s->next)
						max_interlvd =
						    max(max_interlvd,
							ilvd_s->proto.sctp.rtcp.sinfo_stream);
					intlvd->proto.sctp.rtp.sinfo_stream =
					    max_interlvd + 1;
					intlvd->proto.sctp.rtcp.sinfo_stream =
					    max_interlvd + 2;
				}
				if ( !((intlvd->proto.sctp.rtp.sinfo_stream < MAX_SCTP_STREAMS)
				    && (intlvd->proto.sctp.rtcp.sinfo_stream < MAX_SCTP_STREAMS)) ) {
						fnc_log(FNC_LOG_ERR, "Stream id over limit\n");
						send_reply(500, "Stream id over limit", rtsp);
						free(intlvd);
						return ERR_GENERIC;
				}
				// RTP local sockpair
				if ( Sock_socketpair(sock_pair) < 0) {
					fnc_log(FNC_LOG_ERR, "Cannot create AF_LOCAL socketpair for rtp\n");
					send_reply(500, 0, rtsp);
					free(intlvd);
					return ERR_GENERIC;
				}
				transport.rtp_sock = sock_pair[0];
				intlvd->rtp_local = sock_pair[1];
				// RTCP local sockpair
				if ( Sock_socketpair(sock_pair) < 0) {
					fnc_log(FNC_LOG_ERR, "Cannot create AF_LOCAL socketpair for rtcp\n");
					send_reply(500, 0, rtsp);
					Sock_close(transport.rtp_sock);
					Sock_close(intlvd->rtp_local);
					free(intlvd);
					return ERR_GENERIC;
				}
				
				transport.rtcp_sock = sock_pair[0];
				intlvd->rtcp_local = sock_pair[1];

				// copy stream number in rtp_transport struct
				transport.rtp_ch = intlvd->proto.sctp.rtp.sinfo_stream;
				transport.rtcp_ch = intlvd->proto.sctp.rtcp.sinfo_stream;
				
				// insert new interleaved stream in the list
				intlvd->next = rtsp->interleaved;
				rtsp->interleaved = intlvd;
				
				break;	// found a valid transport
#endif
			}
		}
	} while ((transport_tkn = strtok_r(NULL, ",", &saved_ptr)));
	// printf("rtp transport: %d\n", transport.type);

	if (!transport.rtp_sock) {
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
	start_seq = 1 + (unsigned int) (rand() % (0xFFFF));
	start_rtptime = 1 + (unsigned int) (rand() % (0xFFFFFFFF));
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
#if !ENABLE_MEDIATHREAD
	// TODO: multicast with mediathread
	if (is_multicast_dad) {
		if (mediacpy(&rtp_s->current_media, &matching_me)) {
			send_reply(500, 0, rtsp);	// Internal server error
			return ERR_GENERIC;
		}
	}
#endif

	gettimeofday(&now_tmp, 0);
	srand((now_tmp.tv_sec * 1000) + (now_tmp.tv_usec / 1000));
	rtp_s->start_rtptime = start_rtptime;
	rtp_s->start_seq = start_seq;
	memcpy(&rtp_s->transport, &transport, sizeof(transport));
	rtp_s->is_multicast_dad = is_multicast_dad;

	/*xxx */
	rtp_s->sd_descr = matching_descr;

	rtp_s->sched_id = schedule_add(rtp_s);
#if ENABLE_MEDIATHREAD
	rtp_s->track_selector = track_sel;
#endif

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
