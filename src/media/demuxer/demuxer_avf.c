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
#include <stdio.h>
#include <stdbool.h>

#include "fnc_log.h"

#include "media/demuxer.h"
#include "media/mediaparser.h"

#include <libavformat/avformat.h>

/**
 * @brief Simple structure for libavformat demuxer private data
 */
typedef struct avf_private_data {
    AVFormatContext *avfc;

    /**
     * @brief Array of tracks
     *
     * The size of this array is the same as avfc->nb_streams as
     * that's what it is indexed on. This causes a little
     * overallocation if there are tracks that are ignored, but should
     * be acceptable rather than running over all of them when reading
     * packets.
     */
    Track **tracks;
} avf_private_data;

/**
 * @brief Mutex to protect usage of the FFmpeg functions.
 *
 * The avcodec_open() and avcodec_close() functions are not thread
 * safe, and needs lock-protection; the (various) libavformat
 * functions we call for probile the files and getting data out of
 * them call these functions indirectly, and needs thus to be
 * protected.
 */
GMutex *ffmpeg_lock;

static void init_mutex()
{
    static gsize inited;
    if ( g_once_init_enter(&inited) ) {
        ffmpeg_lock = g_mutex_new();

        g_once_init_leave(&inited, true);
    }
}

typedef struct id_tag {
    const int id;
    const int pt;
    const char tag[11];
} id_tag;

//FIXME this should be simplified!
static const id_tag id_tags[] = {
   { CODEC_ID_MPEG1VIDEO, 32, "MPV" },
   { CODEC_ID_MPEG2VIDEO, 32, "MPV" },
   { CODEC_ID_H264, 96, "H264" },
   { CODEC_ID_MP2, 14, "MPA" },
   { CODEC_ID_MP3, 14, "MPA" },
   { CODEC_ID_VORBIS, 96, "VORBIS" },
   { CODEC_ID_THEORA, 96, "THEORA" },
   { CODEC_ID_SPEEX, 96, "SPEEX" },
   { CODEC_ID_AAC, 96, "AAC" },
   { CODEC_ID_MPEG4, 96, "MP4V-ES" },
   { CODEC_ID_H263, 96, "H263P" },
   { CODEC_ID_AMR_NB, 96, "AMR" },
   { CODEC_ID_VP8, 96, "VP8" },
   { CODEC_ID_NONE, 0, "NONE"} //XXX ...
};

static const char *tag_from_id(int id)
{
    const id_tag *tags = id_tags;
    while (tags->id != CODEC_ID_NONE) {
        if (tags->id == id)
            return tags->tag;
        tags++;
    }
    return NULL;
}

static int pt_from_id(int id)
{
    const id_tag *tags = id_tags;
    while (tags->id != CODEC_ID_NONE) {
        if (tags->id == id)
            return tags->pt;
        tags++;
    }
    return 0;
}

#define PROBE_BUF_SIZE 2048

static gboolean avf_probe(const char *filename)
{
    AVProbeData avpd = {
        .filename = filename
    };
    AVInputFormat *avif;
    uint8_t buffer[PROBE_BUF_SIZE];
    size_t rsize;

    FILE *fp = fopen(filename, "r");
    if ( !fp )
        return RESOURCE_ERR;

    rsize = fread(&buffer, 1, sizeof(buffer), fp);
    fclose(fp);

    avpd.buf = &buffer[0];
    avpd.buf_size = rsize;

    av_register_all();

    init_mutex();

    g_mutex_lock(ffmpeg_lock);
    avif = av_probe_input_format(&avpd, 1);
    g_mutex_unlock(ffmpeg_lock);

    return !!avif;
}

static double avf_timescaler (ATTR_UNUSED Resource *r, double res_time) {
    return res_time;
}

