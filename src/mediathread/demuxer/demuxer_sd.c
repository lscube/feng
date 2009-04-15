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

#include "config.h"

#include <stdbool.h>
#include <mqueue.h>

#include "feng.h"
#include "feng_utils.h"
#include "fnc_log.h"

#include "demuxer_module.h"

#include "mediathread/mediaparser.h"

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

static const RTP_static_payload RTP_payload[] ={
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
        { -1,""       ,-1   ,-1,-1,-1 },
        // Dynamic: 96-127
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 },
        { -1, "", -1, -1, -1, -1 }, { -1, "", -1, -1, -1, -1 }
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

static const DemuxerInfo info = {
    "Live Source Description",
    "sd",
    "LScube Team",
    "",
    "sd"
};

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

static int sd_probe(InputStream * i_stream)
{
    char *ext;

    if ((ext = strrchr(i_stream->name, '.')) && (!strcmp(ext, ".sd"))) {
        return RESOURCE_OK;
    }
    return RESOURCE_DAMAGED;
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
    char keyword[80], line[1024], sparam[256];
    Track *track;
    char content_base[256] = "", *separator, track_file[256];
    sdp_field *sdp_private = NULL;

    FILE *fd;

    MediaProperties props_hints;
    TrackInfo trackinfo;

    memset(&props_hints, 0, sizeof(MediaProperties));
    memset(&trackinfo, 0, sizeof(TrackInfo));

    fnc_log(FNC_LOG_DEBUG, "[sd] SD init function");
    fd = fdopen(r->i_stream->fd, "r");

    if ((separator = strrchr(r->i_stream->name, G_DIR_SEPARATOR))) {
        int len = separator - r->i_stream->name + 1;
        if (len >= sizeof(content_base)) {
            fnc_log(FNC_LOG_ERR, "[sd] content base string too long\n");
            return ERR_GENERIC;
        } else {
            strncpy(content_base, r->i_stream->name, len);
            fnc_log(FNC_LOG_DEBUG, "[sd] content base: %s\n", content_base);
        }
    }

    MObject_init(MOBJECT(&props_hints));
    MObject_init(MOBJECT(&trackinfo));

    r->info->duration = HUGE_VAL;

    do {
        int payload_type_forced = 0;
        int clock_rate_forced = 0;

        MObject_0(MOBJECT(&props_hints), MediaProperties);
        MObject_0(MOBJECT(&trackinfo), TrackInfo);
        props_hints.media_source = MS_live;

        *keyword = '\0';
        while (g_ascii_strcasecmp(keyword, SD_STREAM) && !feof(fd)) {
            fgets(line, sizeof(line), fd);
            sscanf(line, "%79s", keyword);
        }
        if (feof(fd))
            return RESOURCE_OK;

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

                separator = strstr(track_file, FNC_PROTO_SEPARATOR);
                if (separator == NULL) {
                    fnc_log(FNC_LOG_ERR, "[sd] missing valid protocol in %s entry\n", SD_FILENAME);
                    trackinfo.mrl = NULL;
                    break;
                }
                else {
                    trackinfo.mrl = g_strdup(track_file);
                    separator = strrchr(track_file, G_DIR_SEPARATOR);
                    g_strlcpy(trackinfo.name, separator + 1, sizeof(trackinfo.name));
                }
            } else if (!g_ascii_strcasecmp(keyword, SD_ENCODING_NAME)) {
                // SD_ENCODING_NAME
                sscanf(line, "%*s%10s", props_hints.encoding_name);

                // Automatic media detection
                const RTP_static_payload *info = probe_stream_info(props_hints.encoding_name);
                if (info) {
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

                sdp_private = g_new(sdp_field, 1);
                sdp_private->type = fmtp;
                sdp_private->field = g_strdup(p);
            } else if (!g_ascii_strcasecmp(keyword, SD_LICENSE)) {

                /*******START CC********/
                // SD_LICENSE
                sscanf(line, "%*s%255s", trackinfo.commons_deed);
            } else if (!g_ascii_strcasecmp(keyword, SD_RDF)) {
                // SD_RDF
                sscanf(line, "%*s%255s", trackinfo.rdf_page);
            } else if (!g_ascii_strcasecmp(keyword, SD_TITLE)) {
                // SD_TITLE
                int i = 0;
                char *p = line;
                while (tolower(*p++) != SD_TITLE[0]);

                p += strlen(SD_TITLE);

                while (p[i] != '\n' && i < 255) {
                    trackinfo.title[i] = p[i];
                    i++;
                }
                trackinfo.title[i] = '\0';
            } else if (!g_ascii_strcasecmp(keyword, SD_CREATOR)) {
                // SD_CREATOR
                int i = 0;
                char *p = line;
                while (tolower(*p++) != SD_CREATOR[0]);
                p += strlen(SD_CREATOR);

                while (p[i] != '\n' && i < 255) {
                    trackinfo.author[i] = p[i];
                    i++;
                }
                trackinfo.author[i] = '\0';
            }
        }        /*end while !STREAM_END or eof */

        if (!trackinfo.mrl)
            continue;

        if (!(track = add_track(r, &trackinfo, &props_hints)))
            return ERR_ALLOC;

        if (sdp_private)
            track->properties->sdp_private =
                g_list_prepend(track->properties->sdp_private, sdp_private);

        if (props_hints.payload_type >= 96)
        {
            sdp_private = g_new(sdp_field, 1);
            sdp_private->type = rtpmap;
            switch (props_hints.media_type) {
                case MP_audio:
                    sdp_private->field = g_strdup_printf ("%s/%d/%d",
                                        props_hints.encoding_name,
                                        props_hints.clock_rate,
                                        props_hints.audio_channels);
                    break;
                case MP_video:
                    sdp_private->field = g_strdup_printf ("%s/%d",
                                        props_hints.encoding_name,
                                        props_hints.clock_rate);
                    break;
                default:
                    break;
            }
            track->properties->sdp_private =
                g_list_prepend(track->properties->sdp_private, sdp_private);
        }

        sdp_private = NULL;
        r->info->media_source = props_hints.media_source;

    } while (!feof(fd));

    return RESOURCE_OK;
}

