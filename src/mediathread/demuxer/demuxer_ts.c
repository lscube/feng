/* * 
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 * 
 * bufferpool is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * bufferpool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with bufferpool; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA 
 *
 * */

#include "demuxer_module.h"

static const DemuxerInfo info = {
    "MPEG TS non-demuxer",
    "mpegts",
    "LScube Team",
    "",
    "ts"
};

typedef struct {
    double first_clock;
    double last_seen_clock;
    double first_time;
    double last_seen_time;
    uint32_t last_pkt_num;
} mp2t_pid_status;

typedef struct {
    int ncopied;
    GHashTable* pid_status_table;
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

static int mpegts_probe(InputStream * i_stream)
{
    return RESOURCE_NOT_FOUND;
}

// XXX factorize defaults.
static double timescaler(Resource *r, double res_time) {
    return res_time;
}

static void value_destroy(gpointer data)
{
    g_free(data);
}

static guint pid_hash(gconstpointer pid)
{
    return (guint)pid;
}

static gboolean pid_equal(gconstpointer v1, gconstpointer v2)
{
    return v1 == v2;
}

static int mpegts_init(Resource * r)
{
    mp2t_priv *priv = g_new0(mp2t_priv, 1);
    MediaProperties props;
    Track *track = NULL;
    TrackInfo trackinfo;

    priv->pid_status_table = g_hash_table_new_full(pid_hash, pid_equal,
                                                   value_destroy,
                                                   value_destroy);

    MObject_init(MOBJECT(&trackinfo));
    MObject_0(MOBJECT(&trackinfo), TrackInfo);
    MObject_init(MOBJECT(&props));
    MObject_0(MOBJECT(&props), MediaProperties);
    props.clock_rate = 90000; //Default
    // XXX fill with the correct informations.

    //open the file

//    r->info->duration = 
    r->timescaler = timescaler;
    r->private_data = priv;
    return RESOURCE_OK;

err_alloc:
    g_free(priv);
    return ERR_PARSE;
}


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

static int mpegts_read_packet(Resource * r)
{
    // feed the parser with the right quantity of bytes...
    return RESOURCE_EOF;
    return RESOURCE_OK;
}

static int mpegts_seek(Resource * r, double time_sec)
{
    return RESOURCE_NOT_SEEKABLE;
}

static int mpegts_uninit(Resource * r)
{
    mp2t_priv* priv = r->priv;

    if(priv) {
        g_hash_table_destroy(priv->pid_status_table);
        g_free(priv);
    }

    return RESOURCE_OK;
}

FNC_LIB_DEMUXER(mpegts);

