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

/** @file RTSP_teardown.c
 * @brief Contains TEARDOWN method and reply handlers
 */

#include <RTSP_utils.h>

#include <fenice/rtsp.h>
#include <fenice/prefs.h>
#include <fenice/schedule.h>
#include <bufferpool/bufferpool.h>
#include <fenice/fnc_log.h>
#include <glib.h>

/**
 * Sends the reply for the teardown method
 * @param rtsp the buffer where to write the reply
 * @param session_id the id of the session closed
 * @return ERR_NOERROR
 */
static int send_teardown_reply(RTSP_buffer * rtsp, unsigned long session_id)
{
    char r[1024];
    char temp[30];
    long int cseq = rtsp->rtsp_cseq;

    /* build a reply message */
    sprintf(r,
        "%s %d %s" RTSP_EL "CSeq: %ld" RTSP_EL "Server: %s/%s" RTSP_EL,
        RTSP_VER, 200, get_stat(200), cseq, PACKAGE, VERSION);
    add_time_stamp(r, 0);
    strcat(r, "Session: ");
    sprintf(temp, "%lu", session_id);
    strcat(r, temp);
    strcat(r, RTSP_EL RTSP_EL);
    bwrite(r, strlen(r), rtsp);

    fnc_log(FNC_LOG_CLIENT, "200 - - ");

    return ERR_NOERROR;
}

/**
 * RTSP TEARDOWN method handler
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_teardown(RTSP_buffer * rtsp)
{
    ConnectionInfo cinfo;
    unsigned long session_id;
    RTSP_session *s;
    RTP_session *rtp_curr, *rtp_prev = NULL, *rtp_temp;
    char *filename;
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
        send_reply(415, 0, rtsp);    // Internal server error
        return ERR_GENERIC;
    }

    if (s->session_id != session_id) {
        send_reply(454, 0, rtsp);    /* Session Not Found */
        return ERR_PARSE;
    }

    fnc_log(FNC_LOG_INFO, "TEARDOWN %s RTSP/1.0 ", url);
    send_teardown_reply(rtsp, session_id);
    log_user_agent(rtsp); // See User-Agent 

    if (strchr(cinfo.object, '='))    /*Compatibility with RealOne and RealPlayer */
        filename = strchr(cinfo.object, '=') + 1;
    else
        filename = cinfo.object;

    filename = g_path_get_basename(filename);

    // Release all URI RTP session
    rtp_curr = s->rtp_session;
    while (rtp_curr != NULL) {

        if (!strcmp(
                r_selected_track(rtp_curr->track_selector)->info->name,
                                 filename) || 
            !strcmp(
                r_selected_track(rtp_curr->track_selector)->parent->info->name,
                                 filename)
            // if multicast don't touch the rtp shared session
            ) {
            rtp_temp = rtp_curr;
            if (rtp_prev != NULL)
                rtp_prev->next = rtp_curr->next;
            else
                s->rtp_session = rtp_curr->next;

            rtp_curr = rtp_curr->next;
            if (!rtp_temp->is_multicast--) {
                // Release the scheduler entry
                schedule_remove(rtp_temp);
                fnc_log(FNC_LOG_DEBUG, "[TEARDOWN] Removed %s", filename);
            } else {
                fnc_log(FNC_LOG_DEBUG, "[TEARDOWN] %s multicast session,"
                                       " %d clients listening",
                                        filename, rtp_temp->is_multicast);
            }
        } else {
            rtp_prev = rtp_curr;
            rtp_curr = rtp_curr->next;
        }
    }

    if (s->rtp_session == NULL) {
    // Release mediathread resource
        mt_resource_close(rtsp->session_list->resource);
        // Release the RTSP session
        free(rtsp->session_list);
        rtsp->session_list = NULL;
    }

    g_free(filename);

    return ERR_NOERROR;

error_management:
    send_reply(error.message.reply_code, error.message.reply_str, rtsp);
    return ERR_GENERIC;
}
