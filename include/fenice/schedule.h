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

#ifndef _SCHEDULEH
#define _SCHEDULEH


#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <fenice/rtp.h>
#include <fenice/prefs.h>
#include <fenice/debug.h>


typedef struct _play_args {
	struct tm playback_time;
	short playback_time_valid;
	short start_time_valid;
	short seek_time_valid;
	float start_time;   //! time in seconds
	float end_time;
} play_args;

typedef struct _schedule_list {
        pthread_mutex_t mux;
	int valid;
	RTP_session *rtp_session;
	RTP_play_action play_action;
} schedule_list;

int schedule_init();

void *schedule_do(void *nothing);

int schedule_add(RTP_session * rtp_session /*,RTSP_session *rtsp_session */ );
int schedule_start(int id, play_args * args);
void schedule_stop(int id);
int schedule_remove(int id);
int schedule_resume(int id, play_args * args);

static inline double gettimeinseconds(void) {
    struct timeval now;
    gettimeofday(&now,NULL);
    return (double)now.tv_sec + (double)now.tv_usec/1000000.0;
}

#endif
