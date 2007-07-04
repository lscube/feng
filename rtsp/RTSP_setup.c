/* * 
 *  $Id$
 *  
 *  This file is part of Feng
 *
 *  Feng -- Standard Streaming Server
 *
 *  Copyright (C) 2007 by
 *      
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
 *    - Eugenio Menegatti     <m.eu@libero.it>
 *    - Stefano Cau
 *    - Giuliano Emma
 *    - Stefano Oldrini
 *    - Dario Gallucci    <dario.gallucci@gmail.com>
 * 
 *  Feng is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Feng is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Feng; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

/** @file RTSP_setup.c
 * @brief Contains SETUP method and reply handlers
 */

#include <glib.h>

#include <fenice/rtsp.h>
#include <fenice/prefs.h>
#include <fenice/multicast.h>
#include <fenice/fnc_log.h>
#include <fenice/schedule.h>
#include <gcrypt.h>
#include <RTSP_utils.h>

extern schedule_list sched[ONE_FORK_MAX_CONNECTION];


/**
 * Splits the path of a requested media finding the trackname and the removing it from the object
 * @param cinfo the connection for which to split the object
 * @param trackname where to save the trackname removed from the object
 * @param trackname_max_len maximum length of the trackname buffer 
 * @return RTSP_Ok or RTSP_InternalServerError if there was no trackname
 */
