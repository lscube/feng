/* * 
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 * 
 * feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * feng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with feng; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA 
 *
 * */

#ifndef FN_MEDIA_THREAD_H
#define FN_MEDIA_THREAD_H

#define ENABLE_MEDIATHREAD 1

#include <glib.h>

#include "demuxer.h"
#include <fenice/server.h>

#define MT_BUFFERSIZE 8192

typedef enum {
    MT_EV_NOP,          //!< Fake event do nothing. ARGS= NULL;
    MT_EV_BUFFER_LOW,   //!< Buffer needs to be filled. ARGS= Track*
    MT_EV_DATA_EOF,     //!< Track has no more data. ARGS= Track*
    MT_EV_DATA_BOUND,   //!< Track reached request bound. ARGS= Track*
    MT_EV_SHUTDOWN,     //!< The server is going to close. ARGS= NULL
    MT_EV_CLOSE         //!< The resource must be closed
} mt_event_id;

typedef struct {
    mt_event_id id;
    void **args;        //! args[0] = sender
} mt_event_item;

typedef struct mt_excl_ins {
    InputStream *i_stream;
    struct mt_excl_ins *next;
} mt_excl_ins;

gpointer *mediathread(gpointer *arg);

int mt_shutdown();
int event_buffer_low(void *sender, Track *src);

struct feng;

Resource *mt_resource_open(struct feng *srv, char * path, char *filename);
void mt_resource_close(Resource *);
int mt_resource_seek(Resource *, double);

#endif // FN_MEDIA_THREAD_H

