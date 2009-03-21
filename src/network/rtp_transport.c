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

#include "rtp.h"
#include "mediathread/mediathread.h"
#include "mediathread/demuxer.h"
#include <fenice/fnc_log.h>
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
    return (session->start_rtptime + (buffer->rtp_time ? buffer->rtp_time : calc_rtptime));
}

static inline double calc_send_time(RTP_session *session, MParserBuffer *buffer) {
    double last_timestamp = (buffer->last_timestamp - session->seek_time);
    return (last_timestamp - session->send_time)/buffer->pkt_num;
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
 * Sends pending RTP packets for the given session
 * @param session the RTP session for which to send the packets
 * @retval ERR_NOERROR
 * @retval ERR_ALLOC Buffer allocation errors
 * @retval ERR_EOF End of stream
 * @return Same error values as event_buffer_low on event emission problems
 */
int RTP_send_packet(RTP_session * session)
{
    int res = ERR_NOERROR, bp_frames = 0;
    MParserBuffer *buffer = NULL;
    ssize_t psize_sent = 0;
    feng *srv = session->srv;
    Track *t = r_selected_track(session->track_selector);
    uint32_t now=time(NULL);

    if ((buffer = bq_consumer_get(session->consumer))) {
        if (!(session->pause && t->properties->media_source == MS_live)) {
            const uint32_t timestamp = RTP_calc_rtptime(session,
                                                        t->properties->clock_rate,
                                                        buffer);
            const size_t packet_size = sizeof(RTP_packet) + buffer->data_size;
            RTP_packet *packet = g_malloc0(packet_size);

            if (packet == NULL) {
                return ERR_ALLOC;
            }

            packet->version = 2;
            packet->padding = 0;
            packet->extension = 0;
            packet->csrc_len = 0;
            packet->marker = buffer->marker & 0x1;
            packet->payload = t->properties->payload_type & 0x7f;
            packet->seq_no = htons(session->seq_no += buffer->seq_delta);
            packet->timestamp = htonl(timestamp);
            packet->ssrc = htonl(session->ssrc);

            fnc_log(FNC_LOG_VERBOSE, "[RTP] Timestamp: %u", ntohl(timestamp));

            memcpy(packet->data, buffer->data, buffer->data_size);

            if ((psize_sent =
                 Sock_write(session->transport.rtp_sock, packet,
                 packet_size, NULL, MSG_DONTWAIT
                 | MSG_EOR)) < 0) {
                fnc_log(FNC_LOG_DEBUG, "RTP Packet Lost\n");
            } else {
                if (!session->send_time) {
                    session->send_time = buffer->timestamp - session->seek_time;
                }
                bp_frames = (fabs(buffer->last_timestamp - buffer->timestamp) /
                    t->properties->frame_duration) + 1;
                session->send_time += calc_send_time(session, buffer);
                session->rtcp_stats[i_server].pkt_count += buffer->seq_delta;
                session->rtcp_stats[i_server].octet_count += buffer->data_size;

                session->last_packet_send_time = now;
            }
            g_free(packet);
        } else {
#warning Remove as soon as feng is fixed
            usleep(1000);
        }
        bq_consumer_next(session->consumer);
    }
    if (!buffer || bp_frames <= srv->srvconf.buffered_frames) {
        res = event_buffer_low(session, t);
    }
    switch (res) {
        case ERR_NOERROR:
            break;
        case ERR_EOF:
            if (buffer) {
                //Wait to empty feng before sending BYE packets.
                res = ERR_NOERROR;
            }
        break;
        default:
            fnc_log(FNC_LOG_FATAL, "Unable to emit event buffer low");
            break;
    }
    return res;
}

/**
 * Receives data from the socket linked to the session and puts it inside the session buffer
 * @param session the RTP session for which to receive the packets
 * @return size of te received data or -1 on error or invalid protocol request
 */
ssize_t RTP_recv(RTP_session * session)
{
    Sock *s = session->transport.rtcp_sock;
    struct sockaddr *sa_p = (struct sockaddr *)&(session->transport.last_stg);

    if (!s)
        return -1;

    switch (s->socktype) {
        case UDP:
            session->rtcp_insize = Sock_read(s, session->rtcp_inbuffer,
                     sizeof(session->rtcp_inbuffer),
                     sa_p, 0);
            break;
        case LOCAL:
            session->rtcp_insize = Sock_read(s, session->rtcp_inbuffer,
                     sizeof(session->rtcp_inbuffer),
                     NULL, 0);
            break;
        default:
            session->rtcp_insize = -1;
            break;
    }

    return session->rtcp_insize;
}

/**
 * Closes a transport linked to a session
 * @param session the RTP session for which to close the transport
 * @return always 0
 */
static int RTP_transport_close(RTP_session * session) {
    port_pair pair;
    pair.RTP = get_local_port(session->transport.rtp_sock);
    pair.RTCP = get_local_port(session->transport.rtcp_sock);

    ev_io_stop(session->srv->loop, session->transport.rtcp_watcher);
    g_free(session->transport.rtcp_watcher);

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

/**
 * Deallocates an RTP session, closing its tracks and transports
 * @param session the RTP session to remove
 * @return the subsequent session
 */
void RTP_session_destroy(RTP_session * session)
{
    RTP_transport_close(session);

    // Close track selector
    r_close_tracks(session->track_selector);

    /* Remove the consumer */
    bq_consumer_free(session->consumer);

    // Deallocate memory
    g_free(session->sd_filename);
    g_free(session);
}