static int avf_init(Resource * r)
{
    AVFormatParameters ap;
    MediaProperties props;
    Track *track = NULL;
    int pt = 96, i;
    unsigned int j;

    avf_private_data priv = { NULL, NULL };

    init_mutex();

    memset(&ap, 0, sizeof(AVFormatParameters));

    priv.avfc = avformat_alloc_context();
    ap.prealloced_context = 1;

    priv.avfc->flags |= AVFMT_FLAG_GENPTS;

    g_mutex_lock(ffmpeg_lock);
    i = av_open_input_file(&priv.avfc, r->mrl, NULL, 0, &ap);
    g_mutex_unlock(ffmpeg_lock);

    if ( i != 0 ) {
        fnc_log(FNC_LOG_DEBUG, "[avf] Cannot open %s", r->mrl);
        goto err_alloc;
    }

    g_mutex_lock(ffmpeg_lock);
    i = av_find_stream_info(priv.avfc);
    g_mutex_unlock(ffmpeg_lock);

    if(i < 0){
        fnc_log(FNC_LOG_DEBUG, "[avf] Cannot find streams in file %s",
                r->mrl);
        goto err_alloc;
    }

    r->duration = (double)priv.avfc->duration /AV_TIME_BASE;
    // make sure we can seek.
    r->seekable = !av_seek_frame(priv.avfc, -1, 0, 0);

    priv.tracks = g_new(Track*, priv.avfc->nb_streams);

    for(j=0; j<priv.avfc->nb_streams; j++) {
        AVStream *st= priv.avfc->streams[j];
        AVCodecContext *codec= st->codec;
        const char *id = tag_from_id(codec->codec_id);
        float frame_rate = 0;

        if( id == NULL ) {
            fnc_log(FNC_LOG_DEBUG, "[avf] Cannot map stream id %d", j);
            continue;
        }

        if (codec->codec_id == CODEC_ID_AAC &&
            !codec->extradata_size) {
            AVPacket pkt;
            st->codec->opaque = av_bitstream_filter_init("aac_adtstoasc");
            if (!st->codec->opaque)
                goto err_alloc;
            while((av_read_frame(priv.avfc, &pkt) >= 0) && !codec->extradata_size) {
                uint8_t *data;
                int size;
                if(pkt.stream_index != j)
                    continue;
                av_bitstream_filter_filter(st->codec->opaque, codec, NULL,
                                           &data, &size,
                                           pkt.data, pkt.size,
                                           pkt.flags & AV_PKT_FLAG_KEY);
                break;
            }
            av_seek_frame(priv.avfc, -1, 0, 0);
            if (!codec->extradata_size)
                goto err_alloc;
        }
        memset(&props, 0, sizeof(MediaProperties));
        props.clock_rate = 90000; //Default
        props.extradata = codec->extradata;
        props.extradata_len = codec->extradata_size;
        strncpy(props.encoding_name, id, 11);
        props.payload_type = pt_from_id(codec->codec_id);
        if (props.payload_type >= 96)
            props.payload_type = pt++;
        fnc_log(FNC_LOG_DEBUG, "[avf] Parsing AVStream %s",
                props.encoding_name);

        switch(codec->codec_type){
            case CODEC_TYPE_AUDIO:
                props.media_type     = MP_audio;
                // Some properties, add more?
                props.audio_channels = codec->channels;
                // Make props an int...
                props.frame_duration       = (double)1 / codec->sample_rate;
                break;

            case CODEC_TYPE_VIDEO:
                props.media_type   = MP_video;
                frame_rate         = av_q2d(st->r_frame_rate);
                props.frame_duration     = (double)1 / frame_rate;
                break;

            default:
                fnc_log(FNC_LOG_DEBUG, "[avf] codec type unsupported");
                continue;
        }

        if ( !(track = priv.tracks[j] = add_track(r, g_strdup_printf("%d", j), &props)) )
            goto err_alloc;

        g_string_append_printf(track->sdp_description,
                               SDP_F_TITLE SDP_F_AUTHOR,
                               priv.avfc->title,
                               priv.avfc->author);

        if ( frame_rate != 0 )
            g_string_append_printf(track->sdp_description,
                                   "a=framerate:%f\r\n",
                                   frame_rate);
    }

    if (track) {
        fnc_log(FNC_LOG_DEBUG, "[avf] duration %f", r->duration);
        r->private_data = g_memdup(&priv, sizeof(priv));
        r->timescaler = avf_timescaler;
        return 0;
    }

 err_alloc:
    for(j = 0; j < priv.avfc->nb_streams; j++)
        if ( priv.tracks[j] )
            free_track(priv.tracks[j], NULL);

    g_mutex_lock(ffmpeg_lock);
    av_close_input_file(priv.avfc);
    g_mutex_unlock(ffmpeg_lock);

    return -1;
}

