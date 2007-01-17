/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *      
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
 *    - Eugenio Menegatti     <m.eu@libero.it>
 *    - Stefano Cau
 *    - Giuliano Emma
 *    - Stefano Oldrini
 * 
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

// #include <time.h>
#include <sys/time.h>

#include <fenice/types.h>
#include <fenice/utils.h>
#include <fenice/schedule.h>
#include <fenice/rtp.h>

extern schedule_list sched[ONE_FORK_MAX_CONNECTION];

int32 schedule_start(int id, play_args *args)
{
    struct timeval now;
    double mnow;

    gettimeofday(&now,NULL);
    mnow=(double)now.tv_sec*1000+(double)now.tv_usec/1000;
    sched[id].rtp_session->cons = 
        OMSbuff_ref(sched[id].rtp_session->current_media->pkt_buffer);

    if (sched[id].rtp_session->cons == NULL)
        return ERR_ALLOC;

/* Iff this session is the first session related to this media_entry,
 * then it runs here
 * */
    if (sched[id].rtp_session->current_media->pkt_buffer->control->refs==1) {
        if (!args->playback_time_valid) {
            sched[id].rtp_session->current_media->mstart = mnow;
        } else {
            sched[id].rtp_session->current_media->mstart = 
                mktime(&(args->playback_time));
        }
        sched[id].rtp_session->current_media->mtime = mnow;
        sched[id].rtp_session->current_media->mstart_offset = 
            args->start_time*1000;
        sched[id].rtp_session->current_media->play_offset =
            args->start_time*1000;
    }
    sched[id].rtp_session->mprev_tx_time = mnow;
    sched[id].rtp_session->pause = 0;
    sched[id].rtp_session->started = 1;
    sched[id].rtp_session->MinimumReached = 0;
    sched[id].rtp_session->MaximumReached = 0;
    sched[id].rtp_session->PreviousCount = 0;
    sched[id].rtp_session->rtcp_stats[i_client].RR_received = 0;
    sched[id].rtp_session->rtcp_stats[i_client].SR_received = 0;
    return ERR_NOERROR;
}
