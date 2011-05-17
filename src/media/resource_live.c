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

#include <stdbool.h>
#include <time.h>
#include <mqueue.h>
#include <fcntl.h> /* for mq_open's O_* options */
#include <errno.h>
#include <unistd.h> /* for usleep() */
#include <string.h>
#include <math.h>

#include "feng.h"
#include "fnc_log.h"

#include "media/media.h"

#define REQUIRED_FLUX_PROTOCOL_VERSION 4

static gpointer flux_read_messages(gpointer ptr);

typedef struct sd_private_data {
    char *mrl;
    mqd_t queue;
} sd_private_data;

/**
 * @brief Uninitialisation function for the demuxer_sd fake parser
 *
 * This function simply frees the slice in Track::private_data as a
 * mqd_t object.
 */
static void live_track_uninit(Track *tr) {
    sd_private_data *priv = tr->private_data;
    g_free(priv->mrl);
    g_free(priv);

    tr->private_data = NULL;
}

/*
 * Struct for automatic probing of live media sources
 */
typedef struct RTP_static_payload {
        char EncName[8];
        int PldType;
        int ClockRate;      // In Hz
        short Channels;
        int BitPerSample;
        float PktLen;       // In msec
} RTP_static_payload;

static const RTP_static_payload RTP_payload[96] ={
        // Audio
        {"PCMU"   , 0, 8000 ,1 ,8 ,20 },
        {""       ,-1, -1   ,-1,-1,-1 },
        {"G726_32", 2, 8000 ,1 ,4 ,20 },
        {"GSM"    , 3, 8000 ,1 ,-1,20 },
        {"G723"   , 4, 8000 ,1 ,-1,30 },
        {"DVI4"   , 5, 8000 ,1 ,4 ,20 },
        {"DVI4"   , 6, 16000,1 ,4 ,20 },
        {"LPC"    , 7, 8000 ,1 ,-1,20 },
        {"PCMA"   , 8, 8000 ,1 ,8 ,20 },
        {"G722"   , 9, 8000 ,1 ,8 ,20 },
        {"L16"    ,10, 44100,2 ,16,20 },
        {"L16"    ,11, 44100,1 ,16,20 },
        {"QCELP"  ,12, 8000 ,1 ,-1,20 },
        {""       ,-1,   -1 ,-1,-1,-1 },
        {"MPA"    ,14, 90000,1 ,-1,-1 },
        {"G728"   ,15, 8000 ,1 ,-1,20 },
        {"DVI4"   ,16, 11025,1 ,4 ,20 },
        {"DVI4"   ,17, 22050,1 ,4 ,20 },
        {"G729"   ,18, 8000 ,1 ,-1,20 },
        {""       ,-1, -1   ,-1,-1,-1 },
        {""       ,-1, -1   ,-1,-1,-1 },
        {""       ,-1, -1   ,-1,-1,-1 },
        {""       ,-1, -1   ,-1,-1,-1 },
        {""       ,-1, -1   ,-1,-1,-1 },
        // Video: 24-95 - Pkt_len in milliseconds is not specified and will be calculated in such a way
        // that each RTP packet contains a video frame (but no more than 536 byte, for UDP limitations)
        {""       ,-1, -1   ,-1,-1,-1 },
        {"CelB"   ,25, 90000,0 ,-1,-1 },
        {"JPEG"   ,26, 90000,0 ,-1,-1 },
        {""       ,-1, -1   ,-1,-1,-1 },
        {"nv"     ,28, 90000,0 ,-1,-1 },
        {""       ,-1, -1   ,-1,-1,-1 },
        {""       ,-1, -1   ,-1,-1,-1 },
        {"H261"   ,31, 90000,0 ,-1,-1 },
        {"MPV"    ,32, 90000,0 ,-1,-1 },
        {"MP2T"   ,33, 90000,0 ,-1,-1 },
        {"H263"   ,34, 90000,0 ,-1,-1 },
        {""       ,-1, -1   ,-1,-1,-1 }
};

/**
 * @defgroup sd2keys SD2 format keys
 *
 * @note These are defined as constant arrays of characters rather
 *       than as literal macros. This is because they are never
 *       injected directly in other literals, and having them this way
 *       can warn if support for them is mistakenly removed from the
 *       logic code.
 *
 * @{
 */
static const char SD2_KEY_MRL            [] = "mrl";
static const char SD2_KEY_CLOCK_RATE     [] = "clock_rate";
static const char SD2_KEY_PAYLOAD_TYPE   [] = "payload_type";
static const char SD2_KEY_ENCODING_NAME  [] = "encoding_name";
static const char SD2_KEY_MEDIA_TYPE     [] = "media_type";
static const char SD2_KEY_AUDIO_CHANNELS [] = "audio_channels";
static const char SD2_KEY_FMTP           [] = "fmtp";

