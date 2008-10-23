/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
 * See AUTHORS for more details 
 *  
 * Feng is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Feng is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *  
 * */

/** @file RTSP_setup.c
 * @brief Contains SETUP method and reply handlers
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <glib.h>

#include <fenice/rtsp.h>
#include <fenice/prefs.h>
#include <fenice/multicast.h>
#include <fenice/fnc_log.h>
#include <fenice/schedule.h>
#include <RTSP_utils.h>


/**
 * Splits the path of a requested media finding the trackname and the removing it from the object
 * @param url the Url for which to split the object
 * @param trackname where to save the trackname removed from the object
 * @param trackname_max_len maximum length of the trackname buffer 
 * @return RTSP_Ok or RTSP_InternalServerError if there was no trackname
 */
static RTSP_Error split_resource_path(Url * url, char * trackname, size_t trackname_max_len)
{
    char * p;

    //if '=' is not present then a file has not been specified
    if (!(p = strchr(url->path, '=')))
        return RTSP_InternalServerError;
    else {
        // SETUP resource!trackname
        g_strlcpy(trackname, p + 1, trackname_max_len);
        // XXX Not really nice...
        while (url->path != p) if (*--p == '/') break;
        *p = '\0';
    }

    return RTSP_Ok;
}

/**
 * bind&connect the socket
 */
static RTSP_Error unicast_transport(RTSP_buffer *rtsp, RTP_transport *transport,
                                    port_pair cli_ports)
{
    char port_buffer[8];
    port_pair ser_ports;
    RTSP_Error error;
    if (RTP_get_port_pair(rtsp->srv, &ser_ports) != ERR_NOERROR) {
        return RTSP_InternalServerError;
    }
    //UDP bind for outgoing RTP packets
    snprintf(port_buffer, 8, "%d", ser_ports.RTP);
    transport->rtp_sock = Sock_bind(NULL, port_buffer, NULL, UDP, NULL);
    //UDP bind for outgoing RTCP packets
    snprintf(port_buffer, 8, "%d", ser_ports.RTCP);
    transport->rtcp_sock = Sock_bind(NULL, port_buffer, NULL, UDP, NULL);
    //UDP connection for outgoing RTP packets
    snprintf(port_buffer, 8, "%d", cli_ports.RTP);
    Sock_connect (get_remote_host(rtsp->sock), port_buffer,
                  transport->rtp_sock, UDP, NULL);
    //UDP connection for outgoing RTCP packets
    snprintf(port_buffer, 8, "%d", cli_ports.RTCP);
    Sock_connect (get_remote_host(rtsp->sock), port_buffer,
                  transport->rtcp_sock, UDP, NULL);

    if (!transport->rtp_sock) {
        // fnc_log(FNC_LOG_ERR,"Unsupported Transport\n");
        set_RTSP_Error(&error, 461, "Unsupported Transport");
        return error;
    }

    return RTSP_Ok;
}

/**
 * set the sockets for the first multicast request, otherwise provide the
 * already instantiated rtp session
 */
static RTSP_Error multicast_transport(feng *srv, RTP_transport *transport,
                                      ResourceInfo *info,
                                      Track *tr,
                                      RTP_session **rtp_s)
{
    char port_buffer[8];
    RTSP_Error error;

    *rtp_s = schedule_find_multicast(srv, tr->info->mrl);

    if (!*rtp_s) {
        snprintf(port_buffer, 8, "%d", tr->info->rtp_port);
        transport->rtp_sock = Sock_connect(info->multicast, port_buffer,
                                                transport->rtp_sock, UDP, 0);
        snprintf(port_buffer, 8, "%d", tr->info->rtp_port + 1);
        transport->rtcp_sock = Sock_connect(info->multicast, port_buffer,
                                                transport->rtcp_sock, UDP, 0);

        if (!transport->rtp_sock) {
            set_RTSP_Error(&error, 461, "Unsupported Transport");
            return error;
        }
    }

    fnc_log(FNC_LOG_DEBUG,"Multicast socket set");
    return RTSP_Ok;
}

/**
 * tcp interleaved transport
 */

