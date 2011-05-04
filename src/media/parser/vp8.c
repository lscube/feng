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

#define VP8_HEADER_SIZE 1
#define VP8_START_PACKET 1

static int vp8_parse(Track *tr, uint8_t *data, size_t len)
{
    int off = 0;
    uint32_t payload = DEFAULT_MTU - VP8_HEADER_SIZE;
    uint8_t *packet = g_slice_alloc0(DEFAULT_MTU);
    int keyframe = data[0] & 1 ? 0 : 2;

    if(!packet) return -1;

    packet[0] = keyframe | VP8_START_PACKET;

    while (len > payload) {
        memcpy(packet + VP8_HEADER_SIZE, data + off, payload);
        mparser_buffer_write(tr,
                             tr->properties.pts,
                             tr->properties.dts,
                             tr->properties.frame_duration,
                             false, 0, 0,
                             packet, DEFAULT_MTU);

        len -= payload;
        off += payload;
        packet[0] = keyframe;
    }

    memcpy(packet + VP8_HEADER_SIZE, data + off, len);
    mparser_buffer_write(tr,
                         tr->properties.pts,
                         tr->properties.dts,
                         tr->properties.frame_duration,
                         true, 0, 0,
                         packet, len + VP8_HEADER_SIZE);

    g_slice_free1(DEFAULT_MTU, packet);
    return 0;
}

FENG_MEDIAPARSER(vp8, "vp8", MP_video);
