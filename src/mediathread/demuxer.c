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

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <fenice/server.h>
#include "demuxer.h"
#include "description.h"
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

// global demuxer modules:
extern Demuxer fnc_demuxer_sd;
extern Demuxer fnc_demuxer_ds;
extern Demuxer fnc_demuxer_avf;

// static array containing all the available demuxers:
static Demuxer *demuxers[] = {
    &fnc_demuxer_sd,
    &fnc_demuxer_ds,
    &fnc_demuxer_avf,
    NULL
};

/*! @brief The resource description cache.
 * This list holds the cache of resource descriptions. This way the mediathread
 * can provide the description buffer without opening the resource every time.
 * This will result in a better performance for RTSP DESCRIBE metod response.
 * Descriptions are ordered for access time, so that the last is the less
 * recently addressed media description and will be chosen for removal if cache
 * reaches the limit.
 * */
static GList *descr_cache=NULL;
//! cache size of descriptions (maybe we need to take it from fenice configuration file?)
#define MAX_DESCR_CACHE_SIZE 10

/*! Private functions for exclusive tracks.
 * */
#define r_descr_find(mrl) g_list_find_custom(descr_cache, mrl, cache_cmp)

static int r_changed(ResourceDescr *descr);

// private functions for specific demuxer

/**
 * Find the correct demuxer for the given resource.
 * First of all, it tries to match the resource extension with one of those
 * served by the demuxers and, if found, probes that demuxer.
 * If no demuxer can be found this way, then it tries every demuxer available.
 * @param i_stream the resource.
 * @return the index of the valid demuxer in the list or -1 if it could not be
 * found.
 * */

static int find_demuxer(InputStream *i_stream)
{
    // this int will contain the index of the demuxer already probed second
    // the extension suggestion, in order to not probe twice the same
    // demuxer that was proved to be not usable.
    int probed = -1;
    char found = 0; // flag for demuxer found.
    int i; // index
    char exts[128], *res_ext, *tkn; /* temp string containing extension
                                     * served by probing demuxer.
                                     */

    // First of all try that with matching extension: we use extension as a
    // hint of resource type.
    if ( (res_ext=strrchr(i_stream->name, '.')) && (res_ext++) ) {
        // extension present
        for (i=0; demuxers[i]; i++) {
            strncpy(exts, demuxers[i]->info->extensions, sizeof(exts));

            for (tkn=strtok(exts, ","); tkn; tkn=strtok(NULL, ",")) {
                if (!strcmp(tkn, res_ext)) {
                    fnc_log(FNC_LOG_DEBUG, "[MT] probing demuxer: \"%s\" "
                                "matches \"%s\" demuxer\n", res_ext,
                                demuxers[i]->info->name);
                    if (demuxers[i]->probe(i_stream) == RESOURCE_OK) {
                        fnc_log(FNC_LOG_DEBUG,
                                "[MT] probing demuxer: \"%s\" demuxer found\n",
                                demuxers[i]->info->name);
                        found = 1;
                        break;
                    }
                }
            }
            if (found) break;
        }
    }
    if (!found) {
        for (i=0; demuxers[i]; i++) {
            if ((i!=probed) && (demuxers[i]->probe(i_stream) == RESOURCE_OK))
            {
                fnc_log(FNC_LOG_DEBUG, "[MT] probing demuxer: \"%s\""
                                       "demuxer found\n",
                                        demuxers[i]->info->name);
                found = 1;
                break;
            }
        }
    }

    return found ? i: -1;
}

// --- Description Cache management --- //
static gint cache_cmp(gconstpointer a, gconstpointer b)
{
    return strcmp( ((ResourceDescr *)a)->info->mrl, (const char *)b );
}

