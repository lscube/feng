/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *  	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 * 
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#if !defined(__INPUTSTREAMH)
#define __INPUTSTREAMH

#include <fenice/types.h>

typedef struct __CACHE{
	uint8 *cache;
	uint32 cache_size;
	uint32 bytes_left;
}Cache;

typedef enum { st_file=0, st_udp, st_tcp, st_pipe, st_device} stream_type;

Cache * create_cache(stream_type);
void flush_cache(Cache *c); /* {c->byte_left=0;} */

typedef struct __INPUTSTREAM{
	stream_type type;
	Cache * cache;
	int fd;
	//... 
}InputStream;

#endif
