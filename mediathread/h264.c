/* * 
 *  $Id: $
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *  h264 parser based on rfc 3984
 *
 *  Copyright (C) 2007 by Luca Barbato <lu_zero@gentoo.org
 *
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */

#include <fenice/demuxer.h>
#include <fenice/fnc_log.h>
#include <fenice/MediaParser.h>
#include <fenice/mediaparser_module.h>
#include <fenice/utils.h>
#include <fenice/types.h>
#include <string.h>

static MediaParserInfo info = {
    "H264",
    MP_video
};

FNC_LIB_MEDIAPARSER(h264);

typedef struct {
	int is_avc;
	uint8_t *packet; //holds the incomplete packet
	unsigned int len; // incomplete packet length
        unsigned int nal_length_size; // used in avc to 
        uint8_t **sps;  // NULL terminated array of pointers to the extradata
        uint8_t **pps;  // idem
} h264_priv;

/* Parse the nal header and return the nal type or ERR_PARSE if doesn't match
 *
 *  +---------------+
 *  |0|1|2|3|4|5|6|7|
 *  +-+-+-+-+-+-+-+-+
 *  |F|NRI|  Type   |
 *  +---------------+
 *
 */

#define RB16(x) ((((uint8_t*)(x))[0] << 8) | ((uint8_t*)(x))[1])

static inline int nal_header(uint8_t header)
{
    if (header >>7) return ERR_PARSE;   // F bit set to 1
    // (header & 0x60)>>5               // NRI we don't care much yet
    return (header & 0x1f);             // Type
}


static int init(MediaProperties *properties, void **private_data)
{
    sdp_field *sdp_private = g_new(sdp_field, 1);
    h264_priv *priv = calloc(1,sizeof(h264_priv));

    if (!priv) return ERR_ALLOC;

    if(properties->extradata && properties->extradata[0] == 1) {
        uint8_t *p = properties->extradata;
        int i, cnt, nalsize;
        if (properties->extradata_len < 7) goto err_alloc;
        // Shamelessly taken from ffmpeg
        priv->is_avc = 1;
        priv->nal_length_size = 2;
        cnt = *(p+5) & 0x1f; // Number of sps
        priv->sps = malloc((cnt + 1) * sizeof(char *));
        if (!priv->sps) goto err_alloc;
        p += 6;
        for (i = 0; i < cnt; i++) {
            if (p > properties->extradata + properties->extradata_len)
                goto err_sps;
            nalsize = RB16(p) + 2; //buf_size
            fnc_log(FNC_LOG_DEBUG, "[h264] nalsize %d\n", nalsize);
            priv->sps[i] = p;
            p += nalsize;
        }
        priv->sps[i] = NULL;
        // Decode pps from avcC
        cnt = *(p++); // Number of pps
        priv->pps = malloc((cnt + 1) * sizeof(char *));
        if (!priv->sps) goto err_sps;
        fnc_log(FNC_LOG_DEBUG, "[h264] pps %d\n", cnt);
        for (i = 0; i < cnt; i++) {
            if (p > properties->extradata + properties->extradata_len)
                goto err_pps;
            nalsize = RB16(p) + 2;
            fnc_log(FNC_LOG_DEBUG, "[h264] nalsize %d\n", nalsize);
            priv->pps[i] = p;
            p += nalsize;
        }
        priv->pps[i] = NULL;
        priv->nal_length_size = (properties->extradata[4]&0x03)+1;
        //FIXME Fill the fmtp with the data....
    }

    sdp_private->type = rtpmap;
    sdp_private->field = g_strdup_printf ("H264/%d",properties->clock_rate);

    properties->sdp_private =
        g_list_prepend(properties->sdp_private, sdp_private);

    INIT_PROPS

    *private_data = priv;
    return ERR_NOERROR;

    err_pps:
        free(priv->pps);
    err_sps:
        free(priv->sps);
    err_alloc:
        free(priv);
    return ERR_ALLOC;
}

static int get_frame2(uint8 *dst, uint32 dst_nbytes, double *timestamp,
                      InputStream *istream, MediaProperties *properties,
                      void *private_data)
{


return ERR_PARSE;
}