static RTSP_Error split_resource_path(ConnectionInfo * cinfo, char * trackname, size_t trackname_max_len)
{
    char * p;

    //if '!' is not present then a file has not been specified
    if (!(p = strchr(cinfo->object, '!')))
        return RTSP_InternalServerError;
    else {
        // SETUP resource!trackname
        strncpy (trackname, p + 1, trackname_max_len);
        // XXX Not really nice...
        while (cinfo->object != p) if (*--p == '/') break;
        *p = '\0';
    }

    return RTSP_Ok;
}

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
                                         RTP_session **rtp_s)
{
    RTSP_Error error;

    port_pair cli_ports;
    port_pair ser_ports;

    char *p  /* = NULL */ ;
    char transport_str[1024];

    char *saved_ptr, *transport_tkn;
    int max_interlvd;
    Sock *sock_pair[2];
    RTSP_interleaved *intlvd, *ilvd_s;

    // Start parsing the Transport header
    if ((p = strstr(rtsp->in_buffer, HDR_TRANSPORT)) == NULL) {
        set_RTSP_Error(&error, 406, "Require: Transport settings"); /* Not Acceptable */
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
        send_reply(415, 0, rtsp);    // Internal server error
        return ERR_GENERIC;
    }*/

//    transport->type = RTP_no_transport;
    do {            // search a good transport string
        if ((p = strstr(transport_tkn, RTSP_RTP_AVP))) {// Transport: RTP/AVP
            p += strlen(RTSP_RTP_AVP);
            if (!*p || (*p == ';') || (*p == ' ') || (!strncmp(p, "/UDP", 4))) {
                if (strstr(transport_tkn, "unicast")) {
                    char port_buffer[8];
                    if ((p = strstr(transport_tkn, "client_port"))) {
                        p = strstr(p, "=");
                        sscanf(p + 1, "%d", &(cli_ports.RTP));
                        p = strstr(p, "-");
                        sscanf(p + 1, "%d", &(cli_ports.RTCP));
                    }
                    if (RTP_get_port_pair(&ser_ports) != ERR_NOERROR) {
                        return RTSP_InternalServerError;
                    }
                    //UDP bind for outgoing RTP packets
                    snprintf(port_buffer, 8, "%d", ser_ports.RTP);
                    transport->rtp_sock = Sock_bind(NULL, port_buffer, UDP, 0);
                    //UDP bind for outgoing RTCP packets
                    snprintf(port_buffer, 8, "%d", ser_ports.RTCP);
                    transport->rtcp_sock = Sock_bind(NULL, port_buffer, UDP, 0);
                    //UDP connection for outgoing RTP packets
                    snprintf(port_buffer, 8, "%d", cli_ports.RTP);
                    Sock_connect (get_remote_host(rtsp->sock), port_buffer,
                                  transport->rtp_sock, UDP, 0);
                    //UDP connection for outgoing RTCP packets
                    snprintf(port_buffer, 8, "%d", cli_ports.RTCP);
                    Sock_connect (get_remote_host(rtsp->sock), port_buffer,
                                  transport->rtcp_sock, UDP, 0);
                }
                else if (*rtsp->session_list->resource->info->multicast) {
                    char port_buffer[8];
                    RTSP_session *s = rtsp->session_list;
                    *rtp_s=NULL;
                    int i;
                    for (i = 0; !*rtp_s && i<ONE_FORK_MAX_CONNECTION; ++i) {
                        pthread_mutex_lock(&sched[i].mux);
                        if (sched[i].valid) {
                            if (!strncmp(sched[i].rtp_session->sd_filename,
                                         s->resource->info->mrl, 255)) {
                                *rtp_s = sched[i].rtp_session;
                                fnc_log(FNC_LOG_DEBUG,
                                        "Found multicast instance.");
                        }
                        pthread_mutex_unlock(&sched[i].mux);
                    }
                    if ((p = strstr(transport_tkn, "client_port"))) {
                        p = strstr(p, "=");
                        sscanf(p + 1, "%d", &(cli_ports.RTP));
                        p = strstr(p, "-");
                        sscanf(p + 1, "%d", &(cli_ports.RTCP));
                    }
                    
                    if (!*rtp_s) {
                        snprintf(port_buffer, 8, "%d",
                                 s->resource->info->port);
                        transport->rtp_sock =
                            Sock_connect(s->resource->info->multicast,
                            port_buffer, NULL, UDP, 0);
                        snprintf(port_buffer, 8, "%d",
                                 s->resource->info->port+1);
                        transport->rtcp_sock =
                            Sock_connect(s->resource->info->multicast,
                            port_buffer, transport->rtcp_sock, UDP, 0);
                    }
                        fnc_log(FNC_LOG_DEBUG,"Multicast socket set");
                    }
                } else 
                    continue;
                break;    // found a valid transport
            } else if (Sock_type(rtsp->sock) == TCP && !strncmp(p, "/TCP", 4)) {    // Transport: RTP/AVP/TCP;interleaved=x-y

                if ( !(intlvd = calloc(1, sizeof(RTSP_interleaved))) ) {
                    set_RTSP_Error(&error, 500, "");
                    return error;
                }

                if ((p = strstr(transport_tkn, "interleaved"))) {
                    p = strstr(p, "=");
                    sscanf(p + 1, "%u", &(intlvd->proto.tcp.rtp_ch));
                    if ((p = strstr(p, "-")))
                        sscanf(p + 1, "%u", &(intlvd->proto.tcp.rtcp_ch));
                    else
                        intlvd->proto.tcp.rtcp_ch = 
                                                intlvd->proto.tcp.rtp_ch + 1;
                } else {    // search for max used interleved channel.
                    max_interlvd = -1;
                    for (ilvd_s = (rtsp->interleaved);
                         ilvd_s;
                         ilvd_s = ilvd_s->next)
                        max_interlvd = max(max_interlvd,
                                           ilvd_s->proto.tcp.rtcp_ch);
                    intlvd->proto.tcp.rtp_ch = max_interlvd + 1;
                    intlvd->proto.tcp.rtcp_ch = max_interlvd + 2;
                }
                if ((intlvd->proto.tcp.rtp_ch > 255) ||
                    (intlvd->proto.tcp.rtcp_ch > 255)) {
                    fnc_log(FNC_LOG_ERR,
                        "Interleaved channel number already reached max\n");
                    set_RTSP_Error(&error, 500, "Interleaved channel number already reached max");
                    free(intlvd);
                    return error;
                }
                // RTP local sockpair
                if ( Sock_socketpair(sock_pair) < 0) {
                    fnc_log(FNC_LOG_ERR,
                            "Cannot create AF_LOCAL socketpair for rtp\n");
                    set_RTSP_Error(&error, 500, "");
                    free(intlvd);
                    return error;
                }
                transport->rtp_sock = sock_pair[0];
                intlvd->rtp_local = sock_pair[1];
                // RTCP local sockpair
                if ( Sock_socketpair(sock_pair) < 0) {
                    fnc_log(FNC_LOG_ERR,
                            "Cannot create AF_LOCAL socketpair for rtcp\n");
                    set_RTSP_Error(&error, 500, "");
                    Sock_close(transport->rtp_sock);
                    Sock_close(intlvd->rtp_local);
                    free(intlvd);
                    return error;
                }

                transport->rtcp_sock = sock_pair[0];
                intlvd->rtcp_local = sock_pair[1];

                // copy stream number in rtp_transport struct
                transport->rtp_ch = intlvd->proto.tcp.rtp_ch;
                transport->rtcp_ch = intlvd->proto.tcp.rtcp_ch;

                // insert new interleaved stream in the list
                intlvd->next = rtsp->interleaved;
                rtsp->interleaved = intlvd;

                break;    // found a valid transport
#ifdef HAVE_LIBSCTP
            } else if (Sock_type(rtsp->sock) == SCTP &&
                       !strncmp(p, "/SCTP", 5)) {
                // Transport: RTP/AVP/SCTP;streams=x-y
                if ( !(intlvd = calloc(1, sizeof(RTSP_interleaved))) ) {
                    set_RTSP_Error(&error, 500, "");
                    return error;
                }

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
                } else {    // search for max used stream.
                    max_interlvd = -1;
                    for (ilvd_s = (rtsp->interleaved);
                         ilvd_s;
                         ilvd_s = ilvd_s->next)
                        max_interlvd = max(max_interlvd,
                                        ilvd_s->proto.sctp.rtcp.sinfo_stream);
                    intlvd->proto.sctp.rtp.sinfo_stream = max_interlvd + 1;
                    intlvd->proto.sctp.rtcp.sinfo_stream = max_interlvd + 2;
                }
                if (
                    !(
                    (intlvd->proto.sctp.rtp.sinfo_stream < MAX_SCTP_STREAMS) &&
                    (intlvd->proto.sctp.rtcp.sinfo_stream < MAX_SCTP_STREAMS)
                    )) {
                    fnc_log(FNC_LOG_ERR, "Stream id over limit\n");
                    set_RTSP_Error(&error, 500, "Stream id over limit");
                    free(intlvd);
                    return error;
                }
                // RTP local sockpair
                if ( Sock_socketpair(sock_pair) < 0) {
                    fnc_log(FNC_LOG_ERR,
                            "Cannot create AF_LOCAL socketpair for rtp\n");
                    set_RTSP_Error(&error, 500, "");
                    free(intlvd);
                    return error;
                }
                transport->rtp_sock = sock_pair[0];
                intlvd->rtp_local = sock_pair[1];
                // RTCP local sockpair
                if ( Sock_socketpair(sock_pair) < 0) {
                    fnc_log(FNC_LOG_ERR,
                            "Cannot create AF_LOCAL socketpair for rtcp\n");
                    set_RTSP_Error(&error, 500, "");
                    Sock_close(transport->rtp_sock);
                    Sock_close(intlvd->rtp_local);
                    free(intlvd);
                    return error;
                }

                transport->rtcp_sock = sock_pair[0];
                intlvd->rtcp_local = sock_pair[1];

                // copy stream number in rtp_transport struct
                transport->rtp_ch = intlvd->proto.sctp.rtp.sinfo_stream;
                transport->rtcp_ch = intlvd->proto.sctp.rtcp.sinfo_stream;

                // insert new interleaved stream in the list
                intlvd->next = rtsp->interleaved;
                rtsp->interleaved = intlvd;

                break;    // found a valid transport
#endif
            }
        }
    } while ((transport_tkn = strtok_r(NULL, ",", &saved_ptr)));

    // printf("rtp transport: %d\n", transport->type);
    if (!transport->rtp_sock) {
        // fnc_log(FNC_LOG_ERR,"Unsupported Transport\n");
        set_RTSP_Error(&error, 461, "Unsupported Transport");
        return error;
    }

    return RTSP_Ok;
}

