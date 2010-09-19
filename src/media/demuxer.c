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

    g_string_free(track->sdp_description, true);

    if ( track->parser && track->parser->uninit )
        track->parser->uninit(track);

    g_slice_free(Track, track);
}


#define ADD_TRACK_ERROR(level, ...) \
    { \
        fnc_log(level, __VA_ARGS__); \
        goto error; \
    }

/**
 * @brief Add track to resource tree.
 *
 * This function adds a new track data struct to resource tree. It
 * used by specific demuxer function in order to obtain the struct to
 * fill.
 *
 * @param r Resource to add the Track to
 * @param name Name of the track to use (will be g_free'd); make sure
 *             to use @ref feng_str_is_unreseved before passing an
 *             user-provided string to this function.
 *
 * @return pointer to newly allocated track struct.
 **/

Track *add_track(Resource *r, char *name, MediaProperties *prop_hints)
{
    Track *t;

    t = g_slice_new0(Track);

    t->lock            = g_mutex_new();
    t->parent          = r;
    t->name            = name;
    t->sdp_description = g_string_new("");

    g_string_append_printf(t->sdp_description,
                           "a=control:"SDP_TRACK_SEPARATOR"%s\r\n",
                           name);

    memcpy(&t->properties, prop_hints, sizeof(MediaProperties));

    switch (t->properties.media_source) {
    case STORED_SOURCE:
        if ( !(t->parser = mparser_find(t->properties.encoding_name)) )
            ADD_TRACK_ERROR(FNC_LOG_FATAL, "Could not find a valid parser\n");

        t->properties.media_type = t->parser->media_type;
        break;

    case LIVE_SOURCE:
        break;

    default:
        g_assert_not_reached();
        break;
    }

    if (t->parser->init && t->parser->init(t) != 0)
        ADD_TRACK_ERROR(FNC_LOG_FATAL, "Could not initialize parser for %s\n",
                        t->properties.encoding_name);

    r->tracks = g_list_append(r->tracks, t);

    return t;

 error:
    free_track(t, NULL);
    return NULL;
}
#undef ADD_TRACK_ERROR

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
