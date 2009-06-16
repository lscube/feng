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

#include <liberis/headers.h>

#include "feng.h"
#include "rtp.h"
#include "rtsp.h"
#include "ragel_parsers.h"
#include "fnc_log.h"
#include "media/demuxer.h"

/**
 * bind&connect the socket
 */
static RTSP_ResponseCode unicast_transport(RTSP_Client *rtsp,
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
 * @brief Check the value parsed out of a transport specification.
 *
 * @param rtsp Client from which the request arrived
 * @param rtp_t The transport instance to set up with the parsed parameters
 * @param transport Structure containing the transport's parameters
 */
gboolean check_parsed_transport(RTSP_Client *rtsp, RTP_transport *rtp_t,
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

        return interleaved_setup_transport(rtsp, rtp_t,
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

        return interleaved_setup_transport(rtsp, rtp_t,
                                           transport->parameters.SCTP.ch_rtp,
                                           transport->parameters.SCTP.ch_rtcp);

    default:
        return false;
    }
}

/**
 * Gets the track requested for the object
 *
 * @param rtsp_s the session where to save the addressed resource
 *
 * @return The pointer to the requested track
 *
 * @retval NULL Unable to find the requested resource or track, or
 *              other errors. The client already received a response.
 */
static Track *select_requested_track(RTSP_Request *req, RTSP_session *rtsp_s)
{
    feng *srv = rtsp_s->srv;
    char *trackname = NULL;
    Track *selected_track = NULL;

    /* Just an extra safety to abort if we have URI and not resource
     * or vice-versa */
    g_assert( (rtsp_s->resource && rtsp_s->resource_uri) ||
              (!rtsp_s->resource && !rtsp_s->resource_uri) );

    /* Check if the requested URL is valid. If we already have a
     * session open, the resource URL has to be the same; otherwise,
     * we have to check if we're given a propr presentation URL
     * (having the SDP_TRACK_SEPARATOR string in it).
     */
    if ( !rtsp_s->resource ) {
        /* Here we don't know the URL and we have to find it out, we
         * check for the presence of the SDP_TRACK_URI_SEPARATOR */
        Url url;
        char *path;

        char *separator = strstr(req->object, SDP_TRACK_URI_SEPARATOR);

        /* If we found no separator it's a resource URI */
        if ( separator == NULL ) {
            rtsp_quick_response(req, RTSP_AggregateOnly);
            return NULL;
        }

        trackname = separator + strlen(SDP_TRACK_URI_SEPARATOR);

        /* Here we set the base resource URI, which is different from
         * the path; since the object is going to be used and freed we
         * have to dupe it here. */
        rtsp_s->resource_uri = g_strndup(req->object, separator - req->object);

        Url_init(&url, rtsp_s->resource_uri);
        path = g_uri_unescape_string(url.path, "/");
        Url_destroy(&url);

        if (!(rtsp_s->resource = r_open(srv, path))) {
            fnc_log(FNC_LOG_DEBUG, "Resource for %s not found\n", path);

            g_free(path);
            g_free(rtsp_s->resource_uri);
            rtsp_s->resource_uri = NULL;

            rtsp_quick_response(req, RTSP_NotFound);
            return NULL;
        }

        g_free(path);
    } else {
        /* We know the URL already */
        const size_t resource_uri_length = strlen(rtsp_s->resource_uri);

        /* Check that it starts with the correct resource URI */
        if ( strncmp(req->object, rtsp_s->resource_uri, resource_uri_length) != 0 ) {
            rtsp_quick_response(req, RTSP_AggregateNotAllowed);
            return NULL;
        }

        /* Now make sure that we got a presentation URL, rather than a
         * resource URL
         */
        if ( strncmp(req->object + resource_uri_length,
                     SDP_TRACK_URI_SEPARATOR,
                     strlen(SDP_TRACK_URI_SEPARATOR)) != 0 ) {
            rtsp_quick_response(req, RTSP_AggregateOnly);
            return NULL;
        }

        trackname = req->object
            + resource_uri_length
            + strlen(SDP_TRACK_URI_SEPARATOR);
    }

    if ( (selected_track = r_find_track(rtsp_s->resource, trackname))
         == NULL )
        rtsp_quick_response(req, RTSP_NotFound);

    return selected_track;
}

/**
 * Sends the reply for the setup method
 * @param rtsp the buffer where to write the reply
 * @param req The client request for the method
 * @param session the new RTSP session allocated for the client
 * @param rtp_s the new RTP session allocated for the client
 */
static void send_setup_reply(RTSP_Client * rtsp, RTSP_Request *req, RTSP_session * session, RTP_session * rtp_s)
{
    RTSP_Response *response = rtsp_response_new(req, RTSP_Ok);
    GString *transport = g_string_new("");

    if (!rtp_s->transport.rtp_sock)
        return;
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
                        g_strdup(eris_hdr_transport),
                        g_string_free(transport, false));

    /* We add the Session here since it was not added by rtsp_response_new (the
     * incoming request had no session).
     */
    g_hash_table_insert(response->headers,
                        g_strdup(eris_hdr_session),
                        g_strdup(session->session_id));

    rtsp_response_send(response);
}

/**
 * RTSP SETUP method handler
 * @param rtsp the buffer for which to handle the method
 * @param req The client request for the method
 */
void RTSP_setup(RTSP_Client * rtsp, RTSP_Request *req)
{
    const char *transport_header = NULL;
    RTP_transport transport;

    Track *req_track = NULL;

    //mediathread pointers
    RTP_session *rtp_s = NULL;
    RTSP_session *rtsp_s;

    // init
    memset(&transport, 0, sizeof(transport));

    if ( !rtsp_request_check_url(req) )
        return;

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

    /* Check if we still have space for new connections, if not, respond with a
     * 453 status (Not Enough Bandwidth), so that client knows what happened. */
    if (rtsp->srv->connection_count > rtsp->srv->srvconf.max_conns) {
        /* @todo should redirect, but we haven't the code to do that just
         * yet. */
        rtsp_quick_response(req, RTSP_NotEnoughBandwidth);
        return;
    }

    /* Here we'd be adding a new session if we supported more than
     * one, and the user didn't provide one. */
    if ( (rtsp_s = rtsp->session) == NULL )
        rtsp_s = rtsp_session_new(rtsp);

    /* Get the selected track; if the URI was invalid or the track
     * couldn't be found, the function will take care of sending out
     * the error response, so we don't need to do anything else.
     */
    if ( (req_track = select_requested_track(req, rtsp_s)) == NULL )
        return;

    if ( !(rtp_s = rtp_session_new(rtsp, rtsp_s, &transport, req->object, req_track)) ) {
        rtsp_quick_response(req, RTSP_InternalServerError);
        return;
    }

    send_setup_reply(rtsp, req, rtsp_s, rtp_s);

    if ( rtsp_s->cur_state == RTSP_SERVER_INIT )
        rtsp_s->cur_state = RTSP_SERVER_READY;
}
