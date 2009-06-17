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
 * @defgroup resources
 * @brief Resource handling functions
 *
 * @{
 */

/**
 * @brief Free a resource object
 *
 * @param resource The resource to free
 * @param user_data Unused
 *
 * @internal Please note that this function will execute the freeing
 *           in the currently running thread, synchronously. It should
 *           thus only be called by the functions that wouldn't block
 *           the mainloop.
 *
 * @note Nobody may hold the @ref Resource::lock mutex when calling
 *       this function.
 *
 * @note This function uses two parameters because it's used as
 *       interface for the closing thread pool.
 */
static void r_free_cb(gpointer resource_p,
                      ATTR_UNUSED gpointer user_data)
{
    Resource *resource = (Resource *)resource_p;
    if (!resource)
        return;

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
            char exts[128], *tkn; /* temp string containing extension
                                   * served by probing demuxer.
                                   */
            strncpy(exts, demuxers[i]->info->extensions, sizeof(exts)-1);

            for (tkn=strtok(exts, ","); tkn; tkn=strtok(NULL, ",")) {
                if (strcmp(tkn, res_ext) == 0)
                    continue;

                fnc_log(FNC_LOG_DEBUG, "[MT] probing demuxer: \"%s\" "
                        "matches \"%s\" demuxer\n", res_ext,
                        demuxers[i]->info->name);

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
 * @brief Open a new resource and create a new instance
 *
 * @param srv The server object for the vhost requesting the resource
 * @param inner_path The path, relative to the avroot, for the
 *                   resource
 *
 * @return A new Resource object
 * @retval NULL Error while opening resource
 */
Resource *r_open(struct feng *srv, const char *inner_path)
{
    Resource *r;
    const Demuxer *dmx;
    gchar *mrl = g_strjoin ("/",
                            srv->config_storage[0].document_root->ptr,
                            inner_path,
                            NULL);
    struct stat filestat;

    if (stat(mrl, &filestat) == -1 ) {
        switch(errno) {
        case ENOENT:
            fnc_log(FNC_LOG_ERR,"%s: file not found\n", mrl);
            break;
        default:
            fnc_log(FNC_LOG_ERR,"Cannot stat file %s\n", mrl);
            break;
        }
        goto error;
    }

    if ( S_ISFIFO(filestat.st_mode) ) {
        fnc_log(FNC_LOG_ERR, "%s: not a file\n");
        goto error;
    }

    if ( (dmx = r_find_demuxer(mrl)) == NULL ) {
        fnc_log(FNC_LOG_DEBUG,
                "[MT] Could not find a valid demuxer for resource %s\n",
                mrl);
        goto error;
    }

    fnc_log(FNC_LOG_DEBUG, "[MT] registrered demuxer \"%s\" for resource"
                               "\"%s\"\n", dmx->info->name, mrl);

    r = g_slice_new0(Resource);

    r->info = g_slice_new0(ResourceInfo);

    r->info->mrl = mrl;
    r->info->mtime = filestat.st_mtime;
    r->info->name = g_path_get_basename(inner_path);
    r->info->seekable = (dmx->seek != NULL);

    r->demuxer = dmx;
    r->srv = srv;

    if (r->demuxer->init(r)) {
        r_free_cb(r, NULL);
        return NULL;
    }

    /* Now that we have opened the actual resource we can proceed with
     * the extras */

    r->lock = g_mutex_new();

#ifdef HAVE_METADATA
    cpd_find_request(srv, r, filename);
#endif

    return r;
 error:
    g_free(mrl);
    return NULL;
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
        fnc_log(FNC_LOG_DEBUG, "Track %s not present in resource %s\n",
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
 * @brief Read data from a resource (unlocked)
 *
 * @param resouce The resource to read from
 *
 * @return The same return value as read_packet
 */
int r_read(Resource *resource)
{
    int ret = RESOURCE_EOF;

    g_mutex_lock(resource->lock);
    if (!resource->eor)
        switch( (ret = resource->demuxer->read_packet(resource)) ) {
        case RESOURCE_OK:
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
            break;
        }

    g_mutex_unlock(resource->lock);

    return ret;
}

/**
 * @brief Request closing of a resource
 *
 * @param resource The resource to close
 *
 * This function only pushes a close request on the resource closing
 * pool; the actual closing will happen asynchronously.
 *
 * @see r_free_cb
 */
void r_close(Resource *resource)
{
    /* This will never be freed but we don't care since we're going to
     * need it till the very end of the process.
     */
    static GThreadPool *closing_pool;

    static gsize created_pool = false;
    if ( g_once_init_enter(&created_pool) ) {
        closing_pool = g_thread_pool_new(r_free_cb, NULL,
                                         -1, false, NULL);

        g_once_init_leave(&created_pool, true);
    }

    g_thread_pool_push(closing_pool, resource, NULL);
}

/**
 * @}
 */

