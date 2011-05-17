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

#include <glib.h>
#include <stdbool.h>
#include <string.h>

#include "media/media.h"
#include "feng.h"
#include "fnc_log.h"

/**
 * @defgroup resources Media backend resources handling
 *
 * @{
 */

#ifdef LIVE_STREAMING
extern Resource *sd2_open(const char *url);
#else
static Resource *sd2_open(const char *url);
{
    fnc_log(FNC_LOG_ERR,
            "unable to stream resource '%s', live streaming support not built in",
            url);

    return false;
}
#endif

#ifdef HAVE_AVFORMAT
extern Resource *avf_open(const char *url);
#else
static Resource *avf_open(const char *url);
{
    fnc_log(FNC_LOG_ERR,
            "unable to stream resource '%s', libavformat support not built in",
            url);

    return false;
}
#endif

/**
 * @brief Mutex regulating access to virtual resources
 *
 * This mutex should be held when looking up, adding to or removing
 * from @ref virtual_resources, to avoid race conditions when multiple
 * clients request virtual resources.
 *
 * @see r_virtual_lock, r_virtual_unlock
 */
static GStaticMutex virtual_resources_lock = G_STATIC_MUTEX_INIT;

/**
 * @brief Virtual resources table
 *
 * Connect a virtual resource URL with an already open Resource
 * instance.
 *
 * @note To access this table, you need to hold @ref
 *       virtual_resources_lock.
 *
 */
static GHashTable *virtual_resources;

/**
 * @brief Lock the mutex used for virtual resources
 */
static inline void r_virtual_lock()
{
    g_static_mutex_lock(&virtual_resources_lock);
}

static inline void r_virtual_unlock()
{
    g_static_mutex_unlock(&virtual_resources_lock);
}

/**
 * @brief Retrieve or create the resource for a given virtual URL
 *
 * @param url The resolved URL of the resource within the virtual/ path.
 *
 * @return Pointer to the Resource designed by @p url or NULL in case
 *         of error.
 *
 * @TODO The interface should probably be changed to return a proper
 *       error code when the resource is not found, not accessible or
 *       not readable.
 *
 * @see r_open
 */
static Resource *r_open_virtual(const char *url)
{
    Resource *r;

    r_virtual_lock();

    if ( ! virtual_resources )
        virtual_resources = g_hash_table_new(g_str_hash, g_str_equal);

    if ( (r = g_hash_table_lookup(virtual_resources, url)) != NULL )
        g_atomic_int_inc(&r->count);
    else
        r = sd2_open(url);

    r_virtual_unlock();
    return r;
}

/**
 * @brief Retrieve or create the resource for a given URL
 *
 * @param url The resolved URL of the resource within the vhost.
 *
 * @return Pointer to the Resource designed by @p url, or NULL in case
 *         of error.
 *
 * @note The @p url parameter has to be the resolved URL, which means
 *       that no parent-directory references (../) are present in
 *       it. This is important, as no further boundary checking is
 *       applied from here on.
 *
 * @TODO The interface should probably be changed to return a proper
 *       error code when the resource is not found, not accessible or
 *       not readable.
 *
 * @see r_open_virtual
 */
Resource *r_open(const char *url)
{
    if ( g_str_has_prefix(url, "/virtual/") )
        return r_open_virtual(url + strlen("/virtual/"));
    else
        return avf_open(url);
}

/**
 * @brief Comparison function to compare a Track to a name
 *
 * @param a Track pointer to the element in the list
 * @param b String with the name to compare to
 *
 * @return The strcmp() result between the correspondent Track name
 *         and the given name.
 *
 * @internal This function should _only_ be used by @ref r_find_track.
 */
static gint r_find_track_cmp_name(gconstpointer a, gconstpointer b)
{
    return strcmp( ((Track *)a)->name, (const char *)b);
}