static int packetize(uint8 *dst, uint32 *dst_nbytes, uint8 *src, uint32 src_nbytes, MediaProperties *properties, void *private_data)
{

return ERR_PARSE;
}

// h264 has provisions for
//  - collating NALS 
//  - fragmenting
//  - feed a single NAL as is.

static int parse(void *track, uint8 *data, long len, uint8 *extradata,
                 long extradata_len)
{
    Track *tr = (Track *)track;
    h264_priv *priv = tr->parser_private;
    OMSSlot *slot;
    uint32_t mtu = len + 4, rem; //FIXME get it from SETUP
    double nal_time; // see page 9 and 7.4.1.2
    int32_t offset;
    int i, nalsize = 0, index = 0;
    uint8 dst[mtu], *p, *q;
    rem = len;

    if (priv->is_avc) {
        uint8_t **sps = priv->sps;

        //FIXME it's ugly, should only happen on frame I and could
        //be done in a better way...
        while (sps[1]) {
            if (mtu >= sps[1] - sps[0]) {
                if (OMSbuff_write(tr->buffer, 0, tr->properties->mtime, 0, 0,
                                      sps[0], sps[1] - sps[0])) {
                        fnc_log(FNC_LOG_ERR, "Cannot write bufferpool\n");
                        return ERR_ALLOC;
                }
                fnc_log(FNC_LOG_DEBUG, "[h264] single NAL\n");
            } else {
            // single NAL, to be fragmented, FU-A and FU-B
                fnc_log(FNC_LOG_DEBUG, "[h264] frags\n");
                return ERR_PARSE; //FIXME broken for now
            }
            sps++;
        }

        sps = priv->pps;
        while (sps[1]) {
            if (mtu >= sps[1] - sps[0]) {
                if (OMSbuff_write(tr->buffer, 0, tr->properties->mtime, 0, 0,
                                      sps[0], sps[1] - sps[0])) {
                        fnc_log(FNC_LOG_ERR, "Cannot write bufferpool\n");
                        return ERR_ALLOC;
                }
                fnc_log(FNC_LOG_DEBUG, "[h264] single NAL\n");
            } else {
            // single NAL, to be fragmented, FU-A and FU-B
                fnc_log(FNC_LOG_DEBUG, "[h264] frags\n");
                return ERR_PARSE; //FIXME broken for now
            }
            sps++;
        }

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
                    fnc_log(FNC_LOG_ERR, "AVC: nal size %d\n", nalsize);
                    return ERR_PARSE;
                }
            }
            if (mtu >= nalsize) {
                if (OMSbuff_write(tr->buffer, 0, tr->properties->mtime, 0, 0,
                                      data + index, nalsize)) {
                        fnc_log(FNC_LOG_ERR, "Cannot write bufferpool\n");
                        return ERR_ALLOC;
                }
                fnc_log(FNC_LOG_DEBUG, "[h264] single NAL\n");
            } else {
            // single NAL, to be fragmented, FU-A and FU-B
                fnc_log(FNC_LOG_DEBUG, "[h264] frags\n");
                return ERR_PARSE; //FIXME broken for now
            }
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
                fnc_log(FNC_LOG_DEBUG, "Sending NAL %d \n",p[0]&0x1f);
                if (OMSbuff_write(tr->buffer, 0, tr->properties->mtime, 0, 0,
                                      p, q - p)) {
                        fnc_log(FNC_LOG_ERR, "Cannot write bufferpool\n");
                        return ERR_ALLOC;
                }
                fnc_log(FNC_LOG_DEBUG, "[h264] single NAL\n");
            } else {
            // single NAL, to be fragmented, FU-A and FU-B
                fnc_log(FNC_LOG_DEBUG, "[h264] frags\n");
                return ERR_PARSE; //FIXME broken for now
            }

            p = q;

        }
        // last NAL
        if (OMSbuff_write(tr->buffer, 0, tr->properties->mtime, 0, 0,
                                  p, len - (p - data))) {
                    fnc_log(FNC_LOG_ERR, "Cannot write bufferpool\n");
                    return ERR_ALLOC;
        }
    }

    fnc_log(FNC_LOG_DEBUG, "[h264]Frame completed\n");
    return ERR_NOERROR;
}

static int uninit(void *private_data)
{

return ERR_NOERROR;
}
