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
#include <unistd.h>
#include <errno.h>

#include "feng.h"
#include "fnc_log.h"

#include "media/media.h"

#include <libavformat/avformat.h>

static int avf_seek(Resource * r, double time_sec);
static void avf_uninit(gpointer rgen);
static int avf_read_packet(Resource * r);

static int fc_lock_manager(void **mutex, enum AVLockOp op)
{
    switch (op) {
        case AV_LOCK_CREATE:  ///< Create a mutex
            *mutex = g_mutex_new();
        break;
        case AV_LOCK_OBTAIN:  ///< Lock the mutex
            g_mutex_lock(*mutex);
        break;
        case AV_LOCK_RELEASE: ///< Unlock the mutex
            g_mutex_unlock(*mutex);
        break;
        case AV_LOCK_DESTROY: ///< Free mutex resources
            g_mutex_free(*mutex);
        break;
        default:
            break;
    }
    return 0;
}

void ffmpeg_init(void)
{
    av_register_all();
    av_lockmgr_register(fc_lock_manager);
}


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
 * @brief Mutex management for ffmpeg
 *
 * The avcodec_open() and avcodec_close() functions are not thread
 * safe, and needs lock-protection;
 */

Resource *avf_open(const char *url)
{
    AVFormatParameters ap;
    Resource *r = NULL;
    Track *track = NULL;
    int pt = 96, i;
    unsigned int j;
    gchar *mrl;

    struct stat filestat;
    avf_private_data priv = { NULL, NULL };

    fnc_log(FNC_LOG_DEBUG,
            "opening resource '%s' through libavformat",
            url);

    mrl = g_strjoin ("/",
                     feng_default_vhost->document_root,
                     url,
                     NULL);

    if ( access(mrl, R_OK) != 0 ) {
        fnc_perror("access");
        goto err_alloc;
    }

    if (stat(mrl, &filestat) < 0 ) {
        fnc_perror("stat");
        goto err_alloc;
    }

    if ( ! S_ISREG(filestat.st_mode) ) {
        fnc_log(FNC_LOG_ERR, "%s: not a file", mrl);
        goto err_alloc;
    }

    memset(&ap, 0, sizeof(AVFormatParameters));

    priv.avfc = avformat_alloc_context();
    ap.prealloced_context = 1;

    priv.avfc->flags |= AVFMT_FLAG_GENPTS;

    i = av_open_input_file(&priv.avfc, mrl, NULL, 0, &ap);

    if ( i != 0 ) {
        fnc_log(FNC_LOG_DEBUG, "[avf] Cannot open %s", mrl);
        goto err_alloc;
    }

    i = av_find_stream_info(priv.avfc);

    if(i < 0){
        fnc_log(FNC_LOG_DEBUG, "[avf] Cannot find streams in file %s",
                mrl);
        goto err_alloc;
    }

    priv.tracks = g_new0(Track*, priv.avfc->nb_streams);

    for(j=0; j<priv.avfc->nb_streams; j++) {
        AVStream *st= priv.avfc->streams[j];
        AVCodecContext *codec= st->codec;
        AVMetadataTag *metadata_tag = NULL;
        static const int metadata_flags = AV_METADATA_DONT_STRDUP_KEY|AV_METADATA_DONT_STRDUP_VAL;
        const char *encoding_name;
        float frame_rate = 0;

        int (*parser_init)(Track *track) = NULL;

        track = track_new(g_strdup_printf("Track_%d", j));

        track->clock_rate = 90000; //Default

        switch(codec->codec_id) {
        case CODEC_ID_MPEG1VIDEO:
        case CODEC_ID_MPEG2VIDEO:
            track->payload_type = 32;
            encoding_name = "MPV";
            track->parse = mpv_parse;
            break;

        case CODEC_ID_H264:
            if (!codec->extradata_size)
                goto err_alloc;

            encoding_name = "H264";
            parser_init = h264_init;

            track->parse = h264_parse;
            break;

        case CODEC_ID_MP2:
        case CODEC_ID_MP3:
            track->payload_type = 14;
            encoding_name = "MPA";
            track->parse = mpa_parse;
            break;

        case CODEC_ID_VORBIS:
            if (!codec->extradata_size)
                goto err_alloc;

            encoding_name = "vorbis";
            parser_init = vorbis_init;

            track->parse = xiph_parse;
            break;

        case CODEC_ID_THEORA:
            if (!codec->extradata_size)
                goto err_alloc;

            encoding_name = "theora";
            parser_init = theora_init;

            track->parse = xiph_parse;
            break;

        case CODEC_ID_SPEEX:
            encoding_name = "SPEEX";
            track->parse = speex_parse;
            break;

        case CODEC_ID_AAC:
            if ( codec->extradata_size == 0 ) {
                AVPacket pkt;

                if ( (st->codec->opaque = av_bitstream_filter_init("aac_adtstoasc")) == NULL )
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

                if (!codec->extradata_size)
                    goto err_alloc;

                av_seek_frame(priv.avfc, -1, 0, 0);
            }

            encoding_name = "mpeg4-generic";
            parser_init = aac_init;

            track->parse = aac_parse;
            break;

        case CODEC_ID_MPEG4:
            if (!codec->extradata_size)
                goto err_alloc;

            encoding_name = "MP4V-ES";
            parser_init = mp4ves_init;

            track->parse = mp4ves_parse;
            break;

        case CODEC_ID_H263:
            encoding_name = "H263-1998";
            parser_init = h263_init;

            track->parse = h263_parse;
            break;

        case CODEC_ID_AMR_NB:
            encoding_name = "AMR";
            parser_init = amr_init;

            track->clock_rate = 8000;
            track->parse = amr_parse;
            break;

        case CODEC_ID_VP8:
            encoding_name = "VP8";
            parser_init = vp8_init;

            track->parse = vp8_parse;
            break;

        default:
            goto discard;
        }

        track->encoding_name = g_strdup(encoding_name);
        if ( track->payload_type == 0 )
            track->payload_type = pt++;

        switch(codec->codec_type){
        case AVMEDIA_TYPE_AUDIO:
            track->media_type     = MP_audio;
            track->audio_channels = codec->channels;
            track->frame_duration = (double)1 / codec->sample_rate;
            break;

        case AVMEDIA_TYPE_VIDEO:
            frame_rate = av_q2d(st->r_frame_rate);
            track->media_type     = MP_video;
            track->frame_duration = (double)1 / frame_rate;
            break;

        default:
            goto discard;
        }

        priv.tracks[j] = track;

        track->extradata = codec->extradata;
        track->extradata_len = codec->extradata_size;

        fnc_log(FNC_LOG_DEBUG, "[avf] Parsing AVStream %s",
                track->encoding_name);

        if ( parser_init && parser_init(track) != 0 )
            goto track_err_alloc;

        if ( (metadata_tag = av_metadata_get(priv.avfc->metadata, "title", NULL, metadata_flags)) )
            g_string_append_printf(track->sdp_description, SDP_F_TITLE, metadata_tag->value);

        /* here we check a few possible alternative tags to use */
        if ( (metadata_tag = av_metadata_get(priv.avfc->metadata, "artist", NULL, metadata_flags)) ||
             (metadata_tag = av_metadata_get(priv.avfc->metadata, "publisher", NULL, metadata_flags)) )
            g_string_append_printf(track->sdp_description, SDP_F_AUTHOR, metadata_tag->value);

        if ( frame_rate != 0 )
            g_string_append_printf(track->sdp_description,
                                   "a=framerate:%f\r\n",
                                   frame_rate);
        continue;

    track_err_alloc:
        track_free(track);
        goto err_alloc;

    discard:
        st->discard = AVDISCARD_ALL;
        track_free(track);
        continue;
    }

    /* Now that we know the resource is valid and we can read it,
       allocate the structure that will be returned. */

    r = g_slice_new0(Resource);

    r->mrl = mrl;
    r->lock = g_mutex_new();
    r->mtime = filestat.st_mtime;

    r->private_data = g_memdup(&priv, sizeof(priv));

    r->read_packet = avf_read_packet;
    r->uninit = avf_uninit;

    /* Try seeking to make sure that we can seek, as libavformat might
       not implement seeking for the format we're using here; if it
       doesn't, do not set a seek method */
    if ( !av_seek_frame(priv.avfc, -1, 0, 0) )
        r->seek = avf_seek;

    r->duration = (double)priv.avfc->duration /AV_TIME_BASE;
    fnc_log(FNC_LOG_DEBUG, "[avf] duration %f", r->duration);

    for(j = 0; j < priv.avfc->nb_streams; j++)
        if ( priv.tracks[j] ) {
            priv.tracks[j]->parent = r;
            r->tracks = g_list_append(r->tracks, priv.tracks[j]);
        }

    return r;

 err_alloc:
    for(j = 0; j < priv.avfc->nb_streams; j++)
        track_free(priv.tracks[j]);

    g_free(priv.tracks);

    av_close_input_file(priv.avfc);

    g_free(mrl);

    return false;
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
retry:
    if(av_read_frame(priv->avfc, &pkt) < 0)
        return RESOURCE_EOF; //FIXME

    if ( (tr = priv->tracks[pkt.stream_index]) == NULL )
        goto retry;

    // push it to the framer
    stream = priv->avfc->streams[pkt.stream_index];

    fnc_log(FNC_LOG_VERBOSE, "[avf] Parsing track %s",
            tr->name);
    if(pkt.dts != AV_NOPTS_VALUE) {
        tr->dts = pkt.dts * av_q2d(stream->time_base);
        fnc_log(FNC_LOG_VERBOSE,
                "[avf] delivery timestamp %f",
                tr->dts);
    } else {
        fnc_log(FNC_LOG_VERBOSE,
                "[avf] missing delivery timestamp");
    }

    if(pkt.pts != AV_NOPTS_VALUE) {
        tr->pts = pkt.pts * av_q2d(stream->time_base);
        fnc_log(FNC_LOG_VERBOSE,
                "[avf] presentation timestamp %f",
                tr->pts);
    } else {
        fnc_log(FNC_LOG_VERBOSE, "[avf] missing presentation timestamp");
    }

    if (pkt.duration) {
        tr->frame_duration = pkt.duration *
            av_q2d(stream->time_base);
    } else { // welcome to the wonderland ehm, hackland...
        switch (stream->codec->codec_id) {
        case CODEC_ID_MP2:
        case CODEC_ID_MP3:
            tr->frame_duration = 1152.0/
                stream->codec->sample_rate;
            break;
        default: break;
        }
    }

    fnc_log(FNC_LOG_VERBOSE, "[avf] packet duration %f",
            tr->frame_duration);

    bsfc = stream->codec->opaque;
    if (bsfc) {
        uint8_t *data = NULL;
        int size = 0;
        av_bitstream_filter_filter(bsfc, stream->codec, NULL,
                                   &data, &size,
                                   pkt.data, pkt.size,
                                   pkt.flags & AV_PKT_FLAG_KEY);
        ret = tr->parse(tr, pkt.data, pkt.size);
    } else {
        ret = tr->parse(tr, pkt.data, pkt.size);
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

    av_close_input_file(priv->avfc);

    g_free(priv->tracks);

    g_free(priv);
}
