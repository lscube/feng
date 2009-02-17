/* * 
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 * 
 * bufferpool is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * bufferpool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with bufferpool; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA 
 *
 * */

#ifndef FN_SCHEDULE_H
#define FN_SCHEDULE_H


#include <time.h>
#include <sys/time.h>
#include <glib.h>
#include "network/rtp.h"
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

void schedule_init(feng *srv);

int schedule_add(RTP_session * rtp_session);
int schedule_start(RTP_session * rtp_session, play_args * args);
int schedule_remove(RTP_session * rtp_session, void *unused);
int schedule_resume(RTP_session * rtp_session, play_args * args);
RTP_session *schedule_find_multicast(feng *srv, const char *mrl);

#endif // FN_SCHEDULE_H
