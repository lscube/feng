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

#include "rtsp.h"

/**
 * RTSP PAUSE method handler
 * @param rtsp the buffer for which to handle the method
 * @param req The client request for the method
 */
void RTSP_pause(RTSP_Client * rtsp, RTSP_Request *req)
{
    RTSP_session *rtsp_sess = rtsp->session;

    /* This is only valid in Playing state */
    if ( !rtsp_check_invalid_state(req, RTSP_SERVER_INIT) ||
         !rtsp_check_invalid_state(req, RTSP_SERVER_READY) )
        return;

    if ( !rtsp_request_check_url(req) )
        return;

    rtp_session_gslist_pause(rtsp_sess->rtp_sessions);

    rtsp_quick_response(req, RTSP_Ok);
    rtsp_sess->cur_state = RTSP_SERVER_READY;
}
