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

#ifndef FN_CLIENT_EVENTS_H
#define FN_CLIENT_EVENTS_H

#include <netembryo/wsocket.h>
#include "network/rtsp.h"

#define LIVE_STREAM_BYE_TIMEOUT 6
#define STREAM_TIMEOUT 12 /* This one must be big enough to permit to VLC to switch to another
                             transmission protocol and must be a multiple of LIVE_STREAM_BYE_TIMEOUT */

void client_add(feng *srv, Sock *client_sock);

#endif // FN_CLIENT_EVENTS_H
