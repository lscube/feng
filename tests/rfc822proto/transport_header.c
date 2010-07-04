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
#include "rtp.h"

static void compare_header(const char *header,
                           const struct ParsedTransport *transports_expected,
                           size_t transports_count)
{
    GSList *transports = ragel_parse_transport_header(header);

    if ( transports_count == 0 ) {
        g_assert(transports == NULL);
    } else {
        GSList *current_transport = transports;
        size_t i;

        g_assert(transports != NULL);
        g_assert_cmpuint(g_slist_length(transports), ==, transports_count);

        for(i = 0; i < transports_count; i++) {
            struct ParsedTransport *transport = current_transport->data;

            g_assert_cmpuint(transport->protocol, ==, transports_expected[i].protocol);
            g_assert_cmpuint(transport->mode, ==, transports_expected[i].mode);
            g_assert_cmpint(transport->rtp_channel, ==, transports_expected[i].rtp_channel);
            g_assert_cmpint(transport->rtcp_channel, ==, transports_expected[i].rtcp_channel);

            g_slice_free(struct ParsedTransport, transport);
            current_transport = g_slist_next(current_transport);
        }

        g_slist_free(transports);
    }
}

#define runtest compare_header(header, expected, sizeof(expected)/sizeof(expected[0]))

void test_transport_header_udp_unicast_short()
{
    static const char header[] = "RTP/AVP;unicast;client_port=5000-5001";
    static const struct ParsedTransport expected[] = {
        {
            .protocol = RTP_UDP,
            .mode = TransportUnicast,
            .rtp_channel = 5000,
            .rtcp_channel = 5001
        }
    };

    runtest;
}

void test_transport_header_udp_unicast()
{
    static const char header[] = "RTP/AVP;unicast;client_port=5000-5001";
    static const struct ParsedTransport expected[] = {
        {
            .protocol = RTP_UDP,
            .mode = TransportUnicast,
            .rtp_channel = 5000,
            .rtcp_channel = 5001
        }
    };

    runtest;
}

void test_transport_header_tcp_interleaved()
{
    static const char header[] = "RTP/AVP/TCP;unicast;interleaved=0-1";
    static const struct ParsedTransport expected[] = {
        {
            .protocol = RTP_TCP,
            .mode = TransportUnicast,
            .rtp_channel = 0,
            .rtcp_channel = 1
        }
    };

    runtest;
}

void test_transport_header_tcp_quicktime_style()
{
    static const char header[] = "RTP/AVP/TCP;unicast";
    static const struct ParsedTransport expected[] = {
        {
            .protocol = RTP_TCP,
            .mode = TransportUnicast,
            .rtp_channel = -1,
            .rtcp_channel = -1
        }
    };

    runtest;
}

void test_transport_header_sctp()
{
    static const char header[] = "RTP/AVP/SCTP;unicast;streams=0-1";
    static const struct ParsedTransport expected[] = {
        {
            .protocol = RTP_SCTP,
            .mode = TransportUnicast,
            .rtp_channel = 0,
            .rtcp_channel = 1
        }
    };

    runtest;
}

void test_transport_header_unknown()
{
    static const char header[] = "RTP/AVP/FENGTEST;unicast";
    compare_header(header, NULL, 0);
}

void test_transport_header_unknown_and_valid()
{
    static const char header[] = "RTP/AVP/FENGTEST;unicast,RTP/AVP;unicast;client_port=5000-5001";
    static const struct ParsedTransport expected[] = {
        {
            .protocol = RTP_UDP,
            .mode = TransportUnicast,
            .rtp_channel = 5000,
            .rtcp_channel = 5001
        }
    };

    runtest;
}

void test_transport_header_multiple()
{
    static const char header[] = "RTP/AVP;unicast;client_port=5000-5001,RTP/AVP/SCTP;unicast;streams=1-2";
    static const struct ParsedTransport expected[] = {
        {
            .protocol = RTP_UDP,
            .mode = TransportUnicast,
            .rtp_channel = 5000,
            .rtcp_channel = 5001
        },
        {
            .protocol = RTP_SCTP,
            .mode = TransportUnicast,
            .rtp_channel = 1,
            .rtcp_channel = 2
        }
    };

    runtest;
}
