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

#include <liberis/headers.h>

#include "rtsp.h"

/**
 * RTSP OPTIONS method handler
 * @param rtsp the buffer for which to handle the method
 * @param req The client request for the method
 * @return ERR_NOERROR
 */
void RTSP_options(ATTR_UNUSED RTSP_Client * rtsp, RTSP_Request *req)
{
    RTSP_Response *response = rtsp_response_new(req, RTSP_Ok);

    g_hash_table_insert(response->headers,
                        g_strdup(eris_hdr_public),
                        g_strdup("OPTIONS,DESCRIBE,SETUP,PLAY,PAUSE,TEARDOWN"));

    rtsp_response_send(response);
}
