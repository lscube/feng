/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
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

/** @file RTSP_play.c
 * @brief Contains PAUSE method and reply handlers
 */

#include <bufferpool/bufferpool.h>
#include <fenice/rtsp.h>
#include <fenice/prefs.h>
#include <fenice/fnc_log.h>
#include <math.h>

#include <RTSP_utils.h>

/**
 * Parses the RANGE HEADER to get the required play range
 * @param rtsp the buffer from which to get the data
 * @param args where to save the play range informations
 * @return RTSP_Ok or RTSP_BadRequest on missing RANGE HEADER
 */
static RTSP_Error parse_play_time_range(RTSP_buffer * rtsp, play_args * args)
{
    int time_taken = 0;
    char *p = NULL, *q = NULL;
    char tmp[5];

    /* Default values */
    args->playback_time_valid = 0;
    args->start_time_valid = 0;
    args->seek_time_valid = 0;
    args->start_time = 0.0;
    args->end_time = HUGE_VAL;

    if ((p = strstr(rtsp->in_buffer, HDR_RANGE)) != NULL) {
        if ((q = strstr(p, "npt")) != NULL) {      // FORMAT: npt
            if ((q = strchr(q, '=')) == NULL)
                return RTSP_BadRequest;    /* Bad Request */

            if (sscanf(q + 1, "%f", &(args->start_time)) != 1) {
                if (sscanf(q + 1, "%4s", tmp) != 1 && !strcasecmp(tmp,"now-")) {
                    return RTSP_BadRequest;
                }
            } else {
                args->seek_time_valid = 1;
            }

            if ((q = strchr(q, '-')) == NULL)
                return RTSP_BadRequest;

            if (sscanf(q + 1, "%f", &(args->end_time)) != 1)
                args->end_time = HUGE_VAL;
        }
        else if ((q = strstr(p, "smpte")) != NULL) { // FORMAT: smpte
            // Currently unsupported. Using default.
        }
        else if ((q = strstr(p, "clock"))!= NULL) { // FORMAT: clock
            // Currently unsupported. Using default.
        }
        else if ((q = strstr(p, "time")) == NULL) { // No specific format. Assuming NeMeSI format.
            double t;
            if ((q = strstr(p, ":"))) {
                // Hours
                sscanf(q + 1, "%lf", &t);
                args->start_time = t * 60 * 60;
            }
            if ((q = strstr(q + 1, ":"))) {
                // Minutes
                sscanf(q + 1, "%lf", &t);
                args->start_time += (t * 60);
            }
            if ((q = strstr(q + 1, ":"))) {
                // Seconds
                sscanf(q + 1, "%lf", &t);
                args->start_time += t;
            }
            args->start_time_valid = 1;
        }
        else {
            // no range defined but start time expressed?
            time_taken = 1;
        }

        if ((q = strstr(p, "time")) == NULL) {
            // Start playing immediately
            memset(&(args->playback_time), 0, sizeof(args->playback_time));
        }
        else {
            // Start playing at desired time
            if (!time_taken) {
                q = strchr(q, '=');
                if (get_utc(&(args->playback_time), q + 1) != ERR_NOERROR) {
                    memset(&(args->playback_time), 0, sizeof(args->playback_time));
                }
                args->playback_time_valid = 1;
            }
        }
    }
    else {
        args->start_time = 0.0;
        args->end_time = HUGE_VAL;
        memset(&(args->playback_time), 0, sizeof(args->playback_time));
    }

    return RTSP_Ok;
}

/**
 * Gets the correct session for the given session_id (actually only 1 session is supported)
 * @param rtsp the buffer from which to get the session
 * @param session_id the id of the session to retrieve
 * @param rtsp_sess where to save the retrieved session
 * @return RTSP_Ok or RTSP_SessionNotFound
 */
