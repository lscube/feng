/* *
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

#include <fenice/demuxer.h>
#include <fenice/fnc_log.h>
#include <fenice/mediaparser.h>
#include <fenice/mediaparser_module.h>
#include <fenice/utils.h>

static const MediaParserInfo info = {
    "mp4v-es",
    MP_video
};

static int mp4ves_uninit(void *private_data)
{
    return ERR_NOERROR;
}

static int mp4ves_init(MediaProperties *properties, void **private_data)
{
    sdp_field *sdp_private;
    char *config;


    if(!properties->extradata_len) {
        fnc_log(FNC_LOG_ERR, "No extradata!");
        return ERR_ALLOC;
    }

    sdp_private = g_new(sdp_field, 1);
    sdp_private->type = fmtp;

    config = extradata2config(properties->extradata,
                              properties->extradata_len);
    if (!config) return ERR_PARSE;

    sdp_private->field = 
        g_strdup_printf("profile-level-id=1;config=%s;", config);
    free(config);

    properties->sdp_private =
        g_list_prepend(properties->sdp_private, sdp_private);

    sdp_private = g_new(sdp_field, 1);
    sdp_private->type = rtpmap;
    sdp_private->field = g_strdup_printf ("MP4V-ES/%d", properties->clock_rate);

    properties->sdp_private =
        g_list_prepend(properties->sdp_private, sdp_private);

    INIT_PROPS

    return ERR_NOERROR;
}

static int mp4ves_get_frame2(uint8_t *dst, uint32_t dst_nbytes, double *timestamp,
                      InputStream *istream, MediaProperties *properties,
                      void *private_data)
{
    return ERR_PARSE;
}

static int mp4ves_packetize(uint8_t *dst, uint32_t *dst_nbytes, uint8_t *src, uint32_t src_nbytes, MediaProperties *properties, void *private_data)
{
    return ERR_PARSE;
}

static int mp4ves_parse(void *track, uint8_t *data, long len, uint8_t *extradata,
                 long extradata_len)
{
    Track *tr = (Track *)track;
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
        if (bp_write(tr->buffer, 0, tr->properties->mtime, 0, 1,
                              data, len)) {
                fnc_log(FNC_LOG_ERR, "Cannot write bufferpool");
                return ERR_ALLOC;
        }
        fnc_log(FNC_LOG_VERBOSE, "[mp4v] no frags");
    } else {
        do {
            offset = len - rem;
            if (bp_write(tr->buffer, 0, tr->properties->mtime, 0, (rem <= mtu),
                                  data + offset, min(rem, mtu))) {
                fnc_log(FNC_LOG_ERR, "Cannot write bufferpool");
                return ERR_ALLOC;
            }
            rem -= mtu;
            fnc_log(FNC_LOG_VERBOSE, "[mp4v] frags");
        } while (rem >= 0);
    }
    fnc_log(FNC_LOG_VERBOSE, "[mp4v]Frame completed");
    return ERR_NOERROR;
}

FNC_LIB_MEDIAPARSER(mp4ves);
