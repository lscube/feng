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
#if ENABLE_MEDIATHREAD
    Track *tr = r_selected_track(sched[id].rtp_session->track_selector);
    double mnow, pkt_len = tr->properties->frame_duration;

    mnow = gettimeinseconds();
/*
    sched[id].rtp_session->current_media->mstart_offset 
        += sched[id].rtp_session->current_media->mtime 
         - sched[id].rtp_session->current_media->mstart 
         + pkt_len;

    if (args->start_time_valid)
        sched[id].rtp_session->current_media->play_offset =
            args->start_time*1000;
    */

    sched[id].rtp_session->start_time = mnow;
//    sched[id].rtp_session->current_media->mtime =
    sched[id].rtp_session->prev_tx_time = mnow - pkt_len;
#endif
    sched[id].rtp_session->pause=0;

    return ERR_NOERROR;
}
