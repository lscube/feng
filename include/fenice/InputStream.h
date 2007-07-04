/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2007 by
 *      
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
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

#ifndef __INPUTSTREAMH
#define __INPUTSTREAMH

#include <stdint.h>

#define CACHE_FILE_SIZE     65536
#define CACHE_NET_SIZE      65536
#define CACHE_PIPE_SIZE      4096
#define CACHE_DEVICE_SIZE    4096
#define CACHE_DEFAULT_SIZE  65536

#define FNC_PROTO_SEPARATOR "://"

#define FNC_FILE       "file"
// #define FNC_PIPE    "pipe"
#define FNC_UDP        "udp"
#define FNC_TCP        "tcp"
#define FNC_DEV        "dev" // it will survive to tomorrow?
// ... devices, ...

#define DEFAULT_ST_TYPE st_file

#ifndef min
#define min(a,b) (a<b)?a:b
#endif // min

typedef struct __CACHE {
    uint8_t *cache;
    uint32_t max_cache_size;
    uint32_t cache_size;
    uint32_t bytes_left;
    int (*read_data)(int /*fd*/, void * /*buf*/, size_t /*nbytes*/); /*can be: read, read_from_net, read_from_device*/
} Cache;

typedef enum { st_unknown=-1, st_file=0, st_net, st_pipe, st_device} stream_type;

Cache *create_cache(stream_type);
// shawill: define static in cache.c
// int read_from_net(int fd, void *buf, size_t nbytes);/*not implemented yet*/
// int read_from_device(int fd, void *buf, size_t nbytes);/*not implemented yet*/

/*Interface to Cache*/
int read_c(uint32_t nbytes, uint8_t *buf, Cache **c, int fd, stream_type);
void flush_cache(Cache *c); 
void free_cache(Cache *c); 

typedef enum { // XXX: all initialized to a power of 2 number.
    IS_FLAGS_INIT = 0,
    IS_EXCLUSIVE=1 // if set, input stream can be opened only once at a time.
} istream_flags;

#define IS_ISINIT(is)    is->flags & IS_FLAGS_INIT
#define IS_ISEXCLUSIVE(is)    is->flags & IS_EXCLUSIVE

typedef struct __INPUTSTREAM {
    char name[255];
    stream_type type;
    Cache *cache;
    int fd;
    //... properties for file, net or device 
    int mmap_on;
    void *mmap_base;
    void *mmap_curr;
    size_t mmap_len;
    istream_flags flags;
} InputStream;


/*Interface to InputStream*/
InputStream *istream_open(char *mrl);
void istream_close(InputStream *);

inline int istream_read(InputStream *is, uint8_t *buf, uint32_t nbytes); 
inline int istream_reset(InputStream *is);
stream_type parse_mrl(char *mrl, char **resource_name);
time_t mrl_mtime(char *mrl);
int mrl_changed(char *mrl, time_t *last_change);

#endif // __INPUTSTREAMH
