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

typedef struct {
    int stacksize;
    int stackcount;
    unsigned char* framestack;
} framestack;

//! Parse extradata and reformat it, most of the code is shamelessly ripped from ffvorbis.
static GByteArray *encode_header(uint8_t *data, int len, xiph_priv *priv)
{
    uint8_t *headers = data;
    int headers_len = len, i , j;
    uint8_t *header_start[3];
    int header_len[3];
    int hash[4];
    static const uint8_t comment[26] =
        /*quite minimal comment */
    { 3, 'v', 'o', 'r', 'b', 'i', 's',
        10, 0, 0, 0,
        'v', 'o', 'r', 'b', 'i', 's', '-', 'r', 't', 'p',
        0, 0, 0, 0,
        1
    };

    GByteArray *res = NULL;

// old way.
    if(headers[0] == 0 && headers[1] == 30) {
        for(i = 0; i < 3; i++){
            header_len[i] = *headers++ << 8;
            header_len[i] += *headers++;
            header_start[i] = headers;
            headers += header_len[i];
        }
// xiphlaced
    } else if(headers[0] == 2) {
        for(j=1, i=0; i<2 ; ++i, ++j) {
            header_len[i]=0;
            while(j<headers_len && headers[j]==0xff) {
                header_len[i]+=0xff;
                ++j;
            }
            if (j>=headers_len) {
                fnc_log(FNC_LOG_ERR, "[vorbis] Extradata corrupt.");
                return res;
            }
            header_len[i]+=headers[j];
        }
        header_len[2] = headers_len - header_len[0] - header_len[1] - j;
        headers += j;
        header_start[0] = headers;
        header_start[1] = header_start[0] + header_len[0];
        header_start[2] = header_start[1] + header_len[1];
    } else {
        fnc_log(FNC_LOG_ERR, "[vorbis] Extradata corrupt.");
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
    res->data[10] = header_len[0];     // 30, always
    res->data[11] = sizeof(comment);   // 26
    memcpy(res->data + 12, header_start[0], header_len[0]);
    memcpy(res->data + 12 + header_len[0], comment, sizeof(comment));
    memcpy(res->data + 12 + header_len[0] + sizeof(comment), header_start[2],
           header_len[2]);
    return res;
}

int vorbis_init(Track *track)
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
