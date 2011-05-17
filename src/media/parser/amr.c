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

#include "media/demuxer.h"
#include "media/mediaparser.h"
#include "fnc_log.h"

#define amr_uninit NULL

static int amr_init(Track *track)
{
    track->clock_rate = 8000;

    g_string_append_printf(track->sdp_description,
                           "a=rtpmap:%u AMR/%d/%d\r\n"
                           "a=fmtp:%u octet-align=1;",

                           /* rtpmap */
                           track->payload_type,
                           track->clock_rate,
                           track->audio_channels,

                           /* fmtp */
                           track->payload_type);

    if ( track->extradata_len > 0 )
        sdp_descr_append_config(track);

    g_string_append(track->sdp_description, "\r\n");

    return 0;
}

/* AMR Payload Header (RFC3267)
    0  1 2 3 4 5   6 7
   +-+-+-+-+-+-+
   | CMR  |   RR     |
   +-+-+-+-+-+-+
   +-+-+-+-+-+-+
   |F|  FT    |Q  PP |
   +-+-+-+-+-+-+
*/

typedef struct
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    uint8_t p:2;       /* padding bits */
    uint8_t q:1;       /* Frame quality indicator. */
    uint8_t ft:4;      /* Frame type index */
    uint8_t f:1;       /*If set to 1, indicates that this frame is followed by
                                another speech frame in this payload; if set to 0, indicates that
                                this frame is the last frame in this payload. */
#elif G_BYTE_ORDER == G_BIG_ENDIAN
    uint8_t f:1;       /*If set to 1, indicates that this frame is followed by
                                another speech frame in this payload; if set to 0, indicates that
                                this frame is the last frame in this payload. */
    uint8_t ft:4;      /* Frame type index */
    uint8_t q:1;       /* Frame quality indicator. */
    uint8_t p:2;       /* padding bits */
#else
#error Neither big nor little
#endif
} toc;

#define AMR_CMR 0xf0

static int amr_parse(Track *tr, uint8_t *data, size_t len)
{
    uint8_t *packet = g_slice_alloc0(DEFAULT_MTU);
    static const uint32_t packet_size[] = {12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0};

    while (len > 0) {
        struct MParserBuffer *buffer;

        uint32_t read_offset = 0, frames = 0;
        uint32_t body_len, i, off;
        toc tocv;

        /* Count the number of frames that fit into the packet */
        while (read_offset < len) {
            memcpy(&tocv, &data[read_offset], 1);
            body_len = packet_size[tocv.ft];
            if (read_offset + 1 + body_len > len)
                break; /* Not enough speech data */
            if (read_offset + 1 + body_len > DEFAULT_MTU - 1)
                break; /* This frame doesn't fit into the current packet */
            read_offset += 1 + body_len;
            frames++;
        }
        if (frames <= 0) /* No frames - bad trailing data? */
            break;

        buffer = g_slice_new0(struct MParserBuffer);

        buffer->timestamp = tr->pts;
        buffer->delivery = tr->dts;
        buffer->duration = tr->frame_duration;

        buffer->data = g_malloc(DEFAULT_MTU);
        buffer->data[0] = AMR_CMR;

        off = 1 + frames; /* Write the body data at this offset */
        for (i = 0; i < frames; i++) {
            memcpy(&tocv, data, 1);
            body_len = packet_size[tocv.ft];
            tocv.f = i < frames - 1;
            memcpy(buffer->data + 1 + i, &tocv, 1);
            memcpy(buffer->data + off, &data[1], body_len);
            off  +=     body_len;
            data += 1 + body_len;
            len  -= 1 + body_len;
        }

        buffer->data_size = off;
        mparser_buffer_write(tr, buffer);
    }

    g_slice_free1(DEFAULT_MTU, packet);
    return 0;
}

FENG_MEDIAPARSER(amr, "amr", MP_audio);
