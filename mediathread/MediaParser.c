/* * 
 *  $Id$
 *  
 *  This file is part of Feng
 *
 *  Feng -- Standard Streaming Server
 *
 *  Copyright (C) 2007 by
 *  	
 *	- Giampaolo Mancini	<giampaolo.mancini@polito.it>
 *	- Francesco Varano	<francesco.varano@polito.it>
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
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

#include <string.h>

#include <fenice/mediaparser.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

// global media parsers modules:
extern MediaParser fnc_mediaparser_mpv;
extern MediaParser fnc_mediaparser_mpa;
extern MediaParser fnc_mediaparser_h264;
extern MediaParser fnc_mediaparser_vorbis;
extern MediaParser fnc_mediaparser_theora;

// static array containing all the available media parsers:
static MediaParser *media_parsers[] = {
	&fnc_mediaparser_mpv,
	&fnc_mediaparser_mpa,
	&fnc_mediaparser_h264,
	&fnc_mediaparser_vorbis,
	&fnc_mediaparser_theora,
	NULL
};

MediaParser *mparser_find(const char *encoding_name)
{
	int i;

	for(i=0; media_parsers[i]; i++) {
		if ( !g_ascii_strcasecmp(encoding_name, media_parsers[i]->info->encoding_name) ) {
			fnc_log(FNC_LOG_DEBUG, "[MT] Found Media Parser for %s\n", encoding_name);
			return media_parsers[i];
		}
	}
        fnc_log(FNC_LOG_DEBUG, "[MT] Media Parser for %s not found\n", encoding_name);
	return NULL;
}

void mparser_unreg(MediaParser *p, void *private_data)
{
	if (p)
		p->uninit(private_data);
}

// shawill: probably these functions will be removed sooner ir later.
MediaParser *add_media_parser(void) 
{
	MediaParser *p;

	if(!(p=malloc(sizeof(MediaParser)))) {
		return NULL;
	}

	p->info = NULL;

	return p;
}

void free_parser(MediaParser *p)
{
	if(p) {
		// p->uninit(p->private_data);
		p->uninit(NULL);
		// free(p);	
	}
}
// ---------------

