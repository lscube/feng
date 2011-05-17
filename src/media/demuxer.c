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
 * @param track Track to free
 */
void track_free(Track *track)
{
    if (!track)
        return;

    g_mutex_free(track->lock);

    g_free(track->name);
    g_free(track->properties.encoding_name);

    g_assert_cmpuint(track->consumers, ==, 0);

    g_cond_free(track->last_consumer);

    if ( track->queue ) {
        /* Destroy elements and the queue */
        g_queue_foreach(track->queue,
                        bq_element_free_internal,
                        NULL);
        g_queue_free(track->queue);
    }

    if ( track->sdp_description )
        g_string_free(track->sdp_description, true);

    if ( track->parser && track->parser->uninit )
        track->parser->uninit(track);

    g_slice_free(Track, track);
}

/**
 * @brief Create a new Track object
 *
 * @param name Name of the track to use (will be g_free'd); make sure
 *             to use @ref feng_str_is_unreseved before passing an
 *             user-provided string to this function.
 *
 * @return pointer to newly allocated track struct.
 */
Track *track_new(char *name)
{
    Track *t;

    t = g_slice_new0(Track);

    t->lock            = g_mutex_new();
    t->last_consumer   = g_cond_new();
    t->name            = name;
    t->sdp_description = g_string_new("");

    g_string_append_printf(t->sdp_description,
                           "a=control:%s\r\n",
                           name);

    bq_producer_reset_queue_internal(t);

    return t;
}
