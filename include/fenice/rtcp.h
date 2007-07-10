/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
 * See AUTHORS for more details 
 *  
 * Feng is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Feng is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *  
 * */

#ifndef _RTCPH
#define _RTCPH

#include <fenice/rtp.h>

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

typedef struct _RTCP_header {
#if (BYTE_ORDER == LITTLE_ENDIAN)
    unsigned int count:5;    //< SC or RC
    unsigned int padding:1;
    unsigned int version:2;
#elif (BYTE_ORDER == BIG_ENDIAN)
    unsigned int version:2;
    unsigned int padding:1;
    unsigned int count:5;    //< SC or RC
#else
#error Neither big nor little
#endif
    unsigned int pt:8;
    unsigned int length:16;
} RTCP_header;

typedef struct _RTCP_header_SR {
    unsigned int ssrc;
    unsigned int ntp_timestampH;
    unsigned int ntp_timestampL;
    unsigned int rtp_timestamp;
    unsigned int pkt_count;
    unsigned int octet_count;
} RTCP_header_SR;

typedef struct _RTCP_header_RR {
    unsigned int ssrc;
} RTCP_header_RR;

typedef struct _RTCP_header_SR_report_block {
    unsigned long ssrc;
    unsigned char fract_lost;
    unsigned char pck_lost[3];
    unsigned int h_seq_no;
    unsigned int jitter;
    unsigned int last_SR;
    unsigned int delay_last_SR;
} RTCP_header_SR_report_block;

typedef struct _RTCP_header_SDES {
    unsigned int ssrc;
    unsigned char attr_name;
    unsigned char len;
} __attribute__((__packed__)) RTCP_header_SDES;

typedef struct _RTCP_header_BYE {
    unsigned int ssrc;
    unsigned char length;
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

#endif
