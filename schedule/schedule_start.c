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

// #include <time.h>
#include <sys/time.h>

#include <fenice/utils.h>
#include <fenice/schedule.h>
#include <fenice/rtp.h>

extern schedule_list sched[ONE_FORK_MAX_CONNECTION];

int schedule_start(int id, play_args *args)
{
    double mnow;
    RTP_session *session = sched[id].rtp_session;
    Track *tr = r_selected_track(session->track_selector);

    mnow = gettimeinseconds(NULL);

    session->cons = bp_ref(tr->buffer);

    if (session->cons == NULL)
        return ERR_ALLOC;

/* Iff this session is the first session related to this media_entry,
 * then it runs here
 * */
    if (tr->buffer->control->refs == 1) {
#if ENABLE_MEDIATHREAD
        if (!args->playback_time_valid) {
            session->start_time = mnow;
        } else {
            session->start_time = mktime(&(args->playback_time));
        }
#else
        if (!args->playback_time_valid) {
            session->current_media->mstart = mnow;
        } else {
            session->current_media->mstart =
                mktime(&(args->playback_time));
        }
        sched[id].rtp_session->current_media->mtime = mnow;
        sched[id].rtp_session->current_media->mstart_offset = 
            args->start_time*1000;
        sched[id].rtp_session->current_media->play_offset =
            args->start_time*1000;
#endif
    }
    session->prev_tx_time = mnow;
    session->pause = 0;
    session->started = 1;
    session->MinimumReached = 0;
    session->MaximumReached = 0;
    session->PreviousCount = 0;
    session->rtcp_stats[i_client].RR_received = 0;
    session->rtcp_stats[i_client].SR_received = 0;
    return ERR_NOERROR;
}
