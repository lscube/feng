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

#include <fenice/schedule.h>
#include <fenice/rtp.h>
#include <fenice/utils.h>

extern schedule_list sched[ONE_FORK_MAX_CONNECTION];

int schedule_resume(int id, play_args *args)
{
    RTP_session *session = sched[id].rtp_session;
    int i;

    session->start_time = args->start_time;
    session->send_time = 0.0;
    session->pause=0;

    //Preload some frames in bufferpool
    for (i=0; i < PRELOADED_FRAMES; i++) {
        event_buffer_low(session, r_selected_track(session->track_selector));
    }

    return ERR_NOERROR;
}
