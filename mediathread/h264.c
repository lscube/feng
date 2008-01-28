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
static MediaParserInfo info = {
    "H264",
    MP_video
};

typedef struct {
    int is_avc;
    uint8_t *packet; //holds the incomplete packet
    unsigned int len; // incomplete packet length
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

static int frag_fu_a(uint8_t *nal, int fragsize, int mtu,
                     BPBuffer * buffer, double timestamp)
{
    int start = 1, fraglen;
    uint8_t fu_header, buf[mtu];
    fnc_log(FNC_LOG_VERBOSE, "[h264] frags");
//                p = data + index;
    buf[0] = (nal[0] & 0xe0) | 28; // fu_indicator
    fu_header = nal[0] & 0x1f;
    nal++;
    fragsize--;
    while(fragsize>0) {
        buf[1] = fu_header;
        if (start) {
            start = 0;
            buf[1] = fu_header | (1<<7);
        }
        fraglen = min(mtu-2, fragsize);
        if (fraglen == fragsize) {
            buf[1] = fu_header | (1<<6);
        }
        memcpy(buf + 2, nal, fraglen);
        fnc_log(FNC_LOG_VERBOSE, "[h264] Frag %d %d",buf[0], buf[1]);
        fragsize -= fraglen;
        nal      += fraglen;
        if (bp_write(buffer, 0, timestamp, 0, 0, buf, fraglen + 2)) {
            fnc_log(FNC_LOG_ERR, "Cannot write bufferpool");
            return ERR_ALLOC;
        }
    }
    return ERR_NOERROR;
}

#define RB16(x) ((((uint8_t*)(x))[0] << 8) | ((uint8_t*)(x))[1])

static char *encode_avc1_header(uint8_t *p, unsigned int len, int packet_mode)
{
    int i, cnt, nalsize;
    unsigned int buf_len = len * 4 / 3 + 12;
    uint8_t *q = p;
    char *sprop = NULL, *out, *buf = g_malloc(buf_len);
    cnt = *(p+5) & 0x1f; // Number of sps
    p += 6;

    for (i = 0; i < cnt; i++) {
        if (p > q + len)
            goto err_alloc;
        nalsize = RB16(p); //buf_size
        p += 2;
        fnc_log(FNC_LOG_VERBOSE, "[h264] nalsize %d", nalsize);
        if (i == 0) {
            out = g_strdup_printf("profile-level-id=%02x%02x%02x; "
                                  "packetization-mode=%d; ",
                                  p[0], p[1], p[2], packet_mode);
            av_base64_encode(buf, buf_len, p, nalsize);
            sprop = g_strdup_printf("%ssprop-parameter-sets=%s", out, buf);
            g_free(out);
        } else {
            av_base64_encode(buf, buf_len, p, nalsize);
            out = g_strdup_printf("%s,%s", sprop, buf);
            g_free(sprop);
            sprop = out;
        }
        p += nalsize;
    }
    // Decode pps from avcC
    cnt = *(p++); // Number of pps
    fnc_log(FNC_LOG_VERBOSE, "[h264] pps %d", cnt);

    for (i = 0; i < cnt; i++) {
        if (p > q + len)
            goto err_alloc;
        nalsize = RB16(p);
        p += 2;
        fnc_log(FNC_LOG_VERBOSE, "[h264] nalsize %d", nalsize);
        av_base64_encode(buf, buf_len, p, nalsize);
        out = g_strdup_printf("%s,%s",sprop, buf);
        g_free(sprop);
        sprop = out;
        p += nalsize;
    }

    g_free(buf);

    return sprop;

    err_alloc:
        g_free(buf);
        if (sprop) g_free(sprop);
    return NULL;
}

static char *encode_header(uint8_t *p, unsigned int len, int packet_mode)
{
    uint8_t *q, *end = p + len;
    unsigned int buf_len = len * 4 / 3 + 12;
    char *sprop = NULL, *out, *buf = g_malloc(buf_len);

    for (q = p; q < end - 3; q++) {
        if (q[0] == 0 && q[1] == 0 && q[2] == 1) {
            q += 3;
            break;
        }
    }

    if (q >= end - 3)
        return NULL;

    p = q; // sps start;

    for (; q < end - 3; q++) {
        if (q[0] == 0 && q[1] == 0 && q[2] == 1) {
            // sps end;
            break;
        }
    }

    //FIXME I'm abusing memory
    // profile-level-id aka the first 3 bytes from sps
    out = g_strdup_printf("profile-level-id=%02x%02x%02x; "
                          "packetization-mode=%d; ",
                            p[0], p[1], p[2], packet_mode);

    av_base64_encode(buf, buf_len, p, q - p);

    sprop = g_strdup_printf("%ssprop-parameter-sets=%s", out, buf);
    g_free(out);
    p = q + 3;

    while (p < end) {
        //seek to the next startcode [0 0 1]
        for (q = p; q < end; q++)
            if (end - q <= 3) continue; // last nal
            if (q[0] == 0 && q[1] == 0 && q[2] == 1) {
                break;
            }
        av_base64_encode(buf, buf_len, p, q - p);
        out = g_strdup_printf("%s,%s",sprop, buf);
        g_free(sprop);
        sprop = out;
        p = q + 3;
    }

    g_free(buf);

    return sprop;
}

#define FU_A 1

static int h264_init(MediaProperties *properties, void **private_data)
{
    sdp_field *sdp_private;
    h264_priv *priv = calloc(1, sizeof(h264_priv));
    char *sprop = NULL;

    if (!priv) return ERR_ALLOC;

    if (properties->extradata) {
        if(properties->extradata && properties->extradata[0] == 1) {
        	if (properties->extradata_len < 7) goto err_alloc;
        	priv->nal_length_size = (properties->extradata[4]&0x03)+1;
            priv->is_avc = 1;
            sprop = encode_avc1_header(properties->extradata,
                                       properties->extradata_len, FU_A);
            if (sprop == NULL) goto err_alloc;
        } else {
            sprop = encode_header(properties->extradata,
                                  properties->extradata_len, FU_A);
            if (sprop == NULL) goto err_alloc;
        }

        sdp_private = g_new(sdp_field, 1);
        sdp_private->type = fmtp;
        sdp_private->field = sprop;
        properties->sdp_private =
            g_list_prepend(properties->sdp_private, sdp_private);
    }
    else {
        fnc_log(FNC_LOG_WARN, "[h264] No Extradata, if loaded from a .sd remember to specify the FMTP option\n");
    }

    sdp_private = g_new(sdp_field, 1);
    sdp_private->type = rtpmap;
    sdp_private->field = g_strdup_printf ("H264/%d",properties->clock_rate);

    properties->sdp_private =
        g_list_prepend(properties->sdp_private, sdp_private);

    INIT_PROPS

    *private_data = priv;

    return ERR_NOERROR;

    err_alloc:
        free(priv);
    return ERR_ALLOC;
}

static int h264_get_frame2(uint8_t *dst, uint32_t dst_nbytes, double *timestamp,
                      InputStream *istream, MediaProperties *properties,
                      void *private_data)
{

return ERR_PARSE;
}

static int h264_packetize(uint8_t *dst, uint32_t *dst_nbytes, uint8_t *src, uint32_t src_nbytes, MediaProperties *properties, void *private_data)
{

return ERR_PARSE;
}

// h264 has provisions for
//  - collating NALS 
//  - fragmenting
//  - feed a single NAL as is.

static int h264_parse(void *track, uint8_t *data, long len, uint8_t *extradata,
                 long extradata_len)
{
    Track *tr = (Track *)track;
    h264_priv *priv = tr->parser_private;
    uint32_t mtu = DEFAULT_MTU; //FIXME get it from SETUP
//    double nal_time; // see page 9 and 7.4.1.2
    int i, nalsize = 0, index = 0, ret;
    uint8_t *p, *q;

    if (priv->is_avc) {
        while (1) {
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
            if (mtu >= nalsize) {
                if (bp_write(tr->buffer, 0, tr->properties->mtime, 0, 0,
                                      data + index, nalsize)) {
                        fnc_log(FNC_LOG_ERR, "Cannot write bufferpool");
                        return ERR_ALLOC;
                }
                fnc_log(FNC_LOG_VERBOSE, "[h264] single NAL");
            } else {
            // single NAL, to be fragmented, FU-A;
                ret = frag_fu_a(data + index, nalsize, mtu, tr->buffer,
                                tr->properties->mtime);
                if (ret) return ret;
            }
            index += nalsize;
        }
    } else {
        //seek to the first startcode
        for (p = data; p<data + len - 3; p++) {
            if (p[0] == 0 && p[1] == 0 && p[2] == 1) {
                p+=3;
                break;
            }
        }
        if (p >= data + len - 3) return ERR_PARSE;

        while (1) {
        //seek to the next startcode [0 0 1]
            for (q = p; q<data+len-3;q++) {
                if (q[0] == 0 && q[1] == 0 && q[2] == 1) {
                    q+=3;
                    break;
                }
            }

            if (q >= data + len - 3) break;

            if (mtu >= q - p) {
                fnc_log(FNC_LOG_VERBOSE, "[h264] Sending NAL %d",p[0]&0x1f);
                if (bp_write(tr->buffer, 0, tr->properties->mtime, 0, 0,
                                      p, q - p)) {
                        fnc_log(FNC_LOG_ERR, "Cannot write bufferpool");
                        return ERR_ALLOC;
                }
                fnc_log(FNC_LOG_VERBOSE, "[h264] single NAL");
            } else {
                //FU-A
                fnc_log(FNC_LOG_VERBOSE, "[h264] frags");
                ret = frag_fu_a(p, q - p, mtu, tr->buffer,
                                tr->properties->mtime);
                if (ret) return ret;
            }

            p = q;

        }
        // last NAL
        fnc_log(FNC_LOG_VERBOSE, "[h264] last NAL %d",p[0]&0x1f);
        if (mtu >= len - (p - data)) {
            fnc_log(FNC_LOG_VERBOSE, "[h264] no frags");
            if (bp_write(tr->buffer, 0, tr->properties->mtime, 0, 0,
                                      p, len - (p - data))) {
                        fnc_log(FNC_LOG_ERR, "Cannot write bufferpool");
                        return ERR_ALLOC;
            }
        } else {
            //FU-A
            fnc_log(FNC_LOG_VERBOSE, "[h264] frags");
            ret = frag_fu_a(p, len - (p - data), mtu, tr->buffer,
                            tr->properties->mtime);
            if (ret) return ret;
        }
    }

    fnc_log(FNC_LOG_VERBOSE, "[h264] Frame completed");
    return ERR_NOERROR;
}

static int h264_uninit(void *private_data)
{
    //that's all?
    if (private_data) free(private_data);
    return ERR_NOERROR;
}

FNC_LIB_MEDIAPARSER(h264);

