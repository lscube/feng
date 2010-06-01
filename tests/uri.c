/*
 * This file is part of feng
 *
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
 * See AUTHORS for more details
 *
 * NetEmbryo is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * NetEmbryo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with NetEmbryo; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include "src/network/uri.h"
#include <glib.h>
#include "gtest-extra.h"

static void test_url(const char * url, const char * scheme,
                     const char * host, const char * port,
                     const char * path)
{
    URI *uri = uri_parse(url);

    g_assert(uri != NULL);

    if ( scheme && !uri->scheme )
        gte_fail("Expected scheme specified, but no scheme identified");
    else if ( !scheme && uri->scheme )
        gte_fail("No scheme expected, but scheme identified: '%s'", uri->scheme);
    else if ( scheme && scheme )
        g_assert_cmpstr(uri->scheme, ==, scheme);

    if ( host && !uri->host )
        gte_fail("Expected host specified, but no host identified");
    else if ( !host && uri->host )
        gte_fail("No host expected, but host identified: '%s'", uri->host);
    else if ( host && host )
        g_assert_cmpstr(uri->host, ==, host);

    if ( port && !uri->port )
        gte_fail("Expected port specified, but no port identified");
    else if ( !port && uri->port )
        gte_fail("No port expected, but port identified: '%s'", uri->port);
    else if ( port && port )
        g_assert_cmpstr(uri->port, ==, port);

    if ( path && !uri->path )
        gte_fail("Expected path specified, but no path identified");
    else if ( !path && uri->path )
        gte_fail("No path expected, but path identified: '%s'", uri->path);
    else if ( path && path )
        g_assert_cmpstr(uri->path, ==, path);

    uri_free(uri);
}

void test_long_url()
{
    test_url("rtsp://this.is.a.very.long.url:123456/this/is/a/very/long/absolute/path/to/file.wmv",
             "rtsp", "this.is.a.very.long.url", "123456", "/this/is/a/very/long/absolute/path/to/file.wmv");
}

void test_split_url()
{
    test_url("rtsp://this.is.the.host/this/is/the/path.avi", "rtsp", "this.is.the.host", NULL, "/this/is/the/path.avi");
}

#if 0
void test_no_proto()
{
    test_url("host:80/file.wmv", NULL, "host", "80", "file.wmv");
}

void test_no_path()
{
    test_url("host/file.wmv", NULL, "host", NULL, "file.wmv");
}

void test_just_host()
{
    test_url("host", NULL, "host", NULL, NULL);
}
#endif

void test_just_proto_host()
{
    test_url("rtsp://host", "rtsp", "host", NULL, NULL);
}

void test_just_proto_host_port()
{
    test_url("rtsp://host:1234", "rtsp", "host", "1234", NULL);
}
