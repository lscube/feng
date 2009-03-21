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
#ifndef FN_MEDIAPARSER_H
#define FN_MEDIAPARSER_H

#include <glib.h>
#include <stdint.h>

#include "mediautils.h"
#include <fenice/sdp_grammar.h>

struct Track;

// return errors
#define MP_PKT_TOO_SMALL -101
#define MP_NOT_FULL_FRAME -102

typedef enum {MP_undef=-1, MP_audio, MP_video, MP_application, MP_data, MP_control} MediaType;

typedef enum {MS_stored=0, MS_live} MediaSource;

MObject_def(MediaProperties_s)
    int bit_rate; /*!< average if VBR or -1 is not useful*/
    int payload_type;
    unsigned int clock_rate;
    char encoding_name[11];
    MediaType media_type;
    MediaSource media_source;
    int codec_id; /*!< Codec ID as defined by ffmpeg */
    int codec_sub_id; /*!< Subcodec ID as defined by ffmpeg */
    double mtime;    //FIXME Move to feng   //time is in seconds
    double frame_duration; //time is in seconds
    float sample_rate;/*!< SamplingFrequency*/
    float OutputSamplingFrequency;
    int audio_channels;
    int bit_per_sample;/*!< BitDepth*/
    float frame_rate;
    int FlagInterlaced;
    unsigned int PixelWidth;
    unsigned int PixelHeight;
    unsigned int DisplayWidth;
    unsigned int DisplayHeight;
    unsigned int DisplayUnit;
    unsigned int AspectRatio;
    uint8_t *ColorSpace;
    float GammaValue;
    uint8_t *extradata;
    long extradata_len;
    sdp_field_list sdp_private;
} MediaProperties;

typedef struct {
    const char *encoding_name; /*i.e. MPV, MPA ...*/
    const MediaType media_type;
} MediaParserInfo;

typedef struct {
    const MediaParserInfo *info;
/*! init: inizialize the module
 *
 *  @param properties: pointer of allocated struct to fill with properties
 *  @param private_data: private data of parser will be, if needed, linked to this pointer (double)
 *  @return: 0 on success, non-zero otherwise.
 * */
    int (*init)(MediaProperties *properties, void **private_data);

/*! parse: take a single elementary unit of the codec stream and prepare the rtp payload out of it.
 *
 *  @param track: track whose feng should be filled,
 *  @param data: packet from the demuxer layer,
 *  @param len: packet length,
 *  @param extradata: codec configuration data,
 *  @param extradata_len: extradata length.
 *  @return: 0 on success, non zero otherwise.
 * */
    int (*parse)(void *track, uint8_t *data, long len, uint8_t *extradata,
                 long extradata_len);
/*! uninit: free the media parser structures.
 *
 *  @param private_data: pointer to parser specific private data.
 *  @return: 0 on success, non zero otherwise
 * */
    int (*uninit)(void *private_data);
} MediaParser;

/**
 * MediaParser Interface, pending overhaul
 * @defgroup MediaParser MediaParser Interface
 * @{
 */
MediaParser *mparser_find(const char *);
void mparser_unreg(MediaParser *, void *);
/**
 * @}
 */

/**
 * @brief Buffer passed between parsers and RTP sessions
 *
 * This is what is being encapsulated by @ref BufferQueue_Element.
 */
typedef struct {
    double timestamp;   /*!< presentation time of packet */
    uint8_t marker;
    size_t data_size;
    uint8_t data[];
} MParserBuffer;

void mparser_buffer_write(struct Track *tr, uint8_t marker,
                          uint8_t *data, size_t data_size);


#define DEFAULT_MTU 1440

#endif // FN_MEDIAPARSER_H
