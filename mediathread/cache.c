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
#include <fenice/fnc_log.h>

// shawill: should we receive cache size as parameter?
Cache *create_cache(stream_type type)
{
	uint32 size;
	Cache *c;
	
	if ( !(c=(Cache *)malloc(sizeof(Cache))) )
		return NULL;

	switch(type) {
		case st_file:
			size=CACHE_FILE_SIZE;
			c->read_data=read;
			break;
		case st_net:
			size=CACHE_NET_SIZE;
			c->read_data=read_from_net;
			break;
		case st_pipe:
			size=CACHE_PIPE_SIZE;
			c->read_data=read;
			break;
		case st_device:
			size=CACHE_DEVICE_SIZE;
			c->read_data=read_from_device;
			break;
		default:
			size=CACHE_DEFAULT_SIZE;
			c->read_data=read;
			break;
	}
	if ( !(c->cache=(uint8 *)malloc(size)) ) {
		free(c);
		return NULL;
	}
	if(c->cache==NULL){
		free(c);
		return NULL;
	}
	c->max_cache_size=size;
	c->bytes_left=0;

	return c;
}


static uint32 read_internal_c(uint32 nbytes, uint8 *buf, Cache *c, int fd)
{
	uint32 bytes;

	if(nbytes==0)
		return 0;
		
	if(c->bytes_left==0) {
		c->bytes_left=c->cache_size=c->read_data(fd,c->cache,c->max_cache_size);/*can be: read, read_from_net, read_from_device*/
		if(c->cache_size==0) /*EOF*/
			return 0;
	}
	bytes=min(nbytes,c->bytes_left);
	memcpy(buf, &c->cache[c->cache_size - c->bytes_left], bytes);
	c->bytes_left-=bytes;
	
	return bytes + read_internal_c(nbytes-bytes, buf+bytes, c, fd);
}


/*Interface to Cache*/
int read_c(uint32 nbytes, uint8 *buf, Cache *c, int fd, stream_type type)
{
	int bytes_read;

	if( !c && !(c=create_cache(type))) { // shawill: should we do it here?
		fnc_log(FNC_LOG_FATAL, "Could not create cache for input stream\n");
		return ERR_FATAL;
	}

	bytes_read=read_internal_c(nbytes, buf, c, fd);
	if(bytes_read==0)
		return ERR_EOF;
	return bytes_read;
}

void flush_cache(Cache *c)
{
	if(c!=NULL)
		c->bytes_left=0;
} 

void free_cache(Cache *c)
{
	if(c!=NULL){
		free(c->cache);
		c->cache=NULL;
		free(c);
		c=NULL;
	}
}

