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

#include "feng.h"
#include "rtp.h"
#include "rtsp.h"
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

    /* Try first the port that the client requested */
    snprintf(port_buffer, 8, "%d", rtp_port);
    transport->rtp_sock = Sock_bind(rtsp->sock->local_host, port_buffer, NULL, UDP, NULL);

    snprintf(port_buffer, 8, "%d", rtcp_port);
    transport->rtcp_sock = Sock_bind(rtsp->sock->local_host, port_buffer, NULL, UDP, NULL);

    if ( transport->rtp_sock == NULL &&
         transport->rtcp_sock == NULL ) {
        /* Both the ports the client requested are busy, we ask some
         * help from the kernel here, then we try to use it for either
         * RTCP (if odd) or RTP (if even).
         */
        Sock *firstsock = Sock_bind(rtsp->sock->local_host, NULL, NULL, UDP, NULL);
        if ( firstsock == NULL )
            return RTSP_UnsupportedTransport;

        switch ( firstsock->local_port % 2 ) {
        case 0:
            transport->rtp_sock = firstsock;
            snprintf(port_buffer, 8, "%d", firstsock->local_port+1);
            transport->rtcp_sock = Sock_bind(rtsp->sock->local_host, port_buffer, NULL, UDP, NULL);
            break;
        case 1:
            transport->rtcp_sock = firstsock;
            snprintf(port_buffer, 8, "%d", firstsock->local_port-1);
            transport->rtp_sock = Sock_bind(rtsp->sock->local_host, port_buffer, NULL, UDP, NULL);
            break;
        }
    }

    /*
     * At this point we have either both ports open, or one of the two
     * failed to open, but just one. We won't do many more trick, and
     * simply get a new socket, it doesn't matter where, let the
     * kernel find one.
     *
     * RFC 3550 Section 11 describe the choice of port numbers for RTP
     * applications; since we're delievering RTP as part of an RTSP
     * stream, we fall in the latest case described. We thus *can*
     * avoid using the even-odd adjacent ports pair for RTP-RTCP.
     */

    if ( transport->rtp_sock == NULL ) {
        if ( (transport->rtp_sock = Sock_bind(rtsp->sock->local_host, NULL, NULL, UDP, NULL)) == NULL ) {
            Sock_close(transport->rtcp_sock);
            return RTSP_UnsupportedTransport;
        }
    } else if ( transport->rtcp_sock == NULL ) {
        if ( (transport->rtcp_sock = Sock_bind(rtsp->sock->local_host, NULL, NULL, UDP, NULL)) == NULL ) {
            Sock_close(transport->rtp_sock);
            return RTSP_UnsupportedTransport;
        }
    }

    if ( Sock_connect (get_remote_host(rtsp->sock), port_buffer,
                       transport->rtp_sock, UDP, NULL) == NULL ||
         Sock_connect (get_remote_host(rtsp->sock), port_buffer,
                       transport->rtcp_sock, UDP, NULL) == NULL ) {
        Sock_close(transport->rtp_sock);
        Sock_close(transport->rtcp_sock);
        return RTSP_UnsupportedTransport;
    }

    return RTSP_Ok;
}

static gboolean interleaved_setup_transport(RTP_transport *transport,
                                            int rtp_ch, int rtcp_ch) {

    if ( rtp_ch > 255 || rtcp_ch > 255 ) {
        fnc_log(FNC_LOG_ERR,
                "Interleaved channel number already reached max\n");
        return false;
    }

    transport->rtp_ch = rtp_ch;
    transport->rtcp_ch = rtcp_ch;

    return true;
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
    rtp_t->protocol = transport->protocol;
    switch ( transport->protocol ) {
    case RTP_UDP:
        if ( transport->mode == TransportUnicast ) {
            return ( unicast_transport(rtsp, rtp_t,
                                       transport->rtp_channel,
                                       transport->rtcp_channel)
                     == RTSP_Ok );
        } else { /* Multicast */
            return false;
        }
    case RTP_TCP:
        if ( transport->rtp_channel &&
             !transport->rtcp_channel )
            transport->rtcp_channel = transport->rtp_channel + 1;

        if ( !transport->rtp_channel ) {
            /** @todo This part was surely broken before, so needs to be
             * written from scratch */
        }


        return interleaved_setup_transport(rtp_t,
                                           transport->rtp_channel,
                                           transport->rtcp_channel);
    case RTP_SCTP:
        if ( transport->rtp_channel &&
             !transport->rtcp_channel )
            transport->rtcp_channel = transport->rtp_channel + 1;

        if ( !transport->rtp_channel ) {
            /** @todo This part was surely broken before, so needs to be
             * written from scratch */
        }

        return interleaved_setup_transport(rtp_t,
                                           transport->rtp_channel,
                                           transport->rtcp_channel);

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
static Track *select_requested_track(RTSP_Client *client, RFC822_Request *req, RTSP_session *rtsp_s)
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
            rtsp_quick_response(client, req, RTSP_AggregateOnly);
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

            rtsp_quick_response(client, req, RTSP_NotFound);
            return NULL;
        }

        g_free(path);
    } else {
        /* We know the URL already */
        const size_t resource_uri_length = strlen(rtsp_s->resource_uri);

        /* Check that it starts with the correct resource URI */
        if ( strncmp(req->object, rtsp_s->resource_uri, resource_uri_length) != 0 ) {
            rtsp_quick_response(client, req, RTSP_AggregateNotAllowed);
            return NULL;
        }

        /* Now make sure that we got a presentation URL, rather than a
         * resource URL
         */
        if ( strncmp(req->object + resource_uri_length,
                     SDP_TRACK_URI_SEPARATOR,
                     strlen(SDP_TRACK_URI_SEPARATOR)) != 0 ) {
            rtsp_quick_response(client, req, RTSP_AggregateOnly);
            return NULL;
        }

        trackname = req->object
            + resource_uri_length
            + strlen(SDP_TRACK_URI_SEPARATOR);
    }

    if ( (selected_track = r_find_track(rtsp_s->resource, trackname))
         == NULL )
        rtsp_quick_response(client, req, RTSP_NotFound);

    return selected_track;
}

