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
#include <ctype.h> /* for tolower */

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

/*! SD common tags */
#define SD_STREAM           "stream"
#define SD_STREAM_END       "stream_end"
#define SD_FILENAME         "file_name"
#define SD_CLOCK_RATE       "clock_rate"
#define SD_PAYLOAD_TYPE     "payload_type"
#define SD_ENCODING_NAME    "encoding_name"
#define SD_MEDIA_TYPE       "media_type"
#define SD_AUDIO_CHANNELS   "audio_channels"
#define SD_FMTP             "fmtp"
/*! Creative commons specific tags */
#define SD_LICENSE          "license"
#define SD_RDF              "verify"
#define SD_TITLE            "title"
#define SD_CREATOR          "creator"

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

static int sd_probe(const char *filename)
{
    return g_str_has_suffix(filename, ".sd");
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

static int sd_init(Resource * r)
{
    int err = RESOURCE_OK;

    FILE *fd;

    fnc_log(FNC_LOG_DEBUG, "[sd] SD init function");
    fd = fopen(r->mrl, "r");

    r->duration = HUGE_VAL;

    do {
        int payload_type_forced = 0;
        int clock_rate_forced = 0;

        sd_private_data priv = { NULL, (mqd_t)-1 };

        Track *track;
        char keyword[80], line[1024];
        char *separator, track_file[256];
        char *name;
        char *fmtp_val = NULL;

        char commons_deed[256] = { 0, };
        char rdf_page[256] = { 0, };
        char title[256] = { 0, };
        char author[256] = { 0, };

        MediaProperties props_hints;

        memset(&props_hints, 0, sizeof(MediaProperties));
        props_hints.media_source = LIVE_SOURCE;

        *keyword = '\0';
        while (g_ascii_strcasecmp(keyword, SD_STREAM) && !feof(fd)) {
            fgets(line, sizeof(line), fd);
            sscanf(line, "%79s", keyword);
        }
        if (feof(fd))
            break;

        /* Allocate and cast TRACK PRIVATE DATA foreach track.
         * (in this case track = elementary stream media file)
         * */
        *keyword = '\0';
        while (g_ascii_strcasecmp(keyword, SD_STREAM_END) && !feof(fd)) {
            fgets(line, sizeof(line), fd);
            sscanf(line, "%79s", keyword);
            if (!g_ascii_strcasecmp(keyword, SD_FILENAME)) {
                // SD_FILENAME
                sscanf(line, "%*s%255s", track_file);

                separator = strstr(track_file, "://");
                if (separator == NULL) {
                    fnc_log(FNC_LOG_ERR, "[sd] missing valid protocol in %s entry", SD_FILENAME);
                    break;
                }
                else {
                    priv.mrl = g_strdup(track_file);
                    separator = strrchr(track_file, G_DIR_SEPARATOR);
                    name = g_strdup(separator +1);
                }
            } else if (!g_ascii_strcasecmp(keyword, SD_ENCODING_NAME)) {
                const RTP_static_payload *info;
                // SD_ENCODING_NAME
                sscanf(line, "%*s%10s", props_hints.encoding_name);

                // Automatic media detection
                if ((info = probe_stream_info(props_hints.encoding_name))) {
                    fnc_log(FNC_LOG_INFO, "[.SD] Static Payload Detected, probing info...");
                    if (!payload_type_forced)
                        set_payload_type(&props_hints, info->PldType);
                    if (!clock_rate_forced)
                        props_hints.clock_rate = info->ClockRate;
                }
            } else if (!g_ascii_strcasecmp(keyword, SD_PAYLOAD_TYPE)) {
                // SD_PAYLOAD_TYPE
                sscanf(line, "%*s %u\n", &props_hints.payload_type);
                set_payload_type(&props_hints, props_hints.payload_type);
                payload_type_forced = 1;
            } else if (!g_ascii_strcasecmp(keyword, SD_CLOCK_RATE)) {
                // SD_CLOCK_RATE
                sscanf(line, "%*s %u\n", &props_hints.clock_rate);
                clock_rate_forced = 1;
            } else if (!g_ascii_strcasecmp(keyword, SD_AUDIO_CHANNELS)) {
                // SD_AUDIO_CHANNELS
                sscanf(line, "%*s %d\n", &props_hints.audio_channels);
            } else if (!g_ascii_strcasecmp(keyword, SD_MEDIA_TYPE)) {
                char sparam[256];
                // SD_MEDIA_TYPE
                sscanf(line, "%*s%10s", sparam);
                if (g_ascii_strcasecmp(sparam, "AUDIO") == 0)
                    props_hints.media_type = MP_audio;
                if (g_ascii_strcasecmp(sparam, "VIDEO") == 0)
                    props_hints.media_type = MP_video;
            } else if (!g_ascii_strcasecmp(keyword, SD_FMTP)) {
                char *p = line;
                while (tolower(*p++) != SD_FMTP[0]);
                p += strlen(SD_FMTP);

                fmtp_val = g_strdup(p);
            } else if (!g_ascii_strcasecmp(keyword, SD_LICENSE)) {

                /*******START CC********/
                // SD_LICENSE
                sscanf(line, "%*s%255s", commons_deed);
            } else if (!g_ascii_strcasecmp(keyword, SD_RDF)) {
                // SD_RDF
                sscanf(line, "%*s%255s", rdf_page);
            } else if (!g_ascii_strcasecmp(keyword, SD_TITLE)) {
                // SD_TITLE
                int i = 0;
                char *p = line;
                while (tolower(*p++) != SD_TITLE[0]);

                p += strlen(SD_TITLE);

                while (p[i] != '\n' && i < 255) {
                    title[i] = p[i];
                    i++;
                }
                title[i] = '\0';
            } else if (!g_ascii_strcasecmp(keyword, SD_CREATOR)) {
                // SD_CREATOR
                int i = 0;
                char *p = line;
                while (tolower(*p++) != SD_CREATOR[0]);
                p += strlen(SD_CREATOR);

                while (p[i] != '\n' && i < 255) {
                    author[i] = p[i];
                    i++;
                }
                author[i] = '\0';
            }
        }        /*end while !STREAM_END or eof */

        if (!(track = add_track(r, name, &props_hints))) {
            err = -1;
            g_free(priv.mrl);
            break;
        }

        track->parser = &demuxer_sd_fake_mediaparser;
        track->private_data = g_memdup(&priv, sizeof(priv));

        g_string_append_printf(track->sdp_description,
                               SDP_F_COMMONS_DEED
                               SDP_F_RDF_PAGE
                               SDP_F_TITLE
                               SDP_F_AUTHOR,
                               commons_deed,
                               rdf_page,
                               title,
                               author);

        if ( fmtp_val ) {
            g_string_append_printf(track->sdp_description,
                                   "a=fmtp:%u %s",
                                   props_hints.payload_type,
                                   fmtp_val);
            fmtp_val = NULL;
        }

        if (props_hints.payload_type >= 96)
        {
            switch (props_hints.media_type) {
                case MP_audio:
                    g_string_append_printf(track->sdp_description,
                                           "a=rtpmap:%u %s/%d/%d\r\n",
                                           props_hints.payload_type,
                                           props_hints.encoding_name,
                                           props_hints.clock_rate,
                                           props_hints.audio_channels);
                    break;
                case MP_video:
                    g_string_append_printf(track->sdp_description,
                                           "a=rtpmap:%u %s/%d\r\n",
                                           props_hints.payload_type,
                                           props_hints.encoding_name,
                                           props_hints.clock_rate);
                    break;
                default:
                    g_assert_not_reached();
                    break;
            }
        }

        r->media_source = props_hints.media_source;

    } while (!feof(fd));

    fclose(fd);
    return err;
}

#define FNC_LIVE_PROTOCOL "mq://"
#define FNC_LIVE_PROTOCOL_LEN 5

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
         (priv->queue = mq_open(priv->mrl+FNC_LIVE_PROTOCOL_LEN,
                         O_RDONLY|O_NONBLOCK, S_IRWXU, NULL)) == (mqd_t)-1) {
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

static int sd_read_packet(Resource * r)
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
#define sd_seek NULL

static void sd_uninit(ATTR_UNUSED gpointer ptr)
{
    return;
}

static const char sd_name[] = "Live Source Description";

FENG_DEMUXER(sd, LIVE_SOURCE);