/**
 * Generates a random session id
 * @return the random session id (actually a number)
 */
static long int generate_session_id()
{        
    struct timeval now_tmp;
    long int session_id;

    // Generate a random Session number
    gettimeofday(&now_tmp, 0);
    srand((now_tmp.tv_sec * 1000) + (now_tmp.tv_usec / 1000));
#ifdef WIN32
    session_id = rand();
#else
    session_id = 1 + (int) (10.0 * rand() / (100000 + 1.0));
#endif
    if (session_id == 0) {
        session_id++;
    }
#if 0 // To support multiple session per socket...
    // XXX Paranoia make sure we have a connection limit set elsewhere...
    while (rtsp_session_from_id(rtsp, session_id))
#ifdef WIN32
        session_id = rand();
#else
        session_id = 1 + (int) (10.0 * rand() / (100000 + 1.0));
#endif

#endif

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
#if 1
    if (!rtsp->session_list) {
        rtsp->session_list = calloc(1, sizeof(RTSP_session));
    }
    rtsp_s = rtsp->session_list;
#else // To support multiple session per socket...
    if (!rtsp->session_list) {
        rtsp->session_list = calloc(1, sizeof(RTSP_session));
        rtsp_s = rtsp->session_list;
    } else {
        RTSP_session *rtsp_s_prec;
        for (rtsp_s = rtsp->session_list; rtsp_s != NULL;
             rtsp_s = rtsp_s->next) {
            rtsp_s_prec = rtsp_s;
        }
        rtsp_s_prec->next = calloc(1, sizeof(RTP_session));
        rtsp_s = rtsp_s_prec->next;
    }
#endif

    return rtsp_s;
}

