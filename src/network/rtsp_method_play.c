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

#include <glib.h>
#include <math.h>
#include <stdbool.h>

#include "feng.h"
#include "rtsp.h"
#include "rtp.h"
#include "fnc_log.h"
#include "media/demuxer.h"

/**
 * Actually starts playing the media using mediathread
 *
 * @param rtsp_sess the session for which to start playing
 *
 * @retval RTSP_Ok Operation successful
 * @retval RTSP_InvalidRange Seek couldn't be completed
 */
static RTSP_ResponseCode do_play(RTSP_session * rtsp_sess)
{
    RTSP_Range *range = g_queue_peek_head(rtsp_sess->play_requests);

    /* Don't try to seek if the source is not seekable;
     * parse_range_header() would have already ensured the range is
     * valid for the resource, and in particular ensured that if the
     * resource is not seekable we only have the “0-” range selected.
     */
    if ( rtsp_sess->resource->seekable &&
         r_seek(rtsp_sess->resource, range->begin_time) )
        return RTSP_InvalidRange;

    rtsp_sess->cur_state = RTSP_SERVER_PLAYING;

    if ( rtsp_sess->rtp_sessions &&
         ((RTP_session*)(rtsp_sess->rtp_sessions->data))->multicast )
        return RTSP_Ok;

    rtsp_sess->started = 1;

    fnc_log(FNC_LOG_VERBOSE, "resuming with parameters %f %f %f",
            range->begin_time, range->end_time, range->playback_time);
    rtp_session_gslist_resume(rtsp_sess->rtp_sessions, range);

    return RTSP_Ok;
}

static void rtp_session_send_play_reply(gpointer element, gpointer user_data)
{
  GString *str = (GString *)user_data;

  RTP_session *p = (RTP_session *)element;
  Track *t = p->track;

  g_string_append_printf(str, "url=%s;seq=1",
                         p->uri);

  if (t->properties.media_source != LIVE_SOURCE)
    g_string_append_printf(str,
			   ";rtptime=%u", p->start_rtptime);

  g_string_append(str, ",");
}

/**
 * @brief Sends the reply for the play method
 *
 * @param req The Request to respond to.
 * @param rtsp_session the session for which to generate the reply
 * @param args The arguments to use to play
 *
 * @todo The args parameter should probably replaced by two begin_time
 *       and end_time parameters, since those are the only values used
 *       out of the structure.
 */
static void send_play_reply(RTSP_Client *client,
                            RFC822_Request *req,
                            RTSP_session * rtsp_session)
{
    RFC822_Response *response = rfc822_response_new(req, RTSP_Ok);
    GString *rtp_info = g_string_new("");

    /* temporary string used for creating headers */
    GString *str = g_string_new("npt=");

    RTSP_Range *range = g_queue_peek_head(rtsp_session->play_requests);

    /* Create Range header */
    if (range->begin_time >= 0)
        g_string_append_printf(str, "%f", range->begin_time);

    g_string_append(str, "-");

    if (range->end_time > 0)
      g_string_append_printf(str, "%f", range->end_time);

    rfc822_headers_set(response->headers,
                       RTSP_Header_Range,
                       g_string_free(str, false));

    /* Create RTP-Info header */
    g_slist_foreach(rtsp_session->rtp_sessions, rtp_session_send_play_reply, rtp_info);

    g_string_truncate(rtp_info, rtp_info->len-1);

    rfc822_headers_set(response->headers,
                       RTSP_Header_RTP_Info,
                       g_string_free(rtp_info, false));

    rfc822_response_send(client, response);
}

/**
 * @brief Parse the Range header and eventually add it to the session
 *
 * @param req The request to check and parse
 *
 *
 * @retval RTSP_Ok Parsing completed correctly.
 *
 * @retval RTSP_NotImplemented The Range: header specifies a format
 *                             that we don't implement (i.e.: clock,
 *                             smtpe or any new range type). Follows
 *                             specification of RFC 2326 Section
 *                             12.29.
 *
 * @retval RTSP_InvalidRange The provided range is not valid, which
 *                           means that the specified range goes
 *                           beyond the end of the resource.
 *
 * @retval RTSP_HeaderFieldNotValidforResource
 *                           A Range header with a range different
 *                           from 0- was present in a live
 *                           presentation session.
 *
 * RFC 2326 only mandates server to know NPT time, clock and smtpe
 * times are optional, and we currently support neither of them.
 *
 * Because both live555-based clients (VLC) and QuickTime always send
 * the Range: header even for live presentations, we have to accept
 * the range "0-" even if the mere presence of the Range header should
 * make the request invalid. Compare RFC 2326 Section 11.3.7.
 *
 * @see ragel_parse_range_header()
 */
