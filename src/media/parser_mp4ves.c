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

#include "media/media.h"
#include "fnc_log.h"

int mp4ves_init(Track *track)
{
    sdp_descr_append_rtpmap(track);
    g_string_append_printf(track->sdp_description,
                           "a=fmtp:%u profile-level-id=1;",

                           /* fmtp */
                           track->payload_type);

    sdp_descr_append_config(track);
    g_string_append(track->sdp_description, "\r\n");

    return 0;
}

int mp4ves_parse(Track *tr, uint8_t *data, ssize_t len)
{
    do {
        struct MParserBuffer *buffer = g_slice_new0(struct MParserBuffer);

        buffer->timestamp = tr->pts;
        buffer->delivery = tr->dts;
        buffer->duration = tr->frame_duration;
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