static RTSP_Error get_session(RTSP_buffer * rtsp, long int session_id, RTSP_session **rtsp_sess)
{
#if 0
    for (rtsp_sess = rtsp->session_list; rtsp_sess != NULL; rtsp_sess++)
        if (rtsp_sess->session_id == session_id) break;
    if (rtsp_sess == NULL) {
#else
    // XXX Feng supports single session atm
    if (((*rtsp_sess = rtsp->session_list) == NULL) ||
        ((*rtsp_sess)->session_id != session_id))
    {
#endif
        return RTSP_SessionNotFound;
    }

    return RTSP_Ok;
}

/**
 * Actually seek through the media using mediathread
 * @param rtsp_sess the session for which to start playing
 * @param args the range to play
 * @return RTSP_Ok or RTSP_InvalidRange
 */
static RTSP_Error do_seek(RTSP_session * rtsp_sess, play_args * args)
{
    Resource *r = rtsp_sess->resource;
    RTP_session *rtp_sess;
    int pause_needed;

    if (args->seek_time_valid && args->start_time >= 0.0) {
        if(mt_resource_seek(r, args->start_time)) {
            return RTSP_InvalidRange;
        }
        for (rtp_sess = rtsp_sess->rtp_session; rtp_sess;
             rtp_sess = rtp_sess->next) {
            if ((pause_needed = (rtp_sess->started && !rtp_sess->pause))) {
                /* Pause scheduler while reiniting RTP session */
                rtp_sess->pause = 1;
            }
            rtp_sess->start_seq = 1 + (unsigned int) (rand() % (0xFFFF));
            rtp_sess->start_rtptime = 1 + (unsigned int) (rand() % (0xFFFFFFFF));
            rtp_sess->seq_no = rtp_sess->start_seq - 1;
            rtp_sess->seek_time = args->start_time;

            if (rtp_sess->cons) {
                while (bp_getreader(rtp_sess->cons)) {
                    /* Drop spurious packets after seek */
                    bp_gotreader(rtp_sess->cons);
                }
            }
            if (pause_needed) {
                schedule_resume (rtp_sess->sched_id, args);
            }
        }
    } else if (args->start_time < 0.0) {
        fnc_log(FNC_LOG_DEBUG,"[RTSP] Negative seek to %f", args->start_time);
        return RTSP_InvalidRange;
    }

    return RTSP_Ok;
}


/**
 * Actually starts playing the media using mediathread
 * @param cinfo the connection for which to start playing
 * @param rtsp_sess the session for which to start playing
 * @param args the range to play
 * @return RTSP_Ok or RTSP_InternalServerError
 */
static RTSP_Error do_play(ConnectionInfo * cinfo, RTSP_session * rtsp_sess, play_args * args)
{
    RTSP_Error error = RTSP_Ok;
    RTP_session *rtp_sess;
    char *q = NULL;

    if (!(q = strchr(cinfo->object, '!'))) {
        //if '!' is not present then a file has not been specified
        // aggregate content requested
        // Perform seek if needed
        if ((error = do_seek(rtsp_sess, args)).got_error) {
            return error;
        }
        for (rtp_sess = rtsp_sess->rtp_session; rtp_sess;
             rtp_sess = rtp_sess->next)
        {
            if (rtp_sess->is_multicast) return RTSP_Ok;
            // Start playing all the presentation
            if (!rtp_sess->started) {
                // Start new
                if (schedule_start (rtp_sess->sched_id, args) == ERR_ALLOC)
                        return RTSP_Fatal_ErrAlloc;
            } else {

                // Resume existing
                if (rtp_sess->pause) {
                    schedule_resume (rtp_sess->sched_id, args);
                }
            }
        }
    } else {
        // FIXME complete with the other stuff
        return RTSP_InternalServerError; /* Internal server error */

        // resource!trackname
//        strcpy (trackname, q + 1);
        // XXX Not really nice...
        while (cinfo->object != q) if (*--q == '/') break;
        *q = '\0';
    }

    return RTSP_Ok;
}

/**
 * Sends the reply for the play method
 * @param rtsp the buffer where to write the reply
 * @param object the object that we wanted to play
 * @param rtsp_session the session for which to generate the reply
 * @return ERR_NOERROR
 */
static int send_play_reply(RTSP_buffer * rtsp, char *object, RTSP_session * rtsp_session)
{
    char r[1024];
    char temp[30];
    RTP_session *p = rtsp_session->rtp_session;
    Track *t;
    /* build a reply message */
    sprintf(r,
        "%s %d %s" RTSP_EL "CSeq: %d" RTSP_EL "Server: %s/%s" RTSP_EL,
        RTSP_VER, 200, get_stat(200), rtsp->rtsp_cseq, PACKAGE,
        VERSION);
    add_time_stamp(r, 0);
    strcat(r, "Session: ");
    sprintf(temp, "%d", rtsp_session->session_id);
    strcat(r, temp);
    strcat(r, RTSP_EL);
    strcat(r, "RTP-info: ");
    do {
        t = r_selected_track(p->track_selector);
        strcat(r, "url=");
        // TODO: we MUST be sure to send the correct url 
        sprintf(r + strlen(r), "rtsp://%s/%s/%s!%s",
            prefs_get_hostname(), p->sd_filename, t->parent->info->name,
            t->info->name);
        strcat(r, ";");
        sprintf(r + strlen(r), "seq=%u;rtptime=%u", p->start_seq,
            p->start_rtptime);
        if (p->next != NULL) {
            strcat(r, ",");
        } else {
            strcat(r, RTSP_EL);
        }
        p = p->next;
    } while (p != NULL);
    // end of message
    strcat(r, RTSP_EL);

    bwrite(r, (unsigned short) strlen(r), rtsp);


    fnc_log(FNC_LOG_CLIENT, "200 - %s ", object);

    return ERR_NOERROR;
}

/**
 * RTSP PLAY method handler
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_play(RTSP_buffer * rtsp)
{
    ConnectionInfo cinfo;
    char url[255];
    long int session_id;
    RTSP_session *rtsp_sess;
    play_args args;

    RTSP_Error error;

    // Parse the input message
    if ( (error = get_cseq(rtsp)).got_error ) // Get the CSeq 
        goto error_management;
    else if ( (error = parse_play_time_range(rtsp, &args)).got_error ) // Get the range
        goto error_management;

    if ( (error = get_session_id(rtsp, &session_id)).got_error )
        goto error_management;
    else if ( session_id == -1 ) {
        set_RTSP_Error(&error, 400, "");
        goto error_management;
    }

    if ( (error = get_session(rtsp, session_id, &rtsp_sess)).got_error ) // Pick correct session
        goto error_management;
    else if ( (error = extract_url(rtsp, url)).got_error ) // Extract the URL
	    goto error_management;
    else if ( (error = validate_url(url, &cinfo)).got_error ) // Validate URL
    	goto error_management;
    else if ( (error = check_forbidden_path(&cinfo)).got_error ) // Check for Forbidden Paths
    	goto error_management;
    else if ( (error = do_play(&cinfo, rtsp_sess, &args)).got_error ) {
        if (error.got_error == ERR_ALLOC) 
            return ERR_ALLOC;
        goto error_management;
    }   

    fnc_log(FNC_LOG_INFO, "PLAY %s RTSP/1.0 ", url);
    send_play_reply(rtsp, cinfo.object, rtsp_sess);
    log_user_agent(rtsp); // See User-Agent 
    return ERR_NOERROR;

error_management:
    send_reply(error.message.reply_code, error.message.reply_str, rtsp);
    return ERR_NOERROR;
}
