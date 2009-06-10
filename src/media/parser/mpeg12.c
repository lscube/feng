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

#include <string.h>
#include "media/demuxer.h"
#include "media/mediaparser.h"
#include "media/mediaparser_module.h"
#include "feng_utils.h"

static const MediaParserInfo info = {
    "MPV",
    MP_video
};

#if 1
// Stolen from ffmpeg (mpegvideo.c)
static uint8_t *find_start_code(uint8_t *p, uint8_t *end, uint32_t *state)
{
    int i;

    if(p>=end)
        return end;

    for(i=0; i<3; i++){
        uint32_t tmp= *state << 8;
        *state= tmp + *(p++);
        if(tmp == 0x100 || p==end)
            return p;
    }

    while(p<end){
        if     (p[-1] > 1      ) p+= 3;
        else if(p[-2]          ) p+= 2;
        else if(p[-3]|(p[-1]-1)) p++;
        else{
            p++;
            break;
        }
    }

    p= MIN(p, end)-4;
    *state= (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);

    return p+4;
}
#endif

static int mpv_init(ATTR_UNUSED Track *track)
{
    return ERR_NOERROR;
}

/* Source code taken from ff_rtp_send_mpegvideo (ffmpeg libavformat) and
 * modified to be fully rfc 2250 compliant
 */
static int mpv_parse(Track *tr, uint8_t *data, size_t len)
{
    int h, b = 1, e = 0 , ffc = 0, ffv = 0, fbv = 0, bfc = 0;
    int frame_type = 0, temporal_reference = 0, begin_of_sequence = 0;
    long rem = len, payload;
    uint8_t dst[DEFAULT_MTU];
    uint8_t *q = dst;
    uint8_t *r, *r1 = data;
    uint8_t *end = data + len;
    uint32_t start_code;

    while (1) {
        start_code = -1;
        r = find_start_code(r1, end, &start_code);
        if ((start_code & 0xffffff00) == 0x100) {
            /* New start code found */
            if (start_code == 0x100) {
                frame_type = (r[1] & 0x38)>> 3;
                temporal_reference = (int)r[0] << 2 | r[1] >> 6;
                if (frame_type == 2 || frame_type == 3) {
                    ffv = (r[3] & 0x04)>> 2;
                    ffc = (r[3] & 0x03)<< 1 | (r[4] & 0x80)>> 7;
                }
                if (frame_type == 3) {
                    fbv = (r[4] & 0x40)>> 6;
                    bfc = (r[4] & 0x38)>> 3;
                }
            }
            if (start_code == 0x1b3) {
                begin_of_sequence = 1;
            }
            r1 = r;
        } else {
            break;
        }
    }

    while (rem > 0) {
        payload = DEFAULT_MTU - 4;

        if (payload >= rem) {
            payload = rem;
            e = 1;
        } else {
            r1 = data;
            while (1) {
                start_code = -1;
                r = find_start_code(r1, end, &start_code);
                if ((start_code & 0xffffff00) == 0x100) {
                    /* New start code found */
                    if (r - data < payload) {
                        /* The current slice fits in the packet */
                        if (b == 0) {
                            /* no slice at the beginning of the packet... */
                            e = 1;
                            payload = r - data - 4;
                            break;
                        }
                        r1 = r;
                    } else {
                        if (r - r1 < DEFAULT_MTU) {
                            payload = r1 - data - 4;
                            e = 1;
                        }
                        break;
                    }
                } else {
                    break;
                }
            }
        }

        if (payload>0) {
            h = 0;
            h |= temporal_reference << 16;
            h |= begin_of_sequence << 13;
            h |= b << 12;
            h |= e << 11;
            h |= frame_type << 8;
            h |= fbv << 7;
            h |= bfc << 4;
            h |= ffv << 3;
            h |= ffc;

            q = dst;
            *q++ = h >> 24;
            *q++ = h >> 16;
            *q++ = h >> 8;
            *q++ = h;

            memcpy(q, data, payload);
            q += payload;

            mparser_buffer_write(tr,
                                 tr->properties.pts,
                                 tr->properties.dts,
                                 tr->properties.frame_duration,
                                 (payload == rem),
                                 dst, q - dst);
            b = e;
            e = 0;
            data += payload;
            rem -= payload;
            begin_of_sequence = 0;
        } else rem = 0;
    }
    return ERR_NOERROR;
}

#define mpv_uninit NULL

FNC_LIB_MEDIAPARSER(mpv);
