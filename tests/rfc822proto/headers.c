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

#define ragel_read_constant_rtsp_headers(headers, string, read_size)  \
    ragel_read_rtsp_headers(headers, string, sizeof(string)-1, read_size)

void test_single_header() {
    GHashTable *headers = rtsp_headers_new();
    size_t read_size = (size_t)-1;
    int res = ragel_read_constant_rtsp_headers(headers, "CSeq: 1\n", &read_size);

    g_assert_cmpint(res, ==, 0);
    g_assert_cmpint(read_size, ==, sizeof("CSeq: 1\n")-1);
    g_assert_cmpint(g_hash_table_size(headers), ==, 1);

    g_assert_cmpstr(rtsp_headers_lookup(headers, RTSP_Header_CSeq), ==, "1");

    rtsp_headers_destroy(headers);
}

void test_single_header_discarding() {
    GHashTable *headers = rtsp_headers_new();
    size_t read_size = (size_t)-1;
    int res = ragel_read_constant_rtsp_headers(headers, "CSeq: 1\nMyTest", &read_size);

    g_assert_cmpint(res, ==, 0);
    g_assert_cmpint(read_size, ==, sizeof("CSeq: 1\n")-1);
    g_assert_cmpint(g_hash_table_size(headers), ==, 1);

    g_assert_cmpstr(rtsp_headers_lookup(headers, RTSP_Header_CSeq), ==, "1");

    rtsp_headers_destroy(headers);
}

void test_two_headers() {
    GHashTable *headers = rtsp_headers_new();
    size_t read_size = (size_t)-1;
    int res = ragel_read_constant_rtsp_headers(headers, "CSeq: 1\nSession: Test\n", &read_size);

    g_assert_cmpint(res, ==, 0);
    g_assert_cmpint(read_size, ==, sizeof("CSeq: 1\nSession: Test\n")-1);
    g_assert_cmpint(g_hash_table_size(headers), ==, 2);

    g_assert_cmpstr(rtsp_headers_lookup(headers, RTSP_Header_CSeq), ==, "1");
    g_assert_cmpstr(rtsp_headers_lookup(headers, RTSP_Header_Session), ==, "Test");

    rtsp_headers_destroy(headers);
}


void test_two_headers_ending() {
    GHashTable *headers = rtsp_headers_new();
    size_t read_size = (size_t)-1;
    int res = ragel_read_constant_rtsp_headers(headers, "CSeq: 1\nSession: Test\n\n", &read_size);

    g_assert_cmpint(res, ==, 1);
    g_assert_cmpint(read_size, ==, sizeof("CSeq: 1\nSession: Test\n\n")-1);
    g_assert_cmpint(g_hash_table_size(headers), ==, 2);

    g_assert_cmpstr(rtsp_headers_lookup(headers, RTSP_Header_CSeq), ==, "1");
    g_assert_cmpstr(rtsp_headers_lookup(headers, RTSP_Header_Session), ==, "Test");

    rtsp_headers_destroy(headers);
}
