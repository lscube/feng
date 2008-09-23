/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2008 by LScube team <team@streaming.polito.it>
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
#include <fenice/utils.h>
#include <fenice/fnc_log.h>


int schedule_add(RTP_session *rtp_session)
{
    int i;
    feng *srv = rtp_session->srv;
    schedule_list *sched = srv->sched;
    for (i=0; i<ONE_FORK_MAX_CONNECTION; ++i) {
    g_mutex_lock(sched[i].mux);
        if (!sched[i].valid) {
            sched[i].valid = 1;
            sched[i].rtp_session = rtp_session;
            sched[i].play_action = RTP_send_packet;
            g_mutex_unlock(sched[i].mux);
            return i;
        }
    g_mutex_unlock(sched[i].mux);
    }
    // if (i >= MAX_SESSION) {
    return ERR_GENERIC;
    // }
}
