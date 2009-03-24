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

#ifndef FN_EVENTLOOP_H
#define FN_EVENTLOOP_H

#include <netembryo/wsocket.h>
#include <fenice/fnc_log.h>
#include "network/rtsp.h"
#include <fenice/server.h>

#define MAX_FDS 800

int feng_bind_port(feng *srv, char *host, char *port, specific_config *s);
void eventloop_init(feng *srv);
void eventloop(feng *srv);
void eventloop_cleanup(feng *srv);

/**
 * @defgroup interleaved_callbacks Callbacks for interleaved streaming
 * @{
 */

void interleaved_read_sctp_cb(struct ev_loop *, ev_io *, int);
void interleaved_read_tcp_cb(struct ev_loop *, ev_io *, int);

/**
 * @}
 */

#endif // FN_EVENTLOOP_H
