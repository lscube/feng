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

typedef struct {
    int stacksize;
    int stackcount;
    unsigned char* framestack;
} framestack;

//! Parse extradata and reformat it, most of the code is shamelessly ripped from ffvorbis.
static char *encode_header(uint8_t *headers, int len, xiph_priv *priv)
{
    uint8_t *header_start[3];
    int header_len[3], i, j;
    static const uint8_t comment[26] =
        /*quite minimal comment */
    { 3, 'v', 'o', 'r', 'b', 'i', 's',
        10, 0, 0, 0,
        'v', 'o', 'r', 'b', 'i', 's', '-', 'r', 't', 'p',
        0, 0, 0, 0,
        1
    };

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
            while(j<len && headers[j]==0xff) {
                header_len[i]+=0xff;
                ++j;
            }
            if (j>=len) {
                fnc_log(FNC_LOG_ERR, "[vorbis] Extradata corrupt.");
                return NULL;
            }
            header_len[i]+=headers[j];
        }
        header_len[2] = len - header_len[0] - header_len[1] - j;
        headers += j;
        header_start[0] = headers;
        header_start[1] = header_start[0] + header_len[0];
        header_start[2] = header_start[1] + header_len[1];
    } else {
        fnc_log(FNC_LOG_ERR, "[vorbis] Extradata corrupt.");
        return NULL;
    }

    return xiph_header_to_conf(priv, headers, len, header_start, header_len, comment, sizeof(comment));
}

int vorbis_init(Track *track)
{
    xiph_priv *priv = track->private_data = g_slice_new(xiph_priv);
    char *conf = encode_header(track->extradata, track->extradata_len,
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
