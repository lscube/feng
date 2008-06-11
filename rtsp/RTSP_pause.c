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
    char r[1024];
    char temp[30];
    /* build a reply message */
    sprintf(r,
        "%s %d %s" RTSP_EL "CSeq: %d" RTSP_EL "Server: %s/%s" RTSP_EL,
        RTSP_VER, 200, get_stat(200), rtsp->rtsp_cseq, PACKAGE,
        VERSION);
    add_time_stamp(r, 0);
    strcat(r, "Session: ");
    sprintf(temp, "%lu", rtsp_session->session_id);
    strcat(r, temp);
    strcat(r, RTSP_EL RTSP_EL);
    bwrite(r, strlen(r), rtsp);
    fnc_log(FNC_LOG_CLIENT, "200 - - ");

    return ERR_NOERROR;
}


/**
 * RTSP PAUSE method handler
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_pause(RTSP_buffer * rtsp)
{
    ConnectionInfo cinfo;
    unsigned long session_id;
    RTSP_session *s;
    RTP_session *r;
    char url[255];

    RTSP_Error error;

    if ( (error = get_cseq(rtsp)).got_error ) // Get the CSeq 
        goto error_management;
    else if ( (error = extract_url(rtsp, url)).got_error ) // Extract the URL
	    goto error_management;
    else if ( (error = validate_url(url, &cinfo)).got_error ) // Validate URL
    	goto error_management;
    else if ( (error = check_forbidden_path(&cinfo)).got_error ) // Check for Forbidden Paths
    	goto error_management;
    else if ( (error = get_session_id(rtsp, &session_id)).got_error ) // Get Session id
        goto error_management;

    s = rtsp->session_list;
    if (s == NULL) {
        send_reply(415, NULL, rtsp);    // Internal server error
        return ERR_GENERIC;
    }
    if (s->session_id != session_id) {
        send_reply(454, NULL, rtsp);    /* Session Not Found */
        return ERR_NOERROR;
    }

    for (r = s->rtp_session; r != NULL; r = r->next) {
        r->pause = 1;
    }

    fnc_log(FNC_LOG_INFO, "PAUSE %s RTSP/1.0 ", url);
    send_pause_reply(rtsp, s);
    log_user_agent(rtsp); // See User-Agent 

    return ERR_NOERROR;

error_management:
    send_reply(error.message.reply_code, error.message.reply_str, rtsp);
    return ERR_GENERIC;
}
