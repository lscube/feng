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

extern schedule_list sched[ONE_FORK_MAX_CONNECTION];
extern int stop_schedule;

void *schedule_do(void *nothing)
{
    int i=0,res=ERR_GENERIC;
    double mnow;
    // Fake timespec for fake nanosleep. See below.
    struct timespec ts = {0,0};
do {
    // Fake waiting. Break the while loop to achieve fair kernel (re)scheduling and fair CPU loads.
    nanosleep(&ts, NULL);

    for (i=0; i<ONE_FORK_MAX_CONNECTION; ++i) {
        pthread_mutex_lock(&sched[i].mux);
            if (sched[i].valid) {

            if (!sched[i].rtp_session->pause) {
                Track *tr = 
                    r_selected_track(sched[i].rtp_session->track_selector);
                mnow = gettimeinseconds();
                if (mnow >= sched[i].rtp_session->start_time &&
                    mnow - sched[i].rtp_session->prev_tx_time >=
                        tr->properties->duration)
                {
#if 1
//TODO DSC will be implemented WAY later.
#else
                    stream_change(sched[i].rtp_session,
                        change_check(sched[i].rtp_session));
#endif
                    RTCP_handler(sched[i].rtp_session);
                /*if RTCP_handler return ERR_GENERIC what do i have to do?*/

                // Send an RTP packet
                    res = sched[i].play_action(sched[i].rtp_session);
                    switch (res) {
                        case ERR_NOERROR: // All fine
                            break;
                        case ERR_EOF:
                            if(tr->properties->media_source == MS_live) {
                                fnc_log(FNC_LOG_WARN,"Pipe empty!\n");
                            } else {
                                fnc_log(FNC_LOG_INFO,"[BYE] Stream Finished");
                                RTCP_send_packet(sched[i].rtp_session, SR);
                                RTCP_send_packet(sched[i].rtp_session, BYE);
                                RTCP_flush(sched[i].rtp_session);
                            }
                            break;
                        case ERR_ALLOC:
                            fnc_log(FNC_LOG_WARN,"Cannot allocate memory!!\n");
                            schedule_stop(i);
                            break;
                        default:
                            fnc_log(FNC_LOG_WARN,"Packet Lost\n");
                            break;
                    }
                    sched[i].rtp_session->prev_tx_time +=
                        tr->properties->duration;
                }
            }
        }
        pthread_mutex_unlock(&sched[i].mux);
    }
} while (!stop_schedule);

    stop_schedule=0;
    return ERR_NOERROR;
}

