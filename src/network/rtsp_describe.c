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
 * @brief Contains DESCRIBE method and reply handlers
 */

#include <stdbool.h>

#include "rtsp.h"
#include <fenice/utils.h>
#include <fenice/prefs.h>
#include "sdp2.h"
#include <fenice/fnc_log.h>

/**
 * Sends the reply for the describe method
 * @param rtsp the buffer where to write the reply
 * @param req The client request for the method
 * @param url the URL for the resource to describe
 * @param descr the description string to send
 */
static void send_describe_reply(RTSP_buffer * rtsp, RTSP_Request *req,
                                Url *url, GString *descr)
{
    RTSP_Response *response = rtsp_response_new(req, RTSP_Ok);
    char *encoded_object;

    /* bluntly put it there */
    response->body = descr;

    /* When we're going to have more than one option, add alternatives here */
    g_hash_table_insert(response->headers,
                        g_strdup("Content-Type"),
                        g_strdup("application/sdp"));

    encoded_object = g_uri_escape_string(url->path, NULL, false);
    g_hash_table_insert(response->headers,
                        g_strdup("Content-Base"),
                        g_strdup_printf("rtsp://%s/%s/",
                                        url->hostname, encoded_object));
    g_free(encoded_object);

    rtsp_response_send(response);
}

/**
 * RTSP DESCRIBE method handler
 * @param rtsp the buffer for which to handle the method
 * @param req The client request for the method
 * @return ERR_NOERROR
 */
int RTSP_describe(RTSP_buffer * rtsp, RTSP_Request *req)
{
    RTSP_ResponseCode error;
    feng *srv = rtsp->srv;

    Url url;
    GString *descr;

    if ( !rtsp_request_get_url(rtsp, req, &url) )
        return ERR_GENERIC;

    if (srv->num_conn > srv->srvconf.max_conns) {
        /* @todo should redirect, but we haven't the code to do that just
         * yet. */
        rtsp_quick_response(req, RTSP_InternalServerError);
        return ERR_GENERIC;
    }

    // Get Session Description
    descr = sdp_session_descr(srv, url.hostname, url.path);

    /* The only error we may have here is when the file does not exist
       or if a demuxer is not available for the given file */
    if ( descr == NULL ) {
      error = RTSP_NotFound;
      goto error_management;
    }

    send_describe_reply(rtsp, req, &url, descr);

    return ERR_NOERROR;

error_management:
    rtsp_quick_response(req, error);
    return ERR_GENERIC;
}
