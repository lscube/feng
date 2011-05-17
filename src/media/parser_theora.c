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

#include <libavutil/md5.h>

#define AV_RB16(x)  ((((uint8_t*)(x))[0] << 8) | ((uint8_t*)(x))[1])
static int ff_split_xiph_headers(uint8_t *extradata, int extradata_size,
                          int first_header_size, uint8_t *header_start[3],
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

//! Parse extradata and reformat it, most of the code is shamelessly ripped from fftheora.
static GByteArray *encode_header(uint8_t *data, int len, xiph_priv *priv)
{
    int headers_len;
    uint8_t *header_start[3];
    int header_len[3];
    int hash[4];
    static const uint8_t comment[] =
        /*quite minimal comment */
    { 0x81,'t','h','e','o','r','a',
      10,0,0,0,
      't','h','e','o','r','a','-','r','t','p',
      0,0,0,0};

    GByteArray *res = NULL;


    if (ff_split_xiph_headers(data, len, 42, header_start, header_len) < 0) {
        fnc_log(FNC_LOG_ERR, "[theora] Extradata corrupt. unknown layout");
        return res;
    }
    if (header_len[2] + header_len[0]>UINT16_MAX) {
        return res;
    }

    av_md5_sum((uint8_t *)hash, data, len);
    priv->ident = hash[0]^hash[1]^hash[2]^hash[3];

    // Envelope size
    headers_len = header_len[0] + sizeof(comment) + header_len[2];
    res = g_byte_array_sized_new(4 +                // count field
                                 3 +                // Ident field
                                 2 +                // pack size
                                 1 +                // headers count (2)
                                 2 +                // headers sizes
                                 headers_len);      // the rest

    res->data[0] = res->data[1] = res->data[2] = 0;
    res->data[3] = 1; //just one packet for now
    // new config
    res->data[4] = (priv->ident >> 16) & 0xff;
    res->data[5] = (priv->ident >> 8) & 0xff;
    res->data[6] = priv->ident & 0xff;
    res->data[7] = (headers_len)>>8;
    res->data[8] = (headers_len) & 0xff;
    res->data[9] = 2;
    res->data[10] = header_len[0];
    res->data[11] = sizeof(comment);
    memcpy(res->data + 12, header_start[0], header_len[0]);
    memcpy(res->data + 12 + header_len[0], comment, sizeof(comment));
    memcpy(res->data + 12 + header_len[0] + sizeof(comment), header_start[2],
           header_len[2]);
    return res;
}

int theora_init(Track *track)
{
    xiph_priv *priv = track->private_data = g_slice_new(xiph_priv);
    GByteArray *conf = encode_header(track->extradata, track->extradata_len,
                                     priv);

    if ( conf == NULL )
        goto err_alloc;

    xiph_sdp_descr_append(track, conf);

    return 0;

 err_alloc:
    g_slice_free(xiph_priv, priv);
    track->private_data = NULL;
    return -1;
}
