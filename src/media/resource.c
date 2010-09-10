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

/**
 * Maximum number of threads to use for resource close/cleanup
 *
 * @todo This should be configurable at runtime via the config file.
 */
#define MAX_RESOURCE_CLOSE_THREADS 8

#include <glib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "demuxer.h"
#include "feng.h"
#include "fnc_log.h"

// global demuxer modules:
#ifdef LIVE_STREAMING
extern Demuxer fnc_demuxer_sd;
#endif
extern Demuxer fnc_demuxer_ds;
extern Demuxer fnc_demuxer_avf;

/**
 * @defgroup resources Media backend resources handling
 *
 * @{
 */

#ifdef LIVE_STREAMING
/**
 * @brief Global lock
 *
 * It should be held while updating shared tracks hash table
 */
static GMutex *shared_resources_lock;

/**
 * @brief Shared Tracks HashTable
 *
 * It holds the reference to tracks that will be shared among different
 * resources
 *
 */
static GHashTable *shared_resources;
#endif


/**
 * @brief Stop the fill thread of a resource
 *
 * @param resource The resource to stop the fill thread of
 *
 * This function takes care of stopping the fill thread for a
 * resource, either for pausing it or before freeing it. It's
 * accomplished by first setting @ref Resource::fill_pool attribute to
 * NULL, then it stops the pool, dropping further threads and waiting
 * for the last one to complete.
 */
static void r_stop_fill(Resource *resource)
{
    GThreadPool *pool;

    if ( (pool = resource->fill_pool) ) {
        g_atomic_pointer_set(&resource->fill_pool, NULL);
        g_thread_pool_free(pool, true, true);
    }
}

/**
 * @brief Free a resource object
 *
 * @param resource The resource to free
 *
 * @internal Please note that this function will execute the freeing
 *           in the currently running thread, synchronously. It should
 *           thus only be called by the functions that wouldn't block
 *           the mainloop.
 *
 * @note Nobody may hold the @ref Resource::lock mutex when calling
 *       this function.
 */
static void r_free(Resource *resource)
{
    if (!resource)
        return;

    r_stop_fill(resource);

    if (resource->lock)
        g_mutex_free(resource->lock);

    g_free(resource->info->mrl);
    g_free(resource->info->name);
    g_slice_free(ResourceInfo, resource->info);

    resource->info = NULL;
    resource->demuxer->uninit(resource);

    if (resource->tracks) {
        g_list_foreach(resource->tracks, free_track, NULL);
        g_list_free(resource->tracks);
    }
    g_slice_free(Resource, resource);
}

/**
 * @brief Find the correct demuxer for the given resource.
 *
 * @param filename Name of the file (to find the extension)
 *
 * @return A constant pointer to the working demuxer.
 *
 * This function first tries to match the resource extension with one
 * of those served by the demuxers, that will be probed; if this fails
 * it tries every demuxer available with direct probe.
 *
 * */

static const Demuxer *r_find_demuxer(const char *filename)
{
    static const Demuxer *const demuxers[] = {
#ifdef LIVE_STREAMING
        &fnc_demuxer_sd,
#endif
        &fnc_demuxer_ds,
        &fnc_demuxer_avf,
        NULL
    };

    int i;
    const char *res_ext;

    /* First of all try that with matching extension: we use extension as a
     * hint of resource type.
     */
    if ( (res_ext = strrchr(filename, '.')) && *(res_ext++) ) {
        for (i=0; demuxers[i]; i++) {
            size_t j;

            for (j = 0; j < demuxers[i]->extensions_count; j++) {
                const char *dmx_ext = demuxers[i]->extensions[j];

                if (g_ascii_strcasecmp(dmx_ext, res_ext) == 0)
                    continue;

                fnc_log(FNC_LOG_DEBUG, "[MT] probing demuxer: \"%s\" "
                        "matches \"%s\" demuxer\n", res_ext,
                        demuxers[i]->name);

                if (demuxers[i]->probe(filename) == RESOURCE_OK)
                    return demuxers[i];
            }
        }
    }

    for (i=0; demuxers[i]; i++)
        if ((demuxers[i]->probe(filename) == RESOURCE_OK))
            return demuxers[i];

    return NULL;
}

