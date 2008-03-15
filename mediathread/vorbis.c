/* * 
 *  $Id: $
 *  
 *  This file is part of Feng
 *
 *  Feng -- Standard Streaming Server
 *  vorbis parser based on the current draft
 *
 *  Copyright (C) 2007 by Luca Barbato <lu_zero@gentoo.org>
 *
 *  Feng is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Feng is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Feng; if not, write to the Free Software
 *  This file is part of Feng
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

#include <libavutil/md5.h>

#ifdef HAVE_AVUTIL
#include <libavutil/base64.h>
#else
static inline char *av_base64_encode(char * buf, int buf_len,
                                     uint8_t * src, int len)
{
    static const char b64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char *ret, *dst;
    unsigned i_bits = 0;
    int i_shift = 0;
    int bytes_remaining = len;

    if (len >= UINT_MAX / 4 ||
        buf_len < len * 4 / 3 + 12)
        return NULL;
    ret = dst = buf;
    if (len) {  // special edge case, what should we really do here?
        while (bytes_remaining) {
            i_bits = (i_bits << 8) + *src++;
            bytes_remaining--;
            i_shift += 8;

            do {
                *dst++ = b64[(i_bits << 6 >> i_shift) & 0x3f];
                i_shift -= 6;
            } while (i_shift > 6 || (bytes_remaining == 0 && i_shift > 0));
        }
        while ((dst - ret) & 3)
            *dst++ = '=';
    }
    *dst = '\0';

    return ret;
}
#endif

static const MediaParserInfo info = {
    "vorbis",
    MP_audio
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
} vorbis_priv;

//! Parse extradata and reformat it, most of the code is shamelessly ripped from ffvorbis.
static int encode_header(uint8_t *data, int len, vorbis_priv *priv)
{
    uint8_t *headers = data;
    int headers_len = len, i , j;
    uint8_t *header_start[3];
    int header_len[3];
    int hash[4];
    uint8_t comment[26] =
        /*quite minimal comment */
    { 3, 'v', 'o', 'r', 'b', 'i', 's',
        10, 0, 0, 0,
        'v', 'o', 'r', 'b', 'i', 's', '-', 'r', 't', 'p',
        0, 0, 0, 0,
        1 
    };

// old way.
    if(headers[0] == 0 && headers[1] == 30) {
        for(i = 0; i < 3; i++){
            header_len[i] = *headers++ << 8;
            header_len[i] += *headers++;
            header_start[i] = headers;
            headers += header_len[i];
        }
// xiphlaced
    } else if(headers[0] == 2) {
        for(j=1, i=0; i<2 ; ++i, ++j) {
            header_len[i]=0;
            while(j<headers_len && headers[j]==0xff) {
                header_len[i]+=0xff;
                ++j;
            }
            if (j>=headers_len) {
                fnc_log(FNC_LOG_ERR, "[vorbis] Extradata corrupt.");
                return -1;
            }
            header_len[i]+=headers[j];
        }
        header_len[2] = headers_len - header_len[0] - header_len[1] - j;
        headers += j;
        header_start[0] = headers;
        header_start[1] = header_start[0] + header_len[0];
        header_start[2] = header_start[1] + header_len[1];
    } else {
        fnc_log(FNC_LOG_ERR, "[vorbis] Extradata corrupt.");
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
    priv->conf[10] = header_len[0];     // 30, always
    priv->conf[11] = sizeof(comment);   // 26
    memcpy(priv->conf + 12, header_start[0], header_len[0]);
    memcpy(priv->conf + 12 + header_len[0], comment, sizeof(comment));
    memcpy(priv->conf + 12 + header_len[0] + sizeof(comment), header_start[2],
           header_len[2]);
    return 0;
}

static int vorbis_uninit(void *private_data)
{
    vorbis_priv *priv = private_data;
    if (priv) {
        if (priv->conf_len) g_free(priv->conf);
        if (priv->len) g_free(priv->packet);
        free(private_data);
    }
    return ERR_NOERROR;
}

static int vorbis_init(MediaProperties *properties, void **private_data)
{
    sdp_field *sdp_private;
    vorbis_priv *priv = calloc(1, sizeof(vorbis_priv));
    int err, buf_len;
    char *buf;

    if (!priv) return ERR_ALLOC;

    if(!properties->extradata_len)
	goto err_alloc;

    err = encode_header(properties->extradata,
                          properties->extradata_len, priv);

    if (err) goto err_alloc;

    buf_len = priv->conf_len * 4 / 3 + 12;
    buf = g_malloc(buf_len);

    if (!buf) goto err_alloc;
    av_base64_encode(buf, buf_len, priv->conf, priv->conf_len);

    sdp_private = g_new(sdp_field, 1);
    sdp_private->type = fmtp;
    sdp_private->field = 
        g_strdup_printf("delivery-method=in_band; configuration=%s;", buf); 
    properties->sdp_private =
        g_list_prepend(properties->sdp_private, sdp_private);

    sdp_private = g_new(sdp_field, 1);
    sdp_private->type = rtpmap;
    sdp_private->field = g_strdup_printf ("vorbis/%d/%d",
                                            properties->clock_rate,
                                            properties->audio_channels);

    properties->sdp_private =
        g_list_prepend(properties->sdp_private, sdp_private);

    INIT_PROPS

    *private_data = priv;

    return ERR_NOERROR;

    err_alloc:
        vorbis_uninit(priv);
    return ERR_ALLOC;
}

static int vorbis_get_frame2(uint8_t *dst, uint32_t dst_nbytes, double *timestamp,
                      InputStream *istream, MediaProperties *properties,
                      void *private_data)
{

return ERR_PARSE;
}

static int vorbis_packetize(uint8_t *dst, uint32_t *dst_nbytes, uint8_t *src, uint32_t src_nbytes, MediaProperties *properties, void *private_data)
{

return ERR_PARSE;
}

#define XIPH_HEADER_SIZE 6
static int vorbis_parse(void *track, uint8_t *data, long len, uint8_t *extradata,
                 long extradata_len)
{
    //XXX handle the last packet on EOF
    Track *tr = (Track *)track;
    vorbis_priv *priv = tr->parser_private;
    int frag, off = 0;
    uint32_t mtu = DEFAULT_MTU;  //FIXME get it from SETUP
    uint32_t payload = mtu - XIPH_HEADER_SIZE;
    uint8_t *packet = calloc(1, mtu);

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
            if (bp_write(tr->buffer, 0, tr->properties->mtime, 0, 0,
                                      packet, mtu)) {
                fnc_log(FNC_LOG_ERR, "Cannot write bufferpool");
                goto err_alloc;
            }

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
    if (bp_write(tr->buffer, 0, tr->properties->mtime, 0, 0,
                      packet, len + XIPH_HEADER_SIZE)) {
        fnc_log(FNC_LOG_ERR, "Cannot write bufferpool");
        goto err_alloc;
    }

    free(packet);
    return ERR_NOERROR;

    err_alloc:
    free(packet);
    return ERR_ALLOC;
}


FNC_LIB_MEDIAPARSER(vorbis);

