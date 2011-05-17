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

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/in.h>

#include "media/media.h"
#include "fnc_log.h"

int mpa_parse(Track *tr, uint8_t *data, ssize_t len)
{
    ssize_t rem = len;

    if (DEFAULT_MTU >= len + 4) {
        struct MParserBuffer *buffer = g_slice_new0(struct MParserBuffer);

        buffer->timestamp = tr->pts;
        buffer->delivery = tr->dts;
        buffer->duration = tr->frame_duration;
        buffer->marker = true;

        buffer->data_size = len + 4;
        buffer->data = g_malloc(buffer->data_size);

        memset(buffer->data, 0, 4);
        memcpy(buffer->data + 4, data, len);

        track_write(tr, buffer);

        fnc_log(FNC_LOG_VERBOSE, "[mp3] no frags");

        return 0;
    }

    do {
        int32_t offset = len - rem;
        struct MParserBuffer *buffer;

        if (offset & 0xffff0000)
            return -1;

        offset = htonl(offset & 0xffff);

        buffer = g_slice_new0(struct MParserBuffer);

        buffer->timestamp = tr->pts;
        buffer->delivery = tr->dts;
        buffer->duration = tr->frame_duration;
        buffer->marker = false;

        buffer->data_size = MIN(DEFAULT_MTU, rem + 4);
        buffer->data = g_malloc(buffer->data_size);

        memcpy(buffer->data, &offset, 4);
        memcpy(buffer->data + 4, data + offset, buffer->data_size - 4);

        track_write(tr, buffer);

        rem -= DEFAULT_MTU - 4;
        fnc_log(FNC_LOG_VERBOSE, "[mp3] frags");
    } while (rem >= 0);

    fnc_log(FNC_LOG_VERBOSE, "[mp3]Frames completed");

    return 0;
}
