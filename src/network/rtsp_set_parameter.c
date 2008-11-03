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
 * @brief Contains SET_PARAMETER method and reply handlers
 */

#include "rtsp.h"
#include <fenice/fnc_log.h>

#include "rtsp_utils.h"

/**
 * Sends the reply for the set_parameter method
 * @param rtsp the buffer where to write the reply
 * @return ERR_NOERROR
 */
static int send_set_parameter_reply(RTSP_buffer * rtsp)
{
    GString *reply = rtsp_generate_response(451, rtsp->rtsp_cseq);

    /* No body */
    g_string_append(reply, RTSP_EL);

    bwrite(reply, rtsp);

    fnc_log(FNC_LOG_CLIENT, "451 - - ");

    return ERR_NOERROR;
}

/**
 * RTSP OPTIONS method handler
 * @param rtsp the buffer for which to handle the method
 * @return ERR_NOERROR
 */
int RTSP_set_parameter(RTSP_buffer * rtsp)
{
    char url[255];
    char method[255];
    char ver[255];

    RTSP_Error error;

    if ( !get_cseq(rtsp) ) {
        error = RTSP_BadRequest;
        goto error_management;
    }

    sscanf(rtsp->in_buffer, " %31s %255s %31s ", method, url, ver);

    fnc_log(FNC_LOG_INFO, "%s %s %s ", method, url, ver);
    send_set_parameter_reply(rtsp);
    log_user_agent(rtsp); // See User-Agent 

    return ERR_NOERROR;

error_management:
    send_reply(error.message.reply_code, error.message.reply_str, rtsp);
    return ERR_GENERIC;
}
