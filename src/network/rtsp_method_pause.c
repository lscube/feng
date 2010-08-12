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
 * @brief Contains PAUSE method and reply handlers
 */

#include "feng.h"
#include "rtsp.h"
#include "rtp.h"

/**
 *  Actually pause playing the media using mediathread
 *
 *  @param rtsp the client requesting pause
 */

void rtsp_do_pause(RTSP_Client *rtsp)
{
    RTSP_session *rtsp_sess = rtsp->session;
    /* Get the first range, so that we can record the pause point */
    RTSP_Range *range = g_queue_peek_head(rtsp_sess->play_requests);

    range->begin_time = ev_now(feng_loop) -
                        range->playback_time;
    range->playback_time = -0.1;

    rtp_session_gslist_pause(rtsp_sess->rtp_sessions);

    ev_timer_stop(rtsp->loop, &rtsp->ev_timeout);

    rtsp_sess->cur_state = RTSP_SERVER_READY;
}

/**
 * RTSP PAUSE method handler
 * @param rtsp the buffer for which to handle the method
 * @param req The client request for the method
 */
void RTSP_pause(RTSP_Client *rtsp, RFC822_Request *req)
{
    /* This is only valid in Playing state */
    if ( !rtsp_check_invalid_state(rtsp, req, RTSP_SERVER_INIT) ||
         !rtsp_check_invalid_state(rtsp, req, RTSP_SERVER_READY) )
        return;

    if ( !rfc822_request_check_url(rtsp, req) )
        return;

    /** @todo we need to check if the client provided a Range
     *        header */
    rtsp_do_pause(rtsp);

    rtsp_quick_response(rtsp, req, RTSP_Ok);
}
