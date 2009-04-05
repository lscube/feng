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
 * @defgroup mediathread Mediathread framework
 *
 * The mediathread framework, now a misnomer, does not create a single
 * thread for processing requests, but uses two thread pools (@ref
 * mt_resource_read_pool and @ref mt_resource_close_pool), set to call
 * two different callback functions (@ref mt_cb_read and @ref
 * mt_cb_close).
 *
 * By calling these two functions, the read and close operations on
 * resources are scheduled to be executed asynchronously in one of the
 * pools' threads. This avoids reimplementing a message-passing system
 * to signal the thread which resource to handle, and at the same time
 * allows for multiple clients to read or close resources at the same
 * time with different threads.
 *
 * @{
 */

/**
 * @brief Callback function to read from a resource
 *
 * @param resource Resource to read data from
 * @param user_data Unused
 *
 * This function is used to read more data from the resource into the
 * buffers, to make sure that there is enough data to send to the
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

    if ( r->demuxer->read_packet(r) != RESOURCE_OK )
        fnc_log(FNC_LOG_FATAL,
                "[MT] read_packet() error.");

    g_mutex_unlock(r->lock);
}

/**
 * @brief Wrapper function to call r_close
 *
 * @param resource The resource to close
 * @param user_data Unused
 *
 * This function wraps around the r_close function to allow closing
 * asynchronously the resource.
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
 *
 * This function has to be called by the main initialisation routine,
 * after threading is enabled but before using mediathread.
 *
 * It creates the threadpool objects that will be used to handle the
 * asynchronous resource requests.
 */
void mt_init() {
    mt_resource_read_pool = g_thread_pool_new(mt_cb_read,
                                              NULL, -1, false, NULL);
    mt_resource_close_pool = g_thread_pool_new(mt_cb_close,
                                               NULL, -1, false, NULL);
}

/**
 * @brief Frees the thread pools
 *
 * This function has to be called by the main shutdown routine, after
 * all the clients exited.
 *
 * It destroys the threadpools, leaving time for the currently-running
 * task to complete to avoid aborting them directly.
 */
void mt_shutdown() {
    g_thread_pool_free(mt_resource_read_pool, true, true);
    g_thread_pool_free(mt_resource_close_pool, true, true);
}

/**
 * @brief Request an asynchronous read on a resource
 *
 * @param resource The resource to request the read for
 *
 * This function is used by the RTSP and RTP server code to request
 * more data to be read for the given resource. Since this is done
 * through a threadpool, the read happens asynchronously without
 * stopping the processing.
 */
void mt_resource_read(Resource *resource) {
    g_thread_pool_push(mt_resource_read_pool,
                       resource,
                       NULL);
}

/**
 * @brief Request an asynchronous close on a resource
 *
 * @param resource The resource to close
 *
 * This function is used by the RTSP server code to close down a given
 * resource, usually after a client disconnected. Since this is done
 * through a threadpool, the closing (and thus the memory free and all
 * the related paths) happens asynchronously without stopping the
 * processing.
 */
void mt_resource_close(Resource *resource) {
    g_thread_pool_push(mt_resource_close_pool,
                       resource,
                       NULL);
}

/**
 * @}
 */
