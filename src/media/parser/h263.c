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

#include "media/demuxer.h"
#include "media/mediaparser.h"

static int h263_init(Track *track)
{
    g_string_append_printf(track->sdp_description,
                           "a=rtpmap:%u H263-1998/%d\r\n",
                           track->payload_type,
                           track->clock_rate);

    return 0;
}

static const uint8_t gob_start_code[] = { 0x04, 0x00 };

static int h263_parse(Track *tr, uint8_t *data, size_t len)
{
    size_t cur = 0;
    int found_gob = 0;

    if (len >= 3 && *data == '\0' && *(data + 1) == '\0'
        && *(data + 2) >= 0x80) {
        found_gob = 1;
    }

    while (len - cur > 0) {
        struct MParserBuffer *buffer = g_slice_new0(struct MParserBuffer);
        size_t payload, header_len;

        buffer->timestamp = tr->pts;
        buffer->delivery = tr->dts;
        buffer->duration = tr->frame_duration;

        buffer->data = g_malloc(buffer->data_size);

        if (cur == 0 && found_gob) {
            payload = MIN(DEFAULT_MTU, len);
            memcpy(buffer->data, data, payload);
            memcpy(buffer->data, gob_start_code, sizeof(gob_start_code));
            header_len = 0;
        } else {
            payload = MIN(DEFAULT_MTU - 2, len - cur);
            memset(buffer->data, 0, 2);
            memcpy(buffer->data + 2, data + cur, payload);
            header_len = 2;
        }

        buffer->marker = (cur + payload >= len);
        buffer->data_size = payload + header_len;

        mparser_buffer_write(tr, buffer);
        cur += payload;
    }

    return 0;
}

#define h263_uninit NULL

FENG_MEDIAPARSER(h263, "H263P", MP_video);
