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

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include "media/demuxer.h"
#include "media/mediaparser.h"
#include "media/mediaparser_module.h"
#include "feng_utils.h"
#include "fnc_log.h"

static const MediaParserInfo info = {
    "MPA",
    MP_audio
};

static int mpa_init(MediaProperties *properties,
                    ATTR_UNUSED void **private_data)
{
    INIT_PROPS

    return 0;
}

static int mpa_parse(void *track, uint8_t *data, long len)
{
    Track *tr = (Track *)track;
    uint32_t mtu = DEFAULT_MTU; //FIXME get it from SETUP
    int32_t offset;
    uint8_t dst[mtu];
    long rem = len;

    if (mtu >= len + 4) {
        memset (dst, 0, 4);
        memcpy (dst + 4, data, len);
        mparser_buffer_write(tr,
                             tr->properties->pts,
                             tr->properties->dts,
                             tr->properties->frame_duration,
                             1,
                             dst, len + 4);
        fnc_log(FNC_LOG_VERBOSE, "[mp3] no frags");
    } else {
        do {
            offset = len - rem;
            if (offset & 0xffff0000) return ERR_ALLOC;
            memcpy (dst + 4, data + offset, MIN(mtu - 4, rem));
            offset = htonl(offset & 0xffff);
            memcpy (dst, &offset, 4);

            mparser_buffer_write(tr,
                                 tr->properties->pts,
                                 tr->properties->dts,
                                 tr->properties->frame_duration,
                                 0,
                                 dst, MIN(mtu, rem + 4));
            rem -= mtu - 4;
            fnc_log(FNC_LOG_VERBOSE, "[mp3] frags");
        } while (rem >= 0);
    }
    fnc_log(FNC_LOG_VERBOSE, "[mp3]Frame completed");
    return ERR_NOERROR;
}

#define mpa_uninit NULL

FNC_LIB_MEDIAPARSER(mpa);

