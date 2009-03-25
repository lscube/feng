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
 *  @brief Contains PLAY method and reply handlers
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <strings.h>

#include <liberis/headers.h>

#include "rtsp.h"
#include <fenice/prefs.h>
#include <fenice/fnc_log.h>
#include <math.h>
#include <stdbool.h>
#include <netembryo/url.h>
#include <glib.h>

extern gboolean ragel_parse_range_header(const char *header,
                                         rtp_play_args *args);

/**
 * Actually seek through the media using mediathread
 * @param rtsp_sess the session for which to start playing
 * @param args the range to play
 * @return RTSP_Ok or RTSP_InvalidRange
 */
static RTSP_ResponseCode do_seek(RTSP_session * rtsp_sess, rtp_play_args * args)
{
    Resource *r = rtsp_sess->resource;

    if (args->seek_time_valid &&
        ((rtsp_sess->started && args->begin_time == 0.0)
         || args->begin_time > 0.0)) {
        if(mt_resource_seek(r, args->begin_time)) {
            return RTSP_HeaderFieldNotValidforResource;
        }
        rtp_session_gslist_seek(rtsp_sess->rtp_sessions, args->begin_time);
    } else if (args->begin_time < 0.0) {
        fnc_log(FNC_LOG_DEBUG,"[RTSP] Negative seek to %f", args->begin_time);
        return RTSP_InvalidRange;
    }

    return RTSP_Ok;
}

/**
 * Actually starts playing the media using mediathread
 * @param url the Url for which to start playing
 * @param rtsp_sess the session for which to start playing
 * @param args the range to play
 * @return RTSP_Ok or RTSP_InternalServerError
 */
static RTSP_ResponseCode do_play(Url *url, RTSP_session * rtsp_sess, rtp_play_args * args)
{
    RTSP_ResponseCode error = RTSP_Ok;
    char *q = NULL;

    if (!(q = strchr(url->path, '='))) {
        //if '=' is not present then a file has not been specified
        // aggregate content requested
        // Perform seek if needed
        if ((error = do_seek(rtsp_sess, args)) != RTSP_Ok) {
            return error;
        }
        if ( rtsp_sess->rtp_sessions &&
             ((RTP_session*)(rtsp_sess->rtp_sessions->data))->is_multicast )
            return RTSP_Ok;

        rtsp_sess->started = 1;

        rtp_session_gslist_resume(rtsp_sess->rtp_sessions, args->start_time);
    } else {
        // FIXME complete with the other stuff
        return RTSP_InternalServerError; /* Internal server error */
#if 0
        // resource!trackname
        strcpy (trackname, q + 1);
        // XXX Not really nice...
        while (url->path != q) if (*--q == '/') break;
        *q = '\0';
#endif
    }

    return RTSP_Ok;
}

typedef struct {
    GString *str;
    char *baseurl;
} rtp_session_send_play_reply_pair;

static void rtp_session_send_play_reply(gpointer element, gpointer user_data)
{
  rtp_session_send_play_reply_pair *pair = (rtp_session_send_play_reply_pair *)user_data;

  RTP_session *p = (RTP_session *)element;
  Track *t = p->track;

  g_string_append_printf(pair->str,
			 "url=%s%s/"SDP2_TRACK_ID"=", pair->baseurl, p->sd_filename);

  g_string_append_printf(pair->str,
			 "%s;seq=%u", t->info->name, p->start_seq);

  if (t->properties->media_source != MS_live)
    g_string_append_printf(pair->str,
			   ";rtptime=%u", p->start_rtptime);

  g_string_append(pair->str, ",");
}

/**
 * @brief Sends the reply for the play method
 *
 * @param rtsp the buffer where to write the reply
 * @param req The Request to respond to.
 * @param url the Url related to the object that we wanted to play
 * @param rtsp_session the session for which to generate the reply
 * @param args The arguments to use to play
 *
 * @todo The args parameter should probably replaced by two begin_time
 *       and end_time parameters, since those are the only values used
 *       out of the structure.
 */
static void send_play_reply(RTSP_Client * rtsp, RTSP_Request *req, Url *url,
                            RTSP_session * rtsp_session, rtp_play_args * args)
{
    RTSP_Response *response = rtsp_response_new(req, RTSP_Ok);
    rtp_session_send_play_reply_pair pair = {
        .str = g_string_new(""),

        /* We create here a base URL parameter with the hostname and port.  If
         * we only use protocol and server name it won't be correct when the
         * object has a port value too.
         */
        .baseurl = g_strdup_printf("%s://%s%s%s/",
                                   url->protocol,
                                   url->hostname,
                                   url->port ? ":" : "",
                                   url->port ? url->port : "")
    };

    /* temporary string used for creating headers */
    GString *str = g_string_new("");

    /* Create Range header */
    g_string_printf(str, "npt=%f-", args->begin_time);
    if (args->end_time != HUGE_VAL)
      g_string_append_printf(str, "%f", args->end_time);

    g_hash_table_insert(response->headers,
                        g_strdup(eris_hdr_range),
                        g_string_free(str, false));

    /* Create RTP-Info header */
    g_slist_foreach(rtsp_session->rtp_sessions, rtp_session_send_play_reply, &pair);
    g_free(pair.baseurl);

    g_string_truncate(pair.str, pair.str->len-1);

    g_hash_table_insert(response->headers,
                        g_strdup(eris_hdr_rtp_info),
                        g_string_free(pair.str, false));

    rtsp_response_send(response);
}

/**
 * RTSP PLAY method handler
 * @param rtsp the buffer for which to handle the method
 * @param req The client request for the method
 */
void RTSP_play(RTSP_Client * rtsp, RTSP_Request *req)
{
    Url url;
    RTSP_session *rtsp_sess = rtsp->session;
    rtp_play_args args = {
        .playback_time = { 0, },
        .playback_time_valid = false,
        .seek_time_valid = false,
        .start_time = gettimeinseconds(NULL),
        .begin_time = 0.0,
        .end_time = rtsp_sess->resource->info->duration
    };

    const char *range_hdr;

    RTSP_ResponseCode error;

    if ( !rtsp_check_invalid_state(req, RTSP_SERVER_INIT) )
        return;

    /* Parse the Range header.
     *
     * If the parse fails, it's because the Range header specifies something we
     * don't know how to deal with (like clock or smtpe ranges) and thus we
     * respond to the client with a 501 "Not Implemented" error, as specified by
     * RFC2326 Section 12.29.
     *
     * RFC2326 only mandates server to know NPT time, clock and smtpe times are
     * optional, and we currently support neither of them.
     */
    range_hdr = g_hash_table_lookup(req->headers, "Range");
    if ( range_hdr && !ragel_parse_range_header(range_hdr, &args) ) {
        rtsp_quick_response(req, RTSP_NotImplemented);
        return;
    }

    if ( rtsp_sess->session_id == 0 ) {
        error = RTSP_BadRequest;
        goto error_management;
    }

    if ( !rtsp_request_get_url(req, &url) )
        return;

    if ( (error = do_play(&url, rtsp_sess, &args)) != RTSP_Ok ) {
        goto error_management;
    }

    send_play_reply(rtsp, req, &url, rtsp_sess, &args);

    Url_destroy(&url);

    rtsp_sess->cur_state = RTSP_SERVER_PLAYING;

    return;

error_management:
    rtsp_quick_response(req, error);
    return;
}