static const char SD2_KEY_LICENSE        [] = "license";
static const char SD2_KEY_RDF_PAGE       [] = "rdf_page";
static const char SD2_KEY_TITLE          [] = "title";
static const char SD2_KEY_CREATOR        [] = "creator";
/**
 * @}
 */

//Probe informations from RTPPTDEFS table form codec_name
static const RTP_static_payload * probe_stream_info(char const *codec_name)
{
    int i;
    for (i=0; i<96; ++i) {
        if (strcmp(RTP_payload[i].EncName, codec_name) == 0)
            return &(RTP_payload[i]);
    }

    return NULL;
}

//Sets payload type and probes media type from payload type
static void set_payload_type(Track *track, int payload_type)
{
    track->payload_type = payload_type;

    // Automatic media_type detection
    if (track->payload_type >= 0 &&
        track->payload_type < 24)
        track->media_type = MP_audio;
    if (track->payload_type > 23 &&
        track->payload_type < 96)
        track->media_type = MP_video;
}

Resource *sd2_open(const char *url)
{
    Resource *r = NULL;
    char *mrl;
    GKeyFile *file = g_key_file_new();
    gchar **tracknames = NULL, **trackgroups = NULL, *currtrack = NULL;
    int next_dynamic_payload = 96;
    TrackList tracks = NULL;

    fnc_log(FNC_LOG_DEBUG,
            "using live streaming demuxer for resource '%s'",
            url);

    mrl = g_strdup_printf("%s/%s.sd2",
                          feng_default_vhost->virtuals_root,
                          url);

    if ( !g_key_file_load_from_file(file, mrl, G_KEY_FILE_NONE, NULL) )
        goto error;

    if ( (tracknames = g_key_file_get_groups(file, NULL)) == NULL )
        goto error;

    trackgroups = tracknames;

    while ( (currtrack = *tracknames++) != NULL ) {
        Track *track = NULL;
        const RTP_static_payload *info;
        sd_private_data *priv;

        gchar *track_mrl, *media_type, *tmpstr;

        if ( !feng_str_is_unreserved(currtrack) ) {
            fnc_log(FNC_LOG_ERR, "[sd2] invalid track name '%s' for '%s'",
                    currtrack, mrl);
            goto corrupted_track;
        }

        track = track_new(currtrack);

        track->private_data = priv = g_slice_new(sd_private_data);
        priv->queue = (mqd_t)-1;

        track_mrl = g_key_file_get_string(file, currtrack,
                                          SD2_KEY_MRL,
                                          NULL);

        /* This section might have to be changed if we end up
           supporting multiple source protocoles. */
        if ( track_mrl == NULL || !g_str_has_prefix(track_mrl, "mq://") ) {
            fnc_log(FNC_LOG_ERR, "[sd2] invalid mrl '%s' for '%s'",
                    track_mrl, mrl);
            goto corrupted_track;
        }

        priv->mrl = strdup(track_mrl + strlen("mq://"));

        media_type = g_key_file_get_string(file, currtrack,
                                           SD2_KEY_MEDIA_TYPE,
                                           NULL);

        /* There is no need to check for media_type being NULL, as
           g_strcmp0 takes care of that */
        if ( g_strcmp0(media_type, "audio") == 0 )
            track->media_type = MP_audio;
        else if ( g_strcmp0(media_type, "video") == 0 )
            track->media_type = MP_video;
        else {
            fnc_log(FNC_LOG_ERR, "[sd2] invalid media type '%s' for '%s'",
                    media_type, mrl);
            goto corrupted_track;
        }

        track->payload_type = g_key_file_get_integer(file, currtrack,
                                                          SD2_KEY_PAYLOAD_TYPE,
                                                          NULL);

        /* The next_dynamic_payload variable is initialised to be the
           first dynamic payload; if the resource define some dynamic
           payload, make sure that the next used is higher. */
        if ( track->payload_type >= next_dynamic_payload )
            next_dynamic_payload = track->payload_type +1;

        if ( track->payload_type < 0 ) {
            fnc_log(FNC_LOG_ERR, "[sd2] invalid payload_type '%d' for '%s'",
                    track->payload_type, mrl);
            goto corrupted_track;
        }

        track->encoding_name = g_strdup(g_key_file_get_string(file, currtrack,
                                                                         SD2_KEY_ENCODING_NAME,
                                                                         NULL));

        track->clock_rate = g_key_file_get_integer(file, currtrack,
                                                        SD2_KEY_CLOCK_RATE,
                                                        NULL);
        if ( track->clock_rate >= INT32_MAX || track->clock_rate <= 0 ) {
            fnc_log(FNC_LOG_ERR, "[sd2] invalid clock_rate '%d' for '%s'",
                    track->clock_rate, mrl);
            goto corrupted_track;
        }

        if ( track->media_type == MP_audio ) {
            track->audio_channels = g_key_file_get_integer(file, currtrack,
                                                                SD2_KEY_AUDIO_CHANNELS,
                                                                NULL);

            if ( track->audio_channels <= 0 ) {
                fnc_log(FNC_LOG_ERR, "[sd2] invalid audio_channels '%d' for '%s'",
                        track->audio_channels, mrl);
                goto corrupted_track;
            }
        }

        if ( (info = probe_stream_info(track->encoding_name)) != NULL ) {
            if ( !g_key_file_has_key(file, currtrack, SD2_KEY_PAYLOAD_TYPE, NULL) )
                set_payload_type(track, info->PldType);
            if ( !g_key_file_has_key(file, currtrack, SD2_KEY_CLOCK_RATE, NULL) )
                track->clock_rate = info->ClockRate;
        } else if ( track->payload_type == 0 ) {
            /* if the user didn't explicit a payload type, and we
             * haven't found a static one for the named encoding,
             * assume a dynamic payload is desired and use the first
             * one available.
             */
            track->payload_type = next_dynamic_payload++;
        }

        track->uninit = live_track_uninit;

        if ( (tmpstr = g_key_file_get_string(file, currtrack,
                                             SD2_KEY_LICENSE,
                                             NULL)) )
             g_string_append_printf(track->sdp_description,
                                    SDP_F_COMMONS_DEED,
                                    tmpstr);

        if ( (tmpstr = g_key_file_get_string(file, currtrack,
                                             SD2_KEY_RDF_PAGE,
                                             NULL)) )
             g_string_append_printf(track->sdp_description,
                                    SDP_F_RDF_PAGE,
                                    tmpstr);

        if ( (tmpstr = g_key_file_get_string(file, currtrack,
                                             SD2_KEY_TITLE,
                                             NULL)) )
             g_string_append_printf(track->sdp_description,
                                    SDP_F_TITLE,
                                    tmpstr);

        if ( (tmpstr = g_key_file_get_string(file, currtrack,
                                             SD2_KEY_CREATOR,
                                             NULL)) )
             g_string_append_printf(track->sdp_description,
                                    SDP_F_AUTHOR,
                                    tmpstr);

        if (track->payload_type >= 96)
        {
            if ( track->media_type == MP_audio &&
                 track->audio_channels > 1 )
                g_string_append_printf(track->sdp_description,
                                       "a=rtpmap:%u %s/%d/%d\r\n",
                                       track->payload_type,
                                       track->encoding_name,
                                       track->clock_rate,
                                       track->audio_channels);
            else
                g_string_append_printf(track->sdp_description,
                                       "a=rtpmap:%u %s/%d\r\n",
                                       track->payload_type,
                                       track->encoding_name,
                                       track->clock_rate);
        }

        /* This goes _after_ rtpmap for compatibility with older
           FFmpeg */
        if ( (tmpstr = g_key_file_get_string(file, currtrack,
                                             SD2_KEY_FMTP,
                                             NULL)) )
            g_string_append_printf(track->sdp_description,
                                   "a=fmtp:%u %s",
                                   track->payload_type,
                                   tmpstr);

        tracks = g_list_append(tracks, track);
        continue;

    corrupted_track:
        fnc_log(FNC_LOG_ERR, "[sd2] corrupted track '%s' from '%s'",
                currtrack, mrl);
        if ( track ) {
            g_free(priv->mrl);
            track_free(track);
        }
    }

    if ( tracks == NULL )
        goto error;

    r = g_slice_new0(Resource);
    r->mrl = mrl;
    r->lock = g_mutex_new();

    r->source = LIVE_SOURCE;
    r->duration = HUGE_VAL;
    r->tracks = tracks;

    for (tracks = g_list_first(r->tracks); tracks != NULL; tracks = g_list_next(tracks)) {
        Track *track = tracks->data;

        track->parent = r;
        g_thread_create(flux_read_messages, track, false, NULL);
    }

    return r;

 error:
    g_slice_free(Resource, r);
    g_strfreev(trackgroups);
    g_key_file_free(file);
    g_free(mrl);
    return NULL;
}

