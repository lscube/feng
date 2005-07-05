/* * 
 *  $Id$
 *  
 *  This file is part of Fenice
 *
 *  Fenice -- Open Media Server
 *
 *  Copyright (C) 2004 by
 *  	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
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

#if !defined(__MEDIAPARSERH)
#define __MEDIAPARSERH

#include <fenice/types.h>
#include <fenice/bufferpool.h>

typedef struct __MEDIAPARSERTYPE {
	const char *encoding_name; /*i.e. MPV, MPA ...*/
	const char *media_entity; /*i.e. audio, video, text*/
	int (*load)();
	int (*read)();
	int (*close)(); /*before called free */
	long int (* calculate_timestamp)();
	void *properties; /*to cast to audio, video or text specific properties*/
} MediaParserType;

int register_media_type(MediaParserType * /*parser_type*/, MediaParser * /*p*/);/*{p->parser_type=parser_type; return ERR_NOERROR;}*/

typedef struct __MEDIAPARSER {
	MediaParserType *parser_type;
} MediaParser;

typedef enum {mc_undefined=-1, mc_frame=0, mc_sample=1} MediaCoding;

#define __PROPERTIES_COMMON_FIELDS 	int32 bit_rate; /*average if VBR or -1 is not usefull*/ \
					MediaCoding coding_type; \
					uint32 payload_type; \
					uint32 clock_rate; \
					uint8 encoding_name[11];

typedef struct __COMMON_PROPERTIES {
	__PROPERTIES_COMMON_FIELDS
} common_prop;

typedef struct __AUDIO_SPEC_PROPERTIES {
	__PROPERTIES_COMMON_FIELDS
	float sample_rate;/*SamplingFrequency*/
	float OutputSamplingFrequency;
	short audio_channels;
	uint32 bit_per_sample;/*BitDepth*/
} audio_spec_prop;

typedef struct __VIDEO_SPEC_PROPERTIES {
	__PROPERTIES_COMMON_FIELDS
	uint32 frame_rate;
	/*Matroska ...*/
	uint32 FlagInterlaced;
	short StereoMode;
	uint32 PixelWidth;
	uint32 PixelHeight;
	uint32 DisplayWidth;
	uint32 DisplayHeight;
	uint32 DisplayUnit;
	uint32 AspectRatio;
	uint8 *ColorSpace;
	float GammaValue;
} video_spec_prop;

typedef struct __TEXT_SPEC_PROPERTIES {
	__PROPERTIES_COMMON_FIELDS
} text_spec_prop;

/*MediaParser Interface*/
void free_parser(MediaParser *);/*TODO*/

#endif

