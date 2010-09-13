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
#include "fnc_log.h"

#define mp4ves_uninit NULL

static int mp4ves_init(Track *track)
{
    char *config;

    if ( (config = extradata2config(&track->properties)) == NULL )
        return -1;

    g_string_append_printf(track->sdp_description,
                           "a=rtpmap:%u MP4V-ES/%d\r\n"
                           "a=fmtp:%u profile-level-id=1;config=%s;\r\n",

                           /* rtpmap */
                           track->properties.payload_type,
                           track->properties.clock_rate,

                           /* fmtp */
                           track->properties.payload_type,
                           config);

    g_free(config);

    return 0;
}

static int mp4ves_parse(Track *tr, uint8_t *data, size_t len)
{
    int32_t offset, rem = len;

    if (DEFAULT_MTU >= len) {
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
                                 (rem <= DEFAULT_MTU),
                                 data + offset, MIN(rem, DEFAULT_MTU));
            rem -= DEFAULT_MTU;
            fnc_log(FNC_LOG_VERBOSE, "[mp4v] frags");
        } while (rem >= 0);
    }
    fnc_log(FNC_LOG_VERBOSE, "[mp4v]Frame completed");
    return 0;
}

FENG_MEDIAPARSER(mp4ves, "mp4v-es", MP_video);
