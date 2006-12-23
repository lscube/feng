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

#if 0 // define MObject with MObject_def
typedef struct {
	MOBJECT_COMMONS; // MObject commons MUST be the first field
#endif
MObject_def(__MEDIA_PROPERTIES)
	int32 bit_rate; /*average if VBR or -1 is not usefull*/
	MediaCodingType coding_type;
	uint32 payload_type;
	uint32 clock_rate;
	char encoding_name[11];
	MediaType media_type;
	// Audio specific properties:
	float sample_rate;/*SamplingFrequency*/
	float OutputSamplingFrequency;
	short audio_channels;
	uint32 bit_per_sample;/*BitDepth*/
	// Video specific properties:
	uint32 frame_rate;
	// more specific video information
	uint32 FlagInterlaced;
	//short StereoMode;
	uint32 PixelWidth;
	uint32 PixelHeight;
	uint32 DisplayWidth;
	uint32 DisplayHeight;
	uint32 DisplayUnit;
	uint32 AspectRatio;
	uint8 *ColorSpace;
	float GammaValue;
	sdp_field_list sdp_private;
} MediaProperties;

typedef struct {
	const char *encoding_name; /*i.e. MPV, MPA ...*/
	const MediaType media_type;
} MediaParserInfo;

typedef struct __MEDIAPARSER {
	MediaParserInfo *info;
	int (*init)(MediaProperties *,void **); // shawill: TODO: specify right parameters
	int (*get_frame)(uint8 *, uint32, double *, InputStream *, MediaProperties *, void *);
	int (*packetize)(uint8 *, uint32 *, uint8 *, uint32, MediaProperties *, void *);
	int (*uninit)(void *); /* parser specific init function */
} MediaParser;

/*MediaParser Interface*/
void free_parser(MediaParser *);
MediaParser *add_media_parser(void); 
MediaParser *mparser_find(const char *);
void mparser_unreg(MediaParser *, void *);
// int set_media_entity(MediaParserType *, char *encoding_name);
#endif

