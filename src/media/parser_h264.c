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

#include <config.h>

#include <string.h>
#include <stdbool.h>

#include "fnc_log.h"
#include "media/media.h"

typedef struct {
    int is_avc;
    unsigned int nal_length_size; // used in avc
} h264_priv;

/* Generic Nal header
 *
 *  +---------------+
 *  |0|1|2|3|4|5|6|7|
 *  +-+-+-+-+-+-+-+-+
 *  |F|NRI|  Type   |
 *  +---------------+
 *
 */
/*  FU header
 *  +---------------+
 *  |0|1|2|3|4|5|6|7|
 *  +-+-+-+-+-+-+-+-+
 *  |S|E|R|  Type   |
 *  +---------------+
 */

static void frag_fu_a(uint8_t *nal, int fragsize, Track *tr)
{
    int start = 1;
    const uint8_t fu_indicator = (nal[0] & 0xe0) | 28;
    const uint8_t fu_header = nal[0] & 0x1f;

    fnc_log(FNC_LOG_VERBOSE, "[h264] frags");

    nal++;
    fragsize--;

    while(fragsize>0) {
        const size_t fraglen = MIN(DEFAULT_MTU-2, fragsize);
        struct MParserBuffer *buffer = g_slice_new0(struct MParserBuffer);

        buffer->timestamp = tr->pts;
        buffer->delivery = tr->dts;
        buffer->duration = tr->frame_duration;

        buffer->data_size = fraglen + 2;
        buffer->data = g_malloc(buffer->data_size);

        buffer->data[0] = fu_indicator;
        buffer->data[1] = fu_header;

        if ( start ) {
            buffer->data[1] |= (1<<7);
            start = 0;
        }

        if (fraglen == fragsize) {
            buffer->marker = true;
            buffer->data[1] |= (1<<6);
        }

        memcpy(buffer->data + 2, nal, fraglen);
        fnc_log(FNC_LOG_VERBOSE, "[h264] Frag %02x%02x", buffer->data[0], buffer->data[1]);

        track_write(tr, buffer);

        fragsize -= fraglen;
        nal      += fraglen;
    }
}

#define RB16(x) ((((uint8_t*)(x))[0] << 8) | ((uint8_t*)(x))[1])

static char *encode_avc1_header(uint8_t *p, unsigned int len, int packet_mode)
{
    int i, cnt, nalsize;
    uint8_t *q = p;
    char *sprop = NULL;
    cnt = *(p+5) & 0x1f; // Number of sps
    p += 6;

    for (i = 0; i < cnt; i++) {
        if (p > q + len)
            goto err_alloc;
        nalsize = RB16(p); //buf_size
        p += 2;
        fnc_log(FNC_LOG_VERBOSE, "[h264] nalsize %d", nalsize);
        if (i == 0) {
            gchar *out = g_strdup_printf("profile-level-id=%02x%02x%02x; "
                                  "packetization-mode=%d; ",
                                  p[0], p[1], p[2], packet_mode);
	    gchar *buf = g_base64_encode(p, nalsize);
            sprop = g_strdup_printf("%ssprop-parameter-sets=%s", out, buf);
	    g_free(buf);
            g_free(out);
        } else {
            gchar *buf = g_base64_encode(p, nalsize);
            gchar *out = g_strdup_printf("%s,%s", sprop, buf);
            g_free(sprop);
	    g_free(buf);
            sprop = out;
        }
        p += nalsize;
    }
    // Decode pps from avcC
    cnt = *(p++); // Number of pps
    fnc_log(FNC_LOG_VERBOSE, "[h264] pps %d", cnt);

    for (i = 0; i < cnt; i++) {
        gchar *out, *buf;

        if (p > q + len)
            goto err_alloc;
        nalsize = RB16(p);
        p += 2;
        fnc_log(FNC_LOG_VERBOSE, "[h264] nalsize %d", nalsize);
	buf = g_base64_encode(p, nalsize);
        out = g_strdup_printf("%s,%s",sprop, buf);
        g_free(sprop);
	g_free(buf);
        sprop = out;
        p += nalsize;
    }

    return sprop;

    err_alloc:
        if (sprop) g_free(sprop);
    return NULL;
}

//Ripped directly from ffmpeg...

