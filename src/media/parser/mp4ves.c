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

#include "media/demuxer.h"
#include "media/mediaparser.h"
#include "media/mediaparser_module.h"
#include "fnc_log.h"

static const MediaParserInfo info = {
    "mp4v-es",
    MP_video
};

#define mp4ves_uninit NULL

static int mp4ves_init(Track *track)
{
    char *config;
    char *sdp_value;

    if ( (config = extradata2config(&track->properties)) == NULL )
        return ERR_PARSE;

    track_add_sdp_field(track, fmtp,
                        g_strdup_printf("profile-level-id=1;config=%s;",
                                        config));

    track_add_sdp_field(track, rtpmap,
                        g_strdup_printf ("MP4V-ES/%d",
                                         track->properties.clock_rate));

    g_free(config);

    return ERR_NOERROR;
}

static int mp4ves_parse(Track *tr, uint8_t *data, long len)
{
    uint32_t mtu = DEFAULT_MTU;
    int32_t offset, rem = len;

#if 0
    uint32_t start_code = 0, ptr = 0;
    while ((ptr = (uint32_t) find_start_code(data + ptr, data + len, &start_code)) < (uint32_t) data + len ) {
        ptr -= (uint32_t) data;
        fnc_log(FNC_LOG_DEBUG, "[mp4v] start code offset %d", ptr);
    }
    fnc_log(FNC_LOG_DEBUG, "[mp4v]no more start codes");
#endif

    if (mtu >= len) {
        mparser_buffer_write(tr,
                             tr->properties.pts,
                             tr->properties.dts,
                             tr->properties.frame_duration,
                             1,
                             data, len);
        fnc_log(FNC_LOG_VERBOSE, "[mp4v] no frags");
    } else {
        do {
            offset = len - rem;
            mparser_buffer_write(tr,
                                 tr->properties.pts,
                                 tr->properties.dts,
                                 tr->properties.frame_duration,
                                 (rem <= mtu),
                                 data + offset, MIN(rem, mtu));
            rem -= mtu;
            fnc_log(FNC_LOG_VERBOSE, "[mp4v] frags");
        } while (rem >= 0);
    }
    fnc_log(FNC_LOG_VERBOSE, "[mp4v]Frame completed");
    return ERR_NOERROR;
}

FNC_LIB_MEDIAPARSER(mp4ves);
