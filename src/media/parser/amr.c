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
#include "feng_utils.h"

static const MediaParserInfo info = {
    "amr",
    MP_audio
};

#define amr_uninit NULL

static int amr_init(Track *track)
{
    char *config = NULL;
    char *sdp_value;

    /*get config content if has*/
    if(track->properties.extradata_len)
    {
        config = extradata2config(&track->properties);
        if (!config)
            return ERR_PARSE;

        sdp_value = g_strdup_printf("octet-align=1; config=%s", config);
        g_free(config);
    } else {
        sdp_value =  g_strdup_printf("octet-align=1");
    }

    track_add_sdp_field(track, fmtp, sdp_value);

    track->properties.clock_rate = 8000;
    track_add_sdp_field(track, rtpmap,
                        g_strdup_printf ("AMR/%d",
                                         track->properties.clock_rate));

    return ERR_NOERROR;
}

/* AMR Payload Header (RFC3267)
    0  1 2 3 4 5   6 7
   +-+-+-+-+-+-+
   | CMR  |   RR     |
   +-+-+-+-+-+-+
   +-+-+-+-+-+-+
   |F|  FT    |Q  PP |
   +-+-+-+-+-+-+
*/

typedef struct
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    uint8_t p:2;       /* padding bits */
    uint8_t q:1;       /* Frame quality indicator. */
    uint8_t ft:4;      /* Frame type index */
    uint8_t f:1;       /*If set to 1, indicates that this frame is followed by
                                another speech frame in this payload; if set to 0, indicates that
                                this frame is the last frame in this payload. */
#elif G_BYTE_ORDER == G_BIG_ENDIAN
    uint8_t f:1;       /*If set to 1, indicates that this frame is followed by
                                another speech frame in this payload; if set to 0, indicates that
                                this frame is the last frame in this payload. */
    uint8_t ft:4;      /* Frame type index */
    uint8_t q:1;       /* Frame quality indicator. */
    uint8_t p:2;       /* padding bits */
#else
#error Neither big nor little
#endif
} toc;

typedef struct
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    uint8_t reserved:4;        /* reserved bits that MUST be set to zero.*/
    uint8_t cmr:4;                  /* Indicates a codec mode request sent to the speech
                                                   encoder at the site of the receiver of this payload. */
#elif G_BYTE_ORDER == G_BIG_ENDIAN
    uint8_t cmr:4;                  /* Indicates a codec mode request sent to the speech
                                                   encoder at the site of the receiver of this payload. */
    uint8_t reserved:4;        /* reserved bits that MUST be set to zero.*/
#else
#error Neither big nor little
#endif
} amr_header;

static int amr_parse(Track *tr, uint8_t *data, size_t len)
{
    uint32_t header_len, off = 1, payload, i, body_len, body_num = 0;
    uint8_t packet[DEFAULT_MTU] = {0};
    amr_header *header = (amr_header *) packet;
    toc tocv,end_tocv;
    const uint32_t packet_size[] = {12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0};
    /*1(toc size) +  unit size of frame body{12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0}*/

    header->cmr = 0xf;

   /*the first byte of data is 'table of content'*/
    /*'f' bit of the end_tocv should be 0*/
    memset(&end_tocv, *data, 1);
    memset(&tocv, *data, 1);
    tocv.f = 1;

    /*get frame body size by tocv and packet_size*/
    /*data len = 1 + frame body_len * frame body number*/
    body_len = packet_size[tocv.ft];

    /*get toc number and header_len, first assume data len > MTU size*/
    if(body_len > 0)
        body_num = (DEFAULT_MTU - 1) / body_len;
    header_len = body_num + 1;
    payload = DEFAULT_MTU - header_len;
    fnc_log(FNC_LOG_DEBUG, "AMR:data len: %d, frame body len: %d", len,body_len);

    if (len > payload) {
        /*fill toc to payload packet*/
        for(i = 1; i < body_num; i++)
        {
            memcpy(packet+ i, &tocv, 1);
        }
        memcpy(packet+body_num, &end_tocv, 1);

        /*fill the frame content*/
        while (len > payload) {
            memcpy(packet + header_len, data + off, payload);
            mparser_buffer_write(tr,
                                 tr->properties.pts,
                                 tr->properties.dts,
                                 tr->properties.frame_duration,
                                 0,
                                 packet, header_len + payload);

            len -= payload;
            off += payload;
        }
    }

    /*remaining frame, the length < payload*/
    /*get toc number and header_len*/
    if(body_len > 0)
        body_num = len / body_len;
    header_len = body_num + 1;
    /*fill toc to payload packet*/
    for(i = 1; i < body_num; i++)
    {
        memcpy(packet+ i, &tocv, 1);
    }
    memcpy(packet+body_num, &end_tocv, 1);

    /*fill the frame content*/
    memcpy(packet + header_len, data + off, len);
    mparser_buffer_write(tr,
                         tr->properties.pts,
                         tr->properties.dts,
                         tr->properties.frame_duration,
                         1,
                         packet, len + body_num);
    return ERR_NOERROR;
}


FNC_LIB_MEDIAPARSER(amr);


