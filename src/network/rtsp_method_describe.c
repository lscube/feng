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

#include "fnc_log.h"
#include "rtsp.h"
#include "feng.h"
#include "media/demuxer.h"
#include "uri.h"

#define SDP_EL "\r\n"
#define DEFAULT_TTL 32

#define NTP_time(t) ((float)t + 2208988800U)

/**
 * @brief Simple pair for compound parameters in foreach functions
 *
 * This data structure is used to pass parameters along with foreach
 * functions, like @ref sdp_mdescr_private_append.
 */
typedef struct {
    /** The string to append the SDP description to */
    GString *descr;
    /** The currently-described Track object */
    Track *track;
} sdp_mdescr_append_pair;

/**
 * @brief Append media private field information to an SDP string
 *
 * @param element An sdp_field object in the list
 * @param user_data An sdp_mdescr_append_pair object
 *
 * @internal This function is only to be called by g_list_foreach().
 */
static void sdp_track_private_append(gpointer element, gpointer user_data)
{
    sdp_field *private = (sdp_field *)element;
    sdp_mdescr_append_pair *pair = (sdp_mdescr_append_pair *)user_data;

    switch (private->type) {
    case empty_field:
        g_string_append_printf(pair->descr, "%s"SDP_EL,
                               private->field);
        break;
    case fmtp:
        g_string_append_printf(pair->descr, "a=fmtp:%u %s"SDP_EL,
                               pair->track->properties.payload_type,
                               private->field);
        break;
    case rtpmap:
        g_string_append_printf(pair->descr, "a=rtpmap:%u %s"SDP_EL,
                               pair->track->properties.payload_type,
                               private->field);
        break;
    default: /* Ignore other private fields */
        break;
    }
}

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
    TrackInfo *t_info = track->info;
    GString *descr = (GString *)user_data;
    char *encoded_media_name;

    /* The following variables are used to read the data out of the
     * track pointer, without calling the same inline function
     * twice.
     */
    MediaType type;
    float frame_rate;

    /* Associative-array of media types and their SDP strings.
     *
     * Note that the space at the end is used to avoid using
     * printf-like interfaces.
     */
    static const char *const sdp_media_types[] = {
        [MP_audio] = "m=audio ",
        [MP_video] = "m=video ",
        [MP_application] = "m=application ",
        [MP_data] = "m=data ",
        [MP_control] = "m=control "
    };

    type = track->properties.media_type;

    /* MP_undef is the only case we don't handle. */
    g_assert(type != MP_undef);

    g_string_append(descr, sdp_media_types[type]);

    /// @TODO shawill: probably the transport should not be hard coded,
    /// but obtained in some way
    g_string_append_printf(descr, "%d RTP/AVP",
                           t_info->rtp_port);

    /* We assume a single payload type, it might not be the correct
     * handling, but since we currently lack some better structure. */
    g_string_append_printf(descr, " %u",
                           track->properties.payload_type);

    g_string_append(descr, SDP_EL);

    // i=*
    // c=*
    // b=*
    // k=*
    // a=*
    encoded_media_name = g_uri_escape_string(t_info->name, NULL, false);

    g_string_append_printf(descr, "a=control:"SDP_TRACK_SEPARATOR"%s"SDP_EL,
                           encoded_media_name);
    g_free(encoded_media_name);

    if ( (frame_rate = track->properties.frame_rate)
         && type == MP_video)
        g_string_append_printf(descr, "a=framerate:%f"SDP_EL,
                               frame_rate);

    /* We assume a single private list, it might not be the correct
     * handling, but since we currently lack some better structure. */
    {
        sdp_mdescr_append_pair pair = {
            .descr = descr,
            .track = track
        };
        g_slist_foreach(track->sdp_fields,
                        sdp_track_private_append,
                        &pair);
    }

    // CC licenses *
    if ( t_info->commons_deed[0] )
        g_string_append_printf(descr, "a=uriLicense:%s"SDP_EL,
                               t_info->commons_deed);
    if ( t_info->rdf_page[0] )
        g_string_append_printf(descr, "a=uriMetadata:%s"SDP_EL,
                               t_info->rdf_page);
    if ( t_info->title[0] )
        g_string_append_printf(descr, "a=title:%s"SDP_EL,
                               t_info->title);
    if ( t_info->author[0] )
        g_string_append_printf(descr, "a=author:%s"SDP_EL,
                               t_info->author);
}

