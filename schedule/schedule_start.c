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

#include <fenice/utils.h>
#include <fenice/schedule.h>
#include <fenice/rtp.h>

int schedule_start(RTP_session *session, play_args *args)
{
    feng *srv = session->srv;
    Track *tr = r_selected_track(session->track_selector);
    int i;

    session->cons = bp_ref(tr->buffer);
    if (session->cons == NULL)
        return ERR_ALLOC;

    session->start_time = args->start_time;
    session->send_time = 0.0;
    session->pause = 0;
    session->started = 1;
    session->MinimumReached = 0;
    session->MaximumReached = 0;
    session->PreviousCount = 0;
    session->rtcp_stats[i_client].RR_received = 0;
    session->rtcp_stats[i_client].SR_received = 0;

    //Preload some frames in bufferpool
    for (i=0; i < srv->srvconf.buffered_frames; i++) {
        event_buffer_low(session, r_selected_track(session->track_selector));
    }

    return ERR_NOERROR;
}
