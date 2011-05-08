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

#include <config.h>

#include <stdbool.h>
#include <string.h>

#include "media/demuxer.h"
#include "media/mediaparser.h"
#include "fnc_log.h"

#define mp4ves_uninit NULL

static int mp4ves_init(Track *track)
{
    if ( track->properties.extradata_len == 0 )
        return -1;

    g_string_append_printf(track->sdp_description,
                           "a=rtpmap:%u MP4V-ES/%d\r\n"
                           "a=fmtp:%u profile-level-id=1;",

                           /* rtpmap */
                           track->properties.payload_type,
                           track->properties.clock_rate,

                           /* fmtp */
                           track->properties.payload_type);

    sdp_descr_append_config(track);
    g_string_append(track->sdp_description, "\r\n");

    return 0;
}

static int mp4ves_parse(Track *tr, uint8_t *data, size_t len)
{
    do {
        struct MParserBuffer *buffer = g_slice_new0(struct MParserBuffer);

        buffer->timestamp = tr->properties.pts;
        buffer->delivery = tr->properties.dts;
        buffer->duration = tr->properties.frame_duration;
        buffer->marker = (len <= DEFAULT_MTU);

        buffer->data_size = MIN(DEFAULT_MTU, len);
        buffer->data = g_malloc(buffer->data_size);

        memcpy(buffer->data, data, buffer->data_size);

        len -= DEFAULT_MTU;
        data += DEFAULT_MTU;
    } while(len > 0);

    fnc_log(FNC_LOG_VERBOSE, "[mp4v]Frame completed");
    return 0;
}

FENG_MEDIAPARSER(mp4ves, "mp4v-es", MP_video);
