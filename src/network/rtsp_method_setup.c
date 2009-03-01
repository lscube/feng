/* *
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 *
 * feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * feng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with feng; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * */

/** @file
 * @brief Contains SETUP method and reply handlers
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdbool.h>
#include <glib.h>

#include "rtsp.h"
#include <fenice/prefs.h>
#include <fenice/fnc_log.h>
#include <fenice/schedule.h>

// XXX move in an header
void eventloop_local_callbacks(RTSP_buffer *rtsp, RTSP_interleaved *intlvd);

/**
 * Splits the path of a requested media finding the trackname and the removing
 * it from the object
 *
 * @param url the Url for which to split the object
 * @param trackname where to save the trackname removed from the object
 * @param trackname_max_len maximum length of the trackname buffer 
 *
 * @retval true No error
 * @retval false No trackname provided
 */
static gboolean split_resource_path(Url * url, char * trackname, size_t trackname_max_len)
{
    char * p;

    //if '=' is not present then a file has not been specified
    if (!(p = strchr(url->path, '=')))
        return false;
    else {
        // SETUP resource!trackname
        g_strlcpy(trackname, p + 1, trackname_max_len);
        // XXX Not really nice...
        while (url->path != p) if (*--p == '/') break;
        *p = '\0';
    }

    return true;
}

/**
 * bind&connect the socket
 */
static RTSP_ResponseCode unicast_transport(RTSP_buffer *rtsp,
                                           RTP_transport *transport,
                                           uint16_t rtp_port, uint16_t rtcp_port)
{
    char port_buffer[8];
    port_pair ser_ports;

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
    snprintf(port_buffer, 8, "%d", rtp_port);
    Sock_connect (get_remote_host(rtsp->sock), port_buffer,
                  transport->rtp_sock, UDP, NULL);
    //UDP connection for outgoing RTCP packets
    snprintf(port_buffer, 8, "%d", rtcp_port);
    Sock_connect (get_remote_host(rtsp->sock), port_buffer,
                  transport->rtcp_sock, UDP, NULL);

    if (!transport->rtp_sock)
        return RTSP_UnsupportedTransport;

    return RTSP_Ok;
}

/**
 * interleaved transport
 */

static gboolean
interleaved_transport(RTSP_buffer *rtsp, RTP_transport *transport,
                      int rtp_ch, int rtcp_ch)
{
    RTSP_interleaved *intlvd;
    Sock *sock_pair[2];

    // RTP local sockpair
    if ( Sock_socketpair(sock_pair) < 0) {
        fnc_log(FNC_LOG_ERR,
                "Cannot create AF_LOCAL socketpair for rtp\n");
        return false;
    }

    intlvd = g_new0(RTSP_interleaved, 2);

    transport->rtp_sock = sock_pair[0];
    intlvd[0].local = sock_pair[1];

    // RTCP local sockpair
    if ( Sock_socketpair(sock_pair) < 0) {
        fnc_log(FNC_LOG_ERR,
                "Cannot create AF_LOCAL socketpair for rtcp\n");
        Sock_close(transport->rtp_sock);
        Sock_close(intlvd[0].local);
        g_free(intlvd);
        return false;
    }

    transport->rtcp_sock = sock_pair[0];
    intlvd[1].local = sock_pair[1];

    // copy stream number in rtp_transport struct
    transport->rtp_ch = intlvd[0].channel = rtp_ch;
    transport->rtcp_ch = intlvd[1].channel = rtcp_ch;

    rtsp->interleaved = g_slist_prepend(rtsp->interleaved, intlvd);

    eventloop_local_callbacks(rtsp, intlvd);

    return true;
}

/**
 * @brief Structure filled by the ragel parser of the transport header.
 */
struct ParsedTransport {
    enum { TransportUDP, TransportTCP, TransportSCTP } protocol;
    //! Mode for UDP transmission, here is easier to access
    enum { TransportUnicast, TransportMulticast } mode;
    union {
        union {
            struct {
                uint16_t port_rtp;
                uint16_t port_rtcp;
            } Unicast;
            struct {
            } Multicast;
        } UDP;
        struct {
            uint16_t ich_rtp;  //!< Interleaved channel for RTP
            uint16_t ich_rtcp; //!< Interleaved channel for RTCP
        } TCP;
        struct {
            uint16_t ch_rtp;  //!< SCTP channel for RTP
            uint16_t ch_rtcp; //!< SCTP channel for RTCP
        } SCTP;
    } parameters;
};

/**
 * @brief Check the value parsed ou of a transport specification.
 *
 * @param rtsp Client from which the request arrived
 * @param transport Structure containing the transport's parameters
 */
