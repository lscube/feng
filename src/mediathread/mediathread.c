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

#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include "mediathread.h"
#include <fenice/fnc_log.h>
#include <fenice/utils.h>
#include <time.h>

#ifdef HAVE_METADATA
#include <metadata/cpd.h>
#endif

static GAsyncQueue *el_head;
static GStaticMutex mt_mutex = G_STATIC_MUTEX_INIT;
static int stopped = 0;

typedef enum {
    MT_EV_BUFFER_LOW,   //!< Buffer needs to be filled. ARGS= Track*
    MT_EV_SHUTDOWN,     //!< The server is going to close. ARGS= NULL
    MT_EV_CLOSE         //!< The resource must be closed
} mt_event_id;

typedef struct {
    mt_event_id id;
    Resource *resource;
} mt_event_item;

static inline int mt_process_event(mt_event_item *ev) {
    Resource *r = ev->resource;

    if (!ev)
        return ERR_GENERIC;

    fnc_log(FNC_LOG_VERBOSE, "[MT] Processing event: %#x", ev->id);

    switch (ev->id) {
    case MT_EV_BUFFER_LOW:
            fnc_log(FNC_LOG_VERBOSE, "[MT] Filling buffer for resource %p", r);

            switch (r->demuxer->read_packet(r)) {
            case RESOURCE_OK:
                fnc_log(FNC_LOG_VERBOSE, "[MT] Done!");
                break;
            case RESOURCE_EOF:
                // Signal the end of stream
                r->eos = 1;
                break;
            default:
                fnc_log(FNC_LOG_FATAL,
                        "[MT] read_packet() error.");
                break;
            }
        break;
    case MT_EV_SHUTDOWN:
        stopped = 1;
        break;
    case MT_EV_CLOSE:
        r_close(r);
        break;
    }

    g_free(ev);
    return ERR_NOERROR;
}

static void mt_add_event(mt_event_id id, Resource *r) {
    mt_event_item *item = g_new0(mt_event_item, 1);

    fnc_log(FNC_LOG_VERBOSE, "[MT] Created event: %#x", item);

    item->id = id;
    item->resource = r;

    /* This is already referenced for this thread; mt_add_event() is
     * called by the main eventloop, which is where the queue was
     * created in the first place. */
    g_async_queue_push(el_head, item);
}

/**
 * @brief MediaThread runner function
 *
 * This is the function that does most of the work for MediaThread, as
 * it waits for events and takes care of processing them properly.
 *
 * @see mt_init
 */
static gpointer *mediathread(gpointer *arg) {
    fnc_log(FNC_LOG_DEBUG, "[MT] Mediathread started");

    g_async_queue_ref(el_head);

    while(!stopped) {
        //this replaces the previous nanosleep loop,
        //as this will block until data is available
        mt_event_item *el_cur = g_async_queue_pop (el_head);
        if (el_cur) {
            g_static_mutex_lock(&mt_mutex);
            mt_process_event(el_cur);
            g_static_mutex_unlock(&mt_mutex);
        }
    }

    return NULL;
}

/**
 * @brief Initialisation for the mediathread thread handling
 *
 * This function takes care of initialising MediaThread in its
 * entirety. It creates the @ref el_head queue and also creates the
 * actual mediathread.
 *
 * @note This function has to be called before any other mt_*
 *       function, but after threads have been initialised.
 *
 * @see mediathread
 */
void mt_init() {
    g_assert(g_thread_supported());

    el_head = g_async_queue_new();

    g_thread_create(mediathread, NULL, FALSE, NULL);
}

Resource *mt_resource_open(feng *srv, const char *filename) {
    // TODO: add to a list in order to close resources on shutdown!

    Resource *res;

    // open AV resource
    res = r_open(srv, filename);

#ifdef HAVE_METADATA
    cpd_find_request(srv, res, filename);
#endif

    return res;
}

void mt_resource_close(Resource *resource) {
    if (!resource)
        return;
    mt_add_event(MT_EV_CLOSE, resource);
}

int mt_resource_seek(Resource *resource, double time) {
    int res;
    g_static_mutex_lock(&mt_mutex);
    res = r_seek(resource, time);
    g_static_mutex_unlock(&mt_mutex);

    return res;
}

void event_buffer_low(Resource *r) {
    mt_add_event(MT_EV_BUFFER_LOW, r);
}

void mt_shutdown() {
    mt_add_event(MT_EV_SHUTDOWN, NULL);
}