/**
 * Sends the reply for the setup method
 * @param rtsp the buffer where to write the reply
 * @param req The client request for the method
 * @param session the new RTSP session allocated for the client
 * @param rtp_s the new RTP session allocated for the client
 */
static void send_setup_reply(RTSP_Client *rtsp, RFC822_Request *req, RTSP_session * session, RTP_session * rtp_s)
{
    RFC822_Response *response = rfc822_response_new(req, RTSP_Ok);
    GString *transport = g_string_new("");

    switch ( rtp_s->transport.protocol ) {
    case RTP_UDP:
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
    case RTP_TCP:
        g_string_append_printf(transport,
                               "RTP/AVP/TCP;interleaved=%d-%d",
                               rtp_s->transport.rtp_ch,
                               rtp_s->transport.rtcp_ch);
        break;
    case RTP_SCTP:
        g_string_append_printf(transport,
                               "RTP/AVP/SCTP;server_streams=%d-%d",
                               rtp_s->transport.rtp_ch,
                               rtp_s->transport.rtcp_ch);
        break;
    default:
        g_assert_not_reached();
        break;
    }
    g_string_append_printf(transport, ";ssrc=%08X", rtp_s->ssrc);

    rfc822_headers_set(response->headers,
                     RTSP_Header_Transport,
                     g_string_free(transport, false));

    /* We add the Session here since it was not added by rtsp_response_new (the
     * incoming request had no session).
     */
    rfc822_headers_set(response->headers,
                    RTSP_Header_Session,
                    g_strdup(session->session_id));

    rfc822_response_send(rtsp, response);
}

/**
 * RTSP SETUP method handler
 * @param rtsp the buffer for which to handle the method
 * @param req The client request for the method
 */
void RTSP_setup(RTSP_Client *rtsp, RFC822_Request *req)
{
    const char *transport_header = NULL;
    RTP_transport transport;

    Track *req_track = NULL;

    //mediathread pointers
    RTP_session *rtp_s = NULL;
    RTSP_session *rtsp_s;

    // init
    memset(&transport, 0, sizeof(transport));

    if ( !rfc822_request_check_url(rtsp, req) )
        return;

    /* Parse the transport header through Ragel-generated state machine.
     *
     * The full documentation of the Transport header syntax is available in
     * RFC2326, Section 12.39.
     *
     * If the parsing returns false, we should respond to the client with a
     * status 461 "Unsupported Transport"
     */
    transport_header = rfc822_headers_lookup(req->headers, RTSP_Header_Transport);
    if ( transport_header == NULL ||
         !ragel_parse_transport_header(rtsp, &transport, transport_header) ) {

        rtsp_quick_response(rtsp, req, RTSP_UnsupportedTransport);
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
    if ( (req_track = select_requested_track(rtsp, req, rtsp_s)) == NULL )
        return;

    if ( !(rtp_s = rtp_session_new(rtsp, rtsp_s, &transport, req->object, req_track)) ) {
        rtsp_quick_response(rtsp, req, RTSP_InternalServerError);
        return;
    }

    send_setup_reply(rtsp, req, rtsp_s, rtp_s);

    if ( rtsp_s->cur_state == RTSP_SERVER_INIT )
        rtsp_s->cur_state = RTSP_SERVER_READY;
}
