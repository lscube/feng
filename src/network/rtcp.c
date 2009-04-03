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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rtcp.h"

#include <fenice/utils.h>
#include <fenice/server.h>
#include <fenice/fnc_log.h>
#include <netinet/in.h>

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

static int RTCP_send_packet(RTP_session * session, rtcp_pkt_type type)
{
    unsigned char *payload = NULL;
    RTCP_header hdr;
    size_t pkt_size = 0, hdr_s = 0, payload_s = 0;
    Track *t = session->track;

    hdr.version = 2;
    hdr.padding = 0;
    hdr.count = 0;
    hdr.pt = type;
    hdr_s = sizeof(hdr);
    switch (type) {
    case SR:{
        struct timespec ntp_time;
        double now;
        RTCP_header_SR hdr_sr;
        payload_s = sizeof(hdr_sr);
        hdr_sr.ssrc = htonl(session->ssrc);
        now = gettimeinseconds(&ntp_time);
        hdr_sr.ntp_timestampH =
            htonl((unsigned int) ntp_time.tv_sec + 2208988800u);
        hdr_sr.ntp_timestampL =
            htonl((((uint64_t) ntp_time.tv_nsec) << 32) / 1000000000u);
        hdr_sr.rtp_timestamp =
            htonl((unsigned int) ((now - session->start_time) *
                  t->properties->clock_rate) + session->start_rtptime);

        hdr_sr.pkt_count = htonl(session->rtcp.server_stats.pkt_count);
        hdr_sr.octet_count = htonl(session->rtcp.server_stats.octet_count);
        payload = g_malloc0(payload_s);
        if (payload == NULL)
            return ERR_ALLOC;
        memcpy(payload, &hdr_sr, payload_s);
        break;
    }
    case RR:{
        RTCP_header_RR hdr_rr;
        payload_s = sizeof(hdr_rr);
        hdr_rr.ssrc = htonl(session->ssrc);
        payload = g_malloc0(payload_s);
        if (payload == NULL)
            return ERR_ALLOC;
        memcpy(payload, &hdr_rr, payload_s);
        break;
    }
    case SDES:{
        RTCP_header_SDES hdr_sdes;
        const char *name = session->transport.rtcp_sock->local_host ?
	  session->transport.rtcp_sock->local_host : "::";
        int hdr_sdes_s = sizeof(hdr_sdes);
        size_t name_s = strlen(name);
        payload_s = hdr_sdes_s + name_s;
        // Padding
        payload_s += (((hdr_s + payload_s) % 4) ? 1 : 0);
        payload = g_malloc0(payload_s);
        if (payload == NULL)
            return ERR_ALLOC;
        hdr.count = 1;
        hdr_sdes.ssrc = htonl(session->ssrc);
        hdr_sdes.attr_name = CNAME;    // 1=CNAME
        hdr_sdes.len = name_s;
        memcpy(payload, &hdr_sdes, hdr_sdes_s);
        memcpy(payload + hdr_sdes_s, name, name_s);
        break;
    }
    case BYE:{
        RTCP_header_BYE hdr_bye;
        int hdr_bye_s = sizeof(hdr_bye);
        static const char reason[20] = "The medium is over.";
        payload_s = hdr_bye_s + sizeof(reason);
        hdr.count = 1;
        hdr_bye.ssrc = htonl(session->ssrc);
        hdr_bye.length = htonl(sizeof(reason)-1);
        payload = g_malloc0(payload_s);
        if (payload == NULL)
            return ERR_ALLOC;
        memcpy(payload, &hdr_bye, hdr_bye_s);
        memcpy(payload + hdr_bye_s, reason, sizeof(reason));
        break;
    }
    default:
        return ERR_NOERROR;
    }

    pkt_size += payload_s + hdr_s;
    hdr.length = htons((pkt_size >> 2) - 1);

    if (session->rtcp.outsize + pkt_size <= sizeof(session->rtcp.outbuffer)) {
        memcpy(session->rtcp.outbuffer + session->rtcp.outsize, &hdr, hdr_s);
        memcpy(session->rtcp.outbuffer + session->rtcp.outsize + hdr_s,
               payload, payload_s);
        session->rtcp.outsize += pkt_size;
    } else {
        fnc_log(FNC_LOG_VERBOSE, "Output RTCP packet lost\n");
    }
    g_free(payload);
    return ERR_NOERROR;
}

/**
 * Send the rtcp payloads in queue
 */

static int RTCP_flush(RTP_session * session)
{
    fd_set wset;
    struct timeval t;
    Sock *rtcp_sock = session->transport.rtcp_sock;

    /*---------------SEE eventloop/rtsp_server.c-------*/
    FD_ZERO(&wset);
    t.tv_sec = 0;
    t.tv_usec = 1000;

    if (session->rtcp.outsize > 0)
        FD_SET(Sock_fd(rtcp_sock), &wset);
    if (select(Sock_fd(rtcp_sock) + 1, 0, &wset, 0, &t) < 0) {
        fnc_log(FNC_LOG_ERR, "select error\n");
        /*send_reply(500, NULL, rtsp); */
        return ERR_GENERIC;
    }

    if (FD_ISSET(Sock_fd(rtcp_sock), &wset)) {
        if (Sock_write(rtcp_sock, session->rtcp.outbuffer,
            session->rtcp.outsize, NULL, MSG_EOR | MSG_DONTWAIT) < 0)
            fnc_log(FNC_LOG_VERBOSE, "RTCP Packet Lost\n");
        session->rtcp.outsize = 0;
        fnc_log(FNC_LOG_VERBOSE, "OUT RTCP\n");
    }

    return ERR_NOERROR;
}

int RTCP_handler(RTP_session * session)
{
    if (session->rtcp.server_stats.pkt_count % 29 == 1) {
        RTCP_send_packet(session, SR);
        RTCP_send_packet(session, SDES);
        return RTCP_flush(session);
    }
    return ERR_NOERROR;
}

/**
 * @brief Send the disconnection sequence
 *
 * @param session The RTP session to disconnect
 */
void RTCP_send_bye(RTP_session *session)
{
    RTCP_send_packet(session, SR);
    RTCP_send_packet(session, BYE);
    RTCP_flush(session);
}
