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

#include <stdbool.h>

#include "feng.h"
#include "rtp.h"
#include "rtsp.h"
#include "fnc_log.h"
#include "mediathread/demuxer.h"

/**
 * Closes a transport linked to a session
 * @param session the RTP session for which to close the transport
 */
static void rtp_transport_close(RTP_session * session)
{
    port_pair pair;
    pair.RTP = get_local_port(session->transport.rtp_sock);
    pair.RTCP = get_local_port(session->transport.rtcp_sock);

    ev_periodic_stop(session->srv->loop, &session->transport.rtp_writer);

    switch (session->transport.rtp_sock->socktype) {
        case UDP:
            RTP_release_port_pair(session->srv, &pair);
        default:
            break;
    }
    Sock_close(session->transport.rtp_sock);
    Sock_close(session->transport.rtcp_sock);
}

/**
 * Deallocates an RTP session, closing its tracks and transports
 *
 * @param session_gen The RTP session to free
 *
 * @see rtp_session_new
 *
 * @internal This function should only be called from g_slist_foreach.
 */
static void rtp_session_free(gpointer session_gen, gpointer unused)
{
    RTP_session *session = (RTP_session*)session_gen;

    rtp_transport_close(session);

    /* Remove the consumer */
    bq_consumer_free(session->consumer);

    /* Deallocate memory */
    g_free(session->uri);
    g_slice_free(RTP_session, session);
}

/**
 * @brief Free a GSList of RTP_sessions
 *
 * @param sessions_list GSList of sessions to free
 *
 * This is a convenience function that wraps around @ref
 * rtp_session_free and calls it with a foreach loop on the list
 */
void rtp_session_gslist_free(GSList *sessions_list) {
    g_slist_foreach(sessions_list, rtp_session_free, NULL);
}

/**
 * @brief Resume (or start) an RTP session
 *
 * @param session_gen The session to resume or start
 * @param range_gen Pointer tp @ref RTSP_Range to start with
 *
 * @todo This function should probably take care of starting eventual
 *       libev events when the scheduler is replaced.
 *
 * This function is used by the PLAY method of RTSP to start or resume
 * a session; since a newly-created session starts as paused, this is
 * the only method available.
 *
 * The use of a pointer to double rather than a double itself is to
 * make it possible to pass this function straight to foreach
 * functions from glib.
 *
 * @internal This function should only be called from g_slist_foreach.
 */
static void rtp_session_resume(gpointer session_gen, gpointer range_gen) {
    RTP_session *session = (RTP_session*)session_gen;
    RTSP_Range *range = (RTSP_Range*)range_gen;
    feng *srv = session->srv;
    int i;

    session->range = range;
    session->start_seq = 1 + session->seq_no;
    session->start_rtptime = g_random_int();
    session->send_time = 0.0;
    session->last_packet_send_time = time(NULL);

    ev_periodic_set(&session->transport.rtp_writer,
                    range->playback_time - 0.05,
                    0, NULL);
    ev_periodic_start(session->srv->loop, &session->transport.rtp_writer);

    /* Prefetch frames */
    r_read(session->track->parent, srv->srvconf.buffered_frames);
}

/**
 * @brief Resume a GSList of RTP_sessions
 *
 * @param sessions_list GSList of sessions to resume
 * @param range The RTSP Range to start from
 *
 * This is a convenience function that wraps around @ref
 * rtp_session_resume and calls it with a foreach loop on the list
 */
void rtp_session_gslist_resume(GSList *sessions_list, RTSP_Range *range) {
    g_slist_foreach(sessions_list, rtp_session_resume, range);
}

/**
 * @brief Pause a session
 *
 * @param session_gen The session to pause
 * @param user_data Unused
 *
 * @todo This function should probably take care of pausing eventual
 *       libev events when the scheduler is replaced.
 *
 * @internal This function should only be called from g_slist_foreach
 */
static void rtp_session_pause(gpointer session_gen, gpointer user_data) {
    RTP_session *session = (RTP_session *)session_gen;

    ev_periodic_stop(session->srv->loop, &session->transport.rtp_writer);
}

/**
 * @brief Pause a GSList of RTP_sessions
 *
 * @param sessions_list GSList of sessions to pause
 *
 * This is a convenience function that wraps around @ref
 * rtp_session_pause  and calls it with a foreach loop on the list
 */
void rtp_session_gslist_pause(GSList *sessions_list) {
    g_slist_foreach(sessions_list, rtp_session_pause, NULL);
}

/**
 * Calculate RTP time from media timestamp or using pregenerated timestamp
 * depending on what is available
 * @param session RTP session of the packet
 * @param clock_rate RTP clock rate (depends on media)
 * @param buffer Buffer of which calculate timestamp
 * @return RTP Timestamp (in local endianess)
 */
static inline uint32_t RTP_calc_rtptime(RTP_session *session,
                                        int clock_rate,
                                        MParserBuffer *buffer)
{
    uint32_t calc_rtptime =
        (buffer->timestamp - session->range->begin_time) * clock_rate;
    return session->start_rtptime + calc_rtptime;
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
static int rtp_packet_send(RTP_session *session, MParserBuffer *buffer)
{
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
        frames = (fabs(session->last_timestamp - buffer->timestamp) /
                  tr->properties->frame_duration) + 1;
        session->last_timestamp = buffer->timestamp;
        session->pkt_count++;
        session->octet_count += buffer->data_size;

        session->last_packet_send_time = time(NULL);
    }
    g_free(packet);

    return frames;
}

