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
 * RTSP PAUSE method handler
 * @param rtsp the buffer for which to handle the method
 * @param req The client request for the method
 */
void RTSP_pause(RTSP_Client * rtsp, RTSP_Request *req)
{
    RTSP_session *rtsp_sess = rtsp->session;
    /* Get the first range, so that we can record the pause point */
    RTSP_Range *range = g_queue_peek_head(rtsp_sess->play_requests);

    /* This is only valid in Playing state */
    if ( !rtsp_check_invalid_state(req, RTSP_SERVER_INIT) ||
         !rtsp_check_invalid_state(req, RTSP_SERVER_READY) )
        return;

    if ( !rtsp_request_check_url(req) )
        return;

    /** @todo we need to check if the client provided a Range
     *        header */
    range->begin_time = ev_now(rtsp_sess->srv->loop)
        - range->playback_time;
    range->playback_time = -0.1;

    rtp_session_gslist_pause(rtsp_sess->rtp_sessions);

    ev_timer_stop(rtsp->srv->loop, &rtsp->ev_timeout);

    rtsp_sess->cur_state = RTSP_SERVER_READY;

    rtsp_quick_response(req, RTSP_Ok);
}