/**
 * @brief Open a new resource descriptor function
 *
 * @param mrl The filesystem path of the resource.
 * @param dmx The demuxer to use to open the resource.
 */
static Resource *r_open_direct(gchar *mrl, const Demuxer *dmx)
{
    Resource *r;
    struct stat filestat;

    if (stat(mrl, &filestat) < 0 ) {
        fnc_perror("stat");
        return NULL;
    }

    if ( S_ISFIFO(filestat.st_mode) ) {
        fnc_log(FNC_LOG_ERR, "%s: not a file");
        return NULL;
    }

    r = g_slice_new0(Resource);

    r->info = g_slice_new0(ResourceInfo);

    r->info->mrl = mrl;
    r->info->mtime = filestat.st_mtime;
    r->info->name = g_path_get_basename(mrl);
    r->info->seekable = (dmx->seek != NULL);

    r->demuxer = dmx;

    if (r->demuxer->init(r)) {
        r_free(r);
        return NULL;
    }
    /* Now that we have opened the actual resource we can proceed with
     * the extras */

    r->lock = g_mutex_new();

    return r;
}

#ifdef LIVE_STREAMING
/**
 * @brief Open a shared resource from the shared resources hash
 *
 * @param mrl The filesystem path of the resource.
 * @param dmx The demuxer to use to open the resource.
 *
 * This function uses a shared hash table to access a resource
 * descriptor shared among multiple clients, and is used for live
 * streaming.
 *
 * During live streaming all the clients will receive the same
 * content, fed by the same thread. Using a shared resource reduces
 * the amount of memory (and threads) required for the clients.
 *
 * If no resource for the given mrl is found, a new one will be opened
 * (via @ref r_open_direct).
 *
 * If the resoruce doesn't have a running "fill thread", it'll be
 * created and started.
 *
 * @note This function will lock the @ref shared_resources_lock mutex.
 * @note This function may lock the @ref Resource::lock mutex.
 */
static Resource *r_open_hashed(gchar *mrl, const Demuxer *dmx)
{
    Resource *r;
    static gsize created_mutex = false;
    if ( g_once_init_enter(&created_mutex) ) {
        shared_resources_lock = g_mutex_new();
        shared_resources = g_hash_table_new(g_str_hash, g_str_equal);
        g_once_init_leave(&created_mutex, true);
    }

    g_mutex_lock(shared_resources_lock);
    r = g_hash_table_lookup(shared_resources, mrl);

    /* only free mrl if it was found already in the hash table,
     * otherwise we stored it for later usage!
     */
    if (!r) {
        if ( (r = r_open_direct(mrl, dmx)) != NULL )
            g_hash_table_insert(shared_resources, mrl, r);
        else
            g_free(mrl);
    } else
        g_free(mrl);

    if ( r ) {
        g_atomic_int_inc(&r->count);

        /* the resource doesn't have a fill pool, we'll create one by
           resuming, then we alsopush a single thread to read */
        if ( r->fill_pool == NULL ) {
            g_mutex_lock(r->lock);

            /* check again to make sure that nobody created this while
               we were waiting for the lock (we should only ever lock
               between two r_open_hashed calls). */
            if ( r->fill_pool == NULL ) {
                r_resume(r);
                g_thread_pool_push(r->fill_pool,
                                   GINT_TO_POINTER(-1), NULL);
            }

            g_mutex_unlock(r->lock);
        }
    }

    g_mutex_unlock(shared_resources_lock);

    return r;
}
#endif

/**
 * @brief Open a new resource and create a new instance
 *
 * @param inner_path The path, relative to the avroot, for the
 *                   resource
 *
 * @return A new Resource object
 * @retval NULL Error while opening resource
 */
