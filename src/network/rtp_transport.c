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


#include <config.h>

#include "feng.h"
#include "rtp.h"
#include "rtcp.h"
#include "mediathread/mediathread.h"
#include "mediathread/demuxer.h"
#include "fnc_log.h"
#include <sys/time.h>

/**
 * @file
 * RTP packets sending and receiving with session handling
 */

/**
 * Calculate RTP time from media timestamp or using pregenerated timestamp
 * depending on what is available
 * @param session RTP session of the packet
 * @param clock_rate RTP clock rate (depends on media)
 * @param buffer Buffer of which calculate timestamp
 * @return RTP Timestamp (in local endianess)
 */
static inline uint32_t RTP_calc_rtptime(RTP_session *session, int clock_rate, MParserBuffer *buffer) {
    uint32_t calc_rtptime = (uint32_t)((buffer->timestamp - session->seek_time) * clock_rate);
    return (session->start_rtptime + calc_rtptime);
}

static inline double calc_send_time(RTP_session *session, MParserBuffer *buffer) {
    double last_timestamp = (session->last_timestamp - session->seek_time);
    return (last_timestamp - session->send_time);
}

typedef struct {
    /* byte 0 */
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    uint8_t csrc_len:4;   /* expect 0 */
    uint8_t extension:1;  /* expect 1, see RTP_OP below */
    uint8_t padding:1;    /* expect 0 */
    uint8_t version:2;    /* expect 2 */
#elif (G_BYTE_ORDER == G_BIG_ENDIAN)
    uint8_t version:2;
    uint8_t padding:1;
    uint8_t extension:1;
    uint8_t csrc_len:4;
#else
#error Neither big nor little
#endif
    /* byte 1 */
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    uint8_t payload:7;    /* RTP_PAYLOAD_RTSP */
    uint8_t marker:1;     /* expect 1 */
#elif (G_BYTE_ORDER == G_BIG_ENDIAN)
    uint8_t marker:1;
    uint8_t payload:7;
#endif
    /* bytes 2, 3 */
    uint16_t seq_no;
    /* bytes 4-7 */
    uint32_t timestamp;
    /* bytes 8-11 */
    uint32_t ssrc;    /* stream number is used here. */

    uint8_t data[]; /**< Variable-sized data payload */
} RTP_packet;

/**
 * @brief Send the actual buffer as an RTP packet to the client
 *
 * @param session The RTP session to send the packet for
 * @param buffer The data for the packet to be sent
 *
 * @return The number of frames sent to the client.
 * @retval -1 Error during writing.
 */
int rtp_packet_send(RTP_session *session, MParserBuffer *buffer) {
    const size_t packet_size = sizeof(RTP_packet) + buffer->data_size;
    RTP_packet *packet = g_malloc0(packet_size);
    Track *tr = session->track;
    const uint32_t timestamp = RTP_calc_rtptime(session,
                                                tr->properties->clock_rate,
                                                buffer);
    int frames = -1;

    packet->version = 2;
    packet->padding = 0;
    packet->extension = 0;
    packet->csrc_len = 0;
    packet->marker = buffer->marker & 0x1;
    packet->payload = tr->properties->payload_type & 0x7f;
    packet->seq_no = htons(++session->seq_no);
    packet->timestamp = htonl(timestamp);
    packet->ssrc = htonl(session->ssrc);

    fnc_log(FNC_LOG_VERBOSE, "[RTP] Timestamp: %u", ntohl(timestamp));

    memcpy(packet->data, buffer->data, buffer->data_size);

    if (Sock_write(session->transport.rtp_sock, packet,
                   packet_size, NULL, MSG_DONTWAIT
                   | MSG_EOR) < 0) {
        fnc_log(FNC_LOG_DEBUG, "RTP Packet Lost\n");
    } else {
        if (!session->send_time)
            session->send_time = buffer->timestamp - session->seek_time;

        frames = (fabs(session->last_timestamp - buffer->timestamp) /
                  tr->properties->frame_duration) + 1;
        session->send_time += calc_send_time(session, buffer);
        session->last_timestamp = buffer->timestamp;
        session->rtcp.server_stats.pkt_count++;
        session->rtcp.server_stats.octet_count += buffer->data_size;

        session->last_packet_send_time = time(NULL);
    }
    g_free(packet);

    return frames;
}

/**
 * Closes a transport linked to a session
 * @param session the RTP session for which to close the transport
 * @return always 0
 */
int RTP_transport_close(RTP_session * session) {
    port_pair pair;
    pair.RTP = get_local_port(session->transport.rtp_sock);
    pair.RTCP = get_local_port(session->transport.rtcp_sock);

    ev_io_stop(session->srv->loop, &session->transport.rtcp_watcher);

    switch (session->transport.rtp_sock->socktype) {
        case UDP:
            RTP_release_port_pair(session->srv, &pair);
        default:
            break;
    }
    Sock_close(session->transport.rtp_sock);
    Sock_close(session->transport.rtcp_sock);
    return 0;
}
