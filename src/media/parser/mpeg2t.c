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

#include <string.h>
#include "media/demuxer.h"
#include "media/mediaparser.h"
#include "media/mediaparser_module.h"
#include "fnc_log.h"
#include "feng_utils.h"

static const MediaParserInfo info = {
    "MP2T",
    MP_video
};

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

#if 0

static void mp2t_pid_status_init(mp2t_pid_status* pid_status, double clock,
                                 double now)
{
    pid_status->first_clock = pid_status->last_seen_clock = clock;
    pid_status->first_time = pid_status->last_seen_time = now;
    pid_status->last_pkt_num = 0;
}

#endif

static int mp2t_init(Track *track)
{
    track->private_data = g_slice_new(mp2t_priv);

    return ERR_NOERROR;
}
#if 0
static void update_ts_pkt_duration(mp2t_priv* priv, uint8_t* pkt, double now)
{
    uint8_t adaption_field_control, adaption_field_length,
            discontinuity_indicator, pcr_flag;
    uint16_t pcr_ext;
    uint32_t pcr_base_high, pid;
    double clock;
    mp2t_pid_status* pid_status = 0;

    priv->ts_pkt_count++;

    if(pkt[0] != TS_SYNC_BYTE) {
        return;
    }

    adaption_field_control = (pkt[3]&0x30) >> 4;

    if(adaption_field_control != 2 &&
            adaption_field_control != 3) {
        //no adaptation header, we aren't interested
        return;
    }

    adaption_field_length = pkt[4];
    if(adaption_field_length == 0) {
        return;
    }

    discontinuity_indicator = pkt[5]&0x80;
    pcr_flag = pkt[5]&0x10;

    if(pcr_flag == 0) {
        //no pcr, we aren't interested
        return;
    }

    pcr_base_high = (pkt[6]<<24) | (pkt[7]<<16) | (pkt[8]<<8) | pkt[9];
    clock = pcr_base_high / 45000.0;
    if((pkt[10]&0x80) != 0) {
        //add in low bit of pcr base if set
        clock += 1.0/90000.0;
    }

    pcr_ext = ((pkt[10]&0x01)<<8) | pkt[11];
    clock += pcr_ext / 27000000.0;

    pid = ((pkt[1]&0x1f)<<8) | pkt[2];

    pid_status = g_hash_table_lookup(priv->pid_status_table,
                                        (gconstpointer)pid);
    if(!pid_status) {
        //we haven't seen this pid before
        pid_status = g_new0(mp2t_pid_status, 1);
        mp2t_pid_status_init(pid_status, clock, now);

        g_hash_table_insert(priv->pid_status_table, (gpointer)pid, pid_status);
    } else {
        //we've seen this pid's pcr before
        //so update our estimate

        double duration_per_pkt = (clock - pid_status->last_seen_clock) /
                (priv->ts_pkt_count - pid_status->last_pkt_num);

        if(priv->pkt_duration_estimate == 0.0) {
            //first time
            priv->pkt_duration_estimate = duration_per_pkt;
        } else if(discontinuity_indicator == 0 && duration_per_pkt >= 0.0) {
            double transmit_duration, playout_duration;

            priv->pkt_duration_estimate =
                        duration_per_pkt * NEW_DURATION_WEIGHT +
                            priv->pkt_duration_estimate*(1-NEW_DURATION_WEIGHT);

            //also adjust the duration estimate to try to ensure that the
            //transmission rate matches the playout rate
            transmit_duration = now - pid_status->first_time;
            playout_duration = clock - pid_status->first_clock;
            if(transmit_duration > playout_duration) {
                //reduce estimate
                priv->pkt_duration_estimate *= TIME_ADJUSTMENT_FACTOR;
            } else if(transmit_duration + MAX_PLAYOUT_BUFFER_DURATION
                    < playout_duration) {
                //increase estimate
                priv->pkt_duration_estimate /= TIME_ADJUSTMENT_FACTOR;
            }
        } else {
            //the PCR has a discontinuity from its previous value, so don't
            //use this one. Reset our PCR values on this pid for next time.
            pid_status->first_clock = clock;
            pid_status->first_time = now;
        }
    }

    pid_status->last_seen_clock = clock;
    pid_status->last_seen_time = now;
    pid_status->last_pkt_num = priv->ts_pkt_count;
}

static int mp2t_get_frame2(uint8_t *dst, uint32_t dst_nbytes,
                           double *timestamp, InputStream *istream,
                           MediaProperties *properties, void *private_data)
{
    double now;
    int num_ts_pkts = dst_nbytes / TS_PKT_SIZE;
    int i, bytes_read, bytes_to_read, ts_pkts_read;

    mp2t_priv* priv = (mp2t_priv*)private_data;
    priv->ncopied = 0;

    bytes_to_read = num_ts_pkts*TS_PKT_SIZE;

    if(!bytes_to_read) {
        return ERR_EOF;
    }

    bytes_read = istream_read(istream, dst, bytes_to_read);

    if(bytes_read <= 0) {
        return ERR_EOF;
    }

    ts_pkts_read = bytes_read / TS_PKT_SIZE;

    //scan through the TS packets and estimate the duration of each packet
    now = gettimeinseconds(NULL);
    for(i=0; i<ts_pkts_read; i++) {
        update_ts_pkt_duration(priv, &dst[i*TS_PKT_SIZE], now);
    }

    *timestamp = priv->timestamp;
    properties->frame_duration = ts_pkts_read * (priv->pkt_duration_estimate);
    priv->timestamp += properties->frame_duration;

    return bytes_read;
}
#endif

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
        fnc_log(FNC_LOG_ERR, "mp2t: dropping %d bytes\n",
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
            mparser_buffer_write(tr,
                                 tr->properties.pts,
                                 tr->properties.dts,
                                 tr->properties.frame_duration,
                                 1, dst, dst_len);
            dst_len = len;
        }
    } while (ret);
    return ERR_NOERROR;
}

static void mp2t_uninit(Track *track)
{
    g_slice_free(mp2t_priv, track->private_data);
}

FNC_LIB_MEDIAPARSER(mp2t);
