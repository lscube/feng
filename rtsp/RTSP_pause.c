/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
 * See AUTHORS for more details 
 *  
 * Feng is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Feng is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *  
 * */

/** @file RTSP_pause.c
 * @brief Contains PAUSE method and reply handlers
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <fenice/rtsp.h>
#include <fenice/fnc_log.h>

#include <RTSP_utils.h>

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

    bwrite(reply, rtsp);

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
int RTSP_pause(RTSP_buffer * rtsp)
{
    Url url;
    guint64 session_id;
    RTSP_session *s;

    RTSP_Error error;

    if ( (error = get_cseq(rtsp)).got_error ) // Get the CSeq 
        goto error_management;
    // Extract and validate the URL
    if ( (error = rtsp_extract_validate_url(rtsp, &url)).got_error )
	goto error_management;
    if ( (error = get_session_id(rtsp, &session_id)).got_error ) // Get Session id
        goto error_management;

    s = rtsp->session;
    if (s == NULL) {
        send_reply(415, NULL, rtsp);    // Internal server error
        return ERR_GENERIC;
    }
    if (s->session_id != session_id) {
        send_reply(454, NULL, rtsp);    /* Session Not Found */
        return ERR_NOERROR;
    }
    
    g_slist_foreach(s->rtp_sessions, rtp_session_pause, NULL);

    fnc_log(FNC_LOG_INFO, "PAUSE %s://%s/%s RTSP/1.0 ",
	    url.protocol, url.hostname, url.path);
    send_pause_reply(rtsp, s);
    log_user_agent(rtsp); // See User-Agent 

    return ERR_NOERROR;

error_management:
    send_reply(error.message.reply_code, error.message.reply_str, rtsp);
    return ERR_GENERIC;
}