static int avf_read_packet(Resource * r)
{
    int ret = RESOURCE_OK;
    AVPacket pkt;
    AVStream *stream;
    AVBitStreamFilterContext *bsfc;
    avf_private_data *priv = r->private_data;
    Track *tr;

// get a packet
    if(av_read_frame(priv->avfc, &pkt) < 0)
        return RESOURCE_EOF; //FIXME

    if ( (tr = priv->tracks[pkt.stream_index]) == NULL )
        return RESOURCE_ERR;

    // push it to the framer
    stream = priv->avfc->streams[pkt.stream_index];

    fnc_log(FNC_LOG_VERBOSE, "[avf] Parsing track %s",
            tr->name);
    if(pkt.dts != AV_NOPTS_VALUE) {
        tr->properties.dts = r->timescaler (r,
                                            pkt.dts * av_q2d(stream->time_base));
        fnc_log(FNC_LOG_VERBOSE,
                "[avf] delivery timestamp %f",
                tr->properties.dts);
    } else {
        fnc_log(FNC_LOG_VERBOSE,
                "[avf] missing delivery timestamp");
    }

    if(pkt.pts != AV_NOPTS_VALUE) {
        tr->properties.pts = r->timescaler (r,
                                            pkt.pts * av_q2d(stream->time_base));
        fnc_log(FNC_LOG_VERBOSE,
                "[avf] presentation timestamp %f",
                tr->properties.pts);
    } else {
        fnc_log(FNC_LOG_VERBOSE, "[avf] missing presentation timestamp");
    }

    if (pkt.duration) {
        tr->properties.frame_duration = pkt.duration *
            av_q2d(stream->time_base);
    } else { // welcome to the wonderland ehm, hackland...
        switch (stream->codec->codec_id) {
        case CODEC_ID_MP2:
        case CODEC_ID_MP3:
            tr->properties.frame_duration = 1152.0/
                stream->codec->sample_rate;
            break;
        default: break;
        }
    }

    fnc_log(FNC_LOG_VERBOSE, "[avf] packet duration %f",
            tr->properties.frame_duration);

    bsfc = stream->codec->opaque;
    if (bsfc) {
        uint8_t *data = NULL;
        int size = 0;
        av_bitstream_filter_filter(bsfc, stream->codec, NULL,
                                   &data, &size,
                                   pkt.data, pkt.size,
                                   pkt.flags & AV_PKT_FLAG_KEY);
        ret = tr->parser->parse(tr, pkt.data, pkt.size);
    } else {
        ret = tr->parser->parse(tr, pkt.data, pkt.size);
    }

    av_free_packet(&pkt);

    return ret;
}

static int avf_seek(Resource * r, double time_sec)
{
    int flags = 0;
    int64_t time_msec = time_sec * AV_TIME_BASE;
    avf_private_data *priv = r->private_data;

    fnc_log(FNC_LOG_DEBUG, "Seeking to %f", time_sec);
    if (priv->avfc->start_time != AV_NOPTS_VALUE)
        time_msec += priv->avfc->start_time;
    if (time_msec < 0) flags = AVSEEK_FLAG_BACKWARD;
    return av_seek_frame(priv->avfc, -1, time_msec, flags);
}

static void avf_uninit(gpointer rgen)
{
    Resource *r = rgen;
    avf_private_data *priv = r->private_data;

    if ( priv == NULL || priv->avfc == NULL )
        return;

    init_mutex();

    g_mutex_lock(ffmpeg_lock);
    av_close_input_file(priv->avfc);
    g_mutex_unlock(ffmpeg_lock);
}

static const char avf_name[] = "libavformat Demuxer";

FENG_DEMUXER(avf, STORED_SOURCE);

