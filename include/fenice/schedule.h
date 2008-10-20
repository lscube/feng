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

#ifndef FN_SCHEDULE_H
#define FN_SCHEDULE_H


#include <time.h>
#include <sys/time.h>
#include <glib.h>
#include <fenice/rtp.h>
#include <fenice/prefs.h>
#include <fenice/debug.h>

typedef struct play_args {
    struct tm playback_time;
    short playback_time_valid;
    short seek_time_valid;
    double start_time;   //! time in seconds
    double begin_time;
    double end_time;
} play_args;

typedef struct schedule_list {
    GMutex *mux;
    int valid;
    RTP_session *rtp_session;
    RTP_play_action play_action;
} schedule_list;

void schedule_init(feng *srv);

int schedule_add(RTP_session * rtp_session);
int schedule_start(RTP_session * rtp_session, play_args * args);
int schedule_remove(RTP_session * rtp_session, void *unused);
int schedule_resume(RTP_session * rtp_session, play_args * args);
#endif // FN_SCHEDULE_H