/**
 * sets up the RTP session
 * @param cinfo the object for which to generate the session
 * @param rtsp the buffer for which to generate the session
 * @param rtsp_s the RTSP session to use
 * @param transport the transport to use
 * @param track_sel the track for which to generate the session
 * @return The newly generated RTP session
 */
static RTP_session * setup_rtp_session(ConnectionInfo * cinfo, RTSP_buffer * rtsp, RTSP_session * rtsp_s, RTP_transport * transport, Selector * track_sel)
{
    struct timeval now_tmp;
    unsigned int start_seq, start_rtptime;
    RTP_session *rtp_s;

// Setup the RTP session
    if (rtsp->session_list->rtp_session == NULL) {
        rtsp->session_list->rtp_session = calloc(1, sizeof(RTP_session));
        rtp_s = rtsp->session_list->rtp_session;
    } else {
        rtp_s = rtsp_s->rtp_session;
        while (rtp_s->next !=  NULL) {
            rtp_s = rtp_s->next;
        };
        rtp_s->next = calloc(1, sizeof(RTP_session));
        rtp_s = rtp_s->next;
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
    //XXX use strdup
    strncpy(rtp_s->sd_filename, cinfo->object, sizeof(rtp_s->sd_filename));

    gettimeofday(&now_tmp, 0);
    srand((now_tmp.tv_sec * 1000) + (now_tmp.tv_usec / 1000));
    rtp_s->start_rtptime = start_rtptime;
    rtp_s->start_seq = start_seq;
    memcpy(&rtp_s->transport, transport, sizeof(RTP_transport));
    rtp_s->track_selector = track_sel;
    rtp_s->sched_id = schedule_add(rtp_s);
    gcry_randomize(&rtp_s->ssrc, sizeof(rtp_s->ssrc), GCRY_STRONG_RANDOM);

    return rtp_s;
}

/**
 * Gets the track requested for the object
 * @param cinfo the object for which to get the requested track
 * @param rtsp_s the session where to save the addressed resource
 * @param trackname the name of the track to open
 * @param track_sel where to save the selector for the opened track
 * @param req_track where to save the data of the opened track
 * @return RTSP_Ok or RTSP_NotFound if the object or the track was not found
 * @return RTSP_InternalServerError if it was not possible to retrieve the data of the opened track
 */
static RTSP_Error select_requested_track(ConnectionInfo * cinfo, RTSP_session * rtsp_s, char * trackname, Selector ** track_sel, Track ** req_track)
{
    RTSP_Error error;

    // it should parse the request giving us object!trackname
    if (!rtsp_s->resource) {
        if (!(rtsp_s->resource = mt_resource_open(prefs_get_serv_root(), cinfo->object))) {
            error = RTSP_NotFound;
            fnc_log(FNC_LOG_DEBUG, "Resource for %s not found\n", cinfo->object);
            return error;
        }
    }

    if (!(*track_sel = r_open_tracks(rtsp_s->resource, trackname, NULL))) {
        error = RTSP_NotFound;
        fnc_log(FNC_LOG_DEBUG, "Track %s not present in resource %s\n",
                trackname, cinfo->object);
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
    char r[1024];
    int w_pos = 0;

    snprintf(r, sizeof(r),
        "%s %d %s" RTSP_EL "CSeq: %d" RTSP_EL "Server: %s/%s" RTSP_EL,
        RTSP_VER, 200, get_stat(200), rtsp->rtsp_cseq, PACKAGE,
        VERSION);
    add_time_stamp(r, 0);
    w_pos = strlen(r);
    w_pos += snprintf(r + w_pos, sizeof(r) - w_pos, "Session: %d" RTSP_EL,
                      session->session_id);

    w_pos += snprintf(r + w_pos, sizeof(r) - w_pos, "Transport: ");
    if (!rtp_s || !rtp_s->transport.rtp_sock)
        return ERR_GENERIC;
    switch (Sock_type(rtp_s->transport.rtp_sock)) {
    case UDP:
        if (Sock_flags(rtp_s->transport.rtp_sock)== IS_MULTICAST) {
            w_pos += sprintf(r + w_pos,
                    "RTP/AVP;multicast;ttl=%d;destination=%s;port=",
//                    session->resource->info->ttl,
                    DEFAULT_TTL,
                    session->resource->info->multicast);
        } else { // XXX handle TLS here
            w_pos += snprintf(r + w_pos, sizeof(r) - w_pos,
                    "RTP/AVP;unicast;client_port=%d-%d;source=%s;server_port=",
                    get_remote_port(rtp_s->transport.rtp_sock),
                    get_remote_port(rtp_s->transport.rtcp_sock),
                    get_local_host(rtsp->sock));
        }

        w_pos +=
            snprintf(r + w_pos, sizeof(r) - w_pos, "%d-%d",
                get_local_port(rtp_s->transport.rtp_sock),
                get_local_port(rtp_s->transport.rtcp_sock));

        break;
    case LOCAL:
        if (Sock_type(rtsp->sock) == TCP) {
            w_pos += snprintf(r + w_pos, sizeof(r) - w_pos,
                 "RTP/AVP/TCP;interleaved=%d-%d",
                 rtp_s->transport.rtp_ch,
                 rtp_s->transport.rtcp_ch);
        }
        else if (Sock_type(rtsp->sock) == SCTP) {
            w_pos += snprintf(r + w_pos, sizeof(r) - w_pos,
                 "RTP/AVP/SCTP;server_streams=%d-%d",
                 rtp_s->transport.rtp_ch,
                 rtp_s->transport.rtcp_ch);
        }
        break;
    default:
        break;
    }
    snprintf(r + w_pos, sizeof(r) - w_pos, ";ssrc=%u", rtp_s->ssrc);

    strcat(r, RTSP_EL RTSP_EL);

    bwrite(r, (unsigned short) strlen(r), rtsp);

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
    ConnectionInfo cinfo;
    char url[255];
    long int session_id = -1;
    char trackname[255];
    RTP_transport transport;

    Selector *track_sel;
    Track *req_track;

    //mediathread pointers
    RTP_session *rtp_s = NULL;
    RTSP_session *rtsp_s;

    RTSP_Error error;

    // init
    memset(&transport, 0, sizeof(transport));

    // Parse the input message
    
    // Extract the URL
    if ( (error = extract_url(rtsp, url)).got_error )
	goto error_management;

    // Validate URL
    if ( (error = validate_url(url, &cinfo)).got_error )
    	goto error_management;

    // Check for Forbidden Paths
    if ( (error = check_forbidden_path(&cinfo)).got_error )
    	goto error_management;

    // Split resource!trackname
    if ( (error = split_resource_path(&cinfo, trackname, sizeof(trackname))).got_error )
        goto error_management;
    
    // Get the CSeq
    if ( (error = get_cseq(rtsp)).got_error ) 
        goto error_management;

    // If there's a Session header we have an aggregate control
    if ( (error = get_session_id(rtsp, &session_id)).got_error )
        goto error_management;
    if (session_id == -1)
        session_id = generate_session_id();

    // Add an RTSP session if necessary
    rtsp_s = append_session(rtsp);

    // Get the selected track
    if ( (error = select_requested_track(&cinfo, rtsp_s, trackname, &track_sel, &req_track)).got_error )
        goto error_management;

    // Parse the RTP/AVP/something string
    if ( (error = parse_transport_header(rtsp, &transport, &rtp_s)).got_error )
        goto error_management;

    // Setup the RTP session
    if (!rtp_s)
        rtp_s = setup_rtp_session(&cinfo, rtsp, rtsp_s, &transport, track_sel);

    // Setup the RTSP session
    rtsp_s->session_id = session_id;
    *new_session = rtsp_s;

    fnc_log(FNC_LOG_INFO, "SETUP %s RTSP/1.0 ", url);
    if(send_setup_reply(rtsp, rtsp_s, rtp_s)) {
        set_RTSP_Error(&error, 500, "Can't write answer");
        goto error_management;
    }
    log_user_agent(rtsp); // See User-Agent 

    return ERR_NOERROR;

error_management:
    send_reply(error.message.reply_code, error.message.reply_str, rtsp);
    return ERR_NOERROR;
}