static RTSP_Error tcp_transport(RTSP_buffer *rtsp, RTP_transport *transport,
                                 int rtp_ch, int rtcp_ch)
{
    RTSP_Error error;
    RTSP_interleaved *intlvd;
    Sock *sock_pair[2];

    // RTP local sockpair
    if ( Sock_socketpair(sock_pair) < 0) {
        fnc_log(FNC_LOG_ERR,
                "Cannot create AF_LOCAL socketpair for rtp\n");
        set_RTSP_Error(&error, 500, "");
        return error;
    }

    intlvd = g_new0(RTSP_interleaved, 1);

    transport->rtp_sock = sock_pair[0];
    intlvd->rtp_local = sock_pair[1];

    // RTCP local sockpair
    if ( Sock_socketpair(sock_pair) < 0) {
        fnc_log(FNC_LOG_ERR,
                "Cannot create AF_LOCAL socketpair for rtcp\n");
        set_RTSP_Error(&error, 500, "");
        Sock_close(transport->rtp_sock);
        Sock_close(intlvd->rtp_local);
        g_free(intlvd);
        return error;
    }

    transport->rtcp_sock = sock_pair[0];
    intlvd->rtcp_local = sock_pair[1];

    // copy stream number in rtp_transport struct
    transport->rtp_ch = intlvd->proto.tcp.rtp_ch = rtp_ch;
    transport->rtcp_ch = intlvd->proto.tcp.rtcp_ch = rtcp_ch;

    // insert new interleaved stream in the list
    rtsp->interleaved = g_slist_prepend(rtsp->interleaved, intlvd);

    return RTSP_Ok;
}

/**
 * sctp interleaved transport
 */

#ifdef HAVE_LIBSCTP
static RTSP_Error sctp_transport(RTSP_buffer *rtsp, RTP_transport *transport,
                                 int rtp_ch, int rtcp_ch)
{
    RTSP_Error error;
    RTSP_interleaved *intlvd;
    Sock *sock_pair[2];

    if ( !((rtp_ch < MAX_SCTP_STREAMS) && (rtcp_ch < MAX_SCTP_STREAMS))) {
        fnc_log(FNC_LOG_ERR, "Stream id over limit\n");
        set_RTSP_Error(&error, 500, "Stream id over limit");
        return error;
    }
    // RTP local sockpair
    if ( Sock_socketpair(sock_pair) < 0) {
        fnc_log(FNC_LOG_ERR,
                "Cannot create AF_LOCAL socketpair for rtp\n");
        set_RTSP_Error(&error, 500, "");
        return error;
    }

    intlvd = g_new0(RTSP_interleaved, 1);

    transport->rtp_sock = sock_pair[0];
    intlvd->rtp_local = sock_pair[1];

    // RTCP local sockpair
    if ( Sock_socketpair(sock_pair) < 0) {
        fnc_log(FNC_LOG_ERR,
                "Cannot create AF_LOCAL socketpair for rtcp\n");
        set_RTSP_Error(&error, 500, "");
        Sock_close(transport->rtp_sock);
        Sock_close(intlvd->rtp_local);
        g_free(intlvd);
        return error;
    }

    transport->rtcp_sock = sock_pair[0];
    intlvd->rtcp_local = sock_pair[1];

    // copy stream number in rtp_transport struct
    transport->rtp_ch = intlvd->proto.sctp.rtp.sinfo_stream = rtp_ch;
    transport->rtcp_ch = intlvd->proto.sctp.rtcp.sinfo_stream = rtcp_ch;

    // insert new interleaved stream in the list
    rtsp->interleaved = g_slist_prepend(rtsp->interleaved, intlvd);

    return RTSP_Ok;
}
#endif

/**
 * Parses the TRANSPORT header from the RTSP buffer
 * @param rtsp the buffer for which to parse the header
 * @param transport where to save the data of the header
 * @return RTSP_Ok or RTSP_BadRequest if the header is malformed
 * @return an RTSP_Error with reply_code 406 if the transport header is missing
 * @return an RTSP_Error with reply_code 461 if the required transport is unsupported
 * @return an RTSP_Error with reply_code 500 if it wasn't possible to allocate a socket for the client
 */
