/* * 
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 * 
 * bufferpool is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * bufferpool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with bufferpool; if not, write to the Free Software
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

typedef enum {
    CNAME = 1,
    NAME = 2,
    EMAIL = 3,
    PHONE = 4,
    LOC = 5,
    TOOL = 6,
    NOTE = 7,
    PRIV = 8
} rtcp_info;

typedef struct RTCP_header {
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    uint8_t count:5;    //< SC or RC
    uint8_t padding:1;
    uint8_t version:2;
#elif (G_BYTE_ORDER == G_BIG_ENDIAN)
    uint8_t version:2;
    uint8_t padding:1;
    uint8_t count:5;    //< SC or RC
#else
#error Neither big nor little
#endif
    uint8_t pt;
    uint16_t length;
} RTCP_header;

typedef struct RTCP_header_SR {
    uint32_t ssrc;
    uint32_t ntp_timestampH;
    uint32_t ntp_timestampL;
    uint32_t rtp_timestamp;
    uint32_t pkt_count;
    uint32_t octet_count;
} RTCP_header_SR;

typedef struct RTCP_header_RR {
    uint32_t ssrc;
} RTCP_header_RR;

typedef struct RTCP_header_SR_report_block {
    uint32_t ssrc;
    uint8_t fract_lost;
    uint8_t pck_lost[3];
    uint32_t h_seq_no;
    uint32_t jitter;
    uint32_t last_SR;
    uint32_t delay_last_SR;
} RTCP_header_SR_report_block;

typedef struct RTCP_header_SDES {
    uint32_t ssrc;
    uint8_t attr_name;
    uint8_t len;
} __attribute__((__packed__)) RTCP_header_SDES;

typedef struct RTCP_header_BYE {
    uint32_t ssrc;
    uint8_t length;
} __attribute__((__packed__)) RTCP_header_BYE;

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
