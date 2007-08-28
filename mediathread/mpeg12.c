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

/*mediaparser_module interface implementation*/
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

/* Proudly stolen without any shame from ff_rtp_send_mpegvideo (ffmpeg libavformat) */
static int mpv_parse(void *track, uint8_t *data, long len, uint8_t *extradata,
          long extradata_len)
{
    Track * tr = track;
    int h, b=1, e=0, mtu = DEFAULT_MTU;
    long rem;
    uint8_t dst[mtu];
    uint8_t *q = dst;
    rem = len;

    while (rem > 0) {
        len = mtu - 4;

        if (len >= rem) {
            len = rem;
            e = 1;
        }

        h = 0;
        h |= b << 12;
        h |= e << 11;
        q = dst;
        *q++ = h >> 24;
        *q++ = h >> 16;
        *q++ = h >> 8;
        *q++ = h;

        memcpy(q, data, len);
        q += len;

        if (bp_write(tr->buffer, 0, tr->properties->mtime, 0, (len == rem),
                     dst, q - dst)) {
            fnc_log(FNC_LOG_ERR, "Cannot write bufferpool");
            return ERR_ALLOC;
        }
        b = 0;
        data += len;
        rem -= len;
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
