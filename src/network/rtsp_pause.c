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


/**
 * Sends the reply for the pause method
 * @param rtsp the buffer where to write the reply
 * @param rtsp_session the session for which to generate the reply
 * @return ERR_NOERROR
 */
static int send_pause_reply(RTSP_buffer * rtsp, RTSP_session * rtsp_session)
{
    GString *reply = rtsp_generate_ok_response(rtsp->rtsp_cseq, rtsp_session->session_id);

    /* No body */
    g_string_append(reply, RTSP_EL);

    rtsp_bwrite(rtsp, reply);

    fnc_log(FNC_LOG_CLIENT, "200 - - ");

    return ERR_NOERROR;
}

static void rtp_session_pause(gpointer element, gpointer user_data)
{
  RTP_session *r = (RTP_session *)element;

  r->pause = 1;
}

/**
 * RTSP PAUSE method handler
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_pause(RTSP_buffer * rtsp, RTSP_Request *req)
{
    Url url;
    RTSP_session *s;

    if ( !rtsp_request_get_url(rtsp, req, &url) )
        return ERR_GENERIC;

    s = rtsp->session;
    
    g_slist_foreach(s->rtp_sessions, rtp_session_pause, NULL);

    send_pause_reply(rtsp, s);

    return ERR_NOERROR;
}
