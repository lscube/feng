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
    "aac",
    MP_audio
};

#define aac_uninit NULL

static int aac_init(Track *track)
{
    char *config;
    char *sdp_value;

    if ( (config = extradata2config(&track->properties)) == NULL )
        return ERR_PARSE;

    sdp_value = g_strdup_printf("streamtype=5;profile-level-id=1;"
                                "mode=AAC-hbr;sizeLength=13;indexLength=3;"
                                "indexDeltaLength=3; config=%s;", config);
    g_free(config);

    track_add_sdp_field(track, fmtp, sdp_value);

    sdp_value = g_strdup_printf ("mpeg4-generic/%d",
                                 track->properties.clock_rate);
    track_add_sdp_field(track, rtpmap, sdp_value);

    return ERR_NOERROR;
}

#define AU_HEADER_SIZE 4

/* au header
      +---------------------------------------+
      |     AU-size                           |
      +---------------------------------------+
      |     AU-Index / AU-Index-delta         |
      +---------------------------------------+
      |     CTS-flag                          |
      +---------------------------------------+
      |     CTS-delta                         |
      +---------------------------------------+
      |     DTS-flag                          |
      +---------------------------------------+
      |     DTS-delta                         |
      +---------------------------------------+
      |     RAP-flag                          |
      +---------------------------------------+
      |     Stream-state                      |
      +---------------------------------------+
*/

//XXX implement aggregation
//#define AAC_EXTRA 7
static int aac_parse(Track *tr, uint8_t *data, size_t len)
{
    //XXX handle the last packet on EOF
    int off = 0;
    uint32_t payload = DEFAULT_MTU - AU_HEADER_SIZE;
    uint8_t *packet = g_malloc0(DEFAULT_MTU);

    if(!packet) return ERR_ALLOC;

// trim away extradata
//    data += AAC_EXTRA;
//    len -= AAC_EXTRA;

    packet[0] = 0x00;
    packet[1] = 0x10;
    packet[2] = (len & 0x1fe0) >> 5;
    packet[3] = (len & 0x1f) << 3;

    if (len > payload) {
        while (len > payload) {
            memcpy(packet + AU_HEADER_SIZE, data + off, payload);
            mparser_buffer_write(tr,
                                 tr->properties.pts,
                                 tr->properties.dts,
                                 tr->properties.frame_duration,
                                 0,
                                 packet, DEFAULT_MTU);

            len -= payload;
            off += payload;
        }
    }

    memcpy(packet + AU_HEADER_SIZE, data + off, len);
    mparser_buffer_write(tr,
                         tr->properties.pts,
                         tr->properties.dts,
                         tr->properties.frame_duration,
                         1,
                         packet, len + AU_HEADER_SIZE);

    g_free(packet);
    return ERR_NOERROR;
}


FNC_LIB_MEDIAPARSER(aac);