/**
 * Send pending RTP packets to a session.
 *
 * @param loop eventloop
 * @param w contains the session the RTP session for which to send the packets
 * @todo implement a saner ratecontrol
 */
static void rtp_write_cb(struct ev_loop *loop, ev_periodic *w, int revents)
{
    RTP_session *session = w->data;
    MParserBuffer *buffer = NULL;
    ev_tstamp next_time = w->offset;
    ev_tstamp delay = 0.0;
    glong extra_cached_frames;

#ifdef HAVE_METADATA
    if (session->metadata)
        cpd_send(session, now);
#endif

    /* If there is no buffer, it means that either the producer
     * has been stopped (as we reached the end of stream) or that
     * there is no data for the consumer to read. If that's the
     * case we just give control back to the main loop for now.
     */
    if ( bq_consumer_stopped(session->consumer) ) {
        /* If the producer has been stopped, we send the
         * finishing packets and go away.
         */
        fnc_log(FNC_LOG_INFO, "[rtp] Stream Finished");
        rtcp_send_sr(session, BYE);
        return;
    }

    /* Check whether we have enough extra frames to send. If we have
     * no extra frames we have a problem, since we're going to send
     * one packet at least.
     *
     * We add one to the number of missing frames so that the value is
     * _never_ zero, otherwise the GThreadPool internal implementation
     * bails out.
     */
    /** @todo It's not really correct to assume one frame per packet,
     *        it's actually quite wrong, in both senses.
     */
    if ( (extra_cached_frames = bq_consumer_unseen(session->consumer)
          - session->srv->srvconf.buffered_frames) <= 0 )
        r_read(session->track->parent, abs(extra_cached_frames)+1);

    /* Get the current buffer, if there is enough data */
    if ( !(buffer = bq_consumer_get(session->consumer)) ) {
        /* We wait a bit of time to get the data but before it is
         * expired.
         */
        next_time += session->track->properties->frame_duration/3;
    } else {
        MParserBuffer *next;
        double delivery  = buffer->delivery;
        double timestamp = buffer->timestamp;
        double duration  = buffer->duration;
        gboolean marker  = buffer->marker;

        rtp_packet_send(session, buffer);

        if (session->pkt_count % 29 == 1)
            rtcp_send_sr(session, SDES);

        if (bq_consumer_move(session->consumer)) {
            next = bq_consumer_get(session->consumer);
            if(marker)
                next_time = session->range->playback_time -
                            session->range->begin_time +
                            next->delivery;
        } else {
            if (marker)
                next_time += duration;
        }
        if(marker)
            delay = ev_time() - w->offset;

#if 0
        fprintf(stderr,
            "[%s] Now: %5.4f, cur %5.4f[%5.4f][%5.4f], next %5.4f, delay %5.4f %s\n",
            session->track->properties->encoding_name,
            ev_now(loop) - session->range->playback_time,
            delivery,
            timestamp,
            duration,
            next_time - session->range->playback_time,
            delay,
            marker? "M" : " ");
#endif

    }
    ev_periodic_set(w, next_time, 0, NULL);
    ev_periodic_again(loop, w);
}

/**
 * @brief Create a new RTP session object.
 *
 * @param rtsp The buffer for which to generate the session
 * @param rtsp_s The RTSP session
 * @param uri The URI for the current RTP session
 * @param transport The transport used by the session
 * @param tr The track that will be sent over the session
 *
 * @return A pointer to a newly-allocated RTP_session, that needs to
 *         be freed with @ref rtp_session_free.
 *
 * @see rtp_session_free
 */
RTP_session *rtp_session_new(RTSP_Client *rtsp, RTSP_session *rtsp_s,
                             RTP_transport *transport, const char *uri,
                             Track *tr) {
    feng *srv = rtsp->srv;
    RTP_session *rtp_s = g_slice_new0(RTP_session);

    /* Make sure we start paused since we have to wait for parameters
     * given by @ref rtp_session_resume.
     */
    rtp_s->uri = g_strdup(uri);

    memcpy(&rtp_s->transport, transport, sizeof(RTP_transport));
    rtp_s->start_rtptime = g_random_int();
    rtp_s->start_seq = g_random_int_range(0, G_MAXUINT16);
    rtp_s->seq_no = rtp_s->start_seq - 1;

    /* Set up the track selector and get a consumer for the track */
    rtp_s->track = tr;
    rtp_s->consumer = bq_consumer_new(tr->producer);

    rtp_s->srv = srv;
    rtp_s->ssrc = g_random_int();
    rtp_s->client = rtsp;

#ifdef HAVE_METADATA
	rtp_s->metadata = rtsp_s->resource->metadata;
#endif

    rtp_s->transport.rtp_writer.data = rtp_s;
    ev_periodic_init(&rtp_s->transport.rtp_writer, rtp_write_cb, 0, 0, NULL);

    // Setup the RTP session
    rtsp_s->rtp_sessions = g_slist_append(rtsp_s->rtp_sessions, rtp_s);

    return rtp_s;
}
