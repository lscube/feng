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

#include "media/demuxer.h"
#include "media/mediaparser.h"
#include "media/mediaparser_module.h"
#include "fnc_log.h"
#include "feng_utils.h"

static const MediaParserInfo info = {
    "amr",
    MP_audio
};

#define amr_uninit NULL

static int amr_init(Track *track)
{
    char *config = NULL;
    char *sdp_value;

    /*get config content if has*/
    if(track->properties.extradata_len)
    {
        config = extradata2config(&track->properties);
        if (!config)
            return -1;

        sdp_value = g_strdup_printf("octet-align=1; config=%s", config);
        g_free(config);
    } else {
        sdp_value =  g_strdup_printf("octet-align=1");
    }

    track_add_sdp_field(track, fmtp, sdp_value);

    track->properties.clock_rate = 8000;
    track_add_sdp_field(track, rtpmap,
                        g_strdup_printf ("AMR/%d/%d",
                                         track->properties.clock_rate,
                                         track->properties.audio_channels));

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

typedef struct
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    uint8_t reserved:4;        /* reserved bits that MUST be set to zero.*/
    uint8_t cmr:4;                  /* Indicates a codec mode request sent to the speech
                                                   encoder at the site of the receiver of this payload. */
#elif G_BYTE_ORDER == G_BIG_ENDIAN
    uint8_t cmr:4;                  /* Indicates a codec mode request sent to the speech
                                                   encoder at the site of the receiver of this payload. */
    uint8_t reserved:4;        /* reserved bits that MUST be set to zero.*/
#else
#error Neither big nor little
#endif
} amr_header;

static int amr_parse(Track *tr, uint8_t *data, size_t len)
{
    uint8_t packet[DEFAULT_MTU] = {0};
    amr_header *header = (amr_header *) packet;
    const uint32_t packet_size[] = {12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0};
    /*1(toc size) +  unit size of frame body{12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0}*/

    header->cmr = 0xf;

    while (len > 0) {
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
        off = 1 + frames; /* Write the body data at this offset */
        for (i = 0; i < frames; i++) {
            memcpy(&tocv, data, 1);
            body_len = packet_size[tocv.ft];
            tocv.f = i < frames - 1;
            memcpy(packet + 1 + i, &tocv, 1);
            memcpy(packet + off, &data[1], body_len);
            off  +=     body_len;
            data += 1 + body_len;
            len  -= 1 + body_len;
        }
        mparser_buffer_write(tr,
                             tr->properties.pts,
                             tr->properties.dts,
                             tr->properties.frame_duration,
                             0,
                             packet, off);
    }

    return 0;
}


FNC_LIB_MEDIAPARSER(amr);


