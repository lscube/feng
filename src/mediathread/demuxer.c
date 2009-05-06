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
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "feng.h"
#include "demuxer.h"
#include "description.h"
#include "fnc_log.h"

// global demuxer modules:
extern Demuxer fnc_demuxer_sd;
extern Demuxer fnc_demuxer_ds;
extern Demuxer fnc_demuxer_avf;

/**
 * @brief Array of available built-in demuxers
 */
static Demuxer *const demuxers[] = {
    &fnc_demuxer_sd,
    &fnc_demuxer_ds,
    &fnc_demuxer_avf,
    NULL
};

/**
 * @brief The resource description cache.
 *
 * This list holds the cache of resource descriptions. This way the
 * mediathread can provide the description buffer without opening the
 * resource every time.  This will result in a better performance for
 * RTSP DESCRIBE metod response.  Descriptions are ordered for access
 * time, so that the last is the less recently addressed media
 * description and will be chosen for removal if cache reaches the
 * limit.
 */
static GList *descr_cache;

//! cache size of descriptions (maybe we need to take it from fenice configuration file?)
#define MAX_DESCR_CACHE_SIZE 10

/**
 * @brief Comparison function used to compare a ResourceDescr to a MRL
 *
 * @param a ResourceDescr pointer of the element in the list
 * @param b String with the MRL to compare to
 *
 * @return The strcmp() result between the correspondent ResourceInfo
 *         MRL and the given MRL.
 *
 * @internal This function should _only_ be used by @ref r_descr_find.
 */
static gint r_descr_find_cmp_mrl(gconstpointer a, gconstpointer b)
{
    return strcmp( ((ResourceDescr *)a)->info->mrl, (const char *)b );
}

/**
 * @brief Find a given ResourceDecr in the cache, from its MRL
 *
 * @param mrl The MRL to look for
 *
 * @return A GList pointer to the requested resource description.
 *
 * @see r_descr_find_cmp_mrl
 */
static GList *r_descr_find(const char *mrl)
{
    return g_list_find_custom(descr_cache, mrl, r_descr_find_cmp_mrl);
}

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

Demuxer *find_demuxer(InputStream *i_stream)
{
    // this int will contain the index of the demuxer already probed second
    // the extension suggestion, in order to not probe twice the same
    // demuxer that was proved to be not usable.
    int probed = -1;
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
                        return demuxers[i];
                    }
                }
            }
        }
    }

    for (i=0; demuxers[i]; i++) {
        if ((i!=probed) && (demuxers[i]->probe(i_stream) == RESOURCE_OK))
            {
                fnc_log(FNC_LOG_DEBUG, "[MT] probing demuxer: \"%s\""
                        "demuxer found\n",
                        demuxers[i]->info->name);
                return demuxers[i];
            }
    }

    return NULL;
}

/**
 * @brief Create new media descriptions
 *
 * @param element The track to create a media description for
 * @param user_data The new resource descriptions to add the
 *                  descriptions to
 *
 * @internal This function should only be called through
 *           g_list_foreach().
 */
static void r_descr_new_mdescr(gpointer element, gpointer user_data)
{
    ResourceDescr *new_descr = (ResourceDescr *)user_data;
    Track *track = (Track*)element;

    MediaDescr *new_mdescr = g_new(MediaDescr, 1);
    new_mdescr->info = track->info;
    MObject_ref(track->info);
    new_mdescr->properties = track->properties;
    MObject_ref(track->properties);
    new_mdescr->last_change = mrl_mtime(track->info->mrl);
    new_descr->media = g_list_append(new_descr->media, new_mdescr);
}

static ResourceDescr *r_descr_new(Resource *r)
{
    ResourceDescr *new_descr = g_new(ResourceDescr, 1);
    new_descr->media = NULL;

    new_descr->info=r->info;
    MObject_ref(r->info);
    new_descr->last_change=mrl_mtime(r->info->mrl);

    g_list_foreach(r->tracks, r_descr_new_mdescr, new_descr);

    return new_descr;
}

/**
 * @brief Unreference a list of media descriptions
 *
 * @param element The media description to unreference
 * @param user_data Unused
 *
 * @internal This function should only be called through
 *           g_list_foreach().
 */
static void r_descr_free_media(gpointer element, gpointer user_data)
{
    MediaDescr *m_descr = (MediaDescr *)element;

    MObject_unref( MOBJECT(m_descr->info) );
    MObject_unref( MOBJECT(m_descr->properties) );
}

