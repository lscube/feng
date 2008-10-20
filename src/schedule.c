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

#include "config.h"

#include <fenice/schedule.h>
#include <fenice/rtp.h>
#include <fenice/rtcp.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

#include <sys/time.h>
#include <unistd.h>

int schedule_init(feng *srv)
{    
    int i;
    GThread *thread;
    schedule_list *sched = g_new(schedule_list, ONE_FORK_MAX_CONNECTION);

    for (i=0; i<ONE_FORK_MAX_CONNECTION; ++i) {
        sched[i].rtp_session = NULL;
        sched[i].play_action = NULL;
        sched[i].valid = 0;
	sched[i].mux = g_mutex_new();
    }

    srv->sched = sched;

    thread = g_thread_create(schedule_do, srv, FALSE, NULL);

    return ERR_NOERROR;
}

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

int schedule_remove(RTP_session *session)
{
    schedule_list *sched = session->srv->sched;
    int id = session->sched_id;
    g_mutex_lock(sched[id].mux);
    sched[id].valid = 0;
    if (sched[id].rtp_session) {
        RTP_session_destroy(sched[id].rtp_session);
        sched[id].rtp_session = NULL;
        fnc_log(FNC_LOG_INFO, "rtp session closed\n");
    }
    g_mutex_unlock(sched[id].mux);
    return ERR_NOERROR;
}

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

void schedule_stop(RTP_session *session)
{
    RTCP_send_packet(session,SR);
    RTCP_send_packet(session,BYE);
    RTCP_flush(session);

    session->pause=1;
    session->started=0;
}

int schedule_resume(RTP_session *session, play_args *args)
{
    feng *srv = session->srv;
    int i;

    session->start_time = args->start_time;
    session->send_time = 0.0;
    session->pause=0;

    //Preload some frames in bufferpool
    for (i=0; i < srv->srvconf.buffered_frames; i++) {
        event_buffer_low(session, r_selected_track(session->track_selector));
    }

    return ERR_NOERROR;
}

#define SCHEDULER_TIMING 16000 //16ms. Sleep time suggested by Intel

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
        g_mutex_lock(sched[i].mux);
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
        g_mutex_unlock(sched[i].mux);
      }
    } while (!srv->stop_schedule);

    srv->stop_schedule = 0;
    return ERR_NOERROR;
}