gboolean check_parsed_transport(RTSP_buffer *rtsp, RTP_transport *rtp_t,
                                struct ParsedTransport *transport)
{
    switch ( transport->protocol ) {
    case TransportUDP:
        if ( transport->mode == TransportUnicast ) {
            return ( unicast_transport(rtsp, rtp_t, 
                                       transport->parameters.UDP.Unicast.port_rtp,
                                       transport->parameters.UDP.Unicast.port_rtcp)
                     == RTSP_Ok );
        } else { /* Multicast */
            return false;
        }
    case TransportTCP:
        if ( transport->parameters.TCP.ich_rtp &&
             !transport->parameters.TCP.ich_rtcp )
            transport->parameters.TCP.ich_rtcp = transport->parameters.TCP.ich_rtp + 1;

        if ( !transport->parameters.TCP.ich_rtp ) {
            /** @todo This part was surely broken before, so needs to be
             * written from scratch */
        }

        if ( transport->parameters.TCP.ich_rtp > 255 &&
             transport->parameters.TCP.ich_rtcp > 255 ) {
            fnc_log(FNC_LOG_ERR,
                    "Interleaved channel number already reached max\n");
            return false;
        }

        return interleaved_transport(rtsp, rtp_t,
                                     transport->parameters.TCP.ich_rtp,
                                     transport->parameters.TCP.ich_rtcp);
    case TransportSCTP:
        if ( transport->parameters.SCTP.ch_rtp &&
             !transport->parameters.SCTP.ch_rtcp )
            transport->parameters.SCTP.ch_rtcp = transport->parameters.SCTP.ch_rtp + 1;

        if ( !transport->parameters.SCTP.ch_rtp ) {
            /** @todo This part was surely broken before, so needs to be
             * written from scratch */
        }

        if ( transport->parameters.SCTP.ch_rtp > 255 &&
             transport->parameters.SCTP.ch_rtcp > 255 ) {
            fnc_log(FNC_LOG_ERR,
                    "Interleaved channel number already reached max\n");
            return false;
        }

        return interleaved_transport(rtsp, rtp_t,
                                     transport->parameters.SCTP.ch_rtp,
                                     transport->parameters.SCTP.ch_rtcp);

    default:
        return false;
    }
}

#include "ragel_transport.c"

static void rtcp_read_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    RTP_recv(w->data);
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

    rtp_s->transport.rtcp_watcher = g_new(ev_io, 1);
    rtp_s->transport.rtcp_watcher->data = rtp_s;
    ev_io_init(rtp_s->transport.rtcp_watcher, rtcp_read_cb, Sock_fd(rtp_s->transport.rtcp_sock), EV_READ);
    ev_io_start(srv->loop, rtp_s->transport.rtcp_watcher);

    return rtp_s;
}

/**
 * Gets the track requested for the object
 *
 * @param path Path of the track to select
 * @param rtsp_s the session where to save the addressed resource
 * @param trackname the name of the track to open
 * @param track_sel where to save the selector for the opened track
 * @param req_track where to save the data of the opened track
 *
 * @retval RTSP_Ok No error
 * @retval RTSP_NotFound Object or track not found
 * @retval RTSP_InternalServerError Impossible to retrieve the data of the opened
 *                                  track
 */
static RTSP_ResponseCode select_requested_track(const char *path, RTSP_session * rtsp_s, char * trackname, Selector ** track_sel, Track ** req_track)
{
    feng *srv = rtsp_s->srv;

    // it should parse the request giving us object!trackname
    if (!rtsp_s->resource) {
        if (!(rtsp_s->resource = mt_resource_open(srv, prefs_get_serv_root(), path))) {
            fnc_log(FNC_LOG_DEBUG, "Resource for %s not found\n", path);
            return RTSP_NotFound;
        }
    }

    if (!(*track_sel = r_open_tracks(rtsp_s->resource, trackname))) {
        fnc_log(FNC_LOG_DEBUG, "Track %s not present in resource %s\n",
                trackname, path);
        return RTSP_NotFound;
    }

    if (!(*req_track = r_selected_track(*track_sel)))
        return RTSP_InternalServerError;    // Internal server error

    return RTSP_Ok;
}

/**
 * Sends the reply for the setup method
 * @param rtsp the buffer where to write the reply
 * @param req The client request for the method
 * @param session the new RTSP session allocated for the client
 * @param rtp_s the new RTP session allocated for the client
 */
