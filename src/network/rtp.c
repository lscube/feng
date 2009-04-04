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
#include "rtcp.h"
#include "fnc_log.h"
#include "mediathread/demuxer.h"
#include "mediathread/mediathread.h"

static ev_tstamp rtp_reschedule_cb(ev_periodic *w, ev_tstamp now)
{
    RTP_session *sess = w->data;
    return now + sess->track->properties->frame_duration/2;
}

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
    g_free(session->sd_filename);
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
 * @param start_time_gen Pointer to the time (in seconds) inside the
 *                       stream to start from.
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
static void rtp_session_resume(gpointer session_gen, gpointer start_time_gen) {
    RTP_session *session = (RTP_session*)session_gen;
    double *start_time = (double*)start_time_gen;
    feng *srv = session->srv;
    int i;

    /* We want to make sure that the session is at least in pause,
     * otherwise something is funky */
    g_assert(session->pause);

    session->start_time = *start_time;

    ev_periodic_start(session->srv->loop, &session->transport.rtp_writer);

    session->send_time = 0.0;
    session->pause = 0;
    session->last_packet_send_time = time(NULL);

    /* Prefetch frames */
    for (i=0; i < srv->srvconf.buffered_frames; i++)
        mt_resource_read(session->track->parent);
}

/**
 * @brief Resume a GSList of RTP_sessions
 *
 * @param sessions_list GSList of sessions to resume
 * @param start_time Time to start the sessions at
 *
 * This is a convenience function that wraps around @ref
 * rtp_session_resume and calls it with a foreach loop on the list
 */
void rtp_session_gslist_resume(GSList *sessions_list, double start_time) {
    g_slist_foreach(sessions_list, rtp_session_resume, &start_time);
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

    session->pause = 1;
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
 * @brief Let a RTP session seek
 *
 * @param session_gen The session to seek for
 * @param seek_time_gen Pointer to the time (in seconds) inside the
 *                      stream to seek to.
 *
 * This function is used by the PLAY method of RTSP to seek further
 * in the stream.
 *
 * The use of a pointer to double rather than a double itself is to
 * make it possible to pass this function straight to foreach
 * functions from glib.
 *
 * @internal This function should only be called from g_slist_foreach.
 */
static void rtp_session_seek(gpointer session_gen, gpointer seek_time_gen) {
    RTP_session *session = (RTP_session*)session_gen;
    double *seek_time = (double*)seek_time_gen;

    session->seek_time = *seek_time;
    session->start_seq = 1 + session->seq_no;
    session->start_rtptime = g_random_int();
}

/**
 * @brief Seek into a GSList of RTP_sessions
 *
 * @param sessions_list GSList of sessions to resume
 * @param seek_time Time to seek the sessions to
 *
 * This is a convenience function that wraps around rtp_session_seek
 * and calls it with a foreach loop on the list
 */
void rtp_session_gslist_seek(GSList *sessions_list, double seek_time) {
    g_slist_foreach(sessions_list, rtp_session_seek, &seek_time);
}

/**
 * @brief Check pre-requisite for sending data for a session
 *
 * @param session The RTP session to check for pre-requisites
 *
 * @retval true The session is ready to send, and the @ref
 *              RTP_session::lock mutex has been locked.
 * @retval false The session is not ready to send, for whatever
 *               reason, and should just be ignored.
 *
 * This function is used to make sure that we're ready to deal with a
 * particular session. In particular this checks if the session is not
 * locked (we can just wait the next iteration if that's the case), is
 * not paused (or it's live), or has to be skipped for any reason at
 * all.
 *
 * @note If this function returned true, you need to unlock the mutex
 *       one way or another!
 */
static gboolean rtp_session_send_prereq(RTP_session *session) {
    /* We have this if here because we need to unlock the mutex if
     * we're not ready after locking it!
     */
    return ( session != NULL &&
             ( !session->pause ||
               session->track->properties->media_source == MS_live
               )
             );
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
        (buffer->timestamp - session->seek_time) * clock_rate;
    return session->start_rtptime + calc_rtptime;
}

static inline double calc_send_time(RTP_session *session)
{
    return session->last_timestamp - session->seek_time - session->send_time;
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
        if (!session->send_time)
            session->send_time = buffer->timestamp - session->seek_time;

        frames = (fabs(session->last_timestamp - buffer->timestamp) /
                  tr->properties->frame_duration) + 1;
        session->send_time += calc_send_time(session);
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
 *
 * @note This function will require exclusive access to the session,
 *       so it'll lock the @ref RTP_session::lock mutex.
 */
static void rtp_write_cb(struct ev_loop *loop, ev_periodic *w, int revents)
{
    RTP_session *session = w->data;
    MParserBuffer *buffer = NULL;

    if ( !rtp_session_send_prereq(session) )
        return;

#ifdef HAVE_METADATA
    if (session->metadata)
        cpd_send(session, now);
#endif

    /* Get the current buffer, if there is enough data */
    if ( !(buffer = bq_consumer_get(session->consumer)) ) {
        /* If there is no buffer, it means that either the producer
         * has been stopped (as we reached the end of stream) or that
         * there is no data for the consumer to read. If that's the
         * case we just give control back to the main loop for now.
         */

        if ( bq_consumer_stopped(session->consumer) ) {
            /* If the producer has been stopped, we send the
             * finishing packets and go away.
             */
            fnc_log(FNC_LOG_INFO, "[SCH] Stream Finished");
            RTCP_send_bye(session);
            return;
        }
        return;
    }

    if (rtp_packet_send(session, buffer) <=
            session->srv->srvconf.buffered_frames) {
        mt_resource_read(session->track->parent);
    }

    RTCP_handler(session);
}

/**
 * @brief Create a new RTP session object.
 *
 * @param rtsp The buffer for which to generate the session
 * @param rtsp_s The RTSP session
 * @param path The path of the resource
 * @param transport The transport used by the session
 * @param tr The track that will be sent over the session
 *
 * @return A pointer to a newly-allocated RTP_session, that needs to
 *         be freed with @ref rtp_session_free.
 *
 * @see rtp_session_free
 */
RTP_session *rtp_session_new(RTSP_Client *rtsp, RTSP_session *rtsp_s,
                             RTP_transport *transport, const char *path,
                             Track *tr) {
    feng *srv = rtsp->srv;
    RTP_session *rtp_s = g_slice_new0(RTP_session);

    /* Make sure we start paused since we have to wait for parameters
     * given by @ref rtp_session_resume.
     */
    rtp_s->pause = 1;
    rtp_s->sd_filename = g_strdup(path);

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
    ev_periodic_init(&rtp_s->transport.rtp_writer, rtp_write_cb,
                     0, 0, rtp_reschedule_cb);

    // Setup the RTP session
    rtsp_s->rtp_sessions = g_slist_append(rtsp_s->rtp_sessions, rtp_s);

    return rtp_s;
}
