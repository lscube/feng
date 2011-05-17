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

#include "config.h"

#include <glib.h>
#include <stdio.h>
#include <stdbool.h>
#include <netinet/in.h>

#if HAVE_CLOCK_GETTIME
# include <time.h>
#else /* use gettimeofday() */
# include <sys/time.h>
#endif

#include "rtp.h"
#include "rtsp.h"
#include "feng.h"
#include "fnc_log.h"
#include "media/demuxer.h"

/**
 * @defgroup rtcp RTCP Handling
 * @brief Data structures and functions to handle RTCP
 *
 * @{
 */

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

typedef struct RTCP_report_block {
    uint32_t ssrc;
    uint8_t fract_lost;
    uint8_t packet_lost[3];
    uint32_t h_seq_no;
    uint32_t jitter;
    uint32_t last_sr;
    uint32_t delay_last_sr;
} RTCP_report_block;

typedef struct RTCP_header_SDES {
    uint32_t ssrc;
    uint8_t attr_name;
    uint8_t len;
    char name[];
} __attribute__((__packed__)) RTCP_header_SDES;

typedef struct RTCP_header_BYE {
    uint32_t ssrc;
    uint8_t length;
    char reason[];
} __attribute__((__packed__)) RTCP_header_BYE;

/**
 * @brief Basic polymorphic structure of a Server Report RTCP compound
 *
 * Please note that since both @ref RTCP_header_SDES and @ref
 * RTCP_header_BYE are variable sized, there can't be anything after
 * the payload.
 *
 * This structure is designed to be directly applied over the memory
 * area that will be sent as a packet. Its definition is described in
 * RFC 3550 Section 6.4.1.
 */
typedef struct {
    /** The header for the SR preamble */
    RTCP_header sr_hdr;
    /** The actual SR preamble */
    RTCP_header_SR sr_pkt;
    /** The header for the payload packet */
    RTCP_header payload_hdr;
    /** Polymorphic payload */
    union {
        /** Payload for the source description packet */
        RTCP_header_SDES sdes;
        /** Payload for the goodbye packet */
        RTCP_header_BYE bye;
    } payload;
} RTCP_SR_Compound;

#if HAVE_CLOCK_GETTIME
static double gettimeinseconds(struct timespec *now) {
    clock_gettime(CLOCK_REALTIME, now);
    return (double)now->tv_sec + (double)now->tv_nsec * .000000001;
}
#else
static double gettimeinseconds(struct timespec *now) {
    struct timeval tmp;
    gettimeofday(&tmp, NULL);
    now->tv_sec = tmp.tv_sec;
    now->tv_nsec = tmp.tv_usec * 1000;
    return (double)tmp.tv_sec + (double)tmp.tv_usec * .000001;
}
#endif

/**
 * @brief Sets the SR preamble for the given compound
 *
 * @param session The session to send the RTCP data of
 * @param outpkt The compound packet to set data in
 */
static void rtcp_set_sr(RTP_session *session, RTCP_SR_Compound *outpkt)
{
    struct timespec ntp_time;
    double now;
    size_t sr_size = sizeof(RTCP_header) + sizeof(RTCP_header_SR);

    outpkt->sr_hdr.version = 2;
    outpkt->sr_hdr.padding = 0;
    outpkt->sr_hdr.count = 0;
    outpkt->sr_hdr.pt = SR;
    outpkt->sr_hdr.length = htons((sr_size>>2) -1);

    outpkt->sr_pkt.ssrc = htonl(session->ssrc);
    now = gettimeinseconds(&ntp_time);
    outpkt->sr_pkt.ntp_timestampH =
        htonl((unsigned int) ntp_time.tv_sec + 2208988800u);
    outpkt->sr_pkt.ntp_timestampL =
        htonl((((uint64_t) ntp_time.tv_nsec) << 32) / 1000000000u);
    outpkt->sr_pkt.rtp_timestamp =
        htonl((unsigned int) ((now - session->range->playback_time) *
                              session->track->clock_rate)
              + session->start_rtptime);

    outpkt->sr_pkt.pkt_count = htonl(session->pkt_count);
    outpkt->sr_pkt.octet_count = htonl(session->octet_count);
}

/**
 * @brief Create a new compound sender report for SDES packets
 *
 * @param session The session to create the report for
 */
static GByteArray *rtcp_pkt_sr_sdes(RTP_session *session)
{
    RTCP_SR_Compound *outpkt;
    GByteArray *outpkt_buffer;

    const char *name = session->client->local_host;
    const size_t name_len = strlen(name);
    size_t sdes_size = sizeof(RTCP_header) + sizeof(RTCP_header_SDES) +
        name_len;
    size_t outpkt_size = sizeof(RTCP_header) + sizeof(RTCP_header_SR) +
        sdes_size;

    /* Pad to 32-bit */
    if ( outpkt_size%4 != 0 ) {
        const size_t padding = 4-(outpkt_size%4);
        sdes_size += padding;
        outpkt_size += padding;
    }

    outpkt_buffer = g_byte_array_sized_new(outpkt_size);
    g_byte_array_set_size(outpkt_buffer, outpkt_size);
    memset(outpkt_buffer->data, 0, outpkt_size);

    outpkt = (RTCP_SR_Compound*)outpkt_buffer->data;

    rtcp_set_sr(session, outpkt);

    outpkt->payload_hdr.version = 2;
    outpkt->payload_hdr.padding = 0; // Avoid padding
    outpkt->payload_hdr.count = 1;
    outpkt->payload_hdr.pt = SDES;

    outpkt->payload_hdr.length = htons((sdes_size>>2) -1);

    outpkt->payload.sdes.ssrc = htonl(session->ssrc);
    outpkt->payload.sdes.attr_name = CNAME;
    outpkt->payload.sdes.len = name_len;

    memcpy(&outpkt->payload.sdes.name, name, name_len);

    return outpkt_buffer;
}

