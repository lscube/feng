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
#include "media/demuxer.h"
#include "media/mediaparser.h"

/*
 * Make sure all the threads get collected
 *
 * @param session rtp session to be cleaned
 */

static void rtp_fill_pool_free(RTP_session *session)
{
    Resource *resource = session->track->parent;
    GThreadPool *pool = resource->fill_pool;

    if (session->track->properties.media_source == LIVE_SOURCE) {
        if (r_count(resource) > 1) {
            // do nothing
            return;
        }
    }
    if (resource->fill_pool) {
        resource->fill_pool = NULL;
        g_thread_pool_free(pool, true, true);
    }
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
static void rtp_session_free(gpointer session_gen,
                             ATTR_UNUSED gpointer unused)
{
    RTP_session *session = (RTP_session*)session_gen;

    /* Close the transport first, so that all the reading thread will
     * stop before continuing to free resources; another way should be
     * to ensure that we're paused before doing this but doesn't
     * matter now.
     */
    ev_periodic_stop(feng_loop, &session->rtp_writer);

    session->close_transport(session);

    rtp_fill_pool_free(session);

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
 * @brief Do the work of filling an RTP session with data
 *
 * @param unused_data Unused
 * @param session_p The session parameter from rtp_session_fill
 *
 *
 * @internal This function is used to initialize @ref
 *           Resource::fill_pool.
 */
static void rtp_session_fill_cb(ATTR_UNUSED gpointer unused_data,
                                gpointer session_p)
{
    RTP_session *session = (RTP_session*)session_p;
    Resource *resource = session->track->parent;
    BufferQueue_Consumer *consumer = session->consumer;
    gulong unseen;
    const gulong buffered_frames = feng_srv.srvconf.buffered_frames;

    while ( g_atomic_int_get(&resource->eor) == 0 &&
            ((unseen = bq_consumer_unseen(consumer)) < buffered_frames ||
             resource->demuxer->info->source == LIVE_SOURCE) &&
            resource->fill_pool != NULL ) {
#if 0
        fprintf(stderr, "calling read_packet from %p for %p[%s] (%u/%d)\n",
                session,
                resource, resource->info->mrl,
                unseen, buffered_frames);
#endif

        if ( r_read(resource) != RESOURCE_OK )
            break;
    }
}

/**
 * @brief Request filling an RTP session with data
 *
 * @param session The session to fill with data
 *
 * This function takes care of filling with data the bufferqueue of
 * the session, by asking to the resource to read more.
 *
 * Previously this was implemented mostly on the Resource side, but
 * given we need more information from the session than the resource
 * it was moved here.
 *
 * This function should be called while holding locks to the session,
 * but not to the resource.
 *
 * @see rtp_session_fill_cb
 */
static void rtp_session_fill(RTP_session *session)
{
    Resource *resource = session->track->parent;

    if ( g_atomic_int_get(&session->track->parent->eor) == 0 )
        g_thread_pool_push(resource->fill_pool,
                           GINT_TO_POINTER(-1), NULL);
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
    Resource *resource = session->track->parent;

    fnc_log(FNC_LOG_VERBOSE, "Resuming session %p", session);

    session->range = range;
    session->start_seq = 1 + session->seq_no;
    session->start_rtptime = g_random_int();
    session->send_time = 0.0;
    session->last_packet_send_time = time(NULL);

    /* Create the new thread pool for the read requests */
    if (session->track->properties.media_source != LIVE_SOURCE ||
        r_count(resource) == 1) {

        resource->fill_pool = g_thread_pool_new(rtp_session_fill_cb, session,
                                               1, true, NULL);

        /* Prefetch frames */
        rtp_session_fill(session);
    }
    ev_periodic_set(&session->rtp_writer,
                    range->playback_time - 0.05,
                    0, NULL);
    ev_periodic_start(feng_loop, &session->rtp_writer);
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
static void rtp_session_pause(gpointer session_gen,
                              ATTR_UNUSED gpointer user_data) {
    RTP_session *session = (RTP_session *)session_gen;

    /* We should assert its presence, we cannot pause a non-running
     * session! */

    rtp_fill_pool_free(session);

    ev_periodic_stop(feng_loop, &session->rtp_writer);
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
static inline
uint32_t rtptime(RTP_session *session, int clock_rate, MParserBuffer *buffer)
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
static void rtp_packet_send(RTP_session *session, MParserBuffer *buffer)
{
    const size_t packet_size = sizeof(RTP_packet) + buffer->data_size;
    GByteArray *outbuf = g_byte_array_sized_new(packet_size);
    RTP_packet *packet = (RTP_packet*)(outbuf->data);
    Track *tr = session->track;
    const uint32_t timestamp = rtptime(session, tr->properties.clock_rate,
                                       buffer);

    outbuf->len = packet_size;

    packet->version = 2;
    packet->padding = 0;
    packet->extension = 0;
    packet->csrc_len = 0;
    packet->marker = buffer->marker & 0x1;
    packet->payload = tr->properties.payload_type & 0x7f;
    packet->seq_no = htons(++session->seq_no);
    packet->timestamp = htonl(timestamp);
    packet->ssrc = htonl(session->ssrc);

    fnc_log(FNC_LOG_VERBOSE, "[RTP] Timestamp: %u", ntohl(timestamp));

    memcpy(packet->data, buffer->data, buffer->data_size);

    if (session->send_rtp(session, outbuf)) {
        session->last_timestamp = buffer->timestamp;
        session->pkt_count++;
        session->octet_count += buffer->data_size;

        session->last_packet_send_time = time(NULL);
    } else {
        fnc_log(FNC_LOG_DEBUG, "RTP Packet Lost");
    }
}

/**
 * Send pending RTP packets to a session.
 *
 * @param loop eventloop
 * @param w contains the session the RTP session for which to send the packets
 * @todo implement a saner ratecontrol
 */
static void rtp_write_cb(struct ev_loop *loop, ev_periodic *w,
                         ATTR_UNUSED int revents)
{
    RTP_session *session = w->data;
    Resource *resource = session->track->parent;
    MParserBuffer *buffer = NULL;
    ev_tstamp next_time = w->offset;

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
     */
    if (g_atomic_int_get(&resource->eor))
        fnc_log(FNC_LOG_INFO,
            "[%s] end of resource %d packets to be fetched",
            session->track->properties.encoding_name,
            bq_consumer_unseen(session->consumer));

    /* Get the current buffer, if there is enough data */
    if ( !(buffer = bq_consumer_get(session->consumer)) ) {
        /* We wait a bit of time to get the data but before it is
         * expired.
         */
        double sleep_for = 0.1;

        if (resource->eor) {
            fnc_log(FNC_LOG_INFO, "[rtp] Stream Finished");
            rtcp_send_sr(session, BYE);
            return;
        }

        if (session->track->properties.frame_duration > 0)
            sleep_for = session->track->properties.frame_duration * 3; // assumed to be enough

        next_time += sleep_for;
        fnc_log(FNC_LOG_INFO, "[%s] nothing to read, waiting %f...",
                session->track->properties.encoding_name, sleep_for);
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
            if(delivery != next->delivery) {
                if (session->track->properties.media_source == LIVE_SOURCE)
                    next_time += next->delivery - delivery;
                else
                    next_time = session->range->playback_time -
                                session->range->begin_time +
                                next->delivery;
            }
        } else {
            /* Wait a bit of time to recover from buffer underrun */
            double sleep_for = duration ? duration * 3 : 0.1;

            next_time += sleep_for;
            fnc_log(FNC_LOG_INFO, "[%s] next packet not available, waiting %f...",
                    session->track->properties.encoding_name, sleep_for);
        }

        fnc_log(FNC_LOG_VERBOSE,
            "[%s] Now: %5.4f, cur %5.4f[%5.4f][%5.4f], next %5.4f %s\n",
            session->track->properties.encoding_name,
            ev_now(loop) - session->range->playback_time,
            delivery,
            timestamp,
            duration,
            next_time - session->range->playback_time,
            marker? "M" : " ");
    }
    ev_periodic_set(w, next_time, 0, NULL);
    ev_periodic_again(loop, w);

    if (session->track->properties.media_source != LIVE_SOURCE)
        rtp_session_fill(session);
}

typedef void (*rtp_transport_init_cb)(RTSP_Client *rtsp,
                                      RTP_session *rtp_s,
                                      struct ParsedTransport *parsed);
/**
 * @brief Create a new RTP session object.
 *
 * @param rtsp The buffer for which to generate the session
 * @param uri The URI for the current RTP session
 * @param tr The track that will be sent over the session
 * @param transports A singly-linked list of transports that the
 *        client suggested.
 *
 * @return A pointer to a newly-allocated RTP_session, that needs to
 *         be freed with @ref rtp_session_free.
 *
 * @see rtp_session_free
 */
RTP_session *rtp_session_new(RTSP_Client *rtsp,
                             const char *uri, Track *tr,
                             GSList *transports) {
    RTP_session *rtp_s;
    ev_periodic *periodic;

    rtp_s = g_slice_new0(RTP_session);
    periodic = &rtp_s->rtp_writer;

    rtp_s->ssrc = g_random_int();

    do {
        struct ParsedTransport *transport = transports->data;
        static const rtp_transport_init_cb
            rtp_transport_init[_RTP_PROTOCOL_MAX] = {
                [RTP_UDP] = rtp_udp_transport,
                [RTP_TCP] = rtp_interleaved_transport,
#ifdef ENABLE_SCTP
                [RTP_SCTP] = rtp_sctp_transport
#endif
        };
        rtp_transport_init[transport->protocol](rtsp, rtp_s, transport);
    } while ( rtp_s->transport_data == NULL &&
              (transports = g_slist_next(transports)) != NULL );

    if ( transports == NULL )
        goto cleanup;

    /* Make sure we start paused since we have to wait for parameters
     * given by @ref rtp_session_resume.
     */
    rtp_s->uri = g_strdup(uri);

    rtp_s->start_rtptime = g_random_int();
    rtp_s->start_seq = g_random_int_range(0, G_MAXUINT16);
    rtp_s->seq_no = rtp_s->start_seq - 1;

    /* Set up the track selector and get a consumer for the track */

    rtp_s->track = tr;
    rtp_s->consumer = bq_consumer_new(track_get_producer(tr));

    rtp_s->client = rtsp;

    periodic->data = rtp_s;
    ev_periodic_init(periodic, rtp_write_cb, 0, 0, NULL);

    return rtp_s;

 cleanup:
    g_slice_free(RTP_session, rtp_s);
    return NULL;
}
