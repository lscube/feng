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


#ifndef FN_DEBUG_H
#define FN_DEBUG_H

#include <config.h>


#if ENABLE_VERBOSE
void dump_buffer(char *buffer);
#define VERBOSE
#endif
#if ENABLE_DUMP
int dump_payload(uint8_t * data_slot, uint32_t data_size, char fname[255]);
#endif


#define DEBUG ENABLE_DEBUG
//      #define POLLED 
//      #define SIGNALED 
#define THREADED
//      #define SELECTED 

#endif // FN_DEBUG_H 
