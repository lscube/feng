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

    if (!bq_producer_consumers(track->producer)) {
        // track->info->mrl is expected to be set by live only
        if(track->info->mrl) {
            g_mutex_lock(track->parent->srv->lock);
            g_hash_table_remove(track->parent->srv->live_mq, track->info->mrl);
            g_mutex_unlock(track->parent->srv->lock);
        }
        bq_producer_unref(track->producer);
    }

    g_free(track->info->mrl);
    g_slice_free(TrackInfo, track->info);

    g_slice_free(MediaProperties, track->properties);

    mparser_unreg(track->parser, track->private_data);
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

    t->lock = g_mutex_new();

    t->info = g_slice_new0(TrackInfo);
    memcpy(t->info, &info, sizeof(TrackInfo));

    t->properties = g_slice_new0(MediaProperties);
    memcpy(t->properties, &prop_hints, sizeof(MediaProperties));

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
        g_mutex_lock(r->srv->lock);
        if(!(t->producer = g_hash_table_lookup(r->srv->live_mq, t->info->mrl)))
            if( !(t->producer = bq_producer_new(g_free)) ) {
                g_mutex_unlock(r->srv->lock);
                ADD_TRACK_ERROR(FNC_LOG_FATAL, "Memory allocation problems\n");
            } else
                g_hash_table_insert(r->srv->live_mq,
                                    t->info->mrl, t->producer);
        g_mutex_unlock(r->srv->lock);
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
