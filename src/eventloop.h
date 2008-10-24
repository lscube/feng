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

#ifndef FN_EVENTLOOP_H
#define FN_EVENTLOOP_H

#include <netembryo/wsocket.h>
#include <fenice/fnc_log.h>
#include "network/rtsp.h"
#include <fenice/server.h>

#define MAX_FDS 800

typedef int (*event_function) (void *data);

int feng_bind_port(char *host, char *port, specific_config *s);
void eventloop_init();
void eventloop(feng *srv);
void eventloop_cleanup();

#endif // FN_EVENTLOOP_H
