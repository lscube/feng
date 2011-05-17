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

#include <libavutil/md5.h>

#include "media/media.h"
#include "fnc_log.h"

#define HEADER_SIZE 6
#define MAX_PAYLOAD_SIZE (DEFAULT_MTU - HEADER_SIZE)

int xiph_parse(Track *tr, uint8_t *data, size_t len)
{
    uint8_t fragment = 0;

    do {
        uint16_t payload_size;

        struct MParserBuffer *buffer = g_slice_new0(struct MParserBuffer);

        if ( fragment == 0 && len <= MAX_PAYLOAD_SIZE )
            fragment = 1;
        else if ( fragment == 0 )
            fragment = 1 << 6; /* first frag */
        else if ( len > MAX_PAYLOAD_SIZE )
            fragment = 2 << 6; /* middle frag */
        else
            fragment = 3 << 6; /* max frag */

        buffer->timestamp = tr->pts;
        buffer->delivery = tr->dts;
        buffer->duration = tr->frame_duration;
        buffer->marker = (len <= MAX_PAYLOAD_SIZE);

        buffer->data_size = MIN(MAX_PAYLOAD_SIZE, len) + HEADER_SIZE;
        buffer->data = g_malloc(buffer->data_size);

        payload_size = htons(buffer->data_size);

        /* 0..2 */
        memcpy(buffer->data, tr->private_data, 3);
        /* 3 */
        buffer->data[3] = fragment;
        /* 4..5 */
        memcpy(buffer->data + 4, &payload_size, sizeof(payload_size));
        /* 6.. */
        memcpy(buffer->data + HEADER_SIZE, data,
               buffer->data_size - HEADER_SIZE);

        track_write(tr, buffer);

        len -= MAX_PAYLOAD_SIZE;
        data += MAX_PAYLOAD_SIZE;
    } while(len > 0);

    return 0;
}

#define AV_RB16(x)  ((((uint8_t*)(x))[0] << 8) | ((uint8_t*)(x))[1])
static int ff_split_xiph_headers(const uint8_t *extradata, int extradata_size,
                                 int first_header_size, const uint8_t *header_start[3],
                                 int header_len[3])
{
    int i, j;

    if (AV_RB16(extradata) == first_header_size) {
        for (i=0; i<3; i++) {
            header_len[i] = AV_RB16(extradata);
            extradata += 2;
            header_start[i] = extradata;
            extradata += header_len[i];
        }
    } else if (extradata[0] == 2) {
        for (i=0,j=1; i<2; i++,j++) {
            header_len[i] = 0;
            for (; j<extradata_size && extradata[j]==0xff; j++) {
                header_len[i] += 0xff;
            }
            if (j >= extradata_size)
                return -1;

            header_len[i] += extradata[j];
        }
        header_len[2] = extradata_size - header_len[0] - header_len[1] - j;
        extradata += j;
        header_start[0] = extradata;
        header_start[1] = header_start[0] + header_len[0];
        header_start[2] = header_start[1] + header_len[1];
    } else {
        return -1;
    }
    return 0;
}

static char *xiph_header_to_conf(uint8_t *ident_bytes,
                                 const uint8_t *headers,
                                 const size_t len,
                                 const int first_header_size,
                                 const uint8_t *comment,
                                 const size_t comment_len)
{
    int hash[4];
    uint8_t *conf;
    char *conf_base64;
    size_t headers_len, conf_len;
    const uint8_t *header_start[3];
    int header_len[3];
    uint32_t ident;

    if (ff_split_xiph_headers(headers, len, first_header_size, header_start, header_len) < 0) {
        fnc_log(FNC_LOG_ERR, "[xiph] extradata corrupt. unknown layout");
        return NULL;
    }

    if (header_len[2] + header_len[0] > UINT16_MAX)
        return NULL;

    av_md5_sum((uint8_t *)hash, headers, len);
    ident = hash[0]^hash[1]^hash[2]^hash[3];

    ident_bytes[0] = (ident >> 16) & 0xff;
    ident_bytes[1] = (ident >> 8)  & 0xff;
    ident_bytes[2] =  ident        & 0xff;

    // Envelope size
    headers_len = header_len[0] + comment_len + header_len[2];
    conf_len =
        4 +                // count field
        3 +                // Ident field
        2 +                // pack size
        1 +                // headers count (2)
        2 +                // headers sizes
        headers_len;      // the rest

    conf = g_malloc(conf_len);
    conf[0] = 0;
    conf[1] = 0;
    conf[2] = 0;
    conf[3] = 1; //just one packet for now
    conf[4] = ident_bytes[0];
    conf[5] = ident_bytes[1];
    conf[6] = ident_bytes[2];
    conf[7] = (headers_len)>>8;
    conf[8] = (headers_len) & 0xff;
    conf[9] = 2;
    conf[10] = header_len[0];
    conf[11] = comment_len;
    memcpy(conf + 12, header_start[0], header_len[0]);
    memcpy(conf + 12 + header_len[0], comment, comment_len);
    memcpy(conf + 12 + header_len[0] + comment_len, header_start[2],
           header_len[2]);

    conf_base64 = g_base64_encode(conf, conf_len);
    g_free(conf);

    return conf_base64;
}

static gboolean xiph_init(Track *track,
                                      int first_header_len,
                                      const uint8_t *comment,
                                      size_t comment_len)
{
    char *conf;

    track->private_data = g_malloc0(4); /* 3 bytes of ident and one for termination */

    conf = xiph_header_to_conf(track->private_data,
                               track->extradata, track->extradata_len,
                               first_header_len,
                               comment, comment_len);

    if ( conf == NULL )
        return false;

    sdp_descr_append_rtpmap(track);
    g_string_append_printf(track->sdp_description,
                           "a=fmtp:%u delivery-method=in_band; configuration=%s;\r\n",

                           /* fmtp */
                           track->payload_type,
                           conf);
    g_free(conf);

    return true;
}

int theora_init(Track *track)
{
    static const uint8_t comment[25] = {
        0x81, 't', 'h', 'e', 'o', 'r', 'a',
        10, 0, 0, 0,
        't', 'h', 'e', 'o', 'r', 'a', '-', 'r', 't', 'p',
        0, 0, 0, 0
    };

    return xiph_init(track, 42, comment, sizeof(comment)) ? 0 : -1;
}

int vorbis_init(Track *track)
{
    static const uint8_t comment[26] ={
        3, 'v', 'o', 'r', 'b', 'i', 's',
        10, 0, 0, 0,
        'v', 'o', 'r', 'b', 'i', 's', '-', 'r', 't', 'p',
        0, 0, 0, 0,
        1
    };

    return xiph_init(track, 42, comment, sizeof(comment)) ? 0 : -1;
}
