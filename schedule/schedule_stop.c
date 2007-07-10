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

#include <fenice/schedule.h>
#include <fenice/rtp.h>
#include <fenice/rtcp.h>

extern schedule_list sched[ONE_FORK_MAX_CONNECTION];

void schedule_stop(int id)
{
    RTCP_send_packet(sched[id].rtp_session,SR);
    RTCP_send_packet(sched[id].rtp_session,BYE);

    sched[id].rtp_session->pause=1;

    sched[id].rtp_session->started=0;
    //sched[id].rtsp_session->cur_state=READY_STATE;
}
