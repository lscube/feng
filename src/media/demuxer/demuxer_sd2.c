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

#include "media/demuxer.h"
#include "media/mediaparser.h"

#define REQUIRED_FLUX_PROTOCOL_VERSION 4

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
static void demuxer_sd_fake_mediaparser_uninit(Track *tr) {
    sd_private_data *priv = tr->private_data;
    g_free(priv->mrl);
    g_free(priv);
}

/**
 * @brief Fake parser description for demuxer_sd tracks
 *
 * This object is used to free the slice in Track::private_data as a
 * mqd_t object.
 */
static const MediaParser demuxer_sd_fake_mediaparser = {
    .encoding_name = NULL,
    .media_type = MP_undef,
    .init = NULL,
    .parse = NULL,
    .uninit = demuxer_sd_fake_mediaparser_uninit
};

/*
 * Struct for automatic probing of live media sources
 */
typedef struct RTP_static_payload {
        int PldType;
        char const *EncName;
        int ClockRate;      // In Hz
        short Channels;
        int BitPerSample;
        float PktLen;       // In msec
} RTP_static_payload;

static const RTP_static_payload RTP_payload[96] ={
        // Audio
        { 0 ,"PCMU"   ,8000 ,1 ,8 ,20 },
        {-1 ,""       ,-1   ,-1,-1,-1 },
        { 2 ,"G726_32",8000 ,1 ,4 ,20 },
        { 3 ,"GSM"    ,8000 ,1 ,-1,20 },
        { 4 ,"G723"   ,8000 ,1 ,-1,30 },
        { 5 ,"DVI4"   ,8000 ,1 ,4 ,20 },
        { 6 ,"DVI4"   ,16000,1 ,4 ,20 },
        { 7 ,"LPC"    ,8000 ,1 ,-1,20 },
        { 8 ,"PCMA"   ,8000 ,1 ,8 ,20 },
        { 9 ,"G722"   ,8000 ,1 ,8 ,20 },
        { 10,"L16"    ,44100,2 ,16,20 },
        { 11,"L16"    ,44100,1 ,16,20 },
        { 12,"QCELP"  ,8000 ,1 ,-1,20 },
        { -1,""       ,  -1 ,-1,-1,-1 },
        { 14,"MPA"    ,90000,1 ,-1,-1 },
        { 15,"G728"   ,8000 ,1 ,-1,20 },
        { 16,"DVI4"   ,11025,1 ,4 ,20 },
        { 17,"DVI4"   ,22050,1 ,4 ,20 },
        { 18,"G729"   ,8000 ,1 ,-1,20 },
        { -1,""       ,-1   ,-1,-1,-1 },
        { -1,""       ,-1   ,-1,-1,-1 },
        { -1,""       ,-1   ,-1,-1,-1 },
        { -1,""       ,-1   ,-1,-1,-1 },
        { -1,""       ,-1   ,-1,-1,-1 },
        // Video: 24-95 - Pkt_len in milliseconds is not specified and will be calculated in such a way
        // that each RTP packet contains a video frame (but no more than 536 byte, for UDP limitations)
        { -1,""       ,-1   ,-1,-1,-1 },
        { 25,"CelB"   ,90000,0 ,-1,-1 },
        { 26,"JPEG"   ,90000,0 ,-1,-1 },
        { -1,""       ,-1   ,-1,-1,-1 },
        { 28,"nv"     ,90000,0 ,-1,-1 },
        { -1,""       ,-1   ,-1,-1,-1 },
        { -1,""       ,-1   ,-1,-1,-1 },
        { 31,"H261"   ,90000,0 ,-1,-1 },
        { 32,"MPV"    ,90000,0 ,-1,-1 },
         { 33,"MP2T"   ,90000,0 ,-1,-1 },
        { 34,"H263"   ,90000,0 ,-1,-1 },
        { -1,""       ,-1   ,-1,-1,-1 }
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

static int sd2_probe(const char *filename)
{
    return g_str_has_suffix(filename, ".sd2");
}

//Sets payload type and probes media type from payload type
static void set_payload_type(MediaProperties *mprops, int payload_type)
{
    mprops->payload_type = payload_type;

    // Automatic media_type detection
    if (mprops->payload_type >= 0 &&
        mprops->payload_type < 24)
        mprops->media_type = MP_audio;
    if (mprops->payload_type > 23 &&
        mprops->payload_type < 96)
        mprops->media_type = MP_video;
}

static int sd2_init(Resource * r)
{
    int tracks;

    GKeyFile *file = g_key_file_new();
    gchar **tracknames, *currtrack;

    if ( !g_key_file_load_from_file(file, r->mrl, G_KEY_FILE_NONE, NULL) )
        goto error;

    if ( (tracknames = g_key_file_get_groups(file, NULL)) == NULL )
        goto error;

    r->duration = HUGE_VAL;
    r->media_source = LIVE_SOURCE;

    while ( (currtrack = *tracknames++) != NULL ) {
        Track *track;
        const RTP_static_payload *info;

        MediaProperties props_hints = {
            .media_source = LIVE_SOURCE
        };

        sd_private_data priv = {
            .mrl = NULL,
            .queue = (mqd_t)-1
        };

        gchar *track_mrl, *media_type, *encoding_name, *tmpstr;

        /**
         * @TODO We should check that the track name is composed only
         * of unreserved characters, and if it's not, refuse it.
         */

        track_mrl = g_key_file_get_string(file, currtrack,
                                          SD2_KEY_MRL,
                                          NULL);

        /* This section might have to be changed if we end up
           supporting multiple source protocoles. */
        if ( track_mrl == NULL || !g_str_has_prefix(track_mrl, "mq://") ) {
            fnc_log(FNC_LOG_ERR, "[sd2] invalid mrl '%s' for '%s'",
                    track_mrl, r->mrl);
            goto corrupted_track;
        }

        track_mrl += strlen("mq://");

        media_type = g_key_file_get_string(file, currtrack,
                                           SD2_KEY_MEDIA_TYPE,
                                           NULL);

        /* There is no need to check for media_type being NULL, as
           g_strcmp0 takes care of that */
        if ( g_strcmp0(media_type, "audio") == 0 )
            props_hints.media_type = MP_audio;
        else if ( g_strcmp0(media_type, "video") == 0 )
            props_hints.media_type = MP_video;
        else {
            fnc_log(FNC_LOG_ERR, "[sd2] invalid media type '%s' for '%s'",
                    media_type, r->mrl);
            goto corrupted_track;
        }

        props_hints.payload_type = g_key_file_get_integer(file, currtrack,
                                                          SD2_KEY_PAYLOAD_TYPE,
                                                          NULL);

        if ( props_hints.payload_type < 0 ) {
            fnc_log(FNC_LOG_ERR, "[sd2] invalid payload_type '%d' for '%s'",
                    props_hints.payload_type, r->mrl);
            goto corrupted_track;
        }

        encoding_name = g_key_file_get_string(file, currtrack,
                                              SD2_KEY_ENCODING_NAME,
                                              NULL);

        props_hints.clock_rate = g_key_file_get_integer(file, currtrack,
                                                        SD2_KEY_CLOCK_RATE,
                                                        NULL);
        if ( props_hints.clock_rate >= INT32_MAX ) {
            fnc_log(FNC_LOG_ERR, "[sd2] invalid clock_rate '%d' for '%s'",
                    props_hints.clock_rate, r->mrl);
            goto corrupted_track;
        }

        if ( props_hints.media_type == MP_audio ) {
            props_hints.audio_channels = g_key_file_get_integer(file, currtrack,
                                                                SD2_KEY_AUDIO_CHANNELS,
                                                                NULL);

            if ( props_hints.audio_channels < 0 ) {
                fnc_log(FNC_LOG_ERR, "[sd2] invalid audio_channels '%d' for '%s'",
                        props_hints.audio_channels, r->mrl);
                goto corrupted_track;
            }
        }

        /* Track has been validated
         *
         * MAKE SURE ALL MEMORY ALLOCATIONS HAPPEN BELOW THIS POINT!
         */
        priv.mrl = g_strdup(track_mrl);

        g_strlcpy(props_hints.encoding_name, encoding_name, sizeof(props_hints.encoding_name));

        if ( (info = probe_stream_info(encoding_name)) != NULL ) {
            if ( !g_key_file_has_key(file, currtrack, SD2_KEY_PAYLOAD_TYPE, NULL) )
                set_payload_type(&props_hints, info->PldType);
            if ( !g_key_file_has_key(file, currtrack, SD2_KEY_CLOCK_RATE, NULL) )
                props_hints.clock_rate = info->ClockRate;
        }

        if ( (track = add_track(r, currtrack, &props_hints)) == NULL ) {
            fnc_log(FNC_LOG_ERR, "[sd2] error adding track '%s' from '%s'",
                    currtrack, r->mrl);
            goto corrupted_track;
        }

        track->parser = &demuxer_sd_fake_mediaparser;
        track->private_data = g_memdup(&priv, sizeof(priv));

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

        if (props_hints.payload_type >= 96)
        {
            if ( props_hints.media_type == MP_audio &&
                 props_hints.audio_channels > 1 )
                g_string_append_printf(track->sdp_description,
                                       "a=rtpmap:%u %s/%d/%d\r\n",
                                       props_hints.payload_type,
                                       props_hints.encoding_name,
                                       props_hints.clock_rate,
                                       props_hints.audio_channels);
            else
                g_string_append_printf(track->sdp_description,
                                       "a=rtpmap:%u %s/%d\r\n",
                                       props_hints.payload_type,
                                       props_hints.encoding_name,
                                       props_hints.clock_rate);
        }

        /* This goes _after_ rtpmap for compatibility with older
           FFmpeg */
        if ( (tmpstr = g_key_file_get_string(file, currtrack,
                                             SD2_KEY_FMTP,
                                             NULL)) )
            g_string_append_printf(track->sdp_description,
                                   "a=fmtp:%u %s",
                                   props_hints.payload_type,
                                   tmpstr);

        tracks++;
        continue;

    corrupted_track:
        g_free(priv.mrl);
        fnc_log(FNC_LOG_ERR, "[sd2] corrupted track '%s' from '%s'",
                currtrack, r->mrl);
    }

 error:
    g_strfreev(tracknames);
    g_key_file_free(file);
    return (tracks >= 0) ? RESOURCE_OK : -1;
}

static int sd_read_packet_track(ATTR_UNUSED Resource *res, Track *tr) {
    double package_start_time;
    uint32_t package_timestamp;
    double package_duration;
    double timestamp;
    double delivery;
    double delta;

    struct mq_attr attr;
    sd_private_data *priv = tr->private_data;

    uint8_t *msg_buffer;
    uint8_t *packet;
    ssize_t msg_len;
    int marker;
    uint16_t seq_no;

    if ( priv->queue == (mqd_t)-1 &&
         (priv->queue = mq_open(priv->mrl, O_RDONLY|O_NONBLOCK, S_IRWXU, NULL)) == (mqd_t)-1) {
        fnc_log(FNC_LOG_ERR, "Unable to open '%s', %s",
                priv->mrl, strerror(errno));
        return RESOURCE_OK;
    }

    mq_getattr(priv->queue, &attr);

    /* Check if there are available packets, if it is empty flux might have recreated it */
    if (!attr.mq_curmsgs) {
        mq_close(priv->queue);
        priv->queue = (mqd_t)-1;
        usleep(30);
        return RESOURCE_OK;
    }

    msg_buffer = g_malloc(attr.mq_msgsize);

    do {
        unsigned int package_version;
        unsigned int package_start_dts;
        unsigned int package_dts;
        double package_insertion_time;

        if ( (msg_len = mq_receive(priv->queue, (char*)msg_buffer,
                                   attr.mq_msgsize, NULL)) < 0 ) {
            fnc_log(FNC_LOG_ERR, "Unable to read from '%s', %s",
                    priv->mrl, strerror(errno));
            g_free(msg_buffer);

            mq_close(priv->queue);
            priv->queue = (mqd_t)-1;

            return RESOURCE_OK;
        }

        package_version = *((unsigned int*)msg_buffer);
        if (package_version != REQUIRED_FLUX_PROTOCOL_VERSION) {
            fnc_log(FNC_LOG_FATAL, "[%s] Invalid Flux Protocol Version, expecting %d got %d",
                                   priv->mrl, REQUIRED_FLUX_PROTOCOL_VERSION, package_version);
            return RESOURCE_ERR;
        }

        package_start_time = *((double*)(msg_buffer+sizeof(unsigned int)));
        package_dts = *((unsigned int*)(msg_buffer+sizeof(double)+sizeof(unsigned int)));
        package_start_dts = *((unsigned int*)(msg_buffer+sizeof(double)+sizeof(unsigned int)*2));
        package_duration = *((double*)(msg_buffer+sizeof(double)+sizeof(unsigned int)*3));
        package_insertion_time = *((double*)(msg_buffer+sizeof(double)*2+sizeof(unsigned int)*3));

        packet = msg_buffer+sizeof(double)*3+sizeof(unsigned int)*3;
        msg_len -= (sizeof(double)*3+sizeof(unsigned int)*3);

        package_timestamp = ntohl(*(uint32_t*)(packet+4));
        delivery = (package_dts - package_start_dts)/((double)tr->properties.clock_rate);
        delta = ev_time() - package_insertion_time;

#if 0
        fprintf(stderr, "[%s] read (%5.4f) BEGIN:%5.4f START_DTS:%u DTS:%u\n",
                priv->mrl, delta, package_start_time, package_start_dts, package_dts);
#endif

        if (delta > 0.5f)
            fnc_log(FNC_LOG_INFO, "[%s] late mq packet %f, discarding..", priv->mrl, delta);

    } while(delta > 0.5f);

    tr->properties.frame_duration = package_duration/((double)tr->properties.clock_rate);
    timestamp = package_timestamp/((double)tr->properties.clock_rate);
    marker = (packet[1]>>7);
    seq_no = ((unsigned)packet[2] << 8) | ((unsigned)packet[3]);

    // calculate the duration while consuming stale packets.
    // This is an HACK that must be moved to Flux, here just to quick fix live problems
    if (!tr->properties.frame_duration) {
        if (tr->properties.dts) {
            tr->properties.frame_duration = (timestamp - tr->properties.dts);
        } else {
            tr->properties.dts = timestamp;
        }
    }


#if 0
    fprintf(stderr, "[%s] packet TS:%5.4f DELIVERY:%5.4f -> %5.4f (%5.4f)\n",
            priv->mrl,
            timestamp,
            delivery,
            package_start_time + delivery,
            ev_time() - (package_start_time + delivery));
#endif

    mparser_live_buffer_write(tr,
                         timestamp,
                         package_timestamp,
                         package_start_time + delivery,
                         tr->properties.frame_duration * 3,
                         seq_no,
                         marker,
                         packet+12, msg_len-12);

    g_free(msg_buffer);
    return RESOURCE_OK;
}

static int sd2_read_packet(Resource * r)
{
    TrackList tr_it;

    if (r->media_source != LIVE_SOURCE)
        return RESOURCE_ERR;

    for (tr_it = g_list_first(r->tracks); tr_it !=NULL; tr_it = g_list_next(tr_it)) {
        int ret;

        if ( (ret = sd_read_packet_track(r, tr_it->data)) != RESOURCE_OK )
            return ret;
    }

    return RESOURCE_OK;
}

/* Define the "sd_seek" macro to NULL so that FNC_LIB_DEMUXER will
 * pick it up and set it to NULL. This actually saves us from having
 * to devise a way to define non-seekable demuxers.
 */
#define sd2_seek NULL

static void sd2_uninit(ATTR_UNUSED gpointer ptr)
{
    return;
}

static const char sd2_name[] = "Live Source Description (v2)";

FENG_DEMUXER(sd2, LIVE_SOURCE);
