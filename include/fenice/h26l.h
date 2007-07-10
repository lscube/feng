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

#ifndef _H26LH
#define _H26LH

#include <fenice/mediainfo.h>
#include <fenice/types.h>

typedef struct {
	unsigned int bufsize;
	long pkt_sent;
	int current_timestamp;
	int next_timestamp;
} static_H26L;

int load_H26L(media_entry * me);
int read_H26L(media_entry * me, uint8 * buffer, uint32 * buffer_size,
	      double *mtime, int *recallme, uint8 * marker);
int free_H26L(void *stat);

#endif
