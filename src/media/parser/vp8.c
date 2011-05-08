/* *
 * This file is part of Feng
 *
 * Copyright (C) 2010 by LScube team <team@lscube.org>
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

#include <string.h>
#include <stdbool.h>

#include "media/demuxer.h"
#include "media/mediaparser.h"
#include "fnc_log.h"

#define vp8_uninit NULL

static int vp8_init(Track *track)
{
    g_string_append_printf(track->sdp_description,
                           "a=rtpmap:%u VP8/%d\r\n",

                           /* rtpmap */
                           track->properties.payload_type,
                           track->properties.clock_rate);

    return 0;
}

/**
 *
 *  VP8 Extension header layout
 *
 *  0 1 2 3 4 5 6 7 8
 *  +-+-+-+-+-+-+-+-+
 *  |           |K|S|
 *  +-+-+-+-+-+-+-+-+
 *
 *  K Keyframe
 *  S Start packet
 */

#define VP8_START_PACKET 1

#define HEADER_SIZE 1
#define MAX_PAYLOAD_SIZE (DEFAULT_MTU - HEADER_SIZE)

static int vp8_parse(Track *tr, uint8_t *data, size_t len)
{
    uint8_t prefix[HEADER_SIZE] = { (data[0] & 1 ? 0 : 2) | VP8_START_PACKET };

    do {
        struct MParserBuffer *buffer = g_slice_new0(struct MParserBuffer);

        buffer->timestamp = tr->properties.pts;
        buffer->delivery = tr->properties.dts;
        buffer->duration = tr->properties.frame_duration;
        buffer->marker = (len <= MAX_PAYLOAD_SIZE);

        buffer->data_size = MIN(MAX_PAYLOAD_SIZE, len) + HEADER_SIZE;
        buffer->data = g_malloc(buffer->data_size);

        memcpy(buffer->data, &prefix[0], HEADER_SIZE);
        memcpy(buffer->data + HEADER_SIZE, data,
               buffer->data_size - HEADER_SIZE);

        mparser_buffer_write(tr, buffer);

        len -= MAX_PAYLOAD_SIZE;
        data += MAX_PAYLOAD_SIZE;

        prefix[0] &= ~VP8_START_PACKET;
    } while (len > 0);

    return 0;
}

FENG_MEDIAPARSER(vp8, "vp8", MP_video);
