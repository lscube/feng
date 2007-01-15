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

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#include <fenice/bufferpool.h>
#include <fenice/rtsp.h>
#include <fenice/utils.h>
#include <fenice/prefs.h>
#include <fenice/fnc_log.h>


/*
     ****************************************************************
     *            PLAY METHOD HANDLING
     ****************************************************************
*/

int RTSP_play(RTSP_buffer * rtsp)
{
    int url_is_file;
    char object[255], server[255], trash[255];
    char url[255];
    unsigned short port;
    char *p = NULL, *q = NULL;
    long int session_id;
    RTSP_session *ptr;
    RTP_session *ptr2;
    play_args args;
    int time_taken = 0;

    // Parse the input message
    // Get the CSeq 
    if ((p = strstr(rtsp->in_buffer, HDR_CSEQ)) == NULL) {
        send_reply(400, 0, rtsp);    /* Bad Request */
        return ERR_NOERROR;
    } else {
        if (sscanf(p, "%254s %d", trash, &(rtsp->rtsp_cseq)) != 2) {
            send_reply(400, 0, rtsp);    /* Bad Request */
            return ERR_NOERROR;
        }
    }
    // Get the range
    args.playback_time_valid = 0;
    args.start_time_valid = 0;
    if ((p = strstr(rtsp->in_buffer, HDR_RANGE)) != NULL) {
        if ((q = strstr(p, "npt")) != NULL) {
            // FORMAT: npt
            if ((q = strchr(q, '=')) == NULL) {
                send_reply(400, 0, rtsp);    /* Bad Request */
                return ERR_NOERROR;
            }
            sscanf(q + 1, "%f", &(args.start_time));
            if ((q = strchr(q, '-')) == NULL) {
                send_reply(400, 0, rtsp);    /* Bad Request */
                return ERR_NOERROR;
            }
            if (sscanf(q + 1, "%f", &(args.end_time)) != 1) {
                args.end_time = 0;
            }
        } else

        if ((q = strstr(p, "smpte")) != NULL) {
        // FORMAT: smpte                        
            // Currently unsupported. Using default.
            args.start_time = 0;
            args.end_time = 0;
        } else

        if ((q = strstr(p, "clock"))!= NULL) {
        // FORMAT: clock
            // Currently unsupported. Using default.
            args.start_time = 0;
            args.end_time = 0;
        } else
        // No specific format. Assuming NeMeSI format.
        if ((q = strstr(p, "time")) == NULL) {
            // Hour
            double t;
            q = strstr(p, ":");
            sscanf(q + 1, "%lf", &t);
            args.start_time = t * 60 * 60;
            // Min
            q = strstr(q + 1, ":");
            sscanf(q + 1, "%lf", &t);
            args.start_time += (t * 60);
            // Sec
            q = strstr(q + 1, ":");
            sscanf(q + 1, "%lf", &t);
            args.start_time += t;

            args.start_time_valid = 1;
        } else {
            // no range defined but start time expressed?
            args.start_time = 0;
            args.end_time = 0;
            time_taken = 1;
        }

        if ((q = strstr(p, "time")) == NULL) {
            // Start playing immediately
            memset(&(args.playback_time), 0,
                   sizeof(args.playback_time));
        } else {
            // Start playing at desired time
            if (!time_taken) {
                q = strchr(q, '=');
                if (get_utc(&(args.playback_time), q + 1)
                    != ERR_NOERROR) {
                    memset(&(args.playback_time), 0,
                           sizeof(args.playback_time));
                }
                args.playback_time_valid = 1;
            }
        }
    } else {
        args.start_time = 0;
        args.end_time = 0;
        memset(&(args.playback_time), 0, sizeof(args.playback_time));
    }
    // CSeq
    if ((p = strstr(rtsp->in_buffer, HDR_CSEQ)) == NULL) {
        send_reply(400, 0, rtsp);    /* Bad Request */
        return ERR_NOERROR;
    }
    // If we get a Session hdr, then we have an aggregate control
    if ((p = strstr(rtsp->in_buffer, HDR_SESSION)) != NULL) {
        if (sscanf(p, "%254s %ld", trash, &session_id) != 2) {
            send_reply(454, 0, rtsp);    /* Session Not Found */
            return ERR_NOERROR;
        }
    } else {
        send_reply(400, 0, rtsp);    /* bad request */
        return ERR_NOERROR;
    }
    /* Extract the URL */
    if (!sscanf(rtsp->in_buffer, " %*s %254s ", url)) {
        send_reply(400, 0, rtsp);    /* bad request */
        return ERR_NOERROR;
    }
    /* Validate the URL */

    switch (parse_url(url, server, sizeof(server), &port, object,
            sizeof(object))) {
    case 1:        // bad request
        send_reply(400, 0, rtsp);
        return ERR_NOERROR;
        break;
    case -1:        // internal server error
        send_reply(500, 0, rtsp);
        return ERR_NOERROR;
        break;
    default:
        break;
    }
    if (strcmp(server, prefs_get_hostname()) != 0) {
    /* Currently this feature is disabled. */
    /* wrong server name */
    // fnc_log(FNC_LOG_ERR,"PLAY request specified an unknown server name.\n");
    // send_reply(404, 0 , rtsp); /* Not Found */
    // return ERR_NOERROR;
    }
    if (strstr(object, "../")) {
        /* disallow relative paths outside of current directory. */
        send_reply(403, 0, rtsp);    /* Forbidden */
        return ERR_NOERROR;
    }
    if (strstr(object, "./")) {
        /* Disallow ./ */
        send_reply(403, 0, rtsp);    /* Forbidden */
        return ERR_NOERROR;
    }
    p = strrchr(object, '.');
    url_is_file = 0;
    if (p == NULL) {
        send_reply(415, 0, rtsp);    /* Unsupported media type */
        return ERR_NOERROR;
    } else {
        url_is_file = is_supported_url(p);
    }
    q = strchr(object, '!');
    if (q == NULL) {
        // PLAY <file.sd>
        ptr = rtsp->session_list;
        if (ptr != NULL) {
            if (ptr->session_id == session_id) {
                // Search for the RTP session
                for (ptr2 = ptr->rtp_session;
                     ptr2 != NULL;
                     ptr2 = ptr2->next) {
                    if (ptr2->current_media->description.priority == 1) {
                        // Start playing all the presentation
                        if (!ptr2->started) {
                            // Start new
                            if (schedule_start (ptr2->sched_id, &args) ==
                                ERR_ALLOC)
                                return ERR_ALLOC;
                        } else {
                            // Resume existing
                            if (!ptr2->pause) {
                                //              fnc_log(FNC_LOG_INFO,"PLAY: already playing\n");
                            } else {
                                schedule_resume (ptr2->sched_id, &args);
                            }
                        }
                    }
                }
            } else {
                send_reply(454, 0, rtsp);    // Session not found
                return ERR_NOERROR;
            }
        } else {
            send_reply(415, 0, rtsp);    // Internal server error
            return ERR_GENERIC;
        }
    } else {
        if (url_is_file) {
            // PLAY <file.sd>!<file>                        
            ptr = rtsp->session_list;
            if (ptr != NULL) {
                if (ptr->session_id != session_id) {
                    send_reply(454, 0, rtsp);    // Session not found
                    return ERR_NOERROR;
                }
                // Search for the RTP session
                for (ptr2 = ptr->rtp_session; ptr2 != NULL;
                     ptr2 = ptr2->next) {
                    if (strcmp
                        (ptr2->current_media->filename,
                         q + 1) == 0) {
                        break;
                    }
                }
                if (ptr2 != NULL) {
                    // FOUND. Start Playing
                    if (schedule_start (ptr2->sched_id, &args) == ERR_ALLOC)
                        return ERR_ALLOC;
                } else {
                    send_reply(454, 0, rtsp);    // Session not found
                    return ERR_NOERROR;
                }
            } else {
                send_reply(415, 0, rtsp);    // Internal server error
                return ERR_GENERIC;
            }
        } else {
            // PLAY <file.sd>!<aggr>
            ptr = rtsp->session_list;
            if (ptr != NULL) {
                if (ptr->session_id != session_id) {
                    send_reply(454, 0, rtsp);    // Session not found
                    return ERR_NOERROR;
                }
                // It's an aggregate control. Play all the RTPs
                for (ptr2 = ptr->rtp_session; ptr2 != NULL;
                     ptr2 = ptr2->next) {
                    if (!ptr2->started) {
                        // Start new
                        if (schedule_start(ptr2->sched_id, &args) == ERR_ALLOC)
                            return ERR_ALLOC;
                    } else {
                        // Resume existing
                        if (!ptr2->pause) {
                        //fnc_log(FNC_LOG_INFO,"PLAY: already playing\n");
                        } else {
                            schedule_resume(ptr2->sched_id, &args);
                        }
                    }
                }
            } else {
                send_reply(415, 0, rtsp);    // Internal server error
                return ERR_GENERIC;
            }
        }
    }

    fnc_log(FNC_LOG_INFO, "PLAY %s RTSP/1.0 ", url);
    send_play_reply(rtsp, object, ptr);
    // See User-Agent 
    if ((p = strstr(rtsp->in_buffer, HDR_USER_AGENT)) != NULL) {
        char cut[strlen(p)];
        strcpy(cut, p);
        p = strstr(cut, "\n");
        cut[strlen(cut) - strlen(p) - 1] = '\0';
        fnc_log(FNC_LOG_CLIENT, "%s\n", cut);
    } else
        fnc_log(FNC_LOG_CLIENT, "- \n");

    return ERR_NOERROR;
}
