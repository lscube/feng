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
/**
 * @file
 *
 * @brief Shared definitions between rtsp_method_setup.h and
 *        ragel_range.rl state machine.
 */

#include <network/rtsp.h>

/**
 * @brief Structure filled by the ragel parser of the transport header.
 *
 * @internal
 */
struct ParsedTransport {
    enum { TransportUDP, TransportTCP, TransportSCTP } protocol;
    //! Mode for UDP transmission, here is easier to access
    enum { TransportUnicast, TransportMulticast } mode;
    union {
        union {
            struct {
                uint16_t port_rtp;
                uint16_t port_rtcp;
            } Unicast;
            struct {
            } Multicast;
        } UDP;
        struct {
            uint16_t ich_rtp;  //!< Interleaved channel for RTP
            uint16_t ich_rtcp; //!< Interleaved channel for RTCP
        } TCP;
        struct {
            uint16_t ch_rtp;  //!< SCTP channel for RTP
            uint16_t ch_rtcp; //!< SCTP channel for RTCP
        } SCTP;
    } parameters;
};

gboolean check_parsed_transport(RTSP_Client *rtsp, RTP_transport *rtp_t,
                                struct ParsedTransport *transport);

