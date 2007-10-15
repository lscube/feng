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

#ifndef __MEDIA_THREAD
#define __MEDIA_THREAD

#define ENABLE_MEDIATHREAD 1

#include <pthread.h>

#include <fenice/demuxer.h>
#include <fenice/InputStream.h>

#define MT_BUFFERSIZE 8192



/*
typedef struct __MT_RESOURCE_ITEM {
    Resource *resource;
    pthread_mutex_t *mutex;
    struct __MT_RESOURCE_ITEM *next;
} mt_resource_item;
*/

typedef enum __MT_EVENT_ID {
    MT_EV_NOP,          //!< Fake event do nothing. ARGS= NULL;
    MT_EV_BUFFER_LOW,   //!< Buffer needs to be filled. ARGS= Track*
    MT_EV_DATA_EOF,     //!< Track has no more data. ARGS= Track*
    MT_EV_DATA_BOUND,   //!< Track reached request bound. ARGS= Track*
    MT_EV_SHUTDOWN,     //!< The server is going to close. ARGS= NULL
    MT_EV_CLOSE         //!< The resource must be closed
} mt_event_id;

typedef struct __MT_EVENT_ITEM {
    mt_event_id id;
    void **args;        //! args[0] = sender
} mt_event_item;

typedef struct __MT_EXCL_INS {
    InputStream *i_stream;
    struct __MT_EXCL_INS *next;
} mt_excl_ins;

void *mediathread(void *arg);

int mt_add_event(mt_event_id, void **args);
inline int mt_process_event(mt_event_item *);
inline void mt_dispose_event(mt_event_item *);
inline void mt_dispose_event_args(mt_event_id, void **args);
int mt_shutdown();
int event_buffer_low(void *sender, Track *src);

Resource *mt_resource_open(char *, char *);
void mt_resource_close(Resource *);
int mt_resource_seek(Resource *, double);

#endif // __MEDIA_THREAD

