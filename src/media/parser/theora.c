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

#include <libavutil/md5.h>

#if 1
#define AV_RB16(x)  ((((uint8_t*)(x))[0] << 8) | ((uint8_t*)(x))[1])
static int ff_split_xiph_headers(uint8_t *extradata, int extradata_size,
                          int first_header_size, uint8_t *header_start[3],
                          int header_len[3])
{
    int i, j;

    if (AV_RB16(extradata) == first_header_size) {
        for (i=0; i<3; i++) {
            header_len[i] = AV_RB16(extradata);
            extradata += 2;
            header_start[i] = extradata;
            extradata += header_len[i];
        }
    } else if (extradata[0] == 2) {
        for (i=0,j=1; i<2; i++,j++) {
            header_len[i] = 0;
            for (; j<extradata_size && extradata[j]==0xff; j++) {
                header_len[i] += 0xff;
            }
            if (j >= extradata_size)
                return -1;

            header_len[i] += extradata[j];
        }
        header_len[2] = extradata_size - header_len[0] - header_len[1] - j;
        extradata += j;
        header_start[0] = extradata;
        header_start[1] = header_start[0] + header_len[0];
        header_start[2] = header_start[1] + header_len[1];
    } else {
        return -1;
    }
    return 0;
}
#endif


static const MediaParserInfo info = {
    "theora",
    MP_video
};

typedef struct {
    int stacksize;
    int stackcount;
    unsigned char* framestack;
} framestack;

/// Private struct for Vorbis
typedef struct {
    uint8_t         *conf;      ///< current configuration
    unsigned int    conf_len;
    uint8_t         *packet;    ///< holds the incomplete packet
    unsigned int    len;        ///< incomplete packet length
    unsigned int    ident;    ///< identification string
//    framestack      stack; XXX use it later
} theora_priv;

//! Parse extradata and reformat it, most of the code is shamelessly ripped from fftheora.
static int encode_header(uint8_t *data, int len, theora_priv *priv)
{
    int headers_len;
    uint8_t *header_start[3];
    int header_len[3];
    int hash[4];
    uint8_t comment[] =
        /*quite minimal comment */
    { 0x81,'t','h','e','o','r','a',
      10,0,0,0,
      't','h','e','o','r','a','-','r','t','p',
      0,0,0,0};


    if (ff_split_xiph_headers(data, len, 42, header_start, header_len) < 0) {
        fnc_log(FNC_LOG_ERR, "[theora] Extradata corrupt. unknown layout");
        return -1;
    }
    if (header_len[2] + header_len[0]>UINT16_MAX) {
        return -1;
    }

    av_md5_sum((uint8_t *)hash, data, len);
    priv->ident = hash[0]^hash[1]^hash[2]^hash[3];

    // Envelope size
    headers_len = header_len[0] + sizeof(comment) + header_len[2];
    priv->conf_len = 4 +                // count field
                     3 +                // Ident field
                     2 +                // pack size
                     1 +                // headers count (2)
                     2 +                // headers sizes
                     headers_len;       // the rest

    priv->conf = g_malloc(priv->conf_len);
    priv->conf[0] = priv->conf[1] = priv->conf[2] = 0;
    priv->conf[3] = 1; //just one packet for now
    // new config
    priv->conf[4] = (priv->ident >> 16) & 0xff;
    priv->conf[5] = (priv->ident >> 8) & 0xff;
    priv->conf[6] = priv->ident & 0xff;
    priv->conf[7] = (headers_len)>>8;
    priv->conf[8] = (headers_len) & 0xff;
    priv->conf[9] = 2;
    priv->conf[10] = header_len[0];
    priv->conf[11] = sizeof(comment);
    memcpy(priv->conf + 12, header_start[0], header_len[0]);
    memcpy(priv->conf + 12 + header_len[0], comment, sizeof(comment));
    memcpy(priv->conf + 12 + header_len[0] + sizeof(comment), header_start[2],
           header_len[2]);
    return 0;
}

static void theora_uninit(Track *track)
{
    theora_priv *priv = track->private_data;
    if (!priv)
        return;

    g_free(priv->conf);
    g_free(priv->packet);
    g_slice_free(theora_priv, priv);
}

static int theora_init(Track *track)
{
    theora_priv *priv;
    char *buf;

    if(track->properties.extradata_len == 0)
        return ERR_ALLOC;

    priv = g_slice_new(theora_priv);

    if ( encode_header(track->properties.extradata,
                       track->properties.extradata_len, priv) ||
         (buf = g_base64_encode(priv->conf, priv->conf_len)) == NULL )
        goto err_alloc;

    track_add_sdp_field(track, fmtp,
                        g_strdup_printf("delivery-method=in_band; configuration=%s;",
                                        buf));
    g_free(buf);

    track_add_sdp_field(track, rtpmap,
                        g_strdup_printf ("theora/%d",
                                         track->properties.clock_rate));

    track->private_data = priv;

    return ERR_NOERROR;

 err_alloc:
    g_free(priv->conf);
    g_free(priv->packet);
    g_slice_free(theora_priv, priv);
    return ERR_ALLOC;
}

#define XIPH_HEADER_SIZE 6
static int theora_parse(Track *tr, uint8_t *data, size_t len)
{
    //XXX handle the last packet on EOF
    theora_priv *priv = tr->private_data;
    int frag, off = 0;
    uint32_t payload = DEFAULT_MTU - XIPH_HEADER_SIZE;
    uint8_t *packet = g_malloc0(DEFAULT_MTU);

    if(!packet) return ERR_ALLOC;

    // the ident is always the same
    packet[0] = (priv->ident>>16)& 0xff;
    packet[1] = (priv->ident>>8) & 0xff;
    packet[2] =  priv->ident     & 0xff;

    if (len > payload) {
        frag = 1; // first frag
        // this is always the same
//        packet[3] |= 0; //frames in packet
        packet[4] = (payload>>8)&0xff;
        packet[5] = payload&0xff;
        while (len > payload) {
            packet[3] = frag << 6;  //frag type
//            packet[3] |= 0 << 4; //data type
            memcpy(packet + XIPH_HEADER_SIZE, data + off, payload);
            mparser_buffer_write(tr,
                                 tr->properties.pts,
                                 tr->properties.dts,
                                 tr->properties.frame_duration,
                                 0,
                                 packet, DEFAULT_MTU);

            len -= payload;
            off += payload;
            frag = 2; // middle frag
        }
        packet[3] = 3 << 6; // last frag
    } else {
//        frag = 0; // no frag
        packet[3] |= 1; //frames in packet
    }

    packet[4] = (len>>8)&0xff;
    packet[5] = len&0xff;
    memcpy(packet + XIPH_HEADER_SIZE, data + off, len);
    mparser_buffer_write(tr,
                         tr->properties.pts,
                         tr->properties.dts,
                         tr->properties.frame_duration,
                         1,
                         packet, len + XIPH_HEADER_SIZE);

    g_free(packet);
    return ERR_NOERROR;
}


FNC_LIB_MEDIAPARSER(theora);

