/* *
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 *
 * feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * feng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with feng; if not, write to the Free Software
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

void schedule_init(feng *srv);

int schedule_add(RTP_session * rtp_session);
int schedule_remove(RTP_session * rtp_session, void *unused);
RTP_session *schedule_find_multicast(feng *srv, const char *mrl);

#endif // FN_SCHEDULE_H
