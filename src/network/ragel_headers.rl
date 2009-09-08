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

#include "rtsp.h"

%% machine rtsp_headers;

int ragel_read_rtsp_headers(GHashTable *headers, const char *msg,
                            size_t length, size_t *read_size)
{
    int cs;
    const char *p = msg, *pe = p + length;
    RTSP_Header header_code = RTSP_Header__Invalid;

    const char *header_str = NULL;
    size_t header_len = 0;

    %%{

        include RFC822Proto "rfc822proto-statemachine.rl";

        action set_header_str {
            header_str = p;
        }

        action set_header_len {
            header_len = p - header_str;
        }

        action save_header {
            if ( header_code != RTSP_Header__Invalid &&
                 header_code != RTSP_Header__Unsupported )
                rtsp_headers_set(headers, header_code,
                                 g_strndup(header_str, header_len));

            header_code = RTSP_Header__Invalid;
            header_str = NULL;
            header_len = 0;
            *read_size = p - msg;
        }

        RTSP_Header = (
                       RTSP_Header_Name . ':' WSP * .
                       VCHAR+ > set_header_str % set_header_len .
                       WSP * . CRLF
                       ) %*save_header;

    RTSP_Headers := RTSP_Header * . CRLF % from{ *read_size = p - msg; fbreak; };

        write data noerror;
        write init;
        write exec noend;
    }%%

    if ( cs < rtsp_headers_first_final )
        return ( p == pe ) ? 0 : -1;

    return 1;
}