#define FNC_LIVE_PROTOCOL "mq://"
#define FNC_LIVE_PROTOCOL_LEN 5

static int sd_read_packet(Resource * r)
{
    TrackList tr_it;

    if (r->info->media_source != MS_live)
        return RESOURCE_NOT_PARSEABLE;

    for (tr_it = g_list_first(r->tracks); tr_it !=NULL; tr_it = g_list_next(tr_it)) {
            Track *tr = (Track*)tr_it->data;
            struct mq_attr attr;
            mqd_t mpd;

            uint8_t *msg_buffer;
            ssize_t msg_len;
            int marker;

            if ((mpd = mq_open(tr->info->mrl+FNC_LIVE_PROTOCOL_LEN, O_RDONLY, S_IRWXU, NULL)) < 0) {
                fnc_log(FNC_LOG_ERR, "Unable to open '%s', %s",
                                     tr->info->mrl, strerror(errno));
                return RESOURCE_EOF;
            }

            mq_getattr(mpd, &attr);
            msg_buffer = g_malloc(attr.mq_msgsize);
            msg_len = mq_receive(mpd, msg_buffer, attr.mq_msgsize, NULL);
            mq_close(mpd);

            if (msg_len < 0) {
                fnc_log(FNC_LOG_ERR, "Unable to read from '%s', %s",
                                     tr->info->mrl, strerror(errno));
                g_free(msg_buffer);
                return RESOURCE_EOF;
            }

            marker = (msg_buffer[1]>>7);

            tr->properties->mtime =
                ((unsigned)msg_buffer[4] << 24 |
                (unsigned)msg_buffer[5] << 16  |
                (unsigned)msg_buffer[6] << 8   |
                (unsigned)msg_buffer[7])/((double)tr->properties->clock_rate);

            mparser_buffer_write(tr, 0.0, marker, msg_buffer+12, msg_len-12);
            g_free(msg_buffer);
    }

    return RESOURCE_OK;
}

static int sd_seek(Resource * r, double time_sec)
{
    return RESOURCE_NOT_SEEKABLE;
}

static int sd_uninit(Resource * r)
{
    return RESOURCE_OK;
}

FNC_LIB_DEMUXER(sd);

