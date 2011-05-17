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

#include <config.h>

#include <stdbool.h>
#include <math.h>

#include "fnc_log.h"
#include "rtsp.h"
#include "feng.h"
#include "media/demuxer.h"
#include "uri.h"

#define SDP_EL "\r\n"
#define DEFAULT_TTL 32

#define NTP_time(t) ((float)t + 2208988800U)

/**
 * @brief Append the description for a given track to an SDP
 *        description.
 *
 * @param element Track instance to get information from
 * @param user_data GString instance to append the description to
 *
 * @internal This function is only to be called by g_list_foreach().
 */
static void sdp_track_descr(gpointer element, gpointer user_data)
{
    Track *track = (Track *)element;
    GString *descr = (GString *)user_data;

    /* The following variables are used to read the data out of the
     * track pointer, without calling the same inline function
     * twice.
     */
    MediaType type;

    /* Associative-array of media types and their SDP strings.
     *
     * Note that the space at the end is used to avoid using
     * printf-like interfaces.
     */
    static const char *const sdp_media_types[] = {
        [MP_audio] = "m=audio",
        [MP_video] = "m=video",
        [MP_application] = "m=application",
        [MP_data] = "m=data",
        [MP_control] = "m=control"
    };

    type = track->media_type;

    /* MP_undef is the only case we don't handle. */
    g_assert(type != MP_undef);

    /*
     * @TODO shawill: probably the transport should not be hard coded,
     * but obtained in some way
     *
     * We assume a single payload type, it might not be the correct
     * handling, but since we currently lack some better structure. */
    g_string_append_printf(descr, "%s 0 RTP/AVP %u",
                           sdp_media_types[type],
                           track->payload_type);

    g_string_append(descr, SDP_EL);

    g_string_append(descr, track->sdp_description->str);
}

/**
 * @brief Create description for an SDP session
 *
 * @param uri URI of the resource to describe
 *
 * @return A new GString containing the complete description of the
 *         session or NULL if the resource was not found or no demuxer
 *         was found to handle it.
 */
static GString *sdp_session_descr(RTSP_Client *rtsp, RFC822_Request *req)
{
    URI *uri = req->uri;
    GString *descr = NULL;
    double duration;

    float currtime_float, restime_float;

    Resource *resource;

    char *path;

    const char *inet_family;

    if ( rtsp->peer_sa == NULL ) {
        fnc_log(FNC_LOG_ERR, "unable to identify address family for connection");
        return NULL;
    }

    inet_family = rtsp->peer_sa->sa_family == AF_INET6 ? "IP6" : "IP4";
    path = g_uri_unescape_string(uri->path, "/");

    fnc_log(FNC_LOG_DEBUG, "[SDP] opening %s", path);
    if ( !(resource = r_open(path)) ) {
        fnc_log(FNC_LOG_ERR, "[SDP] %s not found", path);
        g_free(path);
        return NULL;
    }
    g_free(path);

    descr = g_string_new("v=0"SDP_EL);

    /* Near enough approximation to run it now */
    currtime_float = NTP_time(time(NULL));
    restime_float = resource->mtime ? NTP_time(resource->mtime) : currtime_float;

    /* Network type: Internet; Address type: IP4. */
    g_string_append_printf(descr, "o=- %.0f %.0f IN %s %s"SDP_EL,
                           currtime_float, restime_float,
                           inet_family,
                           uri->host);

    /* We might want to provide a better name */
    g_string_append(descr, "s=RTSP Session\r\n");

    g_string_append_printf(descr,
                           "c=IN %s %s"SDP_EL,
                           inet_family,
                           rtsp->local_host);

    g_string_append(descr, "t=0 0"SDP_EL);

    // type attribute. We offer only broadcast
    g_string_append(descr, "a=type:broadcast"SDP_EL);

    /* Server signature; the same as the Server: header */
    g_string_append_printf(descr, "a=tool:%s"SDP_EL,
                           feng_signature);

    // control attribute. We should look if aggregate metod is supported?
    g_string_append(descr, "a=control:*"SDP_EL);

    if ((duration = resource->duration) > 0 &&
        duration != HUGE_VAL)
        g_string_append_printf(descr, "a=range:npt=0-%f"SDP_EL, duration);

    g_list_foreach(resource->tracks,
                   sdp_track_descr,
                   descr);

    r_close(resource);

    fnc_log(FNC_LOG_INFO, "[SDP] description:\n%s", descr->str);

    return descr;
}

/**
 * RTSP DESCRIBE method handler
 * @param rtsp the buffer for which to handle the method
 * @param req The client request for the method
 */
void RTSP_describe(RTSP_Client *rtsp, RFC822_Request *req)
{
    GString *descr;

    if ( !rfc822_request_check_url(rtsp, req) )
        return;

    // Get Session Description
    descr = sdp_session_descr(rtsp, req);

    /* The only error we may have here is when the file does not exist
       or if a demuxer is not available for the given file */
    if ( descr == NULL ) {
        rtsp_quick_response(rtsp, req, RTSP_NotFound);
    } else {
        RFC822_Response *response = rfc822_response_new(req, RTSP_Ok);

        /* bluntly put it there */
        response->body = descr;

        /* When we're going to have more than one option, add alternatives here */
        rfc822_headers_set(response->headers,
                           RTSP_Header_Content_Type,
                           g_strdup("application/sdp"));

        /* We can trust the req->object value since we already have checked it
         * beforehand. Since the object was already escaped by the client, we just
         * repeat it as it was, saving a further escaping.
         *
         * Note: this _might_ not be what we want if we decide to redirect the
         * stream to different servers, but since we don't do that now...
         */
        rfc822_headers_set(response->headers,
                           RTSP_Header_Content_Base,
                           g_strdup_printf("%s/", req->object));

        rfc822_response_send(rtsp, response);
    }
}
