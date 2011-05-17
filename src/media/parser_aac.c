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

#include <string.h>
#include <stdbool.h>

#include "media/media.h"
#include "fnc_log.h"

int aac_init(Track *track)
{
    g_string_append_printf(track->sdp_description,
                           "a=rtpmap:%u %s/%d\r\n"

                           "a=fmtp:%u streamtype=5;profile-level-id=1;"
                           "mode=AAC-hbr;sizeLength=13;indexLength=3;"
                           "indexDeltaLength=3; ",

                           /* rtpmap */
                           track->payload_type,
                           track->encoding_name,
                           track->clock_rate,

                           /* fmtp */
                           track->payload_type);

    sdp_descr_append_config(track);
    g_string_append(track->sdp_description, "\r\n");

    return 0;
}

#define HEADER_SIZE 4
#define MAX_PAYLOAD_SIZE (DEFAULT_MTU - HEADER_SIZE)

int aac_parse(Track *tr, uint8_t *data, size_t len)
{
    const uint8_t prefix[HEADER_SIZE] = { 0x00, 0x10, (len & 0x1fe0) >> 5, (len & 0x1f) << 3 };

    do {
        struct MParserBuffer *buffer = g_slice_new0(struct MParserBuffer);

        buffer->timestamp = tr->pts;
        buffer->delivery = tr->dts;
        buffer->duration = tr->frame_duration;
        buffer->marker = (len <= MAX_PAYLOAD_SIZE);

        buffer->data_size = MIN(MAX_PAYLOAD_SIZE, len) + HEADER_SIZE;
        buffer->data = g_malloc(buffer->data_size);

        memcpy(buffer->data, &prefix[0], HEADER_SIZE);
        memcpy(buffer->data + HEADER_SIZE, data,
               buffer->data_size - HEADER_SIZE);

        track_write(tr, buffer);

        len -= MAX_PAYLOAD_SIZE;
        data += MAX_PAYLOAD_SIZE;
    } while(len > 0);

    return 0;
}
