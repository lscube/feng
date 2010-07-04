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
#include "mediaparser.h"
#include "fnc_log.h"

static void free_sdp_field(sdp_field *sdp,
                           ATTR_UNUSED void *unused)
{
    if (!sdp)
        return;

    g_free(sdp->field);
    g_slice_free(sdp_field, sdp);
}

static void sdp_fields_free(GSList *fields)
{
    if ( fields == NULL )
        return;

    g_slist_foreach(fields, (GFunc)free_sdp_field, NULL);
    g_slist_free(fields);
}

/**
 * @brief Frees the resources of a Track object
 *
 * @param element Track to free
 * @param user_data Unused, for compatibility with g_list_foreach().
 */
void free_track(gpointer element,
                ATTR_UNUSED gpointer user_data)
{
    Track *track = (Track*)element;

    if (!track)
        return;

    g_mutex_free(track->lock);

    if ( track-> producer )
        bq_producer_unref(track->producer);

    g_free(track->info->mrl);
    g_slice_free(TrackInfo, track->info);

    sdp_fields_free(track->sdp_fields);

    if ( track->parser && track->parser->uninit )
        track->parser->uninit(track);

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

    t->parent = r;

    t->info = g_slice_new0(TrackInfo);
    memcpy(t->info, info, sizeof(TrackInfo));

    memcpy(&t->properties, prop_hints, sizeof(MediaProperties));

    switch (t->properties.media_source) {
    case STORED_SOURCE:
        if ( !(t->parser = mparser_find(t->properties.encoding_name)) )
            ADD_TRACK_ERROR(FNC_LOG_FATAL, "Could not find a valid parser\n");
        if (t->parser->init(t))
            ADD_TRACK_ERROR(FNC_LOG_FATAL, "Could not initialize parser for %s\n",
                            t->properties.encoding_name);

        t->properties.media_type = t->parser->info->media_type;
        break;

    case LIVE_SOURCE:
        break;

    default:
        g_assert_not_reached();
        break;
    }

    r->tracks = g_list_append(r->tracks, t);
    r->num_tracks++;

    return t;

 error:
    free_track(t, NULL);
    return NULL;
}
#undef ADD_TRACK_ERROR

/**
 * @brief Append an SDP field to the track
 *
 * @param track The track to add the fields to
 * @param type The type of the field to add
 * @param value The value of the field to add (already duped)
 */
void track_add_sdp_field(Track *track, sdp_field_type type, char *value)
{
    sdp_field *field = g_slice_new(sdp_field);
    field->type = type;
    field->field = value;

    track->sdp_fields = g_slist_prepend(track->sdp_fields, field);
}

/**
 * @brief Get the producer for the track
 *
 * @param track The track to get the producer for
 */
BufferQueue_Producer *track_get_producer(Track *tr)
{
    g_mutex_lock(tr->lock);

    if ( tr->producer == NULL )
        tr->producer = bq_producer_new(g_free);

    g_mutex_unlock(tr->lock);

    return tr->producer;
}
