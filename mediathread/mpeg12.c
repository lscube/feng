/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
 * See AUTHORS for more details 
 *  
 * Feng is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Feng is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *  
 * */

#include <string.h>
#include <fenice/mpeg.h>
#include <fenice/demuxer.h>
#include <fenice/fnc_log.h>
#include <fenice/mediaparser.h>
#include <fenice/mediaparser_module.h>
#include <fenice/utils.h>

static MediaParserInfo info = {
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

static int mpv_init(MediaProperties *properties, void **private_data)
{
    INIT_PROPS

    return ERR_NOERROR;
}

static int mpv_get_frame2(uint8_t *dst, uint32_t dst_nbytes, double *timestamp, InputStream *istream, MediaProperties *properties, void *private_data)
{
    return ERR_PARSE;
}

static int mpv_packetize(uint8_t *dst, uint32_t *dst_nbytes, uint8_t *src, uint32_t src_nbytes, MediaProperties *properties, void *private_data)
{
    return ERR_PARSE;
}

/* Source code taken from ff_rtp_send_mpegvideo (ffmpeg libavformat) and 
 * modified to be fully rfc 2250 compliant 
 */
static int mpv_parse(void *track, uint8_t *data, long len, uint8_t *extradata,
          long extradata_len)
{
    Track * tr = track;
    int h, b = 1, e = 0 , ffc = 0, ffv = 0, fbv = 0, bfc = 0, mtu = DEFAULT_MTU;
    int frame_type = 0, temporal_reference = 0, begin_of_sequence = 0;
    long rem = len, payload;
    uint8_t dst[mtu];
    uint8_t *q = dst;
    uint8_t *r, *r1 = data;
    uint32_t start_code;

    while (1) {
        start_code = -1;
        r = find_start_code(r1, data + len, &start_code);
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
        payload = mtu - 4;

        if (payload >= rem) {
            payload = rem;
            e = 1;
        } else {
            r1 = data;
            while (1) {
                start_code = -1;
                r = find_start_code(r1, data + len, &start_code);
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
                        if (r - r1 < mtu) {
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

        if (bp_write(tr->buffer, 0, tr->properties->mtime, 0, (payload == rem),
                     dst, q - dst)) {
            fnc_log(FNC_LOG_ERR, "Cannot write bufferpool");
            return ERR_ALLOC;
        }
        b = e;
        e = 0;
        data += payload;
        rem -= payload;
        begin_of_sequence = 0;
    }
    return ERR_NOERROR;
}


int mpv_uninit(void *private_data)
{
    if (private_data)
        free(private_data);
    return ERR_NOERROR;
}

FNC_LIB_MEDIAPARSER(mpv);
