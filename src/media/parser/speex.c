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

#include "media/demuxer.h"
#include "media/mediaparser.h"

static int speex_parse(Track *tr, uint8_t *data, size_t len)
{
    struct MParserBuffer *buffer;

    if (len > DEFAULT_MTU)
        return -1;

    buffer = g_slice_new0(struct MParserBuffer);

    buffer->timestamp = tr->properties.pts;
    buffer->delivery = tr->properties.dts;
    buffer->duration = tr->properties.frame_duration;
    buffer->marker = true;

    buffer->data_size = len;
    buffer->data = g_memdup(data, buffer->data_size);

    mparser_buffer_write(tr, buffer);

    return 0;
}

#define speex_init   NULL
#define speex_uninit NULL

FENG_MEDIAPARSER(speex, "speex", MP_audio);
