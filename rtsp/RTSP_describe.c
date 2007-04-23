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

/** @file RTSP_describe.c
 * @brief Contains DESCRIBE method and reply handlers
 */

#include <fenice/rtsp.h>
#include <fenice/utils.h>
#include <fenice/prefs.h>
#include <fenice/sdp2.h>
#include <fenice/fnc_log.h>

#include <RTSP_utils.h>

/**
 * Sends the reply for the describe method
 * @param rtsp the buffer where to write the reply
 * @param cinfo the connection for which to send the reply
 * @return ERR_NOERROR or ERR_ALLOC on buffer allocation errors
 */
static int send_describe_reply(RTSP_buffer * rtsp, ConnectionInfo * cinfo)
{
    char *r;        /* reply message buffer */
    int mb_len;

    /* allocate buffer */
    mb_len = 1512 + strlen(cinfo->descr);
    r = malloc(mb_len);
    if (!r) {
        fnc_log(FNC_LOG_ERR,
            "send_describe_reply(): unable to allocate memory\n");
        send_reply(500, 0, rtsp);    /* internal server error */
        return ERR_ALLOC;
    }

    /*describe */
    snprintf(r, mb_len,
        "%s %d %s" RTSP_EL "CSeq: %d" RTSP_EL "Server: %s/%s" RTSP_EL,
        RTSP_VER, 200, get_stat(200), rtsp->rtsp_cseq, PACKAGE,
        VERSION);
    add_time_stamp(r, 0);
    switch (cinfo->descr_format) {
        // Add new formats here
    case df_SDP_format:{
            strcat(r, "Content-Type: application/sdp" RTSP_EL);
            break;
        }
    }
    snprintf(r + strlen(r), mb_len - strlen(r),
             "Content-Base: rtsp://%s/%s/" RTSP_EL,
             prefs_get_hostname(), cinfo->object);
    snprintf(r + strlen(r), mb_len - strlen(r),
             "Content-Length: %d" RTSP_EL,
             strlen(cinfo->descr));
    // end of message
    strcat(r, RTSP_EL);

    // concatenate description
    strcat(r, cinfo->descr);
    bwrite(r, (unsigned short) strlen(r), rtsp);

    free(r);

    fnc_log(FNC_LOG_CLIENT, "200 %d %s ", strlen(cinfo->descr), cinfo->object);

    return ERR_NOERROR;
}

/**
 * RTSP DESCRIBE method handler
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_describe(RTSP_buffer * rtsp)
{
    char url[255];
    ConnectionInfo cinfo;

    RTSP_Error error;

    cinfo.descr_format = df_SDP_format; // shawill put to some default
    // Extract the URL
    if ( (error = extract_url(rtsp, url)).got_error )
        goto error_management;
    // Validate URL
    else if ( (error = validate_url(url, &cinfo)).got_error )
        goto error_management;
    // Check for Forbidden Paths 
    else if ( (error = check_forbidden_path(&cinfo)).got_error )
        goto error_management;
    // Disallow Header REQUIRE
    else if ( (error = check_require_header(rtsp)).got_error )
        goto error_management;
    // Get the description format. SDP is recomended
    else if ( (error = get_description_format(rtsp, &cinfo)).got_error )
        goto error_management;
    // Get the CSeq
    else if ( (error = get_cseq(rtsp)).got_error )
        goto error_management;
    // Get Session Description
    else if ( (error = get_session_description(&cinfo)).got_error )
        goto error_management;

    if (max_connection() == ERR_GENERIC) {
        /*redirect */
        return send_redirect_3xx(rtsp, cinfo.object);
    }

    fnc_log(FNC_LOG_INFO, "DESCRIBE %s RTSP/1.0 ", url);
    send_describe_reply(rtsp, &cinfo);
    log_user_agent(rtsp); // See User-Agent 

    return ERR_NOERROR;

error_management:
    send_reply(error.message.reply_code, error.message.reply_str, rtsp);
    return ERR_NOERROR;
}