/**
 * @brief Find the given track name in the resource
 *
 * @param resource The Resource object to look into
 * @param track_name The name of the track to look for
 *
 * @return A pointer to the Track object for the requested track.
 *
 * @retval NULL No track in the resource corresponded to the given
 *              parameters.
 *
 * @todo This only returns the first resource corresponding to the
 *       parameters.
 * @todo Capabilities aren't used yet.
 */
Track *r_find_track(Resource *resource, const char *track_name) {
    GList *track = g_list_find_custom(resource->tracks,
                                      track_name,
                                      r_find_track_cmp_name);

    if ( !track ) {
        fnc_log(FNC_LOG_DEBUG, "Track %s not present in resource %s",
                track_name, resource->mrl);
        return NULL;
    }

    return track->data;
}

/**
 * @brief Resets the BufferQueue producer queue for a given track
 *
 * @param element The Track element from the list
 * @param user_data Unused, for compatibility with g_list_foreach().
 *
 * @see bq_producer_reset_queue
 */
static void r_track_producer_reset_queue(gpointer element,
                                         ATTR_UNUSED gpointer user_data) {
    Track *t = (Track*)element;

    bq_producer_reset_queue(t);
}

/**
 * @brief Seek a resource to a given time in stream
 *
 * @param resource The Resource to seek
 * @param time The time in seconds within the stream to seek to
 *
 * @return The value returned by @ref Resource::seek
 *
 * @note This function will lock the @ref Resource::lock mutex.
 */
int r_seek(Resource *resource, double time) {
    int res;

    g_mutex_lock(resource->lock);

    res = resource->seek(resource, time);

    g_list_foreach(resource->tracks, r_track_producer_reset_queue, NULL);

    g_mutex_unlock(resource->lock);

    return res;
}

/**
 * @brief Callback for the queue filling for the resource
 *
 * @param consumer_p A generic pointer to the consumer for non-live queues
 * @param resource_p A generic pointer to the resource to fill the queue of
 *
 * This function takes care of reading the data from the demuxer (via
 * @ref Resource::read_packet); it will executed repeatedly until
 * either the resources ends (@ref Resource::eor becomes non-zero),
 * the thread is requested to stop (@ref Resource::fill_pool becomes
 * NULL), or, for non-live resources only, when the @ref consumer_p
 * queue is long enough for the client to receive data.
 *
 * @note For live streams, the @ref consume_p pointer should have the
 *       value -1 to indicate that it shouldn't be used.
 *
 * @note This function will lock the @ref Resource::lock mutex
 *       (repeatedly).
 */
static void r_read_cb(gpointer consumer_p, gpointer resource_p)
{
    Resource *resource = (Resource*)resource_p;
    struct RTP_session *consumer = (struct RTP_session*)consumer_p;
    const gboolean live = resource->source == LIVE_SOURCE;
    const gulong buffered_frames = feng_srv.buffered_frames;

    g_assert( (live && consumer == GINT_TO_POINTER(-1)) || (!live && consumer != GINT_TO_POINTER(-1)) );

    do {
        /* setting this to NULL with an atomic, non-locking operation
           is our "stop" signal. */
        if ( g_atomic_pointer_get(&resource->fill_pool) == NULL )
            return;

        /* Only check for enough buffered frames if we're not doing
           live; otherwise keep on filling; we also assume that
           consumer will be NULL in that case. */
        if ( !live && bq_consumer_unseen(consumer) >= buffered_frames )
            return;

        //        fprintf(stderr, "r_read_cb(%p)\n", resource);

        g_mutex_lock(resource->lock);
        switch( resource->read_packet(resource) ) {
        case RESOURCE_OK:
            break;
        case RESOURCE_EOF:
            fnc_log(FNC_LOG_INFO,
                    "r_read_unlocked: %s read_packet() end of file.",
                    resource->mrl);
            resource->eor = true;
            break;
        default:
            fnc_log(FNC_LOG_FATAL,
                    "r_read_unlocked: %s read_packet() error.",
                    resource->mrl);
            resource->eor = true;
            break;
        }
        g_mutex_unlock(resource->lock);
    } while ( g_atomic_int_get(&resource->eor) == 0 );
}

