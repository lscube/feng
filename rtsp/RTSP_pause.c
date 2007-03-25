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

#include <fenice/rtsp.h>
#include <fenice/utils.h>
#include <fenice/prefs.h>
#include <fenice/fnc_log.h>

/*
     ****************************************************************
     *            PAUSE METHOD HANDLING
     ****************************************************************
*/

int RTSP_pause(RTSP_buffer * rtsp)
{
    long int session_id;
    char *p;
    RTSP_session *s;
    RTP_session *r;
    char object[255], server[255], trash[255];
    unsigned short port;
    char url[255];

    // CSeq
    if ((p = strstr(rtsp->in_buffer, HDR_CSEQ)) == NULL) {
        send_reply(400, 0, rtsp);    /* Bad Request */
        return ERR_NOERROR;
    } else {
        if (sscanf(p, "%254s %d", trash, &(rtsp->rtsp_cseq)) != 2) {
            send_reply(400, 0, rtsp);    /* Bad Request */
            return ERR_NOERROR;
        }
    }
    /* Extract the URL */
    if (!sscanf(rtsp->in_buffer, " %*s %254s ", url)) {
        send_reply(400, 0, rtsp);    /* bad request */
        return ERR_NOERROR;
    }
    /* Validate the URL */
    switch (parse_url
        (url, server, sizeof(server), &port, object, sizeof(object))) {
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
    if (strcmp(server, prefs_get_hostname()) != 0) {    /* Currently this feature is disabled. */
        /* wrong server name */
        //      fnc_log(FNC_LOG_ERR,"PAUSE request specified an unknown server name.\n");
        //      send_reply(404, 0 , rtsp); /* Not Found */
        //      return ERR_NOERROR;
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

    p = strrchr(strtok(object, "!"), '.');
    if (p == NULL) {
        send_reply(415, 0, rtsp);    /* Unsupported media type */
        return ERR_NOERROR;
        }
    // Session
    if ((p = strstr(rtsp->in_buffer, HDR_SESSION)) != NULL) {
        if (sscanf(p, "%254s %ld", trash, &session_id) != 2) {
            send_reply(454, 0, rtsp);    /* Session Not Found */
            return ERR_NOERROR;
        }
    } else {
        session_id = -1;
    }
    s = rtsp->session_list;
    if (s == NULL) {
        send_reply(415, 0, rtsp);    // Internal server error
        return ERR_GENERIC;
    }
    if (s->session_id != session_id) {
        send_reply(454, 0, rtsp);    /* Session Not Found */
        return ERR_NOERROR;
    }
    for (r = s->rtp_session; r != NULL; r = r->next) {
        r->pause = 1;
    }

    fnc_log(FNC_LOG_INFO, "PAUSE %s RTSP/1.0 ", url);
    send_pause_reply(rtsp, s);
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
