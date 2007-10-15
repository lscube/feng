/* * 
 *  This file is part of Feng
 * 
 * Copyright (C) 2007 by LScube team <team@streaming.polito.it> 
 * See AUTHORS for more details 
 *  
 * Feng is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * Feng is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License
 * along with Feng; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *  
 * */
#ifndef FN_MEDIAPARSERH
#define FN_MEDIAPARSERH

#include <glib.h>

#include <bufferpool/bufferpool.h>
#include <fenice/mediautils.h>
#include <fenice/InputStream.h>
#include <fenice/sdp_grammar.h>

// return errors
#define MP_PKT_TOO_SMALL -101
#define MP_NOT_FULL_FRAME -102

typedef enum {mc_undefined=-1, mc_frame=0, mc_sample=1} MediaCodingType;

typedef enum {MP_undef=-1, MP_audio, MP_video, MP_application, MP_data, MP_control} MediaType;

typedef enum {MS_stored=0, MS_live} MediaSource;

MObject_def(MediaProperties_s)
    int bit_rate; /*!< average if VBR or -1 is not useful*/
    MediaCodingType coding_type;
    int payload_type;
    unsigned int clock_rate;
    char encoding_name[11];
    MediaType media_type;
    MediaSource media_source;
    int codec_id; /*!< Codec ID as defined by ffmpeg */
    int codec_sub_id; /*!< Subcodec ID as defined by ffmpeg */
    double mtime;    //FIXME Move to bufferpool   //time is in seconds
    double duration; //FIXME Move to bufferpool
    float sample_rate;/*!< SamplingFrequency*/
    float OutputSamplingFrequency;
    int audio_channels;
    int bit_per_sample;/*!< BitDepth*/
    int frame_rate;
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
    MediaParserInfo *info;
/*! init: inizialize the module
 *    
 *  @param properties: pointer of allocated struct to fill with properties
 *  @param private_data: private data of parser will be, if needed, linked to this pointer (double)
 *  @return: 0 on success, non-zero otherwise.
 * */
    int (*init)(MediaProperties *properties, void **private_data);
/*! get_frame: parse one frame from media bitstream.
 *
 *  @param dst: destination memory slot,
 *  @param dst_nbytes: number of bytes of *dest memory area,
 *  @param timestamp; return value for timestap of read frame
 *  @param void *properties: private data specific for each media parser.
 *  @param istream: InputStream of source Elementary Stream,
 *  @return: 0 on success, non zero otherwise.
 * */
    int (*get_frame)(uint8_t *dst, uint32_t dst_nbytes, double *timestamp,
                     InputStream *istream, MediaProperties *properties,
                     void *private_data);
/*! packetize: DEPRECATED*/
    int (*packetize)(uint8_t *dst, uint32_t *dst_nbytes, uint8_t *src, uint32_t src_nbytes, MediaProperties *properties, void *private_data);
/*! parse: take a single elementary unit of the codec stream and prepare the rtp payload out of it.
 *
 *  @param track: track whose bufferpool should be filled,
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
 * @defgroup MediaParser
 * @{
 */
MediaParser *add_media_parser(void); 
MediaParser *mparser_find(const char *);
void mparser_unreg(MediaParser *, void *);
void free_parser(MediaParser *);
/**
 * @}
 */

#define DEFAULT_MTU 1440

#endif