static RTSP_Error parse_transport_header(RTSP_buffer * rtsp,
                                         RTP_transport * transport,
                                         RTP_session **rtp_s,
                                         Track *tr)
{
    RTSP_Error error;

    port_pair cli_ports;

    char *p  /* = NULL */ ;
    char transport_str[1024];

    char *saved_ptr, *transport_tkn;
    int max_interlvd;
    int rtp_ch = 0, rtcp_ch = 0;

    // Start parsing the Transport header
    if ((p = strstr(rtsp->in_buffer, HDR_TRANSPORT)) == NULL) {
        set_RTSP_Error(&error, 406, "Require: Transport settings");
        return error;
    }
    if (sscanf(p, "%*10s%1023s", transport_str) != 1) {
        fnc_log(FNC_LOG_ERR,
            "SETUP request malformed: Transport string is empty");
        return RTSP_BadRequest;
    }

    // tokenize the comma separated list of transport settings:
    if (!(transport_tkn = strtok_r(transport_str, ",", &saved_ptr))) {
        fnc_log(FNC_LOG_ERR,
            "Malformed Transport string from client");
        return RTSP_BadRequest;
    }

/*    if (getpeername(rtsp->fd, (struct sockaddr *) &rtsp_peer, &namelen) !=
        0) {
        send_reply(415, NULL, rtsp);    // Internal server error
        return ERR_GENERIC;
    }*/

    // search a good transport string
    do {
        if ((p = strstr(transport_tkn, RTSP_RTP_AVP))) {
            p += strlen(RTSP_RTP_AVP);
            if (!*p || (*p == ';') || (*p == ' ') || (!strncmp(p, "/UDP", 4))) {
    // Transport: RTP/AVP;unicast
    // Transport: RTP/AVP/UDP;unicast
                if (strstr(transport_tkn, "unicast")) {
                    if ((p = strstr(transport_tkn, "client_port"))) {
                        if (!(p = strstr(p, "=")) ||
                            !sscanf(p + 1, "%d", &(cli_ports.RTP)))
                        goto malformed;
                        if (!(p = strstr(p, "-")) ||
                            !sscanf(p + 1, "%d", &(cli_ports.RTCP)))
                        goto malformed;
                    } else continue;
                    return unicast_transport(rtsp, transport, cli_ports);
                }
    // Transport: RTP/AVP
    // Transport: RTP/AVP;multicast
    // Transport: RTP/AVP/UDP
    // Transport: RTP/AVP/UDP;multicast
                else if (*rtsp->session->resource->info->multicast) {
                    return multicast_transport(rtsp->srv, transport,
                                        rtsp->session->resource->info,
                                        tr, rtp_s);
                } else
                    continue;
                break;  // found a valid transport
    // Transport: RTP/AVP/TCP;interleaved=x-y
            } else if (Sock_type(rtsp->sock) == TCP && !strncmp(p, "/TCP", 4)) {
                if ((p = strstr(transport_tkn, "interleaved"))) {
                    if (!(p = strstr(p, "=")) ||
                        !sscanf(p + 1, "%d", &rtp_ch))
                    goto malformed;
                    if ((p = strstr(p, "-"))) {
                        if (!sscanf(p + 1, "%d", &rtcp_ch))
                            goto malformed;
                    } else
                        rtcp_ch = rtp_ch + 1;
                } else {    // search for max used interleved channel.
                    max_interlvd = -1;
		    max_interlvd = MAX(max_interlvd, rtcp_ch);
                    rtp_ch = max_interlvd + 1;
                    rtcp_ch = max_interlvd + 2;
                }

                if ((rtp_ch > 255) || (rtcp_ch > 255)) {
                    fnc_log(FNC_LOG_ERR,
                        "Interleaved channel number already reached max\n");
                    set_RTSP_Error(&error, 500, "Interleaved channel number already reached max");
                    return error;
                }

                return tcp_transport(rtsp, transport, rtp_ch, rtcp_ch);
#ifdef HAVE_LIBSCTP
            } else if (Sock_type(rtsp->sock) == SCTP &&
                       !strncmp(p, "/SCTP", 5)) {
    // Transport: RTP/AVP/SCTP;streams=x-y
                if ((p = strstr(transport_tkn, "streams"))) {
                    if (!(p = strstr(p, "=")) ||
                        !sscanf(p + 1, "%d", &rtp_ch))
                    goto malformed;
                    if ((p = strstr(p, "-"))) {
                        if (!sscanf(p + 1, "%d", &rtcp_ch))
                            goto malformed;
                    } else
                        rtcp_ch = rtp_ch + 1;
                } else {    // search for max used stream.
                    max_interlvd = -1;
		    max_interlvd = MAX(max_interlvd, rtcp_ch);
                    rtp_ch = max_interlvd + 1;
                    rtcp_ch = max_interlvd + 2;
                }
                return sctp_transport(rtsp, transport, rtp_ch, rtcp_ch);
#endif
            }
        }
    } while ((transport_tkn = strtok_r(NULL, ",", &saved_ptr)));

    // No supported transport found.
    set_RTSP_Error(&error, 461, "Unsupported Transport");
    return error;
    malformed:
    fnc_log(FNC_LOG_ERR, "Malformed Transport string from client");
    return RTSP_BadRequest;
}

