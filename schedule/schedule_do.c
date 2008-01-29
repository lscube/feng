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
#include <sys/time.h>

#include <pthread.h>
#include <fenice/schedule.h>
#include <fenice/rtcp.h>
#include <fenice/utils.h>
#include <fenice/debug.h>
#include <fenice/fnc_log.h>

#define SCHEDULER_TIMING 16000 //16ms. Sleep time suggested by Intel

extern int stop_schedule;

void *schedule_do(void *arg)
{
    int i = 0, res;
    unsigned utime = SCHEDULER_TIMING;
    double now;
    feng *srv = arg;
    schedule_list *sched = srv->sched;

do {
    // Fake waiting. Break the while loop to achieve fair kernel (re)scheduling and fair CPU loads.
    usleep(utime);
    utime = SCHEDULER_TIMING;
    for (i=0; i<ONE_FORK_MAX_CONNECTION; ++i) {
        pthread_mutex_lock(&sched[i].mux);
        if (sched[i].valid && sched[i].rtp_session->started) {
            RTP_session * session = sched[i].rtp_session;
            Track *tr = r_selected_track(session->track_selector);
            if (!session->pause || tr->properties->media_source == MS_live) {
                now = gettimeinseconds(NULL);
                res = ERR_NOERROR;
                while (res == ERR_NOERROR && now >= session->start_time &&
                    now >= session->start_time + session->send_time) {
#if 1
//TODO DSC will be implemented WAY later.
#else
                    stream_change(session,
                        change_check(session));
#endif
                // Send an RTP packet
                    res = sched[i].play_action(session);
                    switch (res) {
                        case ERR_NOERROR: // All fine
                            break;
                        case ERR_EOF:
                            if(tr->properties->media_source != MS_live) {
                                fnc_log(FNC_LOG_INFO,"[SCH] Stream Finished");
                                RTCP_send_packet(session, SR);
                                RTCP_send_packet(session, BYE);
                                RTCP_flush(session);
                            }
                            break;
                        case ERR_ALLOC:
                            fnc_log(FNC_LOG_WARN,"[SCH] Cannot allocate memory");
                            schedule_stop(session);
                            break;
                        default:
                            fnc_log(FNC_LOG_WARN,"[SCH] Packet Lost");
                            break;
                    }
                    RTCP_handler(session);
                    /*if RTCP_handler return ERR_GENERIC what do i have to do?*/
                }
                if (res == ERR_NOERROR) {
                    int next_send_time = fabs((session->start_time +
                                        session->send_time) - now) * 1000000;
                    utime = min(utime, next_send_time);
                }
            }
        }
        pthread_mutex_unlock(&sched[i].mux);
    }
} while (!stop_schedule);

    stop_schedule=0;
    return ERR_NOERROR;
}