static RTSP_ResponseCode parse_range_header(RTSP_Client *client,
                                            RFC822_Request *req)
{
    /* This is the default range to start from, if we need it. It sets
     * end_time and playback_time to negative values (that are invalid
     * and can be replaced), and the begin_time to zero (since if no
     * Range was specified, we want to start from the beginning).
     */
    static const RTSP_Range defaultrange = {
        .begin_time = 0,
        .end_time = -0.1,
        .playback_time = -0.1
    };

    RTSP_session *session = client->session;
    const char *range_hdr = rfc822_headers_lookup(req->headers, RTSP_Header_Range);
    RTSP_Range *range;

    /* If we have no range header and there is no play request queued,
     * we interpret it as a request for the full resource, if there is one already,
     * we just start whatever range is currently selected.
     */
    if ( range_hdr == NULL &&
         (range = g_queue_peek_head(session->play_requests)) != NULL ) {
        range->playback_time = ev_now(client->loop);

        fnc_log(FNC_LOG_VERBOSE,
            "resume PLAY [%f]: %f %f %f\n", ev_now(client->loop),
            range->begin_time, range->end_time, range->playback_time);


        return RTSP_Ok;
    }

    fnc_log(FNC_LOG_VERBOSE, "Range header: %s", range_hdr);

    /* Initialise the RTSP_Range structure by setting the three
     * values to the starting values. */
    range = g_slice_dup(RTSP_Range, &defaultrange);

    /* If there is any kind of parsing error, the range is considered
     * not implemented. It might not be entirely correct but until we
     * have better indications, it should be fine. */
    if (range_hdr) {
        if ( !ragel_parse_range_header(range_hdr, range) ) {
            g_slice_free(RTSP_Range, range);
            /** @todo We should be differentiating between not-implemented and
             *        invalid ranges.
             */
            return RTSP_NotImplemented;
        }

        /* This is a very lax check on what range the client provided;
         * unfortunately both live555-based clients and QuickTime
         * always send a Range: header even if the SDP had no range
         * attribute, and there seems to be no way to tell them not to
         * (at least there is none with the current live555, not sure
         * about QuickTime).
         *
         * But just to be on the safe side, if we're streaming a
         * non-seekable resource (like live) and the resulting value
         * is not 0-inf, we want to respond with a "Header Field Not
         * Valid For Resource". If clients handled this correctly, the
         * mere presence of the Range header in this condition would
         * trigger that response.
         */
        if ( !session->resource->seekable &&
             range->begin_time != 0 &&
             range->end_time != -0.1 ) {
            g_slice_free(RTSP_Range, range);
            return RTSP_HeaderFieldNotValidforResource;
        }
    }

    /* We don't set begin_time if it was not read, since the client
     * didn't request any seek and thus we won't do any seek to that.
     */
    if ( range->end_time < 0 ||
         range->end_time > session->resource->duration)
        range->end_time = session->resource->duration;
/*    else if ( range->end_time > session->resource->duration ) {
        g_slice_free(RTSP_Range, range);
        return RTSP_InvalidRange;
    }
*/

    if ( range->playback_time < 0 )
        range->playback_time = ev_now(client->loop);

    if ( session->cur_state != RTSP_SERVER_PLAYING )
        rtsp_session_editlist_free(session);

    rtsp_session_editlist_append(session, range);

    fnc_log(FNC_LOG_VERBOSE,
            "PLAY [%f]: %f %f %f\n", ev_now(client->loop),
            range->begin_time, range->end_time, range->playback_time);

    return RTSP_Ok;
}

/**
 * RTSP PLAY method handler
 * @param rtsp the buffer for which to handle the method
 * @param req The client request for the method
 */
void RTSP_play(RTSP_Client *rtsp, RFC822_Request *req)
{
    RTSP_session *rtsp_sess = rtsp->session;
    RTSP_ResponseCode error;
    const char *user_agent;

    if ( !rtsp_check_invalid_state(rtsp, req, RTSP_SERVER_INIT) )
        return;

    if ( !rfc822_request_check_url(rtsp, req) )
        return;

    if ( rtsp_sess->session_id == 0 ) {
        error = RTSP_BadRequest;
        goto error_management;
    }

    /* This is a workaround for broken VLC (up to 0.9.9 on *
     * 2008-04-22): instead of using a series of PLAY/PAUSE/PLAY for
     * seeking, it only sends PLAY requests. As per RFC 2326 Section
     * 10.5 this behaviour would suggest creating an edit list,
     * instead of seeking.
    */
    if ( rtsp_sess->cur_state == RTSP_SERVER_PLAYING &&
         (user_agent = rfc822_headers_lookup(req->headers, RTSP_Header_User_Agent)) &&
         strncmp(user_agent, "VLC media player", strlen("VLC media player")) == 0 ) {
        fnc_log(FNC_LOG_WARN, "Working around broken seek of %s", user_agent);
        rtsp_do_pause(rtsp);
    }

    if ( (error = parse_range_header(rtsp, req)) != RTSP_Ok )
        goto error_management;

    if ( rtsp_sess->cur_state != RTSP_SERVER_PLAYING &&
         (error = do_play(rtsp_sess)) != RTSP_Ok )
        goto error_management;

    send_play_reply(rtsp, req, rtsp_sess);

    ev_timer_again (rtsp->loop, &rtsp->ev_timeout);

    return;

error_management:
    rtsp_quick_response(rtsp, req, error);
    return;
}
