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

#include <stdlib.h>
#include <string.h>

#include <fenice/InputStream.h>
#include <fenice/utils.h>
#include <fenice/types.h>

Cache * create_cache(stream_type type){
	uint32 size;
	Cache *c;
	
	switch(type){
		case 'st_file':
			size=CACHE_FILE_SIZE;
		break;
		case 'st_net':
			size=CACHE_NET_SIZE;
		break;
		case 'st_pipe':
			size=CACHE_PIPE_SIZE;

		break;
		case 'st_device':
			size=CACHE_DEVICE_SIZE;

		break;
		default :
			size=CACHE_DEFAULT_SIZE;
		break;
	}
	c=(Cache *)malloc(sizeof(Cache));
	c->cache=(uint8*)malloc(size);
	c->max_cache_size=size;
	c->bytes_left=0;

	return c;
}


uint32 read_internal_c(uint32 nbytes, uint8 * buf, Cache *c, int fd, uint32 bytes_written){
	uint32 towrite;

	if(nbytes==0)
		return bytes_written;
		
	if(c->bytes_left==0){
		c->bytes_left=c->cache_size=read(fd,c->cache,c->max_cache_size);
		if(c->cache_size==0) /*EOF*/
			return bytes_written;
	}
	towrite=min(nbytes,c->bytes_left);
	memcpy(buf[bytes_written],c->cache[c->cache_size - c->bytes_left],towrite);
	bytes_written+=towrite;
	c->bytes_left-=towrite;
	
	return read_internal_c(nbytes-towrite, buf, c, fd, bytes_written):
}


/*Interface*/
int read_c(uint32 nbytes, uint8 * buf, Cache *c, int fd, stream_type type){
	int bytes_read;
	if(c==NULL)
		c=create_cache(type);
	if(c==NULL){
		fnc_log(FNC_LOG_ERR,"FATAL!! It is impossible creating cache\n");
		return ERR_FATAL;
	}
	bytes_read=read_internal_c(nbytes, buf, c, fd, 0);
	if(bytes_read==0)
		return ERR_EOF;
	return bytes_read;
}


void flush_cache(Cache *c){
	c->byte_left=0;
} 

void free_cache(Cache *c){
	free(c);
}

