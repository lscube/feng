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

#include "rtp.h"
#include "rtsp.h"
#include "rtcp.h"
#include "fenice/fnc_log.h"

/**
 * @todo Luca has to document this! :P
 */
static void rtcp_read_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    RTP_recv(w->data);
}

/**
 * @brief Create a new RTP session object.
 *
 * @param rtsp The buffer for which to generate the session
 * @param rtsp_s The RTSP session
 * @param path The path of the URL
 * @param transport The transport
 * @param track_sel The track
 *
 * @return A pointer to a newly-allocated RTP_session, that needs to
 *         be freed with @ref rtp_session_free.
 *
 * @see rtp_session_free
 */
RTP_session *rtp_session_new(RTSP_buffer * rtsp, RTSP_session *rtsp_s,
                             RTP_transport * transport, const char *path,
                             Selector * track_sel) {
    feng *srv = rtsp->srv;
    RTP_session *rtp_s = g_slice_new0(RTP_session);

    rtp_s->lock = g_mutex_new();
    g_mutex_lock(rtp_s->lock);

    rtp_s->pause = 1;
    rtp_s->sd_filename = g_strdup(path);

    memcpy(&rtp_s->transport, transport, sizeof(RTP_transport));
    rtp_s->start_rtptime = g_random_int();
    rtp_s->start_seq = g_random_int_range(0, G_MAXUINT16);
    rtp_s->seq_no = rtp_s->start_seq - 1;
    rtp_s->track_selector = track_sel;
    rtp_s->srv = srv;
    rtp_s->sched_id = schedule_add(rtp_s);
    rtp_s->ssrc = g_random_int();
    rtp_s->rtsp_buffer = rtsp;

#ifdef HAVE_METADATA
	rtp_s->metadata = rtsp_s->resource->metadata;
#endif

    rtp_s->transport.rtcp_watcher = g_new(ev_io, 1);
    rtp_s->transport.rtcp_watcher->data = rtp_s;
    ev_io_init(rtp_s->transport.rtcp_watcher, rtcp_read_cb, Sock_fd(rtp_s->transport.rtcp_sock), EV_READ);
    ev_io_start(srv->loop, rtp_s->transport.rtcp_watcher);

    // Setup the RTP session
    rtsp_s->rtp_sessions = g_slist_append(rtsp_s->rtp_sessions, rtp_s);

    return rtp_s;
}

/**
 * Deallocates an RTP session, closing its tracks and transports
 *
 * @param session The RTP session to free
 *
 * @see rtp_session_new
 */
void rtp_session_free(RTP_session * session)
{
    RTP_transport_close(session);

    g_mutex_free(session->lock);

    // Close track selector
    r_close_tracks(session->track_selector);

    /* Remove the consumer */
    bq_consumer_free(session->consumer);

    // Deallocate memory
    g_free(session->sd_filename);
    g_slice_free(RTP_session, session);
}

/**
 * @brief Internal version to resume an RTP session
 *
 * @param session Session to resume
 * @param start_time The time (in seconds) inside the stream to start
 *                   from.
 *
 * @internal This function does not lock @ref RTP_session::lock, so
 * should only be used by functions that already take care of that.
 *
 * @see rtp_session_start
 * @ses rtp_session_resume
 */
static void rtp_session_resume_internal(RTP_session *session,
                                        double start_time) {
    feng *srv = session->srv;
    int i;

    session->start_time = start_time;
    session->send_time = 0.0;
    session->pause = 0;
    session->started = 1;

    /* Prefetch frames */
    for (i=0; i < srv->srvconf.buffered_frames; i++)
        event_buffer_low(session, r_selected_track(session->track_selector));
}

/**
 * @brief Set up and mark a session as started
 *
 * @param session The session to start
 * @param start_time The time (in seconds) inside the stream to start
 *                   from.
 *
 * @todo This function should probably take care of starting eventual
 *       libev events when the scheduler is replaced.
 * @todo @ref rtp_session_start and @ref rtp_session_resume should be
 *       factored in together. Also check @ref rtp_session_play in
 *       rtsp_method_play.c .
 */
void rtp_session_start(RTP_session *session, double start_time) {
    Track *tr;

    g_mutex_lock(session->lock);

    tr = r_selected_track(session->track_selector);

    session->consumer = bq_consumer_new(tr->producer);

    session->start_time = start_time;
    session->last_timestamp = 0;
    session->MinimumReached = 0;
    session->MaximumReached = 0;
    session->PreviousCount = 0;
    session->rtcp_stats[i_client].RR_received = 0;
    session->rtcp_stats[i_client].SR_received = 0;

    rtp_session_resume_internal(session, start_time);

    g_mutex_unlock(session->lock);
}

/**
 * @brief Resume a paused session
 *
 * @param session The session to start
 * @param start_time The time (in seconds) inside the stream to start
 *                   from.
 *
 * @todo This function should probably take care of starting eventual
 *       libev events when the scheduler is replaced.
 * @todo @ref rtp_session_start and @ref rtp_session_resume should be
 *       factored in together. Also check @ref rtp_session_play in
 *       rtsp_method_play.c .
 */
void rtp_session_resume(RTP_session *session, double start_time) {
    g_mutex_lock(session->lock);

    rtp_session_resume_internal(session, start_time);

    g_mutex_unlock(session->lock);
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
 * @important If this function returned true, you need to unlock the
 *            mutex one way or another!
 */
static gboolean rtp_session_send_prereq(RTP_session *session) {
    Track *tr;

    /* We have this if here because we need to unlock the mutex if
     * we're not ready after locking it!
     */
    if (session == NULL ||
        !g_mutex_trylock(session->lock))
        return false;

    tr = r_selected_track(session->track_selector);

    if ( !session->started ||
         ( session->pause &&
           tr->properties->media_source != MS_live )
         ) {
        g_mutex_unlock(session->lock);
        return false;
    }

    return true;
}

/**
 * Handle sending pending RTP packets to a session.
 *
 * @param session the RTP session for which to send the packets
 *
 * @note This function will require exclusive access to the session,
 *       so it'll lock the @ref RTP_session::lock mutex.
 */
void rtp_session_handle_sending(RTP_session *session)
{
    MParserBuffer *buffer = NULL;
    double now;

    if ( !rtp_session_send_prereq(session) )
        return;

    now = gettimeinseconds(NULL);

#ifdef HAVE_METADATA
    if (session->metadata)
        cpd_send(session, now);
#endif

    while ( now >= session->start_time &&
            now >= (session->start_time + session->send_time) ) {

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
                RTCP_send_packet(session, SR);
                RTCP_send_packet(session, BYE);
                RTCP_flush(session);
                break;
            }

            break;
        }

        if (rtp_packet_send(session, buffer) <= session->srv->srvconf.buffered_frames) {
            switch( event_buffer_low(session,
                                     r_selected_track(session->track_selector)) ) {
            case ERR_EOF:
            case ERR_NOERROR:
                break;
            default:
                fnc_log(FNC_LOG_FATAL, "Unable to emit event buffer low");
                break;
            }
        }

        RTCP_handler(session);
    }

    g_mutex_unlock(session->lock);
}
