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
static int stopped = 0;

typedef void (*mt_callback)(Resource *);

typedef struct {
    mt_callback cb;
    Resource *resource;
} mt_event_item;

/**
 * @brief Buffer low event
 *
 * @param r Resource to read data from
 *
 * This function is used to re-fill the buffer for the resource that
 * it's given, to make sure that there is enough data to send to the
 * clients.
 */
static void mt_cb_buffer_low(Resource *r)
{
    fnc_log(FNC_LOG_VERBOSE, "[MT] Filling buffer for resource %p", r);

    g_mutex_lock(r->lock);

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

    g_mutex_unlock(r->lock);
}

/**
 * @brief Shutdown event
 *
 * @param unused Never used, unimportant garbage
 *
 * Stop the mediathread
 */
static void mt_cb_shutdown(Resource *unused)
{
    stopped =1;
}

static void mt_add_event(mt_callback cb, Resource *r) {
    mt_event_item *item = g_new0(mt_event_item, 1);

    fnc_log(FNC_LOG_VERBOSE, "[MT] Created event: %#x", item);

    item->cb = cb;
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
static gpointer mediathread(gpointer arg) {
    fnc_log(FNC_LOG_DEBUG, "[MT] Mediathread started");

    g_async_queue_ref(el_head);

    while(!stopped) {
        //this replaces the previous nanosleep loop,
        //as this will block until data is available
        mt_event_item *evt = g_async_queue_pop (el_head);
        if (evt) {
            evt->cb(evt->resource);
            g_free(evt);
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
    mt_add_event(r_close, resource);
}

int mt_resource_seek(Resource *resource, double time) {
    return r_seek(resource, time);
}

void event_buffer_low(Resource *r) {
    mt_add_event(mt_cb_buffer_low, r);
}

void mt_shutdown() {
    mt_add_event(mt_cb_shutdown, NULL);
}
