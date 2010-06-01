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
#include "uri.h"

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

        path = g_uri_unescape_string(req->uri->path, "/");

        separator = strstr(path, SDP_TRACK_URI_SEPARATOR);

        *separator = '\0';

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

    rfc822_headers_set(response->headers,
                     RTSP_Header_Transport,
                     rtp_s->transport_string);

    /* We can forget about it it here since we now used it */
    rtp_s->transport_string = NULL;

    /* We add the Session here since it was not added by rtsp_response_new (the
     * incoming request had no session).
     */
    rfc822_headers_set(response->headers,
                    RTSP_Header_Session,
                    g_strdup(session->session_id));

    rfc822_response_send(rtsp, response);
}

static void parsed_transport_free(gpointer transport_gen,
                                  ATTR_UNUSED gpointer unused)
{
    g_slice_free(struct ParsedTransport, transport_gen);
}

/**
 * RTSP SETUP method handler
 * @param rtsp the buffer for which to handle the method
 * @param req The client request for the method
 */
void RTSP_setup(RTSP_Client *rtsp, RFC822_Request *req)
{
    const char *transport_header = NULL;
    GSList *transports;

    Track *req_track = NULL;

    //mediathread pointers
    RTP_session *rtp_s = NULL;
    RTSP_session *rtsp_s;

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
         (transports = ragel_parse_transport_header(transport_header)) == NULL ) {
        rtsp_quick_response(rtsp, req, RTSP_UnsupportedTransport);
        return;
    }

    fnc_log(FNC_LOG_INFO, "got %d transport options",
            g_slist_length(transports));

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

    if ( !(rtp_s = rtp_session_new(rtsp, req->object, req_track, transports)) ) {
        rtsp_quick_response(rtsp, req, RTSP_UnsupportedTransport);
        goto cleanup;
    }

    rtsp_s->rtp_sessions = g_slist_append(rtsp_s->rtp_sessions, rtp_s);

    send_setup_reply(rtsp, req, rtsp_s, rtp_s);

    if ( rtsp_s->cur_state == RTSP_SERVER_INIT )
        rtsp_s->cur_state = RTSP_SERVER_READY;

 cleanup:
    g_slist_foreach(transports, parsed_transport_free, NULL);
    g_slist_free(transports);
}
