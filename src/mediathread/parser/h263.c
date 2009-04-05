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
#include "mediaparser.h"
#include "mediaparser_module.h"

static const MediaParserInfo info = {
    "H263P",
    MP_video
};

/* H263-1998 FRAGMENT Header (RFC4629)
    0                   1
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   RR    |P|V|   PLEN    |PEBIT|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

typedef struct
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    uint16_t plen1:1;       /* Length in bytes of the extra picture header */
    uint16_t v:1;           /* Video Redundancy Coding */
    uint16_t p:1;           /* picture/GOB/slice/EOS/EOSBS start code */
    uint16_t rr:5;          /* Reserved. Shall be zero. */

    uint16_t pebit:3;       /* Bits to ignore in the last byte of the header */
    uint16_t plen2:5;       /* Length in bytes of the extra picture header */
#elif G_BYTE_ORDER == G_BIG_ENDIAN
    uint16_t rr:5;          /* Reserved. Shall be zero. */
    uint16_t p:1;           /* picture/GOB/slice/EOS/EOSBS start code */
    uint16_t v:1;           /* Video Redundancy Coding */
    uint16_t plen1:1;       /* Length in bytes of the extra picture header */

    uint16_t plen2:5;       /* Length in bytes of the extra picture header */
    uint16_t pebit:3;       /* Bits to ignore in the last byte of the header */
#else
#error Neither big nor little
#endif
} h263_header;

static int h263_init(MediaProperties *properties, void **private_data)
{
    sdp_field *sdp_private;

    sdp_private = g_new(sdp_field, 1);
    sdp_private->type = rtpmap;
    sdp_private->field = g_strdup_printf ("H263-1998/%d",
                                            properties->clock_rate);

    properties->sdp_private =
        g_list_prepend(properties->sdp_private, sdp_private);

    INIT_PROPS

    return ERR_NOERROR;
}

static int h263_parse(void *track, uint8_t *data, long len, uint8_t *extradata,
                 long extradata_len)
{
    Track *tr = (Track *)track;
    int mtu = DEFAULT_MTU, cur = 0, payload, header_len, found_gob = 0;
    uint8_t dst[mtu];
    h263_header *header = (h263_header *) dst;

    if (len >= 3 && *data == '\0' && *(data + 1) == '\0'
        && *(data + 2) >= 0x80) {
        found_gob = 1;
    }

    while (len - cur > 0) {
        if (cur == 0 && found_gob) {
            payload = min(mtu, len);
            memcpy(dst, data, payload);
            memset(header, 0, 2);
            header->p = 1;
            header_len = 0;
        } else {
            payload = min(mtu - 2, len - cur);
            memcpy(dst + 2, data + cur, payload);
            memset(header, 0, 2);
            header_len = 2;
        }
        mparser_buffer_write(tr, tr->properties->frame_duration,
                        cur + payload >= len, dst,
                        payload + header_len);
        cur += payload;
    }

    return ERR_NOERROR;
}

static int h263_uninit(void *private_data)
{
    return ERR_NOERROR;
}

FNC_LIB_MEDIAPARSER(h263);