static ResourceDescr *r_descr_new(Resource *r)
{
    ResourceDescr *new_descr;
    MediaDescr *new_mdescr;
    GList *tracks;

    new_descr=g_new(ResourceDescr, 1);
    new_descr->media = NULL;

    new_descr->info=r->info;
    MObject_ref(r->info);
    new_descr->last_change=mrl_mtime(r->info->mrl);

    for (tracks=g_list_first(r->tracks); tracks; tracks=g_list_next(tracks)) {
        new_mdescr = g_new(MediaDescr, 1);
        new_mdescr->info = TRACK(tracks)->info;
        MObject_ref(TRACK(tracks)->info);
        new_mdescr->properties = TRACK(tracks)->properties;
        MObject_ref(TRACK(tracks)->properties);
        new_mdescr->last_change = mrl_mtime(TRACK(tracks)->info->mrl);
        new_descr->media = g_list_prepend(new_descr->media, new_mdescr);
    }
    // put the Media description in the same order of tracks.
    new_descr->media=g_list_reverse(new_descr->media);

    return new_descr;
}

static void r_descr_free(ResourceDescr *descr)
{
    GList *m_descr;

    if (!descr)
        return;

    for (m_descr=g_list_first(descr->media);
         m_descr;
         m_descr=g_list_next(m_descr) )
    {
        MObject_unref( MOBJECT(MEDIA_DESCR(m_descr)->info) );
        MObject_unref( MOBJECT(MEDIA_DESCR(m_descr)->properties) );
    }
    g_list_free(descr->media);

    MObject_unref( MOBJECT(descr->info) );
    g_free(descr);
}

static void r_descr_cache_update(Resource *r)
{
    GList *cache_el;
    ResourceDescr *r_descr=NULL;

    if ( ( cache_el = r_descr_find(r->info->mrl) ) ) {
        r_descr = RESOURCE_DESCR(cache_el);
        // TODO free ResourceDescr
        descr_cache = g_list_remove_link(descr_cache, cache_el);
        if (r_changed(r_descr)) {
            r_descr_free(r_descr);
            r_descr=NULL;
        }
    }
    if (!r_descr)
        r_descr = r_descr_new(r);

    descr_cache=g_list_prepend(descr_cache, r_descr);

    if (g_list_length(descr_cache)>MAX_DESCR_CACHE_SIZE) {
        cache_el = g_list_last(descr_cache);
        r_descr_free(RESOURCE_DESCR(cache_el));
        descr_cache = g_list_delete_link(descr_cache, cache_el);
    }
}

static void resinfo_free(void *resinfo)
{
    if (!resinfo)
        return;

    g_free(((ResourceInfo *)resinfo)->mrl);

    g_free(resinfo);
}

static void trackinfo_free(void *trackinfo)
{
    if (!trackinfo)
        return;

    g_free(((TrackInfo *)trackinfo)->mrl);

    g_free(trackinfo);
}

static void free_sdp_field(sdp_field *sdp, void *unused)
{
    if (!sdp)
        return;
    if (sdp->field)
    {
        g_free(sdp->field);
    }
    g_free(sdp);
}

static void properties_free(void *properties)
{
    MediaProperties *props = properties;

    if (!props)
        return;
    if (props->sdp_private)
        g_list_foreach(props->sdp_private, (GFunc)free_sdp_field, NULL);

    g_free(props);
}

Resource *r_open(struct feng *srv, const char *inner_path)
{
    Resource *r;
    int dmx_idx;
    InputStream *i_stream;
    gchar *mrl = g_strjoin ("/",
                            srv->config_storage[0]->document_root->ptr,
                            inner_path,
                            NULL);

    if( !(i_stream = istream_open(mrl)) )
        return NULL;

    if ( (dmx_idx = find_demuxer(i_stream))<0 ) {
        fnc_log(FNC_LOG_DEBUG, "[MT] Could not find a valid demuxer"
                                       " for resource %s\n", mrl);
        return NULL;
    }

    fnc_log(FNC_LOG_DEBUG, "[MT] registrered demuxer \"%s\" for resource"
                               "\"%s\"\n", demuxers[dmx_idx]->info->name, mrl);

// allocation of all data structures

    r = g_new0(Resource, 1);
    r->lock = g_mutex_new();

// we use MObject_new: that will alloc memory and exits the program
// if something goes wrong
    r->info = MObject_new0(ResourceInfo, 1);
    MObject_destructor(r->info, resinfo_free);

    r->info->mrl = mrl;
    r->info->name = g_path_get_basename(inner_path);
    r->i_stream = i_stream;
    r->demuxer = demuxers[dmx_idx];
    r->srv = srv;

    if (r->demuxer->init(r)) {
        r_close(r);
        return NULL;
    }

    r_descr_cache_update(r);

#ifdef HAVE_METADATA
    cpd_find_request(srv, r, filename);
#endif

    return r;
}

