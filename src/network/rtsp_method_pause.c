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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "rtsp.h"
#include <fenice/fnc_log.h>


static void rtp_session_pause(gpointer element, gpointer user_data)
{
  RTP_session *r = (RTP_session *)element;

  r->pause = 1;
}

/**
 * RTSP PAUSE method handler
 * @param rtsp the buffer for which to handle the method
 * @param req The client request for the method
 */
void RTSP_pause(RTSP_buffer * rtsp, RTSP_Request *req)
{
    Url url;
    RTSP_session *rtsp_sess = rtsp->session;

    /* This is only valid in Playing state */
    if ( !rtsp_check_invalid_state(req, INIT_STATE) ||
         !rtsp_check_invalid_state(req, READY_STATE) )
        return;

    if ( !rtsp_request_get_url(rtsp, req, &url) )
        return;

    g_slist_foreach(rtsp_sess->rtp_sessions, rtp_session_pause, NULL);

    rtsp_quick_response(req, RTSP_Ok);
    rtsp_sess->cur_state = READY_STATE;
}
