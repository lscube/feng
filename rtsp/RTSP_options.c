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

/** @file RTSP_options.c
 * @brief Contains OPTIONS method and reply handlers
 */

#include <fenice/rtsp.h>
#include <fenice/fnc_log.h>

#include <RTSP_utils.h>

/**
 * Sends the reply for the options method
 * @param rtsp the buffer where to write the reply
 * @return ERR_NOERROR
 */
static int send_options_reply(RTSP_buffer * rtsp)
{
    char r[1024];
    long int cseq = rtsp->rtsp_cseq;

    sprintf(r, "%s %d %s" RTSP_EL "CSeq: %ld" RTSP_EL, RTSP_VER, 200,
        get_stat(200), cseq);
    strcat(r, "Public: OPTIONS,DESCRIBE,SETUP,PLAY,PAUSE,TEARDOWN,SET_PARAMETER" RTSP_EL);
    strcat(r, RTSP_EL);
    bwrite(r, (unsigned short) strlen(r), rtsp);

    fnc_log(FNC_LOG_CLIENT, "200 - - ");

    return ERR_NOERROR;
}

/**
 * RTSP OPTIONS method handler
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_options(RTSP_buffer * rtsp)
{
    char url[255];
    char method[255];
    char ver[255];

    RTSP_Error error;

    if ( (error = get_cseq(rtsp)).got_error ) // Get the CSeq 
        goto error_management;

    sscanf(rtsp->in_buffer, " %31s %255s %31s ", method, url, ver);

    fnc_log(FNC_LOG_INFO, "%s %s %s ", method, url, ver);
    send_options_reply(rtsp);
    log_user_agent(rtsp); // See User-Agent 

    return ERR_NOERROR;

error_management:
    send_reply(error.message.reply_code, error.message.reply_str, rtsp);
    return ERR_NOERROR;
}
