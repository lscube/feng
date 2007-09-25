/* *
 *  This file is part of Feng
 *
 * Ogg Speex parser based on the current draft
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

#include <stdio.h>
#include <string.h>
#include <fenice/demuxer.h>
#include <fenice/mediaparser.h>
#include <fenice/mediaparser_module.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>
#include <fenice/debug.h>

static MediaParserInfo info = {
    "speex",
    MP_audio
};

static int speex_init(MediaProperties *properties, void **private_data)
{
    INIT_PROPS

    return ERR_NOERROR;
}

static int speex_get_frame2(uint8_t *dst, uint32_t dst_nbytes, double *timestamp, InputStream *istream, MediaProperties *properties, void *private_data)
{
    return ERR_PARSE;
}

static int speex_packetize(uint8_t *dst, uint32_t *dst_nbytes, uint8_t *src, uint32_t src_nbytes, MediaProperties *properties, void *private_data)
{
    return ERR_PARSE;
}

static int speex_parse(void *track, uint8_t *data, long len, uint8_t *extradata,
          long extradata_len)
{
    Track *tr = (Track *)track;
    int mtu = DEFAULT_MTU;

    if (len > mtu)
        return ERR_ALLOC;

    if (bp_write(tr->buffer, 0, tr->properties->mtime, 0, 0, data, len)) {
        fnc_log(FNC_LOG_ERR, "Cannot write bufferpool");
        return ERR_ALLOC;
    }

    return ERR_NOERROR;
}


static int speex_uninit(void *private_data)
{
    return ERR_NOERROR;
}

FNC_LIB_MEDIAPARSER(speex);