Resource *r_open(const char *inner_path)
{
    Resource *r = NULL;
    const Demuxer *dmx;
    gchar *mrl = g_strjoin ("/",
                            feng_srv.config_storage[0].document_root,
                            inner_path,
                            NULL);

    if ( (dmx = r_find_demuxer(mrl)) == NULL ) {
        fnc_log(FNC_LOG_DEBUG,
                "[MT] Could not find a valid demuxer for resource %s\n",
                mrl);
        g_free(mrl);
        return NULL;
    }

    /* From here on, we don't care any more of the doom of the mrl
     * variable, the called functions will save it for use later, or
     * will free it as needed.
     */

    fnc_log(FNC_LOG_DEBUG, "[MT] registrered demuxer \"%s\" for resource"
                               "\"%s\"\n", dmx->name, mrl);

    switch(dmx->source) {
#ifdef LIVE_STREAMING
        case LIVE_SOURCE:
            r = r_open_hashed(mrl, dmx);
            break;
#endif
        case STORED_SOURCE:
            r = r_open_direct(mrl, dmx);
            break;
        default:
            g_assert_not_reached();
            break;
    }

    return r;
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
    return strcmp( ((Track *)a)->info->name, (const char *)b);
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
                track_name, resource->info->mrl);
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

    if ( t->producer )
        bq_producer_reset_queue(t->producer);
}

/**
 * @brief Seek a resource to a given time in stream
 *
 * @param resource The Resource to seek
 * @param time The time in seconds within the stream to seek to
 *
 * @return The value returned by @ref Demuxer::seek
 *
 * @note This function will lock the @ref Resource::lock mutex.
 */
int r_seek(Resource *resource, double time) {
    int res;

    g_mutex_lock(resource->lock);

    res = resource->demuxer->seek(resource, time);

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
 * @ref Demuxer::read_packet); it will executed repeatedly until
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
    BufferQueue_Consumer *consumer = (BufferQueue_Consumer*)consumer_p;
    const gboolean live = resource->demuxer->source == LIVE_SOURCE;
    const gulong buffered_frames = feng_srv.srvconf.buffered_frames;

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
        switch( resource->demuxer->read_packet(resource) ) {
        case RESOURCE_OK:
        case RESOURCE_AGAIN:
            break;
        case RESOURCE_EOF:
            fnc_log(FNC_LOG_INFO,
                    "r_read_unlocked: %s read_packet() end of file.",
                    resource->info->mrl);
            resource->eor = true;
            break;
        default:
            fnc_log(FNC_LOG_FATAL,
                    "r_read_unlocked: %s read_packet() error.",
                    resource->info->mrl);
            resource->eor = true;
            break;
        }
        g_mutex_unlock(resource->lock);
    } while ( g_atomic_int_get(&resource->eor) == 0 );
}

/**
 * @brief Request closing of a resource
 *
 * @param resource The resource to close
 *
 * For live streaming, closing the resource will not actually free
 * resources; instead the fill thread will be stopped and the producer
 * queue reset (with its buffers freed) waiting for the next client.
 *
 * @see r_free
 */
void r_close(Resource *resource)
{
    if ( resource == NULL )
        return;

#ifdef LIVE_STREAMING
    if (resource->demuxer->source == LIVE_SOURCE) {
        g_mutex_lock(shared_resources_lock);

        if ( g_atomic_int_dec_and_test(&resource->count) ) {
            r_stop_fill(resource);
            g_list_foreach(resource->tracks, r_track_producer_reset_queue, NULL);
        }

        g_mutex_unlock(shared_resources_lock);

        // if ( g_atomic_int_get(&resource->count) > 0 )
            return;
    } else
#endif
        r_free(resource);
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
    if ( resource->demuxer->source == LIVE_SOURCE )
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

    /* running already */
    if ( g_atomic_pointer_get(&resource->fill_pool) != NULL )
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
void r_fill(Resource *resource, BufferQueue_Consumer *consumer)
{
    /* Don't even try to fill a live source! */
    if ( resource->demuxer->source == LIVE_SOURCE )
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

