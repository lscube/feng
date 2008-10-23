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

#include <stdlib.h>
#ifdef ENABLE_DUMA
#include <duma.h>
#endif
#include <string.h>
#include <unistd.h>

#include <glib/gmem.h>

#include <fenice/InputStream.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

static int read_from_net(int fd, void *buf, size_t nbytes);/*not implemented yet*/
static int read_from_device(int fd, void *buf, size_t nbytes);/*not implemented yet*/

// shawill: should we receive cache size as parameter?
static Cache *create_cache(stream_type type)
{
    uint32_t size;
    Cache *c = g_new0(Cache, 1);
    
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
    if ( !(c->cache = g_malloc0(size)) ) {
        g_free(c);
        return NULL;
    }
    if(c->cache==NULL){
        g_free(c);
        return NULL;
    }
    c->max_cache_size=size;
    c->bytes_left=0;

    return c;
}


/*! internal read function
 * if buf pointer is NULL, then the read act like a forward seek
 * */
static uint32_t read_internal_c(uint32_t nbytes, uint8_t *buf, Cache *c, int fd)
{
    uint32_t bytes;

    if(!nbytes)
        return 0;
        
    if(!c->bytes_left) {
        c->bytes_left=c->cache_size=c->read_data(fd,c->cache,c->max_cache_size);/*can be: read, read_from_net, read_from_device*/
        if(!c->cache_size) /*EOF*/
            return 0;
    }
    bytes=min(nbytes,c->bytes_left);
    if (buf)
        memcpy(buf, &c->cache[c->cache_size - c->bytes_left], bytes);
    c->bytes_left-=bytes;
    
    return bytes + read_internal_c(nbytes-bytes, buf+bytes, c, fd);
}


/*Interface to Cache*/
int read_c(uint32_t nbytes, uint8_t *buf, Cache **c, int fd, stream_type type)
{
    int bytes_read;
//FIXME make sure it is thread safe!
    if( !*c && !(*c=create_cache(type))) { // shawill: should we do it here?
        fnc_log(FNC_LOG_FATAL, "Could not create cache for input stream\n");
        return ERR_FATAL;
    }

    bytes_read=read_internal_c(nbytes, buf, *c, fd);

    return bytes_read ? bytes_read : ERR_EOF;
}

void free_cache(Cache *c)
{
    if(c!=NULL){
        g_free(c->cache);
        c->cache=NULL;
        g_free(c);
        c=NULL;
    }
}

static int read_from_net(int fd, void *buf, size_t nbytes)
{
    return -1;
}
static int read_from_device(int fd, void *buf, size_t nbytes)
{
    return -1;
}