static void free_track(Track *t, Resource *r)
{
    if (!t)
        return;

    MObject_unref(MOBJECT(t->info));
    MObject_unref(MOBJECT(t->properties));
    mparser_unreg(t->parser, t->private_data);

    bq_producer_stop(t->producer);

    istream_close(t->i_stream);

    r->tracks = g_list_remove(r->tracks, t);
    g_free(t);
    r->num_tracks--;
}

void r_close(Resource *r)
{
    if(r) {
        g_mutex_free(r->lock);
        istream_close(r->i_stream);
        MObject_unref(MOBJECT(r->info));
        r->info = NULL;
        r->demuxer->uninit(r);

        if(r->tracks)
            g_list_foreach(r->tracks, (GFunc)free_track, r);

        g_free(r);
    }
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
    GList *track;

    for (track=g_list_first(resource->tracks); track; track=g_list_next(track))
        if( !strcmp(TRACK(track)->info->name, track_name) ) {
            return TRACK(track);
        }

    fnc_log(FNC_LOG_DEBUG, "Track %s not present in resource %s\n",
            track_name, resource->info->mrl);

    return NULL;
}

static int r_changed(ResourceDescr *descr)
{
    GList *m_descr = g_list_first(descr->media);

    if ( mrl_changed(descr->info->mrl, &descr->last_change) )
        return 1;

    for (/* m_descr=descr->media */;
         m_descr && MEDIA_DESCR(m_descr)->info->mrl;
         m_descr=g_list_next(m_descr))
    {
        if (mrl_changed(MEDIA_DESCR(m_descr)->info->mrl,
                        &(MEDIA_DESCR(m_descr)->last_change)))
            return 1;
    }

    return 0;
}

static void r_producer_reset_queue(gpointer track_gen, gpointer unused) {
    Track *t = (Track*)track_gen;

    bq_producer_reset_queue(t->producer);
}

int r_seek(Resource *resource, double time) {
    int res;

    g_mutex_lock(resource->lock);
    res = resource->demuxer->seek(resource, time);

    g_list_foreach(resource->tracks, r_producer_reset_queue, NULL);

    g_mutex_unlock(resource->lock);

    return res;
}

#define ADD_TRACK_ERROR(level, ...) \
    { \
        fnc_log(level, __VA_ARGS__); \
        free_track(t, r); \
        return NULL; \
    }

/*! Add track to resource tree.  This function adds a new track data struct to
 * resource tree. It used by specific demuxer function in order to obtain the
 * struct to fill.
 * @param r pointer to resource.
 * @return pointer to newly allocated track struct.
 * */

