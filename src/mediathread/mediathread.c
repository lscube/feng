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

#include "config.h"

#include <glib.h>
#include <stdbool.h>

#include "mediathread.h"
#include "demuxer.h"
#include "fnc_log.h"

#ifdef HAVE_METADATA
#include "metadata/cpd.h"
#endif

/**
 * @brief Buffer low event
 *
 * @param resource Resource to read data from
 * @param user_data Unused
 *
 * This function is used to re-fill the buffer for the resource that
 * it's given, to make sure that there is enough data to send to the
 * clients.
 *
 * @internal This function is used to initialise @ref
 *           mt_resource_read_pool.
 */
static void mt_cb_read(gpointer resource, gpointer user_data)
{
    Resource *r = (Resource*)resource;

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
 * @brief Wrapper function to call r_close
 *
 * @param resource The resource to close
 * @param user_data Unused
 *
 * @internal This function is used to initialise @ref
 *           mt_resource_close_pool.
 */
static void mt_cb_close(gpointer resource, gpointer user_data)
{
    r_close((Resource*)resource);
}

GThreadPool *mt_resource_read_pool = NULL;
GThreadPool *mt_resource_close_pool = NULL;

/**
 * @brief Initialises the thread pools.
 */
void mt_init() {
    mt_resource_read_pool = g_thread_pool_new(mt_cb_read,
                                              NULL, -1, false, NULL);
    mt_resource_close_pool = g_thread_pool_new(mt_cb_close,
                                               NULL, -1, false, NULL);
}

/**
 * @brief Frees the thread pools
 */
void mt_shutdown() {
    g_thread_pool_free(mt_resource_read_pool, true, true);
    g_thread_pool_free(mt_resource_close_pool, true, true);
}

void mt_resource_read(Resource *resource) {
    g_thread_pool_push(mt_resource_read_pool,
                       resource,
                       NULL);
}

void mt_resource_close(Resource *resource) {
    g_thread_pool_push(mt_resource_close_pool,
                       resource,
                       NULL);
}
