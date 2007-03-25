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

#include <fenice/rtsp.h>
#include <fenice/fnc_log.h>

/*
     ****************************************************************
     *            OPTIONS METHOD HANDLING
     ****************************************************************
*/

int RTSP_options(RTSP_buffer * rtsp)
{

    char *p;
    char trash[255];
    char url[255];
    char method[255];
    char ver[255];
    unsigned int cseq;


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
    cseq = rtsp->rtsp_cseq;

    sscanf(rtsp->in_buffer, " %31s %255s %31s ", method, url, ver);

    fnc_log(FNC_LOG_INFO, "%s %s %s ", method, url, ver);
    send_options_reply(rtsp, cseq);
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
