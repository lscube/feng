/* * 
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 * 
 * bufferpool is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * bufferpool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with bufferpool; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA 
 *
 * */

/** @file
 * @brief Contains DESCRIBE method and reply handlers
 */

#include "rtsp.h"
#include <fenice/utils.h>
#include <fenice/prefs.h>
#include "sdp2.h"
#include <fenice/fnc_log.h>

#include "rtsp_utils.h"

/**
 * Sends the reply for the describe method
 * @param rtsp the buffer where to write the reply
 * @param url the URL for the resource to describe
 * @param descr the description string to send
 * @param descr_format the description format to use
 * @return ERR_NOERROR or ERR_ALLOC on buffer allocation errors
 */
static int send_describe_reply(RTSP_buffer * rtsp, Url *url, GString *descr,
			       RTSP_description_format descr_format)
{
    GString *reply = rtsp_generate_ok_response(rtsp->rtsp_cseq, 0);
    char encoded_object[256];

    /* allocate buffer */
    if (!reply) {
        fnc_log(FNC_LOG_ERR,
            "send_describe_reply(): unable to allocate memory\n");
        send_reply(500, NULL, rtsp);    /* internal server error */
        return ERR_ALLOC;
    }

    switch (descr_format) {
        // Add new formats here
    case df_SDP_format:{
	    g_string_append(reply, "Content-Type: application/sdp" RTSP_EL);
            break;
        }
    }
    Url_encode (encoded_object, url->path, sizeof(encoded_object));
    g_string_append_printf(reply,
			   "Content-Base: rtsp://%s/%s/" RTSP_EL,
			   url->hostname, encoded_object);
             
    g_string_append_printf(reply,
			   "Content-Length: %zd" RTSP_EL,
			   descr->len);
             
    // end of message
    g_string_append(reply, RTSP_EL);

    // concatenate description
    g_string_append(reply, descr->str);

    bwrite(reply, rtsp);

    fnc_log(FNC_LOG_CLIENT, "200 %d %s ", descr->len, url->path);

    return ERR_NOERROR;
}

/**
 * Gets the required media description format from the RTSP request
 * @param rtsp the buffer of the request
 * @return The enumeration for the format
 */
static RTSP_description_format get_description_format(RTSP_buffer *rtsp)
{
    if (strstr(rtsp->in_buffer, HDR_ACCEPT) != NULL) {
        if (strstr(rtsp->in_buffer, "application/sdp") != NULL)
	  return df_SDP_format;
	else
	  return df_Unsupported; // Add here new description formats
    }
    
    return df_SDP_format;
    // For now default to SDP if unknown
    // return df_Unknown;
}

/**
 * RTSP DESCRIBE method handler
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_describe(RTSP_buffer * rtsp)
{
    RTSP_Error error;
    feng *srv = rtsp->srv;

    Url url;
    GString *descr;
    RTSP_description_format descr_format;

    // Extract the URL
    // Extract and validate the URL
    if ( (error = rtsp_extract_validate_url(rtsp, &url)).got_error )
	goto error_management;
    // Disallow Header REQUIRE
    if ( (error = check_require_header(rtsp)).got_error )
        goto error_management;
    // Get the CSeq
    if ( (error = get_cseq(rtsp)).got_error )
        goto error_management;

    // Get the description format. SDP is the only supported
    if ( (descr_format = get_description_format(rtsp)) == df_Unsupported ) {
      error = RTSP_OptionNotSupported;
      goto error_management;
    }
    
    if (srv->num_conn > srv->srvconf.max_conns) {
        /*redirect */
        return send_redirect_3xx(rtsp, url.path);
    }

    // Get Session Description
    descr = sdp_session_descr(srv, url.hostname, url.path);

    /* The only error we may have here is when the file does not exist
       or if a demuxer is not available for the given file */
    if ( descr == NULL ) {
      error = RTSP_NotFound;
      goto error_management;
    }

    fnc_log(FNC_LOG_INFO, "DESCRIBE %s RTSP/1.0 ", url);
    send_describe_reply(rtsp, &url, descr, descr_format);
    log_user_agent(rtsp); // See User-Agent 

    g_string_free(descr, TRUE);

    return ERR_NOERROR;

error_management:
    send_reply(error.message.reply_code, error.message.reply_str, rtsp);
    return ERR_GENERIC;
}