static gpointer flux_read_messages(gpointer ptr) {
    Track *tr = ptr;

    sd_private_data *priv = tr->private_data;;

    while ( tr ) {
        double package_start_time;
        uint32_t package_timestamp;
        double package_duration;
        double timestamp;
        double delivery;
        double delta;
        unsigned int package_version;
        unsigned int package_start_dts;
        unsigned int package_dts;
        double package_insertion_time;

        uint8_t *msg_buffer = NULL;
        uint8_t *packet;
        ssize_t msg_len;
        int marker;
        uint16_t seq_no;

        struct mq_attr attr;
        struct MParserBuffer *buffer;

        if ( priv->queue == (mqd_t)-1 &&
             (priv->queue = mq_open(priv->mrl, O_RDONLY|O_NONBLOCK, S_IRWXU, NULL)) == (mqd_t)-1) {

            fnc_log(FNC_LOG_ERR, "Unable to open '%s', %s",
                    priv->mrl, strerror(errno));
            goto reiterate;
        }

        mq_getattr(priv->queue, &attr);

        /* Check if there are available packets, if it is empty flux might have recreated it */
        if (!attr.mq_curmsgs) {
            mq_close(priv->queue);
            priv->queue = (mqd_t)-1;
            usleep(30);

            goto reiterate;
        }

        msg_buffer = g_malloc(attr.mq_msgsize);

        if ( (msg_len = mq_receive(priv->queue, (char*)msg_buffer,
                                   attr.mq_msgsize, NULL)) < 0 ) {
            fnc_log(FNC_LOG_ERR, "Unable to read from '%s', %s",
                    priv->mrl, strerror(errno));

            mq_close(priv->queue);
            priv->queue = (mqd_t)-1;

            goto reiterate;
        }

        package_version = *((unsigned int*)msg_buffer);
        if (package_version != REQUIRED_FLUX_PROTOCOL_VERSION) {
            fnc_log(FNC_LOG_FATAL, "[%s] Invalid Flux Protocol Version, expecting %d got %d",
                    priv->mrl, REQUIRED_FLUX_PROTOCOL_VERSION, package_version);

            goto reiterate;
        }

        package_start_time = *((double*)(msg_buffer+sizeof(unsigned int)));
        package_dts = *((unsigned int*)(msg_buffer+sizeof(double)+sizeof(unsigned int)));
        package_start_dts = *((unsigned int*)(msg_buffer+sizeof(double)+sizeof(unsigned int)*2));
        package_duration = *((double*)(msg_buffer+sizeof(double)+sizeof(unsigned int)*3));
        package_insertion_time = *((double*)(msg_buffer+sizeof(double)*2+sizeof(unsigned int)*3));

        packet = msg_buffer+sizeof(double)*3+sizeof(unsigned int)*3;
        msg_len -= (sizeof(double)*3+sizeof(unsigned int)*3);

        package_timestamp = ntohl(*(uint32_t*)(packet+4));
        delivery = (package_dts - package_start_dts)/((double)tr->clock_rate);
        delta = ev_time() - package_insertion_time;

#if 0
        fprintf(stderr, "[%s] read (%5.4f) BEGIN:%5.4f START_DTS:%u DTS:%u\n",
                priv->mrl, delta, package_start_time, package_start_dts, package_dts);
#endif

        if (delta > 0.5f) {
            fnc_log(FNC_LOG_INFO, "[%s] late mq packet %f/%f, discarding..", priv->mrl, package_insertion_time, delta);
            goto reiterate;
        }

        tr->frame_duration = package_duration/((double)tr->clock_rate);
        timestamp = package_timestamp/((double)tr->clock_rate);
        marker = (packet[1]>>7);
        seq_no = ((unsigned)packet[2] << 8) | ((unsigned)packet[3]);

        // calculate the duration while consuming stale packets.
        // This is an HACK that must be moved to Flux, here just to quick fix live problems
        if (!tr->frame_duration) {
            if (tr->dts) {
                tr->frame_duration = (timestamp - tr->dts);
            } else {
                tr->dts = timestamp;
            }
        }

        buffer = g_slice_new0(struct MParserBuffer);

        buffer->timestamp = timestamp;
        buffer->delivery = package_start_time + delivery;
        buffer->duration = tr->frame_duration * 3;

        buffer->marker = marker;
        buffer->seq_no = seq_no;
        buffer->rtp_timestamp = package_timestamp;

        buffer->data_size = msg_len - 12;
        buffer->data = g_memdup(packet + 12, buffer->data_size);

#if 0
        fprintf(stderr, "[%s] packet TS:%5.4f DELIVERY:%5.4f -> %5.4f (%5.4f)\n",
                priv->mrl,
                timestamp,
                delivery,
                package_start_time + delivery,
                ev_time() - (package_start_time + delivery));
#endif

        track_write(tr, buffer);

    reiterate:
        g_free(msg_buffer);
    }

    return NULL;
}
