/*
 *  $Id: $
 *
 *  Copyright (C) 2006 by Luca Barbato
 *
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  libnms is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libnms; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* ripped from MPlayer demuxer */

#include <string.h>
#include <stdio.h>

#include <fenice/demuxer.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

#include <fenice/multicast.h>	/*is_valid_multicast_address */
#include <fenice/rtsp.h>	/*parse_url */
#include <fenice/rtpptdefs.h>	/*payload type definitions */

#include <fenice/demuxer_module.h>

#include <avformat.h>

static DemuxerInfo info = {
	"Avformat Demuxer",
	"avf",
	"OMSP Team",
	"",
	"mov, nut, mkv, mxf" // the others are a problem
};

FNC_LIB_DEMUXER(avf);
/*
Demuxer fnc_demuxer_avf =
{
        &info,
        probe,
        init,
        read_packet,
        seek,
        uninit
};
*/

typedef struct id_tag {
    const int id;
    const int pt;
    const char tag[11];
} id_tag;

//FIXME this should be simplified!
const id_tag id_tags[] = {
   { CODEC_ID_MPEG1VIDEO, 32, "MPV" },
   { CODEC_ID_MPEG2VIDEO, 32, "MPV" },
   { CODEC_ID_MP3, 14, "MPA"},
   { CODEC_ID_NONE, 0, "NONE"} //XXX ...
};

typedef struct lavf_priv{
    AVInputFormat *avif;
    AVFormatContext *avfc;
    ByteIOContext pb;
//    int audio_streams;
//    int video_streams;
    int64_t last_pts; //Use it or not?
} lavf_priv_t;

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

#if 0
//if we want to use inputstream we need to implement some more functions...
static int fnc_open(URLContext *h, const char *filename, int flags){
    return 0;
}

static int fnc_write(URLContext *h, unsigned char *buf, int size){
    return -1;
}

static int fnc_close(URLContext *h){
    return 0;
}

static int fnc_read(URLContext *h, unsigned char *buf, int size){
    InputStream *stream = (InputStream*)h->priv_data;
    int ret;

    ret = istream_read(size, buf, stream);

    fnc_log(FNC_LOG_DEBUG, "%d=fnc_read(%p, %p, %d)\n", ret, h, buf, size);

    return ret;
}

static offset_t fnc_seek(URLContext *h, offset_t pos, int whence){
    InputStream *stream = (InputStream*)h->priv_data;
    
    fnc_log(FNC_LOG_DEBUG, "fnc_seek(%p, %d, %d)\n", h, (int)pos, whence);

    if(whence == SEEK_CUR)
        pos += istream_tell(stream);
    else if(whence == SEEK_END)
        pos += stream->end_pos;
    else if(whence != SEEK_SET)
        return -1;

    if(pos<stream->end_pos && stream->eof)
        istream_reset(stream);
    if(istream_seek(stream, pos)==0)
        return -1;

    return pos;
}

static URLProtocol fnc_protocol = {
    "fnc",
    fnc_open,
    fnc_read,
    fnc_write,
    fnc_seek,
    fnc_close,
};
#endif

#define PROBE_BUF_SIZE 2048

static int probe(InputStream * i_stream)
{
    AVProbeData avpd;
    uint8_t buf[PROBE_BUF_SIZE];
    lavf_priv_t *priv;

    av_register_all();

    if (istream_read(i_stream, buf, PROBE_BUF_SIZE) != PROBE_BUF_SIZE)
	return RESOURCE_DAMAGED;

    avpd.filename= i_stream->name;
    avpd.buf= buf;
    avpd.buf_size= PROBE_BUF_SIZE;
    priv->avif = av_probe_input_format(&avpd, 1);
    if(!priv->avif){
        return RESOURCE_DAMAGED;
    }

    return RESOURCE_OK;
}