static void r_descr_free(ResourceDescr *descr)
{
    if (!descr)
        return;

    g_list_foreach(descr->media, r_descr_free_media, NULL);
    g_list_free(descr->media);

    MObject_unref( MOBJECT(descr->info) );
    g_free(descr);
}

/**
 * @brief Check if a given resource has changed on disk
 *
 * @param descr The description of the resource to change
 *
 * @retval true The resource has changed on disk since last time
 * @retval false The resource hasn't changed on disk
 *
 * @see ResourceDescr::last_change
 */
static gboolean r_changed(ResourceDescr *descr)
{
    GList *m_descr_it;

    if ( mrl_changed(descr->info->mrl, &descr->last_change) )
        return true;

    for (m_descr_it = g_list_first(descr->media);
         m_descr_it;
         m_descr_it = g_list_next(m_descr_it) )
    {
        MediaDescr *m_descr = (MediaDescr *)m_descr_it->data;

        /* Why is it possible that this hits? No clue! */
        if (m_descr->info->mrl == NULL)
            break;

        if (mrl_changed(m_descr->info->mrl,
                        &(m_descr->last_change)))
            return true;
    }

    return false;
}

void r_descr_cache_update(Resource *r)
{
    GList *cache_el;
    ResourceDescr *r_descr=NULL;

    if ( ( cache_el = r_descr_find(r->info->mrl) ) ) {
        r_descr = cache_el->data;
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
        r_descr_free(cache_el->data);
        descr_cache = g_list_delete_link(descr_cache, cache_el);
    }
}

/**
 * @brief Free a @ref ResourceInfo object
 */
static void resinfo_free(void *resinfo)
{
    if (!resinfo)
        return;

    g_free(((ResourceInfo *)resinfo)->mrl);

    g_free(resinfo);
}

/**
 * @brief Creates a new @ref ResourceInfo object
 */
ResourceInfo *resinfo_new() {
    ResourceInfo *rinfo;

    rinfo = MObject_new0(ResourceInfo, 1);
    MObject_destructor(rinfo, resinfo_free);

    return rinfo;
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

    g_free(sdp->field);
    g_free(sdp);
}

static void properties_free(void *properties)
{
    MediaProperties *props = properties;

    if (!props)
        return;

    g_list_foreach(props->sdp_private, (GFunc)free_sdp_field, NULL);

    g_free(props);
}


/**
 * @brief Frees the resources of a Track object
 *
 * @param element Track to free
 * @param user_data Unused, for compatibility with g_list_foreach().
 */
void free_track(gpointer element, gpointer user_data)
{
    Track *track = (Track*)element;

    if (!track)
        return;

    MObject_unref(MOBJECT(track->info));
    MObject_unref(MOBJECT(track->properties));
    mparser_unreg(track->parser, track->private_data);
    if (!bq_producer_consumers(track->producer)) {
        g_hash_table_remove(track->parent->srv->live_mq, track->info->mrl);
        bq_producer_unref(track->producer);
    }
    istream_close(track->i_stream);

    g_slice_free(Track, track);
}


#define ADD_TRACK_ERROR(level, ...) \
    { \
        fnc_log(level, __VA_ARGS__); \
        goto error; \
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
    // TODO: search first of all in exclusive tracks

    if(r->num_tracks>=MAX_TRACKS)
        return NULL;

    t = g_slice_new0(Track);

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

    case MS_live:
        if(!(t->producer = g_hash_table_lookup(r->srv->live_mq, t->info->mrl)))
            if( !(t->producer = bq_producer_new(g_free)) ) {
                ADD_TRACK_ERROR(FNC_LOG_FATAL, "Memory allocation problems\n");
            } else
                g_hash_table_insert(r->srv->live_mq,
                                    t->info->mrl, t->producer);
        break;

    default:
        ADD_TRACK_ERROR(FNC_LOG_FATAL, "Media source not supported!");
        break;
    }

    t->parent = r;
    r->tracks = g_list_append(r->tracks, t);
    r->num_tracks++;

    return t;

 error:
    free_track(t, NULL);
    return NULL;
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

    return cache_el->data;
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
        MediaDescr *m_descr_list = (MediaDescr *)m_descr_list_it->data;

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
        found_list = g_list_append(found_list, m_descr);
        new_m_descrs->pdata[i] = found_list;
    } else {
        MediaDescrList new_list = g_list_append(NULL, m_descr);
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
    MediaDescrListArray new_m_descrs =
        g_ptr_array_sized_new(g_list_position(r_descr->media,
                                              g_list_last(r_descr->media))+1);

    g_list_foreach(r_descr->media, r_descr_find_media, new_m_descrs);

    return new_m_descrs;
}