static void send_setup_reply(RTSP_buffer * rtsp, RTSP_Request *req, RTSP_session * session, RTP_session * rtp_s)
{
    RTSP_Response *response = rtsp_response_new(req, RTSP_Ok);
    GString *transport = g_string_new("");

    if (!rtp_s || !rtp_s->transport.rtp_sock)
        return ERR_GENERIC;
    switch (Sock_type(rtp_s->transport.rtp_sock)) {
    case UDP:
        /*
          if (Sock_flags(rtp_s->transport.rtp_sock)== IS_MULTICAST) {
	  g_string_append_printf(reply,
				 "RTP/AVP;multicast;ttl=%d;destination=%s;port=",
				 // session->resource->info->ttl,
				 DEFAULT_TTL,
				 session->resource->info->multicast);
                 } else */
        { // XXX handle TLS here
            g_string_append_printf(transport,
                    "RTP/AVP;unicast;source=%s;"
                    "client_port=%d-%d;server_port=",
                    get_local_host(rtsp->sock),
                    get_remote_port(rtp_s->transport.rtp_sock),
                    get_remote_port(rtp_s->transport.rtcp_sock));
        }

        g_string_append_printf(transport, "%d-%d",
                               get_local_port(rtp_s->transport.rtp_sock),
                               get_local_port(rtp_s->transport.rtcp_sock));
        
        break;
    case LOCAL:
        if (Sock_type(rtsp->sock) == TCP) {
            g_string_append_printf(transport,
                                   "RTP/AVP/TCP;interleaved=%d-%d",
                                   rtp_s->transport.rtp_ch,
                                   rtp_s->transport.rtcp_ch);
        }
        else if (Sock_type(rtsp->sock) == SCTP) {
            g_string_append_printf(transport,
                                   "RTP/AVP/SCTP;server_streams=%d-%d",
                                   rtp_s->transport.rtp_ch,
                                   rtp_s->transport.rtcp_ch);
        }
        break;
    default:
        break;
    }
    g_string_append_printf(transport, ";ssrc=%08X", rtp_s->ssrc);

    g_hash_table_insert(response->headers,
                        g_strdup("Transport"),
                        g_string_free(transport, false));

    /* We add the Session here since it was not added by rtsp_response_new (the
     * incoming request had no session).
     */
    g_hash_table_insert(response->headers,
                        g_strdup("Session"),
                        g_strdup(session->session_id));

    rtsp_response_send(response);
}

/**
 * RTSP SETUP method handler
 * @param rtsp the buffer for which to handle the method
 * @param req The client request for the method
 */
void RTSP_setup(RTSP_buffer * rtsp, RTSP_Request *req)
{
    Url url;
    char trackname[255];
    RTP_transport transport;

    Selector *track_sel = NULL;
    Track *req_track = NULL;

    //mediathread pointers
    RTP_session *rtp_s = NULL;
    RTSP_session *rtsp_s = rtsp->session;

    RTSP_ResponseCode error;

    char *transport_header = NULL;

    // init
    memset(&transport, 0, sizeof(transport));

    if ( !rtsp_request_get_url(req, &url) )
        return;

    /* Check if we still have space for new connections, if not, respond with a
     * 453 status (Not Enough Bandwidth), so that client knows what happened. */
    if (rtsp->srv->num_conn > rtsp->srv->srvconf.max_conns) {
        /* @todo should redirect, but we haven't the code to do that just
         * yet. */
        rtsp_quick_response(req, RTSP_NotEnoughBandwidth);
        return;
    }

    // Split resource!trackname
    if ( !split_resource_path(&url, trackname, sizeof(trackname)) ) {
        rtsp_quick_response(req, RTSP_InternalServerError);
        return;
    }

    /* Here we'd be adding a new session if we supported more than one */
    if ( rtsp->session == NULL )
        rtsp_s = rtsp_session_new(rtsp);

    // Get the selected track
    if ( (error = select_requested_track(url.path, rtsp_s, trackname, &track_sel, &req_track)) != RTSP_Ok ) {
        rtsp_quick_response(req, error);
        return;
    }

    /* Parse the transport header through Ragel-generated state machine.
     *
     * The full documentation of the Transport header syntax is available in
     * RFC2326, Section 12.39.
     *
     * If the parsing returns false, we should respond to the client with a
     * status 461 "Unsupported Transport"
     */
    transport_header = g_hash_table_lookup(req->headers, "Transport");
    if ( transport_header == NULL ||
         !ragel_parse_transport_header(rtsp, &transport, transport_header) ) {

        rtsp_quick_response(req, RTSP_UnsupportedTransport);
        return;
    }

    // Setup the RTP session
    rtp_s = setup_rtp_session(&url, rtsp, rtsp_s, &transport, track_sel);

    // Metadata Begin
#ifdef HAVE_METADATA
    // Setup Metadata Session
    if (rtsp_s->resource->metadata!=NULL)
	rtp_s->metadata = rtsp_s->resource->metadata;
    else
	rtp_s->metadata = NULL;
#endif
    // Metadata End

    send_setup_reply(rtsp, req, rtsp_s, rtp_s);

    if ( rtsp_s->cur_state == RTSP_SERVER_INIT )
        rtsp_s->cur_state = RTSP_SERVER_READY;
}