static void free_track(gpointer element,
                       ATTR_UNUSED gpointer user_data)
{
    Track *track = (Track*)element;
    track_free(track);
}

/**
 * @brief Request closing of a resource
 *
 * @param resource The resource to close
 *
 * For virtual resources, closing the resource will not actually free
 * anything; only the count value will be decremented.
 *
 * This function stops the fill thread for a resource, before freeing
 * it. It's accomplished by first setting @ref Resource::fill_pool
 * attribute to NULL, then it stops the pool, dropping further threads
 * and waiting for the last one to complete.
 */
void r_close(Resource *resource)
{
    GThreadPool *pool;

    if ( resource == NULL )
        return;

    if (resource->source == LIVE_SOURCE) {
        (void)g_atomic_int_dec_and_test(&resource->count);
        return;
    }

    if ( (pool = resource->fill_pool) ) {
        g_atomic_pointer_set(&resource->fill_pool, NULL);
        g_thread_pool_free(pool, true, true);
    }

    if (resource->lock)
        g_mutex_free(resource->lock);

    g_free(resource->mrl);

    if ( resource->uninit != NULL )
        resource->uninit(resource);

    if (resource->tracks) {
        g_list_foreach(resource->tracks, free_track, NULL);
        g_list_free(resource->tracks);
    }

    g_slice_free(Resource, resource);
}

/**
 * @brief Pause a running non-live resource
 *
 * @param resource The resource to pause
 *
 * This function stops the "fill thread" for the resource, when it is
 * not shared among clients (i.e.: it's not a live resource).
 *
 * It removes @ref Resource::fill_pool and sets it to NULL to stop the
 * thread running.
 *
 * @note This function will lock the @ref Resource::lock mutex.
 */
void r_pause(Resource *resource)
{
    GThreadPool *pool;

    /* Don't even try to pause a live source! */
    if ( resource->source == LIVE_SOURCE )
        return;

    /* we paused already */
    if ( resource->fill_pool == NULL )
        return;

    g_mutex_lock(resource->lock);

    pool = resource->fill_pool;
    g_atomic_pointer_set(&resource->fill_pool, NULL);
    g_thread_pool_free(pool, true, true);

    g_mutex_unlock(resource->lock);
}

/**
 * @brief Resume a paused (or non-standard) non-live resource
 *
 * @param resource The resource to resume
 *
 * This functions creates a new instance for @ref Resource::fill_pool,
 * and sets it into the resource so that it can be used to fill the
 * queue.
 */
void r_resume(Resource *resource)
{
    GThreadPool *pool;

    /* running already, or auto-filled */
    if ( g_atomic_pointer_get(&resource->fill_pool) != NULL ||
         g_atomic_pointer_get(&resource->read_packet) == NULL )
        return;

    pool = g_thread_pool_new(r_read_cb, resource,
                             1, true, NULL);

    g_atomic_pointer_set(&resource->fill_pool, pool);
}

/**
 * @brief Fill the queue for a non-live resource
 *
 * @param resource The resource to fill the queue for
 * @param consumer The consumer of the queue to fill
 *
 * This function will create a new thread (or push a new one to be
 * executed afterwars) so that the consumer gets enough frames to send
 * the client.
 *
 * @note This function is no-op for live streams as they take care of
 *       the filling themselves.
 *
 * @note This function will lock the @ref Resource::lock mutex.
 */
void r_fill(Resource *resource, struct RTP_session *consumer)
{
    /* Don't even try to fill a live source! */
    if ( resource->source == LIVE_SOURCE )
        return;

    g_mutex_lock(resource->lock);

    if ( resource->fill_pool == NULL )
        goto end;

    g_thread_pool_push(resource->fill_pool,
                       consumer, NULL);

 end:
    g_mutex_unlock(resource->lock);
}

/**
 * @}
 */

