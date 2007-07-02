/* * 
 *  $Id$
 *  
 *  This file is part of Feng
 *
 *  Feng -- Standard Streaming Server
 *
 *  Copyright (C) 2007 by
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
 *  Feng is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Feng is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Feng; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#include <stdio.h>

#include <fenice/schedule.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

extern schedule_list sched[ONE_FORK_MAX_CONNECTION];

int schedule_remove(int id)
{
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
