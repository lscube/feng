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

#include "demuxer.h"
#include "fnc_log.h"
#include "mediaparser.h"
#include "mediaparser_module.h"
#include "mediautils.h"

static const MediaParserInfo info = {
    "aac",
    MP_audio
};

static int aac_uninit(void *private_data)
{
    return ERR_NOERROR;
}

static int aac_init(MediaProperties *properties, void **private_data)
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
        g_strdup_printf("streamtype=5;profile-level-id=1;"
                        "mode=AAC-hbr;sizeLength=13;indexLength=3;"
                        "indexDeltaLength=3; config=%s;", config);
    g_free(config);

    properties->sdp_private =
        g_list_prepend(properties->sdp_private, sdp_private);

    sdp_private = g_new(sdp_field, 1);
    sdp_private->type = rtpmap;
    sdp_private->field = g_strdup_printf ("mpeg4-generic/%d",
                                            properties->clock_rate);

    properties->sdp_private =
        g_list_prepend(properties->sdp_private, sdp_private);

    INIT_PROPS

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
static int aac_parse(void *track, uint8_t *data, long len, uint8_t *extradata,
                 long extradata_len)
{
    //XXX handle the last packet on EOF
    Track *tr = (Track *)track;
    int off = 0;
    uint32_t mtu = DEFAULT_MTU;  //FIXME get it from SETUP
    uint32_t payload = mtu - AU_HEADER_SIZE;
    uint8_t *packet = g_malloc0(mtu);

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
            mparser_buffer_write(tr, tr->properties->mtime,
                                 tr->properties->frame_duration,
                                 0,
                                 packet, mtu);

            len -= payload;
            off += payload;
        }
    }

    memcpy(packet + AU_HEADER_SIZE, data + off, len);
    mparser_buffer_write(tr, tr->properties->mtime,
                         tr->properties->frame_duration,
                         1,
                         packet, len + AU_HEADER_SIZE);

    g_free(packet);
    return ERR_NOERROR;

    err_alloc:
    g_free(packet);
    return ERR_ALLOC;
}


FNC_LIB_MEDIAPARSER(aac);

