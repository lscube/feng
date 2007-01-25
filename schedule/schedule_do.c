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

#include <stdio.h>
// #include <time.h>
#include <sys/time.h>

#include <pthread.h>
#include <fenice/schedule.h>
//#include <fenice/intnet.h>
#include <fenice/rtcp.h>
#include <fenice/utils.h>
#include <fenice/debug.h>
#include <fenice/fnc_log.h>

extern schedule_list sched[ONE_FORK_MAX_CONNECTION];
extern int stop_schedule;

#ifdef SELECTED    
void *schedule_do(void *nothing)
#endif
#ifdef THREADED
void *schedule_do(void *nothing)
#endif
#ifdef POLLED
void schedule_do(int sig)
#endif
#ifdef SIGNALED
void schedule_do(int sig)
#endif
{
    int i=0,res=ERR_GENERIC;
    struct timeval now;
    double mnow;
    // Fake timespec for fake nanosleep. See below.
    struct timespec ts = {0,0};
#ifdef SELECTED
    struct timeval t;
    fd_set rs;
#endif    
#ifdef THREADED
do {
#endif    
#ifdef SELECTED    
do {
    t.tv_sec=0;
    t.tv_usec=26122;
    FD_ZERO(&rs);
    if (select(0,&rs,&rs,&rs,&t)<0) {
        fnc_log(FNC_LOG_FATAL,"failed!\n");
    }
#endif
    // Fake waiting. Break the while loop to achieve fair kernel (re)scheduling and fair CPU loads.
    // See also main.c
    nanosleep(&ts, NULL);

    for (i=0; i<ONE_FORK_MAX_CONNECTION; ++i) {
        if (sched[i].valid) {
            if (!sched[i].rtp_session->pause) {
                gettimeofday(&now,NULL);
                mnow=(double)now.tv_sec*1000+(double)now.tv_usec/1000;
#if ENABLE_MEDIATHREAD
#warning Write mt equivalent
#else
                if (mnow >= sched[i].rtp_session->current_media->mstart &&
                    mnow - sched[i].rtp_session->mprev_tx_time >=
                        sched[i].rtp_session->current_media->description.pkt_len)
#endif
                {
#if ENABLE_MEDIATHREAD
#warning Write mt equivalent
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
                            if(r_selected_track(sched[i].rtp_session->track_selector)->msource==live) {
                                    fnc_log(FNC_LOG_WARN,"Pipe empty!\n");
                            } else {
                                    fnc_log(FNC_LOG_INFO,"Stream Finished\n");
                                    schedule_stop(i);
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
#if ENABLE_MEDIATHREAD
#warning Write mt equivalent
#else
                    sched[i].rtp_session->mprev_tx_time
                        += sched[i].rtp_session->current_media->description.pkt_len;
#endif
                }
            }

        } else if (sched[i].rtp_session) {
            if(sched[i].rtp_session->is_multicast_dad) {
                /*unicast always is a multicast_dad*/
                // fprintf(stderr, "rtp session not valid, but still present...\n");
                RTP_session_destroy(sched[i].rtp_session);
                sched[i].rtp_session = NULL;
                fnc_log(FNC_LOG_INFO, "rtp session closed\n");
            }
        }
    }
#ifdef THREADED
} while (!stop_schedule);
stop_schedule=0;
return ERR_NOERROR;
#endif
#ifdef SELECTED
} while (1);
#endif
}

