/* * 
 *  $Id: $
 *  
 *  This file is part of Feng
 *
 *  theora parser based on the current draft
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#include <fenice/demuxer.h>
#include <fenice/fnc_log.h>
#include <fenice/mediaparser.h>
#include <fenice/mediaparser_module.h>
#include <fenice/utils.h>

#ifdef HAVE_AVUTIL
#include <ffmpeg/base64.h>
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


static MediaParserInfo info = {
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
    uint8_t comment[] =
        /*quite minimal comment */
    { 0x81,'t','h','e','o','r','a',
      10,0,0,0, 
      't','h','e','o','r','a','-','r','t','p',
      0,0,0,0};


    if (ff_split_xiph_headers(data, len, 42, header_start, header_len) < 0) {
        fnc_log(FNC_LOG_ERR, "Extradata corrupt. unknown layout");
        return -1;
    }
    if (header_len[2] + header_len[0]>UINT16_MAX) {
        return -1;
    }

    priv->ident = md_32(data, len);

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

static int theora_uninit(void *private_data)
{
    theora_priv *priv = private_data;
    if (priv) {
        if (priv->conf_len) g_free(priv->conf);
        if (priv->len) g_free(priv->packet);
        free(private_data);
    }
    return ERR_NOERROR;
}

static int theora_init(MediaProperties *properties, void **private_data)
{
    sdp_field *sdp_private;
    theora_priv *priv = calloc(1, sizeof(theora_priv));
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
    sdp_private->field = g_strdup_printf ("theora/%d",
                                            properties->clock_rate);

    properties->sdp_private =
        g_list_prepend(properties->sdp_private, sdp_private);

    INIT_PROPS

    *private_data = priv;

    return ERR_NOERROR;

    err_alloc:
        theora_uninit(priv);
    return ERR_ALLOC;
}

static int theora_get_frame2(uint8_t *dst, uint32_t dst_nbytes, double *timestamp,
                      InputStream *istream, MediaProperties *properties,
                      void *private_data)
{

return ERR_PARSE;
}

static int theora_packetize(uint8_t *dst, uint32_t *dst_nbytes, uint8_t *src, uint32_t src_nbytes, MediaProperties *properties, void *private_data)
{

return ERR_PARSE;
}


#define XIPH_HEADER_SIZE 6
static int theora_parse(void *track, uint8_t *data, long len, uint8_t *extradata,
                 long extradata_len)
{
    //XXX handle the last packet on EOF
    Track *tr = (Track *)track;
    theora_priv *priv = tr->parser_private;
    int frag, off = 0;
    uint32_t mtu = DEFAULT_MTU;  //FIXME get it from SETUP
    uint32_t payload = mtu - XIPH_HEADER_SIZE;
    uint8_t *packet = calloc(1, mtu);

    if(!packet) goto err_alloc;

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
            if (OMSbuff_write(tr->buffer, 0, tr->properties->mtime, 0, 0,
                                      packet, mtu)) {
                fnc_log(FNC_LOG_ERR, "Cannot write bufferpool\n");
                return ERR_ALLOC;
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
    if (OMSbuff_write(tr->buffer, 0, tr->properties->mtime, 0, 0,
                      packet, len + XIPH_HEADER_SIZE)) {
        fnc_log(FNC_LOG_ERR, "Cannot write bufferpool\n");
        return ERR_ALLOC;
    }

    free(packet);
    return ERR_NOERROR;

    err_alloc:
    free(packet);
    return ERR_ALLOC;
}


FNC_LIB_MEDIAPARSER(theora);