/**
 * Generates a random session id
 * @return the random session id (actually a number)
 */
static guint64 generate_session_id()
{
    guint64 session_id = 0;

    while (session_id == 0) {
      session_id   = g_random_int();
      session_id <<= 32;
      session_id  |= g_random_int();
    }

    return session_id;
}

/**
 * Appends a new session to the buffer (actually only 1 session is supported)
 * @param rtsp the buffer where to add the session
 * @return The newly allocated session
 */
static RTSP_session * append_session(RTSP_buffer * rtsp)
{
    RTSP_session *rtsp_s;

    // XXX Append a new session if one isn't present already!
    if (!rtsp->session) {
        rtsp->session = g_new0(RTSP_session, 1);
    }
    rtsp_s = rtsp->session;

    return rtsp_s;
}

/**
 * sets up the RTP session
 * @param url the Url for which to generate the session
 * @param rtsp the buffer for which to generate the session
 * @param rtsp_s the RTSP session to use
 * @param transport the transport to use
 * @param track_sel the track for which to generate the session
 * @return The newly generated RTP session
 */
static RTP_session * setup_rtp_session(Url * url, RTSP_buffer * rtsp, RTSP_session * rtsp_s, RTP_transport * transport, Selector * track_sel)
{
    feng *srv = rtsp->srv;
    RTP_session *rtp_s = g_new0(RTP_session, 1);

    // Setup the RTP session
    rtsp_s->rtp_sessions = g_slist_append(rtsp_s->rtp_sessions, rtp_s);

    rtp_s->pause = 1;
    rtp_s->sd_filename = g_strdup(url->path);

    memcpy(&rtp_s->transport, transport, sizeof(RTP_transport));
    rtp_s->start_rtptime = g_random_int();
    rtp_s->start_seq = g_random_int_range(0, G_MAXUINT16);
    rtp_s->seq_no = rtp_s->start_seq - 1;
    rtp_s->track_selector = track_sel;
    rtp_s->srv = srv;
    rtp_s->sched_id = schedule_add(rtp_s);
    rtp_s->ssrc = g_random_int();

    return rtp_s;
}

/**
 * Gets the track requested for the object
 * @param url the object for which to get the requested track
 * @param rtsp_s the session where to save the addressed resource
 * @param trackname the name of the track to open
 * @param track_sel where to save the selector for the opened track
 * @param req_track where to save the data of the opened track
 * @return RTSP_Ok or RTSP_NotFound if the object or the track was not found
 * @return RTSP_InternalServerError if it was not possible to retrieve the data of the opened track
 */
static RTSP_Error select_requested_track(Url *url, RTSP_session * rtsp_s, char * trackname, Selector ** track_sel, Track ** req_track)
{
    RTSP_Error error;
    feng *srv = rtsp_s->srv;

    // it should parse the request giving us object!trackname
    if (!rtsp_s->resource) {
        if (!(rtsp_s->resource = mt_resource_open(srv, prefs_get_serv_root(), url->path))) {
            error = RTSP_NotFound;
            fnc_log(FNC_LOG_DEBUG, "Resource for %s not found\n", url->path);
            return error;
        }
    }

    if (!(*track_sel = r_open_tracks(rtsp_s->resource, trackname))) {
        error = RTSP_NotFound;
        fnc_log(FNC_LOG_DEBUG, "Track %s not present in resource %s\n",
                trackname, url->path);
        return error;
    }

    if (!(*req_track = r_selected_track(*track_sel)))
        return RTSP_InternalServerError;    // Internal server error

    return RTSP_Ok;
}

/**
 * Sends the reply for the setup method
 * @param rtsp the buffer where to write the reply
 * @param session the new RTSP session allocated for the client
 * @param rtp_s the new RTP session allocated for the client
 * @return ERR_NOERROR
 */
