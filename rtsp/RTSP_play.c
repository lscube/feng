/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *      
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
 *    - Eugenio Menegatti     <m.eu@libero.it>
 *    - Stefano Cau
 *    - Giuliano Emma
 *    - Stefano Oldrini
 * 
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#include <fenice/bufferpool.h>
#include <fenice/rtsp.h>
#include <fenice/prefs.h>
#include <fenice/fnc_log.h>

#include <RTSP_utils.h>

RTSP_Error parse_play_time_range(RTSP_buffer * rtsp, play_args * args);
RTSP_Error do_play(RTSP_buffer * rtsp, ConnectionInfo * cinfo, RTSP_session * rtsp_sess, play_args * args);
RTSP_Error get_session(RTSP_buffer * rtsp, long int session_id, RTSP_session **rtsp_sess);

/*
     ****************************************************************
     *            PLAY METHOD HANDLING
     ****************************************************************
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
    else if ( (error = do_play(rtsp, &cinfo, rtsp_sess, &args)).got_error ) {
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

RTSP_Error parse_play_time_range(RTSP_buffer * rtsp, play_args * args)
{
    int time_taken = 0;
    char *p = NULL, *q = NULL;

    args->playback_time_valid = 0;
    args->start_time_valid = 0;
    if ((p = strstr(rtsp->in_buffer, HDR_RANGE)) != NULL) {
        if ((q = strstr(p, "npt")) != NULL) {      // FORMAT: npt
            if ((q = strchr(q, '=')) == NULL)
                return RTSP_BadRequest;    /* Bad Request */

            sscanf(q + 1, "%f", &(args->start_time));

            if ((q = strchr(q, '-')) == NULL)
                return RTSP_BadRequest;

            if (sscanf(q + 1, "%f", &(args->end_time)) != 1)
                args->end_time = 0;
        } 
        else if ((q = strstr(p, "smpte")) != NULL) { // FORMAT: smpte                        
            // Currently unsupported. Using default.
            args->start_time = 0;
            args->end_time = 0;
        }
        else if ((q = strstr(p, "clock"))!= NULL) { // FORMAT: clock
            // Currently unsupported. Using default.
            args->start_time = 0;
            args->end_time = 0;
        } 
        else if ((q = strstr(p, "time")) == NULL) { // No specific format. Assuming NeMeSI format.
            // Hour
            double t;
            q = strstr(p, ":");
            sscanf(q + 1, "%lf", &t);
            args->start_time = t * 60 * 60;
            // Min
            q = strstr(q + 1, ":");
            sscanf(q + 1, "%lf", &t);
            args->start_time += (t * 60);
            // Sec
            q = strstr(q + 1, ":");
            sscanf(q + 1, "%lf", &t);
            args->start_time += t;

            args->start_time_valid = 1;
        } 
        else {
            // no range defined but start time expressed?
            args->start_time = 0;
            args->end_time = 0;
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
        args->start_time = 0;
        args->end_time = 0;
        memset(&(args->playback_time), 0, sizeof(args->playback_time));
    }

    return RTSP_Ok;
}

RTSP_Error get_session(RTSP_buffer * rtsp, long int session_id, RTSP_session **rtsp_sess)
{
#if 0
    for (rtsp_sess = rtsp->session_list; rtsp_sess != NULL; rtsp_sess++)
        if (rtsp_sess->session_id == session_id) break;
    if (rtsp_sess == NULL) {
#else
    // XXX Fenice supports single session atm
    if (((*rtsp_sess = rtsp->session_list) == NULL) ||
        ((*rtsp_sess)->session_id != session_id))
    {
#endif
        return RTSP_SessionNotFound;
    }

    return RTSP_Ok;
}

#if ENABLE_MEDIATHREAD
RTSP_Error do_play(RTSP_buffer * rtsp, ConnectionInfo * cinfo, RTSP_session * rtsp_sess, play_args * args)
{
    RTP_session *rtp_sess;
    char *q = NULL;

    if (!(q = strchr(cinfo->object, '!'))) {
        //if '!' is not present then a file has not been specified
        // aggregate content requested
        for (rtp_sess = rtsp_sess->rtp_session;
             rtp_sess != NULL;
             rtp_sess = rtp_sess->next)
        {
            // Start playing all the presentation
            if (!rtp_sess->started) {
                // Start new
                if (schedule_start (rtp_sess->sched_id, args) == ERR_ALLOC)
                        return RTSP_Fatal_ErrAlloc;
            } else {
                // Resume existing
                if (!rtp_sess->pause) {
                //  fnc_log(FNC_LOG_INFO,"PLAY: already playing\n");
                } else {
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
#else
RTSP_Error do_play(RTSP_buffer * rtsp, ConnectionInfo * cinfo, RTSP_session * rtsp_sess, play_args * args)
{
    RTSP_Error error = RTSP_Ok;
    RTP_session *rtp_sess;

    char *q = strchr(cinfo->object, '!');
    if (q == NULL) {
        // PLAY <file.sd>
        // Search for the RTP session
        for (rtp_sess = rtsp_sess->rtp_session;
             rtp_sess != NULL;
             rtp_sess = rtp_sess->next) {
            if (rtp_sess->current_media->description.priority == 1) {
            // Start playing all the presentation
                if (!rtp_sess->started) {
                    // Start new
                    if (schedule_start (rtp_sess->sched_id, args) == ERR_ALLOC) {
                        return RTSP_Fatal_ErrAlloc;
                    }
                } else {
                    // Resume existing
                    if (!rtp_sess->pause) {
                    //  fnc_log(FNC_LOG_INFO,"PLAY: already playing\n");
                    } else {
                            schedule_resume (rtp_sess->sched_id, args);
                    }
                }
            }
        }
    } else {
        if (url_is_file) {
            // PLAY <file.sd>!<file>
                // Search for the RTP session
            for (rtp_sess = rtsp_sess->rtp_session; rtp_sess != NULL;
                 rtp_sess = rtp_sess->next) {
                if (strcmp (rtp_sess->current_media->filename,
                            q + 1) == 0) {
                    break;
                }
            }
            if (rtp_sess != NULL) {
                // FOUND. Start Playing
                if (schedule_start (rtp_sess->sched_id, args) == ERR_ALLOC) {
                    return RTSP_Fatal_ErrAlloc;
                }
            } else {
                return RTSP_SessionNotFound;
            }
        } else {
            // PLAY <file.sd>!<aggr>
                // It's an aggregate control. Play all the RTPs
            for (rtp_sess = rtsp_sess->rtp_session; rtp_sess != NULL;
                 rtp_sess = rtp_sess->next) {
                if (!rtp_sess->started) {
                    // Start new
                    if (schedule_start(rtp_sess->sched_id, args) == ERR_ALLOC)
                        return RTSP_Fatal_ErrAlloc;
                } else {
                    // Resume existing
                    if (!rtp_sess->pause) {
                    //fnc_log(FNC_LOG_INFO,"PLAY: already playing\n");
                    } else {
                        schedule_resume(rtp_sess->sched_id, args);
                    }
                }
            }
        }
    }

    return RTSP_Ok;
}
#endif
