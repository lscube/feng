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
#include "fnc_log.h"

static int mpa_init(ATTR_UNUSED Track *track)
{
    return 0;
}

static int mpa_parse(Track *tr, uint8_t *data, size_t len)
{
    int32_t offset;
    uint8_t *dst = g_slice_alloc0(DEFAULT_MTU);
    ssize_t rem = len;

    if (DEFAULT_MTU >= len + 4) {
        memset (dst, 0, 4);
        memcpy (dst + 4, data, len);
        mparser_buffer_write(tr,
                             tr->properties.pts,
                             tr->properties.dts,
                             tr->properties.frame_duration,
                             1,
                             dst, len + 4);
        fnc_log(FNC_LOG_VERBOSE, "[mp3] no frags");
    } else {
        do {
            offset = len - rem;
            if (offset & 0xffff0000) return -1;
            memcpy (dst + 4, data + offset, MIN(DEFAULT_MTU - 4, rem));
            offset = htonl(offset & 0xffff);
            memcpy (dst, &offset, 4);

            mparser_buffer_write(tr,
                                 tr->properties.pts,
                                 tr->properties.dts,
                                 tr->properties.frame_duration,
                                 0,
                                 dst, MIN(DEFAULT_MTU, rem + 4));
            rem -= DEFAULT_MTU - 4;
            fnc_log(FNC_LOG_VERBOSE, "[mp3] frags");
        } while (rem >= 0);
    }
    fnc_log(FNC_LOG_VERBOSE, "[mp3]Frame completed");

    g_slice_free1(DEFAULT_MTU, dst);
    return 0;
}

#define mpa_uninit NULL

FENG_MEDIAPARSER(mpa, "MPA", MP_audio);
