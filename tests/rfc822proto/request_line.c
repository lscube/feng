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

#define ragel_parse_constant_request_line()                 \
    ragel_parse_request_line(line, sizeof(line)-1, &req)

#define line_len                                \
    (strstr(line, "\r\n") - line + 2)

void test_request_line_rtsp10()
{
    size_t resval;
    RTSP_Request req;
    static const char line[] = "PLAY rtsp://host/object RTSP/1.0\r\n";

    resval = ragel_parse_constant_request_line();

    g_assert_cmpint(resval, ==, line_len);
    g_assert_cmpint(req.method, ==, RTSP_Method_PLAY);
    g_assert_cmpint(req.protocol, ==, RFC822_Protocol_RTSP10);
    g_assert_cmpstr(req.method_str, ==, "PLAY");
    g_assert_cmpstr(req.protocol_str, ==, "RTSP/1.0");
    g_assert_cmpstr(req.object, ==, "rtsp://host/object");
}

void test_request_line_rtsp10_discard()
{
    size_t resval;
    RTSP_Request req;
    static const char line[] = "PLAY rtsp://host/object RTSP/1.0\r\n         ";

    resval = ragel_parse_constant_request_line();

    g_assert_cmpint(resval, ==, line_len);
    g_assert_cmpint(req.method, ==, RTSP_Method_PLAY);
    g_assert_cmpint(req.protocol, ==, RFC822_Protocol_RTSP10);
    g_assert_cmpstr(req.method_str, ==, "PLAY");
    g_assert_cmpstr(req.protocol_str, ==, "RTSP/1.0");
    g_assert_cmpstr(req.object, ==, "rtsp://host/object");
}

void test_request_line_rtsp20()
{
    size_t resval;
    RTSP_Request req;
    static const char line[] = "PLAY rtsp://host/object RTSP/2.0\r\n";

    resval = ragel_parse_constant_request_line();

    g_assert_cmpint(resval, ==, line_len);
    g_assert_cmpint(req.method, ==, RTSP_Method_PLAY);
    g_assert_cmpint(req.protocol, ==, RFC822_Protocol_RTSP_UnsupportedVersion);
    g_assert_cmpstr(req.method_str, ==, "PLAY");
    g_assert_cmpstr(req.protocol_str, ==, "RTSP/2.0");
    g_assert_cmpstr(req.object, ==, "rtsp://host/object");
}

void test_request_line_rtsp10_method_unsupported()
{
    size_t resval;
    RTSP_Request req;
    static const char line[] = "MYMETHOD rtsp://host/object RTSP/1.0\r\n";

    resval = ragel_parse_constant_request_line();

    g_assert_cmpint(resval, ==, line_len);
    g_assert_cmpint(req.method, ==, RTSP_Method__Unsupported);
    g_assert_cmpint(req.protocol, ==, RFC822_Protocol_RTSP10);
    g_assert_cmpstr(req.method_str, ==, "MYMETHOD");
    g_assert_cmpstr(req.protocol_str, ==, "RTSP/1.0");
    g_assert_cmpstr(req.object, ==, "rtsp://host/object");
}

void test_request_line_rtsp10_method_invalid()
{
    size_t resval;
    RTSP_Request req;
    static const char line[] = "%My13Method rtsp://host/object RTSP/1.0\r\n";

    resval = ragel_parse_constant_request_line();

    g_assert_cmpint(resval, ==, -1);
}

void test_request_line_protocol_unsupported()
{
    size_t resval;
    RTSP_Request req;
    static const char line[] = "MYMETHOD * MYPROTO/2.1\r\n";

    resval = ragel_parse_constant_request_line();

    g_assert_cmpint(resval, ==, line_len);
    g_assert_cmpint(req.method, ==, RTSP_Method__Unsupported);
    g_assert_cmpint(req.protocol, ==, RFC822_Protocol_Unsupported);
    g_assert_cmpstr(req.method_str, ==, "MYMETHOD");
    g_assert_cmpstr(req.protocol_str, ==, "MYPROTO/2.1");
    g_assert_cmpstr(req.object, ==, "*");
}

void test_request_line_protocol_invalid()
{
    size_t resval;
    RTSP_Request req;
    static const char line[] = "MYMETHOD * MYPROTO\r\n";

    resval = ragel_parse_constant_request_line();

    g_assert_cmpint(resval, ==, -1);
}

