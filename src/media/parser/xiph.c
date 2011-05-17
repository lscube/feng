/*
 * This file is part of Feng
 *
 * Copyright (C) 2011 by LScube team <team@lscube.org>
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
 */

#include <config.h>

#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "media/demuxer.h"

#define HEADER_SIZE 6
#define MAX_PAYLOAD_SIZE (DEFAULT_MTU - HEADER_SIZE)

int xiph_parse(Track *tr, uint8_t *data, size_t len)
{
    xiph_priv *priv = tr->private_data;

    uint8_t prefix[HEADER_SIZE] = {
        (priv->ident>>16)& 0xff,
        (priv->ident>>8) & 0xff,
        priv->ident     & 0xff
    };

    do {
        uint16_t payload_size;

        struct MParserBuffer *buffer = g_slice_new0(struct MParserBuffer);

        if ( prefix[3] == 0 && len <= MAX_PAYLOAD_SIZE )
            prefix[3] = 1;
        else if ( prefix[3] == 0 )
            prefix[3] = 1 << 6; /* first frag */
        else if ( len > MAX_PAYLOAD_SIZE )
            prefix[3] = 2 << 6; /* middle frag */
        else
            prefix[3] = 3 << 6; /* max frag */

        buffer->timestamp = tr->pts;
        buffer->delivery = tr->dts;
        buffer->duration = tr->frame_duration;
        buffer->marker = (len <= MAX_PAYLOAD_SIZE);

        buffer->data_size = MIN(MAX_PAYLOAD_SIZE, len) + HEADER_SIZE;
        buffer->data = g_malloc(buffer->data_size);

        payload_size = htons(buffer->data_size);

        memcpy(buffer->data, &prefix[0], HEADER_SIZE);
        memcpy(buffer->data + 4, &payload_size, sizeof(payload_size));
        memcpy(buffer->data + HEADER_SIZE, data,
               buffer->data_size - HEADER_SIZE);

        mparser_buffer_write(tr, buffer);

        len -= MAX_PAYLOAD_SIZE;
        data += MAX_PAYLOAD_SIZE;
    } while(len > 0);

    return 0;
}

void xiph_uninit(Track *track)
{
    xiph_priv *priv = track->private_data;
    if (!priv)
        return;

    g_free(priv->conf);
    g_free(priv->packet);
    g_slice_free(xiph_priv, priv);

    track->private_data = NULL;
}