static int init(Resource * r)
{
    AVFormatContext *avfc;
    AVFormatParameters ap;
    lavf_priv_t *priv =  av_mallocz(sizeof(lavf_priv_t));
    MediaProperties props;
    Track *track;
    TrackInfo trackinfo;
    int i;

    memset(&ap, 0, sizeof(AVFormatParameters));
// make avf use our stuff or not?
//register_protocol(&fnc_protocol);

    avfc = av_alloc_format_context();
    ap.prealloced_context = 1;

    url_fopen(&priv->pb, r->info->mrl, URL_RDONLY);
// same as before...
//((URLContext*)(priv->pb.opaque))->priv_data = r->i_stream; 
    
//    if(av_open_input_stream(&avfc, &priv->pb, r->i_stream->name,
//                            priv->avif, &ap)<0) {
     if (av_open_input_file(&avfc, r->info->mrl, NULL, 0, &ap)) {
        fnc_log(FNC_LOG_DEBUG, "[MT] Cannot open %s\n", r->info->mrl);
        goto err_alloc;
    }

    r->private_data = priv;
    priv->avfc = avfc;

    if(av_find_stream_info(avfc) < 0){
        fnc_log(FNC_LOG_DEBUG, "[MT] Cannot find streams in file %s\n",
                r->i_stream->name);
        goto err_alloc;
    }

    MObject_init(MOBJECT(&trackinfo));
    MObject_0(MOBJECT(&trackinfo), TrackInfo);

/* throw it in the sdp?
    if(avfc->title    [0])
    if(avfc->author   [0])
    if(avfc->copyright[0])
    if(avfc->comment  [0])
    if(avfc->album    [0])
    if(avfc->year        )
    if(avfc->track       )
    if(avfc->genre    [0]) */

    // make them pointers?
    strncpy(trackinfo.title, avfc->title, 80);
    strncpy(trackinfo.author, avfc->author, 80);

    for(i=0; i<avfc->nb_streams; i++){
        AVStream *st= avfc->streams[i];
        AVCodecContext *codec= st->codec;
        trackinfo.id = i;
        const char *id = tag_from_id(codec->codec_id);

// XXX: Check!
        MObject_init(MOBJECT(&props));
        MObject_0(MOBJECT(&props), MediaProperties);

        props.extradata = codec->extradata;
        props.extradata_len = codec->extradata_size;
        // make them pointers?
        if (id) 
        { 
            strncpy(props.encoding_name, id, 11);
            props.payload_type = pt_from_id(codec->codec_id);
            fnc_log(FNC_LOG_DEBUG, "[MT] Parsing AVStream %s\n",
                    props.encoding_name);
        } else {
            fnc_log(FNC_LOG_DEBUG, "[MT] Cannot map stream id %d\n",
                    codec->codec_id);
            goto err_alloc;
        }
        switch(codec->codec_type){
            case CODEC_TYPE_AUDIO:{//alloc track?
                props.media_type       = MP_audio;
                // Some properties, add more?
                props.bit_rate         = codec->bit_rate;
                props.audio_channels   = codec->channels;
                // Make props an int...
                props.sample_rate      = codec->sample_rate;
                props.bit_per_sample   = codec->bits_per_sample;
                snprintf(trackinfo.name, sizeof(trackinfo.name), "%d", i);
                if (!(track = add_track(r, &trackinfo, &props)))
                    goto err_alloc;
            break;}
            case CODEC_TYPE_VIDEO:{//alloc track?
                props.media_type   = MP_video;
                props.frame_rate   = av_q2d(st->r_frame_rate); //XXX check
                props.AspectRatio  = codec->width * 
                                      codec->sample_aspect_ratio.num /
                                      (float)(codec->height *
                                              codec->sample_aspect_ratio.den);
// addtrack must init the parser, the parser may need the extradata
                snprintf(trackinfo.name,sizeof(trackinfo.name),"%d",i);
                if (!(track = add_track(r, &trackinfo, &props)))
		    goto err_alloc;
                break;}
        }
    }

//    return ERR_ALLOC;
    return RESOURCE_OK;

err_alloc:
    av_freep(&priv);
    return ERR_ALLOC;
}

static int read_packet(Resource * r)
{
    int ret = -1;
#if 0
    Selector *sel;
#else
    TrackList tr;
#endif
    AVPacket pkt;
    AVStream *stream;
    lavf_priv_t *priv = r->private_data;

// get a packet
    if(av_read_frame(priv->avfc, &pkt) < 0)
        return EOF; //FIXME

// for each track selected
#if 0
    for (sel = g_list_first(r->sel);
         sel !=NULL;
         sel = g_list_next(r->sel)) {
        if (pkt.stream_index == sel->cur->info->id) {
// push it to the framer
            stream = priv->avfc->streams[sel->cur->info->id];
            ret = sel->cur->parser->parse(sel->cur, pkt.data, pkt.size,
                                    stream->codec->extradata,
                                    stream->codec->extradata_size);
            break;
        }
    }
#else
    for (tr = g_list_first(r->tracks);
         tr !=NULL;
         tr = g_list_next(tr)) {
        if (pkt.stream_index == TRACK(tr)->info->id) {
// push it to the framer
            stream = priv->avfc->streams[TRACK(tr)->info->id];
            if(pkt.pts != AV_NOPTS_VALUE) 
                TRACK(tr)->properties->mtime = 
                    pkt.pts * av_q2d(stream->time_base);

            ret = TRACK(tr)->parser->parse(TRACK(tr), pkt.data, pkt.size,
                                    stream->codec->extradata,
                                    stream->codec->extradata_size);
            break;
        }
    }
#endif

    av_free_packet(&pkt);

    return ret;
}

static int seek(Resource * r, int64_t time_msec)
{
//XXX check the timebase....
    lavf_priv_t *priv = r->private_data;
    return av_seek_frame(priv->avfc, -1, time_msec, 0);

//	return RESOURCE_NOT_SEEKABLE;
}

static int uninit(Resource * r)
{
    lavf_priv_t* priv = r->private_data;

// avf stuff
    if (priv) {
        if(priv->avfc) {
            av_close_input_file(priv->avfc); priv->avfc = NULL;
        }
        free(priv);
    }

// generic unint
    r_close(r);

    return RESOURCE_OK;
}