/**
 * @brief Create a new compound sender report for BYE packets
 *
 * @param session The session to create the report for
 *
 * @todo The reason should probably be a parameter
 */
static GByteArray *rtcp_pkt_sr_bye(RTP_session *session)
{
    static const char reason[] = "The medium is over.";

    RTCP_SR_Compound *outpkt;
    GByteArray *outpkt_buffer;
    size_t bye_size = sizeof(RTCP_header) + sizeof(RTCP_header_BYE) +
        sizeof(reason);
    size_t outpkt_size = sizeof(RTCP_header) + sizeof(RTCP_header_SR) +
        bye_size;

    /* Pad to 32-bit */
    if ( outpkt_size%4 != 0 ) {
        const size_t padding = 4-(outpkt_size%4);
        bye_size += padding;
        outpkt_size += padding;
    }

    outpkt_buffer = g_byte_array_sized_new(outpkt_size);
    g_byte_array_set_size(outpkt_buffer, outpkt_size);
    memset(outpkt_buffer->data, 0, outpkt_size);

    outpkt = (RTCP_SR_Compound*)outpkt_buffer->data;

    rtcp_set_sr(session, outpkt);

    outpkt->payload_hdr.version = 2;
    outpkt->payload_hdr.padding = 0;
    outpkt->payload_hdr.count = 1;
    outpkt->payload_hdr.pt = BYE;
    outpkt->payload_hdr.length = htons((bye_size>>2) -1);

    outpkt->payload.bye.ssrc = htonl(session->ssrc);
    outpkt->payload.bye.length = htonl(sizeof(reason)-1);

    memcpy(&outpkt->payload.bye.reason, &reason, sizeof(reason));

    return outpkt_buffer;
}

/**
 * @brief Send a compound RTCP sender report
 *
 * @param session The RTP session to send the command for
 * @param type The type of packet to send after the SR preamble
 *
 * Warning! This function will actually send a compound response of
 * _two_ packets, the first is the SR packet, and the second is the
 * one requested with the @p type parameter.
 *
 * Since the two packets are sent with a single message, only one call
 * is needed.
 */
gboolean rtcp_send_sr(RTP_session *session, rtcp_pkt_type type)
{
    GByteArray *outpkt = NULL;

    switch(type) {
    case SDES:
        outpkt = rtcp_pkt_sr_sdes(session);
        break;
    case BYE:
        outpkt = rtcp_pkt_sr_bye(session);
        break;
    default:
        g_assert_not_reached();
    }

    g_assert(outpkt != NULL);

    return session->send_rtcp(session, outpkt);
}


#define rtcp_pt_to_string(pt)\
    ((pt == SR) ?  "Sender Report" : \
     (pt == RR) ?  "Receiver Report" : \
     (pt == SDES)? "Source Description" : \
     (pt == BYE) ? "Bye" : \
     (pt == APP) ? "Application" : \
                   "Unknown")
/**
 * @brief parse Receiver Reports and update the statistics accordingly
 */

static void parse_receiver_report(uint8_t *packet, int count)
{
    int ssrc = ntohl(((RTCP_header_RR*)packet)->ssrc);
    fnc_log(FNC_LOG_VERBOSE, "[RTCP] Receiver report for %u", ssrc);
    packet+=4;
    while(count--) {
        RTCP_report_block *report = (RTCP_report_block *)packet;
        fnc_log(FNC_LOG_VERBOSE,
                "[RTCP] ssrc %u, fraction %d, lost %d, "
                "sequence %u, jitter %u, "
                "last Sender Report %u, delay %u",
                report->ssrc, report->fract_lost,
                report->packet_lost[0]<<3|
                report->packet_lost[1]<<2|
                report->packet_lost[2],
                ntohl(report->h_seq_no),
                ntohl(report->last_sr),
                ntohl(report->delay_last_sr));
        packet+=sizeof(RTCP_report_block);
    }
}

/**
 * @brief Parse and handle an incoming RTCP packet.
 */
void rtcp_handle(RTP_session *session, uint8_t *packet, size_t len)
{
    int rtcp_size = 0;

    fnc_log(FNC_LOG_VERBOSE, "[RTCP] Handling a %zd byte packet", len);
    while (len > sizeof(RTCP_header)) {
        RTCP_header *rtcp = (RTCP_header *)packet;
        rtcp_size = (ntohs(rtcp->length)+1)<<2;

        fnc_log(FNC_LOG_VERBOSE, "[RTCP] %s (%d) packet found %d byte",
                rtcp_pt_to_string(rtcp->pt), rtcp->pt, rtcp_size);

        if (rtcp_size > len) {
            fnc_log(FNC_LOG_WARN, "[RTCP]  Malformed packet %d > %zd",
                    rtcp_size, len);
            return;
        }

        switch (rtcp->pt) {
            case SR:
            case RR:
                parse_receiver_report(packet+4, rtcp->count);
            case SDES:
            default:
                break;
        }
        len -= rtcp_size;
        packet += rtcp_size;
    }
}

/**
 * @}
 */