/**
 * @brief Create description for an SDP session
 *
 * @param srv Pointer to the server-specific data instance.
 * @param uri URI of the resource to describe
 *
 * @return A new GString containing the complete description of the
 *         session or NULL if the resource was not found or no demuxer
 *         was found to handle it.
 */
static GString *sdp_session_descr(RTSP_Client *rtsp, RFC822_Request *req)
{
    struct feng *srv = rtsp->srv;
    URI *uri = req->uri;
    GString *descr = NULL;
    double duration;

    const char *resname;
    float currtime_float, restime_float;

    Resource *resource;
    ResourceInfo *res_info;

    char *path = g_uri_unescape_string(uri->path, "/");

    int inet_family = rtsp->local_sock->local_sa->sa_family;

    fnc_log(FNC_LOG_DEBUG, "[SDP] opening %s", path);
    if ( !(resource = r_open(srv, path)) ) {
        fnc_log(FNC_LOG_ERR, "[SDP] %s not found", path);
        g_free(path);
        return NULL;
    }
    g_free(path);

    res_info = resource->info;
    g_assert(res_info != NULL);

    descr = g_string_new("v=0"SDP_EL);

    /* Near enough approximation to run it now */
    currtime_float = NTP_time(time(NULL));
    restime_float = res_info->mtime ? NTP_time(res_info->mtime) : currtime_float;

    if ( (resname = res_info->description) == NULL )
        resname = "RTSP Session";

    /* Network type: Internet; Address type: IP4. */
    g_string_append_printf(descr, "o=- %.0f %.0f IN %s %s"SDP_EL,
                           currtime_float, restime_float,
                           inet_family == AF_INET6 ? "IP6" : "IP4",
                           uri->host);

    g_string_append_printf(descr, "s=%s"SDP_EL,
                           resname);
    // u=
    if (res_info->descrURI)
        g_string_append_printf(descr, "u=%s"SDP_EL,
                               res_info->descrURI);

    // e=
    if (res_info->email)
        g_string_append_printf(descr, "e=%s"SDP_EL,
                               res_info->email);
    // p=
    if (res_info->phone)
        g_string_append_printf(descr, "p=%s"SDP_EL,
                               res_info->phone);

    // c=
    /* Network type: Internet. */
    /* Address type: IP4. */
    g_string_append_printf(descr, "c=IN %s ",
                           inet_family == AF_INET6 ? "IP6" : "IP4");

    if(res_info->multicast[0]) {
        g_string_append_printf(descr, "%s/",
                               res_info->multicast);
        if (res_info->ttl[0])
            g_string_append_printf(descr, "%s"SDP_EL,
                                   res_info->ttl);
        else
            /* @TODO the possibility to change ttl.
             * See multicast.h, RTSP_setup.c, send_setup_reply.c*/
            g_string_append_printf(descr, "%d"SDP_EL,
                                   DEFAULT_TTL);
    } else
        g_string_append_printf(descr, "%s"SDP_EL,
                               inet_family == AF_INET6 ? "::" : "0.0.0.0");

    // b=
    // t=
    g_string_append(descr, "t=0 0"SDP_EL);
    // r=
    // z=
    // k=
    // a=
    // type attribute. We offer only broadcast
    g_string_append(descr, "a=type:broadcast"SDP_EL);
    // tool attribute. Feng promo
    /// @TODO choose a better session description
    g_string_append_printf(descr, "a=tool:%s %s Streaming Server"SDP_EL,
                           PACKAGE, VERSION);
    // control attribute. We should look if aggregate metod is supported?
    g_string_append(descr, "a=control:*"SDP_EL);

    if ((duration = res_info->duration) > 0 &&
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
