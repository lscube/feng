/* -*- c -*- */
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

#include "network/rtsp.h"

%% machine rtsp_request_line;

size_t ragel_parse_request_line(const char *msg, const size_t length, RTSP_Request *req) {
    int cs;
    const char *p = msg, *pe = p + length, *s = NULL;
    RTSP_Method method_code = RTSP_Method__Invalid;
    RFC822_Protocol protocol_code = RFC822_Protocol_Invalid;

    const char *method_str = NULL;
    size_t method_len = 0;

    const char *protocol_str = NULL;
    size_t protocol_len = 0;

    const char *object_str = NULL;
    size_t object_len = 0;

    %%{

        include RFC822Proto "rfc822proto-statemachine.rl";

        action set_s {
            s = p;
        }

        action end_method {
            method_str = s;
            method_len = p-s;
        }

        action end_protocol {
            protocol_str = s;
            protocol_len = p-s;
        }

        action end_object {
            object_str = s;
            object_len = p-s;
        }

        Request_Line := (
                RTSP_Method > set_s % end_method . SP
                (print*) > set_s % end_object . SP
                RFC822_Protocol > set_s % end_protocol . CRLF % from{ fbreak; } );

        write data noerror;
        write init;
        write exec noend;
    }%%

    if ( cs < rtsp_request_line_first_final )
        return ( p == pe ) ? 0 : -1;

    /* Only set these when the parsing was successful: an incomplete
     * request line won't help!
     */
    req->protocol = protocol_code;
    req->method = method_code;
    req->method_str = g_strndup(method_str, method_len);
    req->protocol_str = g_strndup(protocol_str, protocol_len);
    req->object = g_strndup(object_str, object_len);

    return p-msg;
}
