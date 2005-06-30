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

#define CACHE_FILE_SIZE 65536
#define CACHE_NET_SIZE 65536
#define CACHE_PIPE_SIZE 4096
#define CACHE_DEVICE_SIZE 4096
#define CACHE_DEFAULT_SIZE 65536

#ifndef min
#define min(a,b) (a<b)?a:b
#endif // min

typedef struct __CACHE {
	uint8 *cache;
	uint32 max_cache_size;
	uint32 cache_size;
	uint32 bytes_left;
	int (*read_data)(int /*fd*/, void * /*buf*/, size_t /*nbytes*/); /*can be: read, read_from_net, read_from_device*/
} Cache;

typedef enum { st_file=0, st_net, st_pipe, st_device} stream_type;

Cache *create_cache(stream_type);
int read_from_net(int fd, void *buf, size_t nbytes);/*not implemented yet*/
int read_from_device(int fd, void *buf, size_t nbytes);/*not implemented yet*/

/*Interface to Cache*/
int read_c(uint32 nbytes, uint8 *buf, Cache *c, int fd, stream_type);
void flush_cache(Cache *c); 
void free_cache(Cache *c); 

typedef struct __INPUTSTREAM {
	char name[255];
	stream_type type;
	Cache *cache;
	int fd;
	//... properties for file, net or device 
} InputStream;


/*Interface to InputStream*/
InputStream *create_inputstream(uint8 *mrl);
inline int read_stream(uint32 nbytes, uint8 *buf, InputStream *is); 
int parse_mrl(uint8 *mrl, stream_type *type, int *fd);

#endif // __INPUTSTREAMH
