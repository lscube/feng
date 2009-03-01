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
 * @brief Contains OPTIONS method and reply handlers
 */

#include "rtsp.h"
#include <fenice/fnc_log.h>


/**
 * Sends the reply for the options method
 * @param rtsp the buffer where to write the reply
 * @param req The client request for the method
 * @return ERR_NOERROR
 */
static int send_options_reply(RTSP_buffer * rtsp, RTSP_Request *req)
{
    GString *reply = rtsp_generate_ok_response(req);

    g_string_append(reply,
		    "Public: OPTIONS,DESCRIBE,SETUP,PLAY,PAUSE,TEARDOWN,SET_PARAMETER" RTSP_EL);
    g_string_append(reply, RTSP_EL);

    rtsp_bwrite(rtsp, reply);

    fnc_log(FNC_LOG_CLIENT, "200 - - ");

    return ERR_NOERROR;
}

/**
 * RTSP OPTIONS method handler
 * @param rtsp the buffer for which to handle the method
 * @param req The client request for the method
 * @return ERR_NOERROR
 */
int RTSP_options(RTSP_buffer * rtsp, RTSP_Request *req)
{
    send_options_reply(rtsp, req);

    return ERR_NOERROR;
}