Track *add_track(Resource *r, TrackInfo *info, MediaProperties *prop_hints)
{
    Track *t;
    char *shm_name;

    // TODO: search first of all in exclusive tracks

    if(r->num_tracks>=MAX_TRACKS)
        return NULL;

    t = g_new0(Track, 1);

    if (info)
        t->info = MObject_dup(info, sizeof(TrackInfo));
    else
        t->info = MObject_new0(TrackInfo, 1);

    MObject_destructor(t->info, trackinfo_free);

    if (prop_hints)
        t->properties = MObject_dup(prop_hints, sizeof(MediaProperties));
    else
        t->properties = MObject_new0(MediaProperties, 1);

    MObject_destructor(t->properties, properties_free);

    switch (t->properties->media_source) {
    case MS_stored:
        if( !(t->producer = bq_producer_new(g_free)) )
            ADD_TRACK_ERROR(FNC_LOG_FATAL, "Memory allocation problems\n");
        if ( t->info->mrl && !(t->i_stream = istream_open(t->info->mrl)) )
            ADD_TRACK_ERROR(FNC_LOG_ERR, "Could not open %s\n", t->info->mrl);
        if ( !(t->parser = mparser_find(t->properties->encoding_name)) )
            ADD_TRACK_ERROR(FNC_LOG_FATAL, "Could not find a valid parser\n");
        if (t->parser->init(t->properties, &t->parser_private))
            ADD_TRACK_ERROR(FNC_LOG_FATAL, "Could not initialize parser for %s\n",
                            t->properties->encoding_name);
        break;

/*
    case MS_live:
	shm_name = strstr(t->info->mrl, FNC_PROTO_SEPARATOR) + strlen(FNC_PROTO_SEPARATOR);
        if( !(t->buffer = bp_shm_map(shm_name)) )
            ADD_TRACK_ERROR(FNC_LOG_FATAL, "Shared memory problems\n");
        break;
*/

    default:
        ADD_TRACK_ERROR(FNC_LOG_FATAL, "Media source not supported!");
        break;
    }

    t->parent = r;
    r->tracks = g_list_append(r->tracks, t);
    r->num_tracks++;

    return t;
}
#undef ADD_TRACK_ERROR

ResourceDescr *r_descr_get(struct feng *srv, const char *inner_path)
{
    GList *cache_el;
    gchar *mrl = g_strjoin ("/",
                            srv->config_storage[0]->document_root->ptr,
                            inner_path,
                            NULL);

    if ( !(cache_el=r_descr_find(mrl)) ) {
        Resource *r;
        if ( !(r = r_open(srv, inner_path)) )
            return NULL;
        cache_el = r_descr_find(mrl);
        r_close(r);
    }

    g_free(mrl);

    return RESOURCE_DESCR(cache_el);
}

/**
 * @brief Iteration function to find all the media of a given type
 *        with the given name.
 *
 * @param element The current media_description to test
 * @param user_data The array of lists
 *
 * @internal This function should only be called by g_list_foreach().
 */
static void r_descr_find_media(gpointer element, gpointer user_data) {
    MediaDescr *m_descr = (MediaDescr *)element;
    MediaDescrListArray new_m_descrs = (MediaDescrListArray)user_data;

    gboolean found = 0;
    guint i;

    for (i = 0; i < new_m_descrs->len; ++i) {
        MediaDescrList m_descr_list_it = g_ptr_array_index(new_m_descrs, i);
        MediaDescr *m_descr_list = MEDIA_DESCR(m_descr_list_it);

        if ( m_descr_list == NULL )
            continue;

        if ( (m_descr_type(m_descr) ==
              m_descr_type(m_descr_list)) &&
             !strcmp(m_descr_name(m_descr),
                     m_descr_name(m_descr_list)) ) {
            found = true;
            break;
        }
    }

    if (found) {
        MediaDescrList found_list = g_ptr_array_index(new_m_descrs, i);
        found_list = g_list_prepend(found_list, m_descr);
        new_m_descrs->pdata[i] = found_list;
    } else {
        MediaDescrList new_list = g_list_prepend(NULL, m_descr);
        g_ptr_array_add(new_m_descrs, new_list);
    }
}

/**
 * @brief Creates an array of MediaDescrList containing media
 *        descriptions.
 *
 * @return An array, each element of which is a MediaDescrList
 *         containing all the media of the same type with the same
 *         name. All the elements in each list can be included
 *         together in the sdp description, in a single 'm=' block.
 *
 * @param r_descr Resource description containing all the media.
 */
MediaDescrListArray r_descr_get_media(ResourceDescr *r_descr)
{
    MediaDescrListArray new_m_descrs;
    guint i;

    new_m_descrs = g_ptr_array_sized_new(
                        g_list_position(r_descr->media,
                                        g_list_last(r_descr->media))+1);

    g_list_foreach(r_descr->media, r_descr_find_media, new_m_descrs);

    for (i = 0; i < new_m_descrs->len; ++i) {
        MediaDescrList m_descr_list = g_ptr_array_index(new_m_descrs, i);
        m_descr_list = g_list_reverse(m_descr_list);
    }

    return new_m_descrs;
}