static int send_setup_reply(RTSP_buffer * rtsp, RTSP_session * session, RTP_session * rtp_s)
{
    GString *reply = rtsp_generate_ok_response(rtsp->rtsp_cseq, session->session_id);

    g_string_append(reply, "Transport: ");

    if (!rtp_s || !rtp_s->transport.rtp_sock)
        return ERR_GENERIC;
    switch (Sock_type(rtp_s->transport.rtp_sock)) {
    case UDP:
        if (Sock_flags(rtp_s->transport.rtp_sock)== IS_MULTICAST) {
	  g_string_append_printf(reply,
				 "RTP/AVP;multicast;ttl=%d;destination=%s;port=",
				 // session->resource->info->ttl,
				 DEFAULT_TTL,
				 session->resource->info->multicast);
        } else { // XXX handle TLS here
	  g_string_append_printf(reply,
                    "RTP/AVP;unicast;source=%s;"
                    "client_port=%d-%d;server_port=",
                    get_local_host(rtsp->sock),
                    get_remote_port(rtp_s->transport.rtp_sock),
                    get_remote_port(rtp_s->transport.rtcp_sock));
        }

	g_string_append_printf(reply, "%d-%d",
			       get_local_port(rtp_s->transport.rtp_sock),
			       get_local_port(rtp_s->transport.rtcp_sock));

        break;
    case LOCAL:
        if (Sock_type(rtsp->sock) == TCP) {
	  g_string_append_printf(reply,
				 "RTP/AVP/TCP;interleaved=%d-%d",
				 rtp_s->transport.rtp_ch,
				 rtp_s->transport.rtcp_ch);
        }
        else if (Sock_type(rtsp->sock) == SCTP) {
	  g_string_append_printf(reply,
				 "RTP/AVP/SCTP;server_streams=%d-%d",
				 rtp_s->transport.rtp_ch,
				 rtp_s->transport.rtcp_ch);
        }
        break;
    default:
        break;
    }
    g_string_append_printf(reply, ";ssrc=%08X" RTSP_EL RTSP_EL, rtp_s->ssrc);

    bwrite(reply, rtsp);

    fnc_log(FNC_LOG_CLIENT, "200 - %s ",
            r_selected_track(rtp_s->track_selector)->info->name);

    return ERR_NOERROR;
}

/**
 * RTSP SETUP method handler
 * @param rtsp the buffer for which to handle the method
 * @param new_session where to save the newly allocated RTSP session
 * @return ERR_NOERROR
 */
int RTSP_setup(RTSP_buffer * rtsp, RTSP_session ** new_session)
{
    Url url;
    guint64 session_id = 0;
    char trackname[255];
    RTP_transport transport;

    Selector *track_sel = NULL;
    Track *req_track = NULL;

    //mediathread pointers
    RTP_session *rtp_s = NULL;
    RTSP_session *rtsp_s;

    RTSP_Error error;

    // init
    memset(&transport, 0, sizeof(transport));

    // Parse the input message

    // Extract and validate the URL
    if ( (error = rtsp_extract_validate_url(rtsp, &url)).got_error )
	goto error_management;

    // Split resource!trackname
    if ( (error = split_resource_path(&url, trackname, sizeof(trackname))).got_error )
        goto error_management;

    // Get the CSeq
    if ( (error = get_cseq(rtsp)).got_error )
        goto error_management;

    // If there's a Session header we have an aggregate control
    if ( (error = get_session_id(rtsp, &session_id)).got_error )
        goto error_management;
    if (session_id == 0)
        session_id = generate_session_id();

    // Add an RTSP session if necessary
    rtsp_s = append_session(rtsp);

    // Get the selected track
    if ( (error = select_requested_track(&url, rtsp_s, trackname, &track_sel, &req_track)).got_error )
        goto error_management;

    // Parse the RTP/AVP/something string
    if ( (error = parse_transport_header(rtsp, &transport, &rtp_s, req_track)).got_error )
        goto error_management;

    // Setup the RTP session
    // XXX refactor
    if (!rtp_s)
        rtp_s = setup_rtp_session(&url, rtsp, rtsp_s, &transport, track_sel);
    else { // multicast
        rtp_s->is_multicast++;
        rtsp_s->rtp_sessions = g_slist_prepend(NULL, rtp_s);
    }

    // Setup the RTSP session
    rtsp_s->session_id = session_id;
    *new_session = rtsp_s;

    fnc_log(FNC_LOG_INFO, "SETUP %s://%s/%s RTSP/1.0 ",
	    url.protocol, url.hostname, url.path);
    if(send_setup_reply(rtsp, rtsp_s, rtp_s)) {
        set_RTSP_Error(&error, 500, "Can't write answer");
        goto error_management;
    }
    log_user_agent(rtsp); // See User-Agent

    return ERR_NOERROR;

error_management:
    send_reply(error.message.reply_code, error.message.reply_str, rtsp);
    return ERR_GENERIC;
}
