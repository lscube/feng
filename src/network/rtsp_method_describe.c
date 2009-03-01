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
 * @param req The client request for the method
 * @param url the URL for the resource to describe
 * @param descr the description string to send
 */
static void send_describe_reply(RTSP_Request *req, GString *descr)
{
    RTSP_Response *response = rtsp_response_new(req, RTSP_Ok);
    char *encoded_object;

    /* bluntly put it there */
    response->body = descr;

    /* When we're going to have more than one option, add alternatives here */
    g_hash_table_insert(response->headers,
                        g_strdup("Content-Type"),
                        g_strdup("application/sdp"));

    /* We can trust the req->object value since we already have checked it
     * beforehand. Since the object was already escaped by the client, we just
     * repeat it as it was, saving a further escaping.
     *
     * Note: this _might_ not be what we want if we decide to redirect the
     * stream to different servers, but since we don't do that now...
     */
    g_hash_table_insert(response->headers,
                        g_strdup("Content-Base"),
                        g_strdup_printf("%s/", req->object));

    rtsp_response_send(response);
}

/**
 * RTSP DESCRIBE method handler
 * @param rtsp the buffer for which to handle the method
 * @param req The client request for the method
 */
void RTSP_describe(RTSP_buffer * rtsp, RTSP_Request *req)
{
    feng *srv = rtsp->srv;

    Url url;
    GString *descr;

    if ( !rtsp_request_get_url(req, &url) )
        return ERR_GENERIC;

    // Get Session Description
    descr = sdp_session_descr(srv, url.hostname, url.path);

    /* The only error we may have here is when the file does not exist
       or if a demuxer is not available for the given file */
    if ( descr == NULL ) {
        rtsp_quick_response(req, RTSP_NotFound);
        return;
    }

    send_describe_reply(req, descr);
}
