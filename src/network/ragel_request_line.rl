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
    const char *p = msg, *pe = p + length + 1, *s, *eof;

    /* We want to express clearly which versions we support, so that we
     * can return right away if an unsupported one is found.
     */

    %%{
        SP = ' ';
        CRLF = "\r\n";

        action set_s {
            s = p;
        }

        action end_method {
            req->method = g_strndup(s, p-s);
        }

        Supported_Method =
            "DESCRIBE" % { req->method_id = RTSP_ID_DESCRIBE; } |
            "OPTIONS" % { req->method_id = RTSP_ID_OPTIONS; } |
            "PAUSE" % { req->method_id = RTSP_ID_PAUSE; } |
            "PLAY" % { req->method_id = RTSP_ID_PLAY; } |
            "SETUP" % { req->method_id = RTSP_ID_SETUP; } |
            "TEARDOWN" % { req->method_id = RTSP_ID_TEARDOWN; };

        Method = (Supported_Method | alpha+ )
            > set_s % end_method;

        action end_version {
            req->version = g_strndup(s, p-s);
        }

        Version = (alpha+ . '/' . [0-9] '.' [0-9]);

        action end_object {
            req->object = g_strndup(s, p-s);
        }

        Request_Line = (Supported_Method | Method) . SP
            (print*) > set_s % end_object . SP
            Version > set_s % end_version . CRLF;

        Header = (alpha|'-')+  . ':' . SP . print+ . CRLF;

        action stop_parser {
            return p-msg;
        }

        main := Request_Line % stop_parser . /.*/;

        write data;
        write init;
        write exec;
    }%%

    if ( cs < rtsp_request_line_first_final )
        return 0;

    return p-msg;
}
