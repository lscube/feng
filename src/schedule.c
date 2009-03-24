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

#include <fenice/schedule.h>
#include "network/rtp.h"
#include "network/rtsp.h"
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

#include <unistd.h>

#ifdef HAVE_METADATA
#include <metadata/cpd.h>
#endif

static struct RTP_session **sessions;
static GStaticRWLock sessions_lock = G_STATIC_RW_LOCK_INIT;

/** @brief Locks the @ref sessions_lock R/W lock in write mode */
static void schedule_writer_lock() {
    g_static_rw_lock_writer_lock(&sessions_lock);
}

/** @brief Release the @ref sessions_lock R/W lock in write mode */
static void schedule_writer_unlock() {
    g_static_rw_lock_writer_unlock(&sessions_lock);
}

/** @brief Locks the @ref sessions_lock R/W lock in read mode */
static void schedule_reader_lock() {
    g_static_rw_lock_reader_lock(&sessions_lock);
}

/** @brief Release the @ref sessions_lock R/W lock in read mode */
static void schedule_reader_unlock() {
    g_static_rw_lock_reader_unlock(&sessions_lock);
}

int schedule_add(RTP_session *rtp_session)
{
    int i, ret = ERR_GENERIC;
    feng *srv = rtp_session->srv;

    schedule_writer_lock();
    for (i=0; i<ONE_FORK_MAX_CONNECTION; ++i) {
        if ( sessions[i] == NULL ) {
            sessions[i] = rtp_session;
            ret = i;
            break;
        }
    }
    schedule_writer_unlock();

    return ret;
}

int schedule_remove(RTP_session *session, void *unused)
{
    int id = session->sched_id;

    schedule_writer_lock();

    RTP_session_destroy(session);
    fnc_log(FNC_LOG_INFO, "rtp session closed\n");
    sessions[id] = NULL;

    schedule_writer_unlock();

    return ERR_NOERROR;
}

int schedule_start(RTP_session *session, play_args *args)
{
    feng *srv = session->srv;
    Track *tr;
    int i;

    g_mutex_lock(session->lock);

    tr = r_selected_track(session->track_selector);

    session->consumer = bq_consumer_new(tr->producer);

    session->start_time = args->start_time;
    session->send_time = 0.0;
    session->last_timestamp = 0;
    session->pause = 0;
    session->started = 1;
    session->MinimumReached = 0;
    session->MaximumReached = 0;
    session->PreviousCount = 0;
    session->rtcp_stats[i_client].RR_received = 0;
    session->rtcp_stats[i_client].SR_received = 0;

    //Preload some frames in feng
    for (i=0; i < srv->srvconf.buffered_frames; i++) {
        event_buffer_low(session, r_selected_track(session->track_selector));
    }

    g_mutex_unlock(session->lock);

    return ERR_NOERROR;
}

static void schedule_stop(RTP_session *session)
{
    g_mutex_lock(session->lock);
    RTCP_send_packet(session,SR);
    RTCP_send_packet(session,BYE);
    RTCP_flush(session);

    session->pause=1;
    session->started=0;
    g_mutex_unlock(session->lock);
}

int schedule_resume(RTP_session *session, play_args *args)
{
    feng *srv = session->srv;
    int i;

    g_mutex_lock(session->lock);

    session->start_time = args->start_time;
    session->send_time = 0.0;
    session->pause=0;

    //Preload some frames in feng
    for (i=0; i < srv->srvconf.buffered_frames; i++) {
        event_buffer_low(session, r_selected_track(session->track_selector));
    }

    g_mutex_unlock(session->lock);

    return ERR_NOERROR;
}

static void *schedule_do(void *arg)
{
    int i = 0, res;
    double now;
    feng *srv = arg;

    do {
        g_thread_yield();

        schedule_reader_lock();
        for (i = 0; i < ONE_FORK_MAX_CONNECTION; ++i)
            rtp_session_handle_sending(sessions[i]);
        schedule_reader_unlock();
    } while (!g_atomic_int_get(&srv->stop_schedule));

    return ERR_NOERROR;
}

void schedule_init(feng *srv)
{
    sessions = g_new0(RTP_session *, ONE_FORK_MAX_CONNECTION);

    g_thread_create(schedule_do, srv, FALSE, NULL);
}

/**
 * @brief Find a multicast instance from the MRL parameter
 *
 * @param srv The server instance in use (for ONE_FORK_MAX_CONNECTION)
 * @param mrl MRL to look for
 *
 * @return The RTP instance for the multicast stream, or NULL if none
 *         is found.
 */
RTP_session *schedule_find_multicast(feng *srv, const char *mrl)
{
    int i;
    RTP_session *rtp_s = NULL;

    schedule_reader_lock();
    for (i = 0; !rtp_s && i<ONE_FORK_MAX_CONNECTION; ++i) {
        if (sessions[i] == NULL)
            continue;

        Track *tr2 = r_selected_track(sessions[i]->track_selector);
        if (!strncmp(tr2->info->mrl, mrl, 255)) {
            rtp_s = sessions[i];
            fnc_log(FNC_LOG_DEBUG,
                    "Found multicast instance.");
            break;
        }
    }
    schedule_reader_unlock();

    return rtp_s;
}
