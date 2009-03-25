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

#ifndef FN_RTCP_H
#define FN_RTCP_H

#include "rtp.h"

typedef enum {
    SR = 200,
    RR = 201,
    SDES = 202,
    BYE = 203,
    APP = 204
} rtcp_pkt_type;

int RTCP_send_packet(RTP_session * session, rtcp_pkt_type type);
int RTCP_recv_packet(RTP_session * session);
int RTCP_handler(RTP_session * session);
int RTCP_flush(RTP_session * session);

int RTCP_get_pkt_lost(RTP_session * session);

float RTCP_get_fract_lost(RTP_session * session);

unsigned int RTCP_get_jitter(RTP_session * session);

unsigned int RTCP_get_RR_received(RTP_session * session);

unsigned int RTCP_get_total_packet(RTP_session * session);

unsigned int RTCP_get_total_payload_octet(RTP_session * session);

#endif // FN_RTCP_H
