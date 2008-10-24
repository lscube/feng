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

#include <inttypes.h> /* For PRIu64 */

#include <rtsp_utils.h>

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
static int send_teardown_reply(RTSP_buffer * rtsp, guint64 session_id)
{
    GString *reply = rtsp_generate_ok_response(rtsp->rtsp_cseq, session_id);
    g_string_append(reply, RTSP_EL);

    bwrite(reply, rtsp);

    fnc_log(FNC_LOG_CLIENT, "200 - - ");

    return ERR_NOERROR;
}

typedef struct {
  RTSP_session *s;
  gchar *filename;
} rtp_session_release_pair;

static void rtp_session_release(gpointer element, gpointer user_data)
{
  RTP_session *rtp_curr = (RTP_session *)element;
  rtp_session_release_pair *pair = (rtp_session_release_pair *)user_data;

  if (!strcmp(
	      r_selected_track(rtp_curr->track_selector)->info->name,
	      pair->filename) || 
      !strcmp(
	      r_selected_track(rtp_curr->track_selector)->parent->info->name,
	      pair->filename)
      // if multicast don't touch the rtp shared session
      ) {
    pair->s->rtp_sessions = g_slist_remove(pair->s->rtp_sessions, rtp_curr);

    if (!rtp_curr->is_multicast--) {
      // Release the scheduler entry
      schedule_remove(rtp_curr, NULL);
      fnc_log(FNC_LOG_DEBUG, "[TEARDOWN] Removed %s", pair->filename);
    } else {
      fnc_log(FNC_LOG_DEBUG, "[TEARDOWN] %s multicast session,"
	      " %d clients listening",
	      pair->filename, rtp_curr->is_multicast);
    }
  }
}

/**
 * RTSP TEARDOWN method handler
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_teardown(RTSP_buffer * rtsp)
{
    Url url;
    guint64 session_id;
    RTSP_session *s;
    char *filename;
    rtp_session_release_pair pair;

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
        send_reply(415, 0, rtsp);    // Internal server error
        return ERR_GENERIC;
    }

    if (s->session_id != session_id) {
        send_reply(454, 0, rtsp);    /* Session Not Found */
        return ERR_PARSE;
    }

    fnc_log(FNC_LOG_INFO, "TEARDOWN %s://%s/%s RTSP/1.0 ",
	    url.protocol, url.hostname, url.path);
    send_teardown_reply(rtsp, session_id);
    log_user_agent(rtsp); // See User-Agent 

    if (strchr(url.path, '='))    /*Compatibility with RealOne and RealPlayer */
        filename = strchr(url.path, '=') + 1;
    else
        filename = url.path;

    filename = g_path_get_basename(filename);

    // Release all URI RTP session
    pair.filename = filename; pair.s = s;
    g_slist_foreach(s->rtp_sessions, rtp_session_release, &pair);

    if (s->rtp_sessions == NULL) {
      // Release mediathread resource
        mt_resource_close(rtsp->session->resource);
        // Release the RTSP session
        g_free(rtsp->session);
        rtsp->session = NULL;
    }

    g_free(filename);

    return ERR_NOERROR;

error_management:
    send_reply(error.message.reply_code, error.message.reply_str, rtsp);
    return ERR_GENERIC;
}
