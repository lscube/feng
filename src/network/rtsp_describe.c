/* * 
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 * 
 * feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * feng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with feng; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA 
 *
 * */

/** @file
 * @brief Contains DESCRIBE method and reply handlers
 */

#include <stdbool.h>

#include "rtsp.h"
#include <fenice/utils.h>
#include <fenice/prefs.h>
#include "sdp2.h"
#include <fenice/fnc_log.h>

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
    char *encoded_object;

    /* allocate buffer */
    if (!reply) {
        fnc_log(FNC_LOG_ERR,
            "send_describe_reply(): unable to allocate memory\n");
        rtsp_send_reply(rtsp, RTSP_InternalServerError);
        return ERR_ALLOC;
    }

    switch (descr_format) {
        // Add new formats here
    case df_SDP_format:{
	    g_string_append(reply, "Content-Type: application/sdp" RTSP_EL);
            break;
        }
    }
    encoded_object = g_uri_escape_string(url->path, NULL, false);
    g_string_append_printf(reply,
			   "Content-Base: rtsp://%s/%s/" RTSP_EL,
			   url->hostname, encoded_object);
    g_free(encoded_object);
             
    g_string_append_printf(reply,
			   "Content-Length: %zd" RTSP_EL,
			   descr->len);
             
    // end of message
    g_string_append(reply, RTSP_EL);

    // concatenate description
    g_string_append(reply, descr->str);

    rtsp_bwrite(rtsp, reply);

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
 * @brief Checks if the RTSP message is a request with supported
 *        options
 *
 * The HDR_REQUIRE header is not supported by feng so it will always
 * be not supported if present.
 * 
 * @param rtsp the buffer of the request to check
 *
 * @retval true No Require header present.
 * @retval false Require header present, not supported.
 */
static gboolean check_require_header(RTSP_buffer * rtsp)
{
    return strstr(rtsp->in_buffer, HDR_REQUIRE) == NULL;
}

/**
 * RTSP DESCRIBE method handler
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_describe(RTSP_buffer * rtsp)
{
    RTSP_ResponseCode error;
    feng *srv = rtsp->srv;

    Url url;
    GString *descr;
    RTSP_description_format descr_format;

    if ( !rtsp_get_url(rtsp, &url) )
        return ERR_GENERIC;

    // Disallow Header REQUIRE
    if ( !check_require_header(rtsp) ) {
        error = RTSP_OptionNotSupported;
        goto error_management;
    }

    // Get the description format. SDP is the only supported
    if ( (descr_format = get_description_format(rtsp)) == df_Unsupported ) {
      error = RTSP_OptionNotSupported;
      goto error_management;
    }
    
    if (srv->num_conn > srv->srvconf.max_conns) {
        /* @todo should redirect, but we haven't the code to do that just
         * yet. */
        rtsp_send_reply(rtsp, RTSP_InternalServerError);
        return ERR_GENERIC;
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
    rtsp_send_reply(rtsp, error);
    return ERR_GENERIC;
}
