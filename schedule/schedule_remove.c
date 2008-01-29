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

#include <stdio.h>

#include <fenice/schedule.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

int schedule_remove(RTP_session *session)
{
    schedule_list *sched = session->srv->sched;
    int id = session->sched_id;
    pthread_mutex_lock(&sched[id].mux);
    sched[id].valid = 0;
    if (sched[id].rtp_session) {
        RTP_session_destroy(sched[id].rtp_session);
        sched[id].rtp_session = NULL;
        fnc_log(FNC_LOG_INFO, "rtp session closed\n");
    }
    pthread_mutex_unlock(&sched[id].mux);
    return ERR_NOERROR;
}