static const uint8_t *find_startcode_internal(const uint8_t *p,
                                             const uint8_t *end)
{
        const uint8_t *a = p + 4 - ((intptr_t)p & 3);

    for (end -= 3; p < a && p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    for (end -= 3; p < end; p += 4) {
        uint32_t x = *(const uint32_t*)p;
//      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
//      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
        if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
            if (p[1] == 0) {
                if (p[0] == 0 && p[2] == 1)
                    return p;
                if (p[2] == 0 && p[3] == 1)
                    return p+1;
            }
            if (p[3] == 0) {
                if (p[2] == 0 && p[4] == 1)
                    return p+2;
                if (p[4] == 0 && p[5] == 1)
                    return p+3;
            }
        }
    }

    for (end += 3; p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    return end + 3;
}

static const uint8_t *find_startcode(const uint8_t *p, const uint8_t *end){
    const uint8_t *out = find_startcode_internal(p, end);
    if(p<out && out<end && !out[-1]) out--;
    return out;
}

static char *encode_header(uint8_t *p, unsigned int len, int packet_mode)
{
    uint8_t *end = p + len;
    char *sprop = NULL, *out, *buf = NULL;
    const uint8_t *r = find_startcode(p, end);

    while (r < end) {
        const uint8_t *r1;
        uint8_t nal_type;

        while (!*(r++));
        nal_type = *r & 0x1f;
        r1 = find_startcode(r, end);
        if (nal_type != 7 && nal_type != 8) { /* Only output SPS and PPS */
            r = r1;
            continue;
        }
        if (buf == NULL) {
            // profile-level-id aka the first 3 bytes from sps
            out = g_strdup_printf("profile-level-id=%02x%02x%02x; "
                                  "packetization-mode=%d; ",
                                  r[0], r[1], r[2], packet_mode);
            buf = g_base64_encode(r, r1-r);
            sprop = g_strdup_printf("%ssprop-parameter-sets=%s", out, buf);
            g_free(out);
            g_free(buf);
        } else {
            buf = g_base64_encode(r, r1-r);
            out = g_strdup_printf("%s,%s",sprop, buf);
            g_free(sprop);
            g_free(buf);
            sprop = out;
        }
    }

    return sprop;
}

#define FU_A 1

int h264_init(Track *track)
{
    h264_priv *priv;
    char *sprop = NULL;

    priv = g_new0(h264_priv, 1);

    if(track->extradata[0] == 1) {
        if (track->extradata_len < 7) goto err_alloc;
        priv->nal_length_size = (track->extradata[4]&0x03)+1;
        priv->is_avc = 1;
        sprop = encode_avc1_header(track->extradata,
                                   track->extradata_len, FU_A);
        if (sprop == NULL) goto err_alloc;
    } else {
        sprop = encode_header(track->extradata,
                              track->extradata_len, FU_A);
        if (sprop == NULL) goto err_alloc;
    }

    g_string_append_printf(track->sdp_description,
                           "a=rtpmap:%u %s/%d\r\n"
                           "a=fmtp:%u %s\r\n",

                           /* rtpmap */
                           track->payload_type,
                           track->encoding_name,
                           track->clock_rate,

                           /* fmtp */
                           track->payload_type,
                           sprop);

    track->private_data = priv;

    g_free(sprop);

    return 0;

 err_alloc:
    g_slice_free(h264_priv, priv);
    return -1;
}

// h264 has provisions for
//  - collating NALS
//  - fragmenting
//  - feed a single NAL as is.

int h264_parse(Track *tr, uint8_t *data, size_t len)
{
    h264_priv *priv = tr->private_data;
//    double nal_time; // see page 9 and 7.4.1.2
    size_t nalsize = 0, index = 0;
    uint8_t *p, *q;

    if (priv->is_avc) {
        while (1) {
            unsigned int i;
            if(index >= len) break;
            //get the nal size
            nalsize = 0;
            for(i = 0; i < priv->nal_length_size; i++)
                nalsize = (nalsize << 8) | data[index++];
            if(nalsize <= 1 || nalsize > len) {
                if(nalsize == 1) {
                    index++;
                    continue;
                } else {
                    fnc_log(FNC_LOG_VERBOSE, "[h264] AVC: nal size %d", nalsize);
                    break;
                }
            }
            if (DEFAULT_MTU >= nalsize) {
                struct MParserBuffer *buffer = g_slice_new0(struct MParserBuffer);

                buffer->timestamp = tr->pts;
                buffer->delivery = tr->dts;
                buffer->duration = tr->frame_duration;
                buffer->marker = true;

                buffer->data_size = nalsize;
                buffer->data = g_memdup(data + index, buffer->data_size);

                track_write(tr, buffer);

                fnc_log(FNC_LOG_VERBOSE, "[h264] single NAL");
            } else {
            // single NAL, to be fragmented, FU-A;
                frag_fu_a(data + index, nalsize, tr);
            }
            index += nalsize;
        }
    } else {
        //seek to the first startcode
        for (p = data; p<data + len - 3; p++) {
            if (p[0] == 0 && p[1] == 0 && p[2] == 1) {
                break;
            }
        }
        if (p >= data + len) return -1;

        while (1) {
        //seek to the next startcode [0 0 1]
            for (q = p; q<data+len-3;q++) {
                if (q[0] == 0 && q[1] == 0 && q[2] == 1) {
                    break;
                }
            }

            if (q >= data + len) break;

            if (DEFAULT_MTU >= q - p) {
                struct MParserBuffer *buffer = g_slice_new0(struct MParserBuffer);

                buffer->timestamp = tr->pts;
                buffer->delivery = tr->dts;
                buffer->duration = tr->frame_duration;
                buffer->marker = true;

                buffer->data_size = q - p;
                buffer->data = g_memdup(p, buffer->data_size);

                track_write(tr, buffer);

                fnc_log(FNC_LOG_VERBOSE, "[h264] Sending single NAL %d",p[0]&0x1f);
            } else {
                //FU-A
                fnc_log(FNC_LOG_VERBOSE, "[h264] frags");
                frag_fu_a(p, q - p, tr);
            }

            p = q;

        }
        // last NAL
        fnc_log(FNC_LOG_VERBOSE, "[h264] last NAL %d",p[0]&0x1f);
        if (DEFAULT_MTU >= len - (p - data)) {
            struct MParserBuffer *buffer = g_slice_new0(struct MParserBuffer);

            buffer->timestamp = tr->pts;
            buffer->delivery = tr->dts;
            buffer->duration = tr->frame_duration;
            buffer->marker = true;

            buffer->data_size = len - (p - data);
            buffer->data = g_memdup(p, buffer->data_size);

            track_write(tr, buffer);

            fnc_log(FNC_LOG_VERBOSE, "[h264] no frags");
        } else {
            //FU-A
            fnc_log(FNC_LOG_VERBOSE, "[h264] frags");
            frag_fu_a(p, len - (p - data), tr);
        }
    }

    fnc_log(FNC_LOG_VERBOSE, "[h264] Frame completed");
    return 0;
}
