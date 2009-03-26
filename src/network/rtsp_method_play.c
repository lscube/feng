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
#include "ragel_parsers.h"
#include <fenice/prefs.h>
#include <fenice/fnc_log.h>
#include <math.h>
#include <stdbool.h>
#include <netembryo/url.h>
#include <glib.h>

/**
 * Actually starts playing the media using mediathread
 *
 * @param rtsp_sess the session for which to start playing
 * @param range The parsed value of the Range: header
 *
 * @retval RTSP_Ok Operation successful
 * @retval RTSP_InvalidRange Seek couldn't be completed
 */
static RTSP_ResponseCode do_play(RTSP_session * rtsp_sess,
                                 ParsedRange *range)
{
    /* If a seek was requested, execute it */
    if ( !isnan(range->begin_time) ) {
        if ( r_seek(rtsp_sess->resource, range->begin_time) )
            return RTSP_InvalidRange;
        else
            rtp_session_gslist_seek(rtsp_sess->rtp_sessions, range->begin_time);
    }

    if ( rtsp_sess->rtp_sessions &&
         ((RTP_session*)(rtsp_sess->rtp_sessions->data))->is_multicast )
        return RTSP_Ok;

    rtsp_sess->started = 1;

    /** @todo This is always fixed to “now”, it should not be and
     *        should read the ParsedRange::playback_time value
     *        instead. */
    rtp_session_gslist_resume(rtsp_sess->rtp_sessions, gettimeinseconds(NULL));

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
 * @param req The Request to respond to.
 * @param url the Url related to the object that we wanted to play
 * @param rtsp_session the session for which to generate the reply
 * @param args The arguments to use to play
 *
 * @todo The args parameter should probably replaced by two begin_time
 *       and end_time parameters, since those are the only values used
 *       out of the structure.
 */
static void send_play_reply(RTSP_Request *req, Url *url,
                            RTSP_session * rtsp_session, ParsedRange *range)
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
    GString *str = g_string_new("npt=");

    /* Create Range header */
    if (!isnan(range->begin_time))
        g_string_append_printf(str, "%f", range->begin_time);

    g_string_append(str, "-");

    if (!isnan(range->end_time))
      g_string_append_printf(str, "%f", range->end_time);

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
 * @brief Parse the Range header
 *
 * @param rtsp_sess The session to seek for
 * @param range_hdr The range header to parse (if NULL, the default
 *                  will apply)
 * @param range The structure carrying the parsed data
 *
 * @retval RTSP_Ok The range header was parsed correctly.
 * @retval RTSP_NotImplemented The range header specified the time in
 *                             a format we don't support
 * @retval RTSP_InvalidRange The range specified was not valid
 *
 * If the parse fails, it's because the Range header specifies something we
 * don't know how to deal with (like clock or smtpe ranges) and thus we
 * respond to the client with a 501 “Not Implemented” error, as specified by
 * RFC2326 Section 12.29.
 *
 * RFC2326 only mandates server to know NPT time, clock and smtpe times are
 * optional, and we currently support neither of them.
 *
 * @todo For now the only reason why we return RTSP_InvalidRange is
 *       that the parsed values are negative. We should probably check
 *       that they also don't go over the end of the trace _if_ the
 *       stream is not a live stream.
 * @todo If the stream is live we should not even call this function,
 *       the mere presence of the range header should warrant an error
 *       response with code 456 ”Header Field Not Valid For Resource”
 *       as suggested by RFC2326 Section 11.3.7.
 */
RTSP_ResponseCode parse_range_header(RTSP_session *rtsp_sess, const char *range_hdr,
                                     ParsedRange *range)
{
    const double not_read = nan("");

    /* Initialise the ParsedRange structure by setting the three
     * values all at our nan constant. */
    range->begin_time = not_read;
    range->end_time = not_read;
    range->playback_time = not_read;

    /* If there is any kind of parsing error, the range is considered
     * not implemented. It might not be entirely certainl but until we
     * have better indications, it should be fine. */
    if ( range_hdr &&
         !ragel_parse_range_header(range_hdr, range) )
        return RTSP_NotImplemented;

    /* We don't set begin_time if it was not read, since the client
     * didn't request any seek and thus we won't do any seek to that.
     */
    if ( isnan(range->end_time) )
        range->end_time = rtsp_sess->resource->info->duration;
    if ( isnan(range->playback_time) )
        range->playback_time = gettimeinseconds(NULL);

    if ( !isnan(range->begin_time) &&
         range->begin_time < 0.0 )
        return RTSP_InvalidRange;

    /* We still check that it's not a NaN because we might want to
     * remove the fallback above, similarly to begin_time. */
    if ( !isnan(range->end_time) &&
         range->end_time < 0.0 )
        return RTSP_InvalidRange;

    /** @todo We might want to error out if the time is also
     *  impossibly back in the past */
    if ( range->playback_time < 0.0 )
        return RTSP_InvalidRange;

    return RTSP_Ok;
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
    RTSP_ResponseCode error;

    ParsedRange range;

    if ( !rtsp_check_invalid_state(req, RTSP_SERVER_INIT) )
        return;

    if ( !rtsp_request_get_url(req, &url) )
        return;

    if ( rtsp_sess->session_id == 0 ) {
        error = RTSP_BadRequest;
        goto error_management;
    }

    if ( (error = parse_range_header(rtsp->session,
                                     g_hash_table_lookup(req->headers, "Range"),
                                     &range)) != RTSP_Ok )
        goto error_management;


    if ( (error = do_play(rtsp_sess, &range)) != RTSP_Ok )
        goto error_management;

    send_play_reply(req, &url, rtsp_sess, &range);

    Url_destroy(&url);

    rtsp_sess->cur_state = RTSP_SERVER_PLAYING;

    ev_timer_again (rtsp->srv->loop, rtsp->ev_timeout);

    return;

error_management:
    Url_destroy(&url);
    rtsp_quick_response(req, error);
    return;
}
