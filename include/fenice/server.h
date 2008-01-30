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

/**
 * @file server.h
 * Server instance structure
 */

#ifndef FN_SERVER_H
#define FN_SERVER_H

// #include <fenice/schedule.h>
#include <fenice/prefs.h>
#include <netembryo/wsocket.h>
#include <pwd.h>
#include <grp.h>

typedef struct feng_s {
    Sock *main_sock;
    Sock *sctp_main_sock;
    pthread_t mth;
    int port;
    struct _schedule_list *sched;
    pref_record *prefs;
    int start_port;
    int *port_pool;
    gid_t gid;
    uid_t uid;
/** eventloop stuff */
    struct _RTSP_buffer *rtsp_list;
    int num_conn;
    int conn_count;
    int child_count;
} feng;

#endif // FN_SERVER_H
