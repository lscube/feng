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
    const char *p = msg, *pe = p + length + 1, *s = NULL, *eof = pe;

    /* Map the variable used by the state machine directly into the
     * RTSP_Request structure so we avoid temporary variables. */
    #define method_code req->method
    #define protocol_code req->protocol

    %%{

        include RFC822Proto "rfc822proto-statemachine.rl";

        action set_s {
            s = p;
        }

        action end_method {
            req->method_str = g_strndup(s, p-s);
        }

        action end_protocol {
            req->protocol_str = g_strndup(s, p-s);
        }

        action end_object {
            req->object = g_strndup(s, p-s);
        }

        Request_Line = RTSP_Method > set_s % end_method . SP
            (print*) > set_s % end_object . SP
            RFC822_Protocol > set_s % end_protocol . CRLF;

        Header = (alpha|'-')+  . ':' . SP . print+ . CRLF;

        action stop_parser {
            return p-msg;
        }

        main := Request_Line % stop_parser . /.*/;

        write data noerror;
        write init;
        write exec;
    }%%

    if ( cs < rtsp_request_line_first_final )
        return 0;

    cs = rtsp_request_line_en_main; // Kill a warning

    return p-msg;
}