void test_request_line_bogus()
{
    size_t resval;
    RTSP_Request req;
    static const char line[] = "NOT_A_REQUEST_LINE\r\n";

    resval = ragel_parse_constant_request_line();

    g_assert_cmpint(resval, ==, -1);
}

void test_request_line_incomplete_missing_newline()
{
    size_t resval;
    RTSP_Request req = {
        .method = RTSP_Method__Invalid,
        .protocol = RFC822_Protocol_Invalid,
        .method_str = NULL,
        .protocol_str = NULL,
        .object = NULL
    };
    static const char line[] = "PLAY rtsp://host/object RTSP/1.0";

    resval = ragel_parse_constant_request_line();

    g_assert_cmpint(resval, ==, 0);
    g_assert_cmpint(req.method, ==, RTSP_Method__Invalid);
    g_assert_cmpint(req.protocol, ==, RFC822_Protocol_Invalid);
    g_assert_cmpstr(req.method_str, ==, NULL);
    g_assert_cmpstr(req.protocol_str, ==, NULL);
    g_assert_cmpstr(req.object, ==, NULL);
}

void test_request_line_incomplete_missing_protocol()
{
    size_t resval;
    RTSP_Request req = {
        .method = RTSP_Method__Invalid,
        .protocol = RFC822_Protocol_Invalid,
        .method_str = NULL,
        .protocol_str = NULL,
        .object = NULL
    };
    static const char line[] = "PLAY rtsp://host/object";

    resval = ragel_parse_constant_request_line();

    g_assert_cmpint(resval, ==, 0);
    g_assert_cmpint(req.method, ==, RTSP_Method__Invalid);
    g_assert_cmpint(req.protocol, ==, RFC822_Protocol_Invalid);
    g_assert_cmpstr(req.method_str, ==, NULL);
    g_assert_cmpstr(req.protocol_str, ==, NULL);
    g_assert_cmpstr(req.object, ==, NULL);
}

void test_request_line_incomplete_partial_object()
{
    size_t resval;
    RTSP_Request req = {
        .method = RTSP_Method__Invalid,
        .protocol = RFC822_Protocol_Invalid,
        .method_str = NULL,
        .protocol_str = NULL,
        .object = NULL
    };
    static const char line[] = "PLAY rtsp://";

    resval = ragel_parse_constant_request_line();

    g_assert_cmpint(resval, ==, 0);
    g_assert_cmpint(req.method, ==, RTSP_Method__Invalid);
    g_assert_cmpint(req.protocol, ==, RFC822_Protocol_Invalid);
    g_assert_cmpstr(req.method_str, ==, NULL);
    g_assert_cmpstr(req.protocol_str, ==, NULL);
    g_assert_cmpstr(req.object, ==, NULL);
}

void test_request_line_incomplete_missing_object()
{
    size_t resval;
    RTSP_Request req = {
        .method = RTSP_Method__Invalid,
        .protocol = RFC822_Protocol_Invalid,
        .method_str = NULL,
        .protocol_str = NULL,
        .object = NULL
    };
    static const char line[] = "PLAY";

    resval = ragel_parse_constant_request_line();

    g_assert_cmpint(resval, ==, 0);
    g_assert_cmpint(req.method, ==, RTSP_Method__Invalid);
    g_assert_cmpint(req.protocol, ==, RFC822_Protocol_Invalid);
    g_assert_cmpstr(req.method_str, ==, NULL);
    g_assert_cmpstr(req.protocol_str, ==, NULL);
    g_assert_cmpstr(req.object, ==, NULL);
}

void test_request_line_incomplete_empty()
{
    size_t resval;
    RTSP_Request req = {
        .method = RTSP_Method__Invalid,
        .protocol = RFC822_Protocol_Invalid,
        .method_str = NULL,
        .protocol_str = NULL,
        .object = NULL
    };
    static const char line[] = "";

    resval = ragel_parse_constant_request_line();

    g_assert_cmpint(resval, ==, 0);
    g_assert_cmpint(req.method, ==, RTSP_Method__Invalid);
    g_assert_cmpint(req.protocol, ==, RFC822_Protocol_Invalid);
    g_assert_cmpstr(req.method_str, ==, NULL);
    g_assert_cmpstr(req.protocol_str, ==, NULL);
    g_assert_cmpstr(req.object, ==, NULL);
}
