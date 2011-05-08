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
#include <stdlib.h>
#include <stdbool.h>

#include "media/demuxer.h"
#include "media/mediaparser.h"
#include "fnc_log.h"

typedef struct {
    double first_clock;
    double last_seen_clock;
    double first_time;
    double last_seen_time;
    uint32_t last_pkt_num;
} mp2t_pid_status;

typedef struct {
    size_t ncopied;
    double pkt_duration_estimate;
    int ts_pkt_count;
    double timestamp;
} mp2t_priv;


#define TS_PKT_SIZE 188
#define TARGET_TS_PKT_COUNT 7
#define TS_SYNC_BYTE 0x47

#define NEW_DURATION_WEIGHT 0.5
#define TIME_ADJUSTMENT_FACTOR 0.8
#define MAX_PLAYOUT_BUFFER_DURATION 0.1

static int mp2t_init(Track *track)
{
    track->private_data = g_slice_new(mp2t_priv);

    return 0;
}

static int mp2t_packetize(uint8_t *dst, size_t *dst_nbytes, uint8_t *src,
                          size_t src_nbytes, Track *tr)
{
    size_t to_copy = 0; ssize_t i;
    mp2t_priv* priv = tr->private_data;

    for(i=TARGET_TS_PKT_COUNT; i>=0; i--) {
        if(src_nbytes - priv->ncopied
            >= i*TS_PKT_SIZE) {
            //we can grab i packets
            break;
        }
    }

    if(i==0) {
        fnc_log(FNC_LOG_ERR, "mp2t: dropping %d bytes",
            src_nbytes - priv->ncopied);
        return 0;
    }

    to_copy = i*TS_PKT_SIZE;

    memcpy(dst, src + priv->ncopied, to_copy);

    *dst_nbytes = to_copy;
    priv->ncopied += to_copy;

    return !(priv->ncopied >= src_nbytes);
}

static int mp2t_parse(Track *tr, uint8_t *data, size_t len)
{
    int ret;
    size_t dst_len = len;
    uint8_t dst[dst_len];

    do {
        // dst_len remains unchanged,
        // the return value is either EOF or the size
        ret = mp2t_packetize(dst, &dst_len, data, len, tr);
        if (ret >= 0) {
            struct MParserBuffer *buffer = g_slice_new0(struct MParserBuffer);

            buffer->timestamp = tr->properties.pts;
            buffer->delivery = tr->properties.dts;
            buffer->duration = tr->properties.frame_duration;
            buffer->marker = true;

            buffer->data_size = dst_len;
            buffer->data = g_memdup(dst, buffer->data_size);

            mparser_buffer_write(tr, buffer);

            dst_len = len;
        }
    } while (ret);
    return 0;
}

static void mp2t_uninit(Track *track)
{
    g_slice_free(mp2t_priv, track->private_data);
}

FENG_MEDIAPARSER(mp2t, "MP2T", MP_video);
