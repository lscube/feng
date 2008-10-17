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
    GString *reply = g_string_new("");
    char encoded_object[256];

    /* allocate buffer */
    if (!reply) {
        fnc_log(FNC_LOG_ERR,
            "send_describe_reply(): unable to allocate memory\n");
        send_reply(500, NULL, rtsp);    /* internal server error */
        return ERR_ALLOC;
    }

    /*describe */
    g_string_printf(reply,
        "%s %d %s" RTSP_EL "CSeq: %d" RTSP_EL "Server: %s/%s" RTSP_EL,
        RTSP_VER, 200, get_stat(200), rtsp->rtsp_cseq, PACKAGE,
        VERSION);

    add_time_stamp_g(reply, 0);

    switch (cinfo->descr_format) {
        // Add new formats here
    case df_SDP_format:{
	    g_string_append(reply, "Content-Type: application/sdp" RTSP_EL);
            break;
        }
    }
    Url_encode (encoded_object, cinfo->url.path, sizeof(encoded_object));
    g_string_append_printf(reply,
			   "Content-Base: rtsp://%s/%s/" RTSP_EL,
			   cinfo->url.hostname, encoded_object);
             
    g_string_append_printf(reply,
			   "Content-Length: %zd" RTSP_EL,
			   cinfo->descr->len);
             
    // end of message
    g_string_append(reply, RTSP_EL);

    // concatenate description
    g_string_append(reply, cinfo->descr->str);

    bwrite(reply->str, reply->len, rtsp);
    g_string_free(reply, TRUE);

    fnc_log(FNC_LOG_CLIENT, "200 %d %s ", cinfo->descr->len, cinfo->url.path);

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
    RTSP_Error error;
    feng *srv = rtsp->srv;

    // Set some defaults
    ConnectionInfo cinfo = {
      .descr_format = df_SDP_format
    };

    // Extract the URL
    if ( (error = extract_url(rtsp, url)).got_error )
        goto error_management;
    // Validate URL
    else if ( (error = validate_url(url, &cinfo.url)).got_error )
        goto error_management;
    // Check for Forbidden Paths 
    else if ( (error = check_forbidden_path(&cinfo.url)).got_error )
        goto error_management;
    // Disallow Header REQUIRE
    else if ( (error = check_require_header(rtsp)).got_error )
        goto error_management;
    // Get the description format. SDP is recommended
    else if ( (error = get_description_format(rtsp, &cinfo)).got_error )
        goto error_management;
    // Get the CSeq
    else if ( (error = get_cseq(rtsp)).got_error )
        goto error_management;

    if (srv->num_conn > srv->srvconf.max_conns) {
        /*redirect */
        return send_redirect_3xx(rtsp, cinfo.url.path);
    }

    // Get Session Description
    cinfo.descr = sdp_session_descr(srv, cinfo.url.hostname, cinfo.url.path);

    /* The only error we may have here is when the file does not exist
       or if a demuxer is not available for the given file */
    if ( cinfo.descr == NULL ) {
      error = RTSP_NotFound;
      goto error_management;
    }

    fnc_log(FNC_LOG_INFO, "DESCRIBE %s RTSP/1.0 ", url);
    send_describe_reply(rtsp, &cinfo);
    log_user_agent(rtsp); // See User-Agent 

    g_string_free(cinfo.descr, TRUE);

    return ERR_NOERROR;

error_management:
    send_reply(error.message.reply_code, error.message.reply_str, rtsp);
    return ERR_GENERIC;
}
