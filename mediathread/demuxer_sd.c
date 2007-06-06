/* * 
 *  $Id$
 *
 *  This file is part of Feng
 *
 *  Feng -- Standard Streaming Server
 *
 *  Copyright (C) 2007 by
 *
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
 * 
 *  Feng is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Feng is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Feng; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * */
#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>

#include <fenice/utils.h>
#include <fenice/fnc_log.h>

#include <fenice/multicast.h>    /*is_valid_multicast_address */
#include <fenice/rtsp.h>    /*parse_url */
#include <fenice/rtpptdefs.h>    /*payload type definitions */

#include <fenice/demuxer_module.h>

static DemuxerInfo info = {
    "Source Description",
    "sd",
    "OMSP Team",
    "",
    "sd"
};


static int sd_probe(InputStream * i_stream)
{
    char *ext;

    if ((ext = strrchr(i_stream->name, '.')) && (!strcmp(ext, ".sd"))) {
        return RESOURCE_OK;
    }
    return RESOURCE_DAMAGED;
}

static int sd_init(Resource * r)
{
    char keyword[80], line[256], sparam[10];
    Track *track;
    char object[255], server[255];
    unsigned short port;
    char content_base[256] = "", *separator, track_file[256];

    FILE *fd;

    MediaProperties props_hints;
    TrackInfo trackinfo;

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

    do {
        MObject_0(MOBJECT(&props_hints), MediaProperties);
        MObject_0(MOBJECT(&trackinfo), TrackInfo);

        *keyword = '\0';
        while (strcasecmp(keyword, SD_STREAM) && !feof(fd)) {
            fgets(line, sizeof(line), fd);
            sscanf(line, "%79s", keyword);
            /*
             *- validate multicast
             *- validate twin
             * */
            if (!strcasecmp(keyword, SD_TWIN)) {
                sscanf(line, "%*s%s", r->info->twin);
                parse_url(r->info->twin, server, sizeof(server),
                          &port, object, sizeof (object));    //FIXME
//                printf("SD: server:%s\npath:%s\n", server, object);
            } else if (!strcasecmp(keyword, SD_MULTICAST)) {
                sscanf(line, "%*s%15s", r->info->multicast);
                // if (!is_valid_multicast_address(sd->multicast))
                if (!is_valid_multicast_address(r->info->multicast))
                    //strcpy(sd->multicast, DEFAULT_MULTICAST_ADDRESS);
                    strcpy(r->info->multicast, DEFAULT_MULTICAST_ADDRESS);
            }
        }
        if (feof(fd))
            return RESOURCE_OK;

        /* Allocate and cast TRACK PRIVATE DATA foreach track.
         * (in this case track = elementary stream media file)
         * */
        *keyword = '\0';
        while (strcasecmp(keyword, SD_STREAM_END) && !feof(fd)) {
            fgets(line, sizeof(line), fd);
            sscanf(line, "%s", keyword);
            if (!strcasecmp(keyword, SD_FILENAME)) {
                // SD_FILENAME
                sscanf(line, "%*s%255s", track_file);
                // if ( *track_file == '/')
                if ((*track_file == G_DIR_SEPARATOR)
                    || (separator = strstr(track_file, FNC_PROTO_SEPARATOR)))
                    // is filename absolute path or complete mrl?
                    trackinfo.mrl = g_strdup(track_file);
                else
                    trackinfo.mrl = g_strdup_printf("%s%s", content_base, track_file);

                if ((separator = strrchr(track_file, G_DIR_SEPARATOR)))
                    g_strlcpy(trackinfo.name, separator + 1, sizeof(trackinfo.name));
                else
                    g_strlcpy(trackinfo.name, track_file, sizeof(trackinfo.name));

            } else if (!strcasecmp(keyword, SD_ENCODING_NAME)) {
                // SD_ENCODING_NAME
                sscanf(line, "%*s%10s", props_hints.encoding_name);
            } else if (!strcasecmp(keyword, SD_PRIORITY)) {
                // SD_PRIORITY //XXX once pt change is back...
//                sscanf(line, "%*s %d\n", &me.data.priority);
            } else if (!strcasecmp(keyword, SD_BITRATE)) {
                // SD_BITRATE
                sscanf(line, "%*s %d\n", &props_hints.bit_rate);
            } else if (!strcasecmp(keyword, SD_PAYLOAD_TYPE)) {
                // SD_PAYLOAD_TYPE
                sscanf(line, "%*s %u\n", &props_hints.payload_type);
                // Automatic media_type detection
                if (props_hints.payload_type >= 0 &&
                    props_hints.payload_type < 24)
                    props_hints.media_type = MP_audio;
                if (props_hints.payload_type > 23 &&
                    props_hints.payload_type < 96)
                    props_hints.media_type = MP_video;
            } else if (!strcasecmp(keyword, SD_CLOCK_RATE)) {
                // SD_CLOCK_RATE
                sscanf(line, "%*s %u\n", &props_hints.clock_rate);
            } else if (!strcasecmp(keyword, SD_AUDIO_CHANNELS)) {
                // SD_AUDIO_CHANNELS
                sscanf(line, "%*s %d\n", &props_hints.audio_channels);
            } else if (!strcasecmp(keyword, SD_SAMPLE_RATE)) {
                // SD_SAMPLE_RATE
                sscanf(line, "%*s%f", &props_hints.sample_rate);
            } else if (!strcasecmp(keyword, SD_BIT_PER_SAMPLE)) {
                // SD_BIT_PER_SAMPLE
                sscanf(line, "%*s%u", &props_hints.bit_per_sample);
            } else if (!strcasecmp(keyword, SD_CODING_TYPE)) {
                // SD_CODING_TYPE
                sscanf(line, "%*s%10s", sparam);
            //XXX remove this later...
                if (strcasecmp(sparam, "FRAME") == 0)
                    props_hints.coding_type = mc_frame;
                else if (strcasecmp(sparam, "SAMPLE") == 0)
                    props_hints.coding_type = mc_sample;
            } else if (!strcasecmp(keyword, SD_FRAME_RATE)) {
                // SD_FRAME_RATE
                sscanf(line, "%*s%u", &props_hints.frame_rate);
            } else if (!strcasecmp(keyword, SD_MEDIA_SOURCE)) {
                // SD_MEDIA_SOURCE
                sscanf(line, "%*s%10s", sparam);
                if (strcasecmp(sparam, "STORED") == 0)
                    props_hints.media_source = MS_stored;
                if (strcasecmp(sparam, "LIVE") == 0)
                    props_hints.media_source = MS_live;
            } else if (!strcasecmp(keyword, SD_MEDIA_TYPE)) {
                // SD_MEDIA_TYPE
                sscanf(line, "%*s%10s", sparam);
                if (strcasecmp(sparam, "AUDIO") == 0)
                    props_hints.media_type = MP_audio;
                if (strcasecmp(sparam, "VIDEO") == 0)
                    props_hints.media_type = MP_video;
            } else if (!strcasecmp(keyword, SD_LICENSE)) {

                /*******START CC********/
                // SD_LICENSE
                sscanf(line, "%*s%s", trackinfo.commons_deed);
            } else if (!strcasecmp(keyword, SD_RDF)) {
                // SD_RDF
                sscanf(line, "%*s%s", trackinfo.rdf_page);
            } else if (!strcasecmp(keyword, SD_TITLE)) {
                // SD_TITLE
                int i = 0;
                char *p = strcasestr(line, SD_TITLE) + strlen(SD_TITLE);

                while (p[i] != '\n') {
                    trackinfo.title[i] = p[i];
                    i++;
                }
                trackinfo.title[i] = '\0';
            } else if (!strcasecmp(keyword, SD_CREATOR)) {
                // SD_CREATOR
                int i = 0;
                char *p = strcasestr(line, SD_CREATOR) + strlen(SD_CREATOR);

                while (p[i] != '\n') {
                    trackinfo.author[i] = p[i];
                    i++;
                }
                trackinfo.author[i] = '\0';
            }

            /********END CC*********/
        }        /*end while !STREAM_END or eof */

        if (!(track = add_track(r, &trackinfo, &props_hints)))
            return ERR_ALLOC;

        r->info->media_source = props_hints.media_source;

    } while (!feof(fd));

    return RESOURCE_OK;
}

static int sd_read_packet(Resource * r)
{
    switch(r->info->media_source) {
        case MS_stored:
            return RESOURCE_NOT_PARSEABLE;
        case MS_live:
            return RESOURCE_OK;
        default:
            return RESOURCE_DAMAGED;
    }
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

