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


/* Parse the nal header and return the nal type or ERR_PARSE if doesn't match
 *
 *  +---------------+
 *  |0|1|2|3|4|5|6|7|
 *  +-+-+-+-+-+-+-+-+
 *  |F|NRI|  Type   |
 *  +---------------+
 *
 */

static inline int nal_header(uint8_t header)
{
    if (header >>7) return ERR_PARSE;   // F bit set to 1
    // (header & 0x60)>>5               // NRI we don't care much yet
    return (header & 0x1f);             // Type
}


static int init(MediaProperties *properties, void **private_data)
{
    sdp_field *sdp_private = g_new(sdp_field, 1);

    sdp_private->type = rtpmap;
    sdp_private->field = g_strdup_printf ("H264/%d",properties->clock_rate);

    properties->sdp_private =
        g_list_prepend(properties->sdp_private, sdp_private);

    INIT_PROPS

    return ERR_NOERROR;
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
    OMSSlot *slot;
    uint32_t mtu = len + 4, rem; //FIXME get it from SETUP
    double nal_time; // see page 9 and 7.4.1.2
    int32_t offset;
    uint8 dst[mtu];
    rem = len;
    int is_avc = 0;

    // which bitstream? Should I move it at init time?
#if 0
    if (extradata_len>0 && extradata && extradata[0]==1) //avc?
    {
        if (extradata_len < 7) {
            return ERR_PARSE;
        } else {
            is_avc = 1;
            // decode the nal lenght size
        }
    } else { // something unknown
        return ERR_PARSE;
    }
    // not avc, let's packet nals!

    // decode current nal size

    // packet it
#endif
    // single NAL, not fragmented, straight copy.
    if (mtu >= len) {
        if (OMSbuff_write(tr->buffer, 0, tr->properties->mtime, 0, 0,
                              data, len)) {
                fnc_log(FNC_LOG_ERR, "Cannot write bufferpool\n");
                return ERR_ALLOC;
        }
        fnc_log(FNC_LOG_DEBUG, "[h264] single NAL\n");
    } else {
    // single NAL, to be fragmented, FU-A and FU-B
        return ERR_PARSE; // broken for now
        do {
            uint8_t indicator, header;  // FU common headers
            uint16_t don;               // FU-B don header
            int header_size = 2;            // 2 - FU-A | 4 - FU-B

            // If interleaved the use FU-B for the first frag
            // Otherwise use FU-A
            // We do not support interleaved h264 yet!
            indicator = data[0]&0xe0|28; // [F|NRI|Type = 28]

//            memcpy (dst + , data + offset, min(mtu, rem));
            header = (1<<7)|             // S bit : Start
                     (1<<6)|             // E bit : End
                     28;                  // Type = 28
            
            dst[0] = indicator;
            dst[1] = header;
            memcpy(dst+header_size, data, min(mtu, rem));
            if (OMSbuff_write(tr->buffer, 0, tr->properties->mtime, 0, 0,
                                  dst, min(mtu, rem) + header_size)) { 
                fnc_log(FNC_LOG_ERR, "Cannot write bufferpool\n");
                return ERR_ALLOC;
            }
            fnc_log(FNC_LOG_DEBUG, "[h264] frags\n");
        } while (rem - mtu > 0);
    }
    fnc_log(FNC_LOG_DEBUG, "[h264]Frame completed\n");
    return ERR_NOERROR;
}

static int uninit(void *private_data)
{

return ERR_NOERROR;
}
