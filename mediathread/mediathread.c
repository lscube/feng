/* * 
*  $Id$
*  
*  This file is part of Feng
*
*  Feng -- Standard Streaming Server
*
*  Copyright (C) 2007 by
*
*	- Dario Gallucci	<dario.gallucci@polito.it>
* 
*  Feng is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  Feng is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with Feng; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*  
* */

#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include <fenice/mediathread.h>
#include <fenice/fnc_log.h>
#include <fenice/utils.h>
#include <time.h>

static GQueue *el_head;
static pthread_mutex_t el_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mt_mutex = PTHREAD_MUTEX_INITIALIZER;

void *mediathread(void *arg) {
    mt_event_item *el_cur;
    struct timespec ts = {0, 0};

    el_head = g_queue_new();

    fnc_log(FNC_LOG_DEBUG, "[MT] Mediathread started");

    while(1) {
        while (!g_queue_is_empty(el_head)) {
            pthread_mutex_lock(&el_mutex);
            el_cur = g_queue_pop_head (el_head);
            pthread_mutex_unlock(&el_mutex);

            if (el_cur) {
                pthread_mutex_lock(&mt_mutex);
                mt_process_event(el_cur);
                mt_dispose_event(el_cur);
                pthread_mutex_unlock(&mt_mutex);
            }
        }
        //to avoid 100% cpu usage with empty eventlist
        nanosleep(&ts, NULL);
    }
    return NULL;
}

int mt_add_event(mt_event_id id, void **args) {
    mt_event_item *item;

    if (!(item = g_new0(mt_event_item, 1))) {
        mt_dispose_event_args(id, args);
        fnc_log(FNC_LOG_FATAL, "[MT] Allocation failure in mt_create_event()");
        return ERR_GENERIC;
    }

    fnc_log(FNC_LOG_VERBOSE, "[MT] Created event: %#x", item);

    item->id = id;
    item->args = args;

    pthread_mutex_lock(&el_mutex);
    g_queue_push_tail(el_head, item);
    pthread_mutex_unlock(&el_mutex);

    return ERR_NOERROR;
}

inline int mt_process_event(mt_event_item *ev) {

    if (!ev)
        return ERR_GENERIC;

    fnc_log(FNC_LOG_VERBOSE, "[MT] Processing event: %#x", ev->id);

    switch (ev->id) {
    case MT_EV_BUFFER_LOW:
        {
            Track *t = ev->args[1];
            Resource *r = t->parent;

            fnc_log(FNC_LOG_VERBOSE, "[MT] Filling buffer for track %p", t);

            switch (r->demuxer->read_packet(r)) {
            case RESOURCE_OK:
                fnc_log(FNC_LOG_VERBOSE, "[MT] Done!");
                break;
            case RESOURCE_NOT_PARSEABLE:
                {
                    static uint8 buffer[MT_BUFFERSIZE];
                    long n;
                    if((n = t->parser->get_frame(buffer, MT_BUFFERSIZE,
                                                 &(t->properties->mtime),
                                                 t->i_stream, t->properties,
                                                 t->parser_private)) > 0) {
                        fnc_log(FNC_LOG_DEBUG, "[MT] Timestamp: %f!",
                                                         t->properties->mtime);
                        t->parser->parse(t, buffer, n, NULL, 0);
                                                }
                    fnc_log(FNC_LOG_VERBOSE, "[MT] Done legacy!");
                }
                break;
            case EOF:
                // Signal the end of stream
                r->eos = 1;
                break;
            default:
                fnc_log(FNC_LOG_VERBOSE,
                        "[MT] read_packet() error.");
                break;
            }
        }
        break;

    default:
        break;
    }
    return ERR_NOERROR;
}

void mt_disable_event(mt_event_item *ev, void *sender) {
    if (!ev)
        return;

    fnc_log(FNC_LOG_VERBOSE, "[MT] Disabling event: %#x", ev);

    if (ev->args && ev->args[0] == sender) {
        mt_dispose_event_args(ev->id, ev->args);
        ev->id = MT_EV_NOP;
        ev->args = NULL;
    }
}

inline void mt_dispose_event(mt_event_item *ev) {
    if (!ev)
        return;
    if (ev->args)
        mt_dispose_event_args(ev->id, ev->args);
    g_free(ev);
}

inline void mt_dispose_event_args(mt_event_id id, void ** args) {
    switch (id) {
    default:
        break;
    }
    g_free(args);
}

Resource *mt_resource_open(char * path, char *filename) {
    // TODO: add to a list in order to close resources on shutdown!
    return r_open(path, filename);
}

void mt_resource_close(Resource *resource) {
    struct timespec ts = {0, 0};
    if (!resource)
        return;

    nanosleep(&ts, NULL);

    pthread_mutex_lock(&mt_mutex);
    r_close(resource);
    pthread_mutex_unlock(&mt_mutex);
}

int mt_resource_seek(Resource *resource, double time) {
    pthread_mutex_lock(&mt_mutex);
    resource->demuxer->seek(resource, time);
    pthread_mutex_unlock(&mt_mutex);
    return 0;
}

int event_buffer_low(void *sender, Track *src) {
    void **args;
    if (src->parent->eos) return ERR_EOF;
    args = g_new(void *, 2);
    args[0] = sender;
    args[1] = src;
    return mt_add_event(MT_EV_BUFFER_LOW, args);
}

int mt_disable_events(void *sender) {
    pthread_mutex_lock(&el_mutex);
    g_queue_foreach(el_head, mt_disable_event, sender);
    pthread_mutex_unlock(&el_mutex);
    return ERR_NOERROR;
}
