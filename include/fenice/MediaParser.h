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
#include <fenice/bifferpool.h>

typedef struct __MEDIAPARSER {
	/*bufferpool*/
	OMSBuffer * buffer;
} MediaParser;

typedef struct __MEDIAPARSERTYPE {
	const char *encoding_name; /*i.e. MPV, MPA ...*/
	const char *media_entity: /*i.e. audio, video, text*/
	int (*load)();
	int (*read)();
	int (*close)(); /*before called free */
	void *properties; /*to cast to audio, video or text specific properties*/
} MediaParserType;

int register_media(MediaParserType *);

typedef struct __COMMON_PROPERTIES {
	uint32 bit_rate; /*average if VBR*/
} common_prop;

typedef struct __AUDIO_SPEC_PROPERTIES {
	common_prop *cprop;
} audio_spec_prop;

typedef struct __VIDEO_SPEC_PROPERTIES {
	common_prop *cprop;
	uint32 frame_rate;
} video_spec_prop;

typedef struct __TEXT_SPEC_PROPERTIES {
	common_prop *cprop;
} text_spec_prop;

#endif

