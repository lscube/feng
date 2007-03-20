/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *      
 *    - Giampaolo Mancini    <giampaolo.mancini@polito.it>
 *    - Francesco Varano    <francesco.varano@polito.it>
 *    - Marco Penno        <marco.penno@polito.it>
 *    - Federico Ridolfo    <federico.ridolfo@polito.it>
 * 
 *  Fenice is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Fenice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Fenice; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * */
#ifndef __MEDIAPARSERH
#define __MEDIAPARSERH

#include <glib.h>

#include <fenice/types.h>
#include <fenice/bufferpool.h>
#include <fenice/mediautils.h>
#include <fenice/InputStream.h>
#include <fenice/sdp_grammar.h>

// return errors
#define MP_PKT_TOO_SMALL -101
#define MP_NOT_FULL_FRAME -102

typedef enum {mc_undefined=-1, mc_frame=0, mc_sample=1} MediaCodingType;

typedef enum {MP_undef=-1, MP_audio, MP_video, MP_application, MP_data, MP_control} MediaType;

MObject_def(__MEDIA_PROPERTIES)
    int bit_rate; /*! average if VBR or -1 is not useful*/
    MediaCodingType coding_type;
    int payload_type;
    unsigned int clock_rate;
    char encoding_name[11];
    MediaType media_type;
    double mtime;    //FIXME Move to bufferpool   //time is in seconds
    double duration; //FIXME Move to bufferpool
    //! Audio specific properties:
    //! \defgroup Audio Properties
    //\@{
    float sample_rate;/*! SamplingFrequency*/
    float OutputSamplingFrequency;
    int audio_channels;
    int bit_per_sample;/*! BitDepth*/
    //\@}
    //! Video specific properties:
    //! \defgroup Video Properties
    //\@{
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
    //\@}
    uint8_t *extradata;
    long extradata_len;
    sdp_field_list sdp_private;
} MediaProperties;

typedef struct {
    const char *encoding_name; /*i.e. MPV, MPA ...*/
    const MediaType media_type;
} MediaParserInfo;

typedef struct __MEDIAPARSER {
    MediaParserInfo *info;
    int (*init)(MediaProperties *,void **); // shawill: TODO: specify right parameters
    int (*get_frame)(uint8 *, uint32, double *, InputStream *,
                     MediaProperties *, void *);
    int (*packetize)(uint8 *, uint32 *, uint8 *, uint32, MediaProperties *, void *);
    int (*parse)(void *track, uint8 *data, long len, uint8 *extradata,
                 long extradata_len);
    int (*uninit)(void *);
} MediaParser;

/*MediaParser Interface*/
void free_parser(MediaParser *);
MediaParser *add_media_parser(void); 
MediaParser *mparser_find(const char *);
void mparser_unreg(MediaParser *, void *);

#define DEFAULT_MTU 1440

#endif

