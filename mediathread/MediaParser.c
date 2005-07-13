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
 *	- Federico Ridolfo	<federico.ridolfo@polito.it>
 *	- Marco Penno		<marco.penno@polito.it>
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

#include <string.h>

#include <fenice/MediaParser.h>
#include <fenice/utils.h>

static int register_mediatype_functions(MediaParserType *pt, char *encoding_name)
{

#if 0
	if (!strcmp(encoding_name,"H26L")) {
		//pt->load = load_H26L;
		pt->read = read_H26L;
		pt->close= free_H26L;
	} 
	else if(!strcmp(encoding_name,"MPV")) {
		//pt->load = load_MPV; 
		pt->read = read_MPEG_video;
		pt->close= free_MPV; 
	} 
	else if(!strcmp(encoding_name,"MP2T")) {
		//pt->load = load_MP2T; 
		pt->read = read_MPEG_ts;
		pt->close= free_MP2T; 		
	}
	else if(!strcmp(encoding_name,"MP4V-ES")) {
		//pt->load = load_MP4ES; 
		pt->read = read_MPEG4ES_video; 
		pt->close= free_MP4ES; 		
	}	
	else if(!strcmp(encoding_name,"MPA")) {
		//pt->load = load_MPA; 
		pt->read = read_MP3; 
		pt->close= free_MPA;	
	}
	else if(!strcmp(encoding_name,"L16")) {
		//pt->load = load_L16; 
		pt->read = read_PCM; 
		pt->close= free_L16;	
	}
	else if(!strcmp(encoding_name,"GSM")) {
		//pt->load = load_GSM; 
		pt->read = read_GSM;
		pt->close= free_GSM;	
	}
	else
		return ERR_GENERIC; /*unknown*/
#endif
	return ERR_NOERROR;
}

int register_media_type(MediaParserType * parser_type, MediaParser * p)
{
	p->parser_type=parser_type; 
	return ERR_NOERROR;
}

void free_parser(MediaParser *p)
{
	if(p->parser_type!=NULL) {
		free(p->parser_type->properties);
		p->parser_type->properties=NULL;
		free(p->parser_type);
		p->parser_type=NULL;
	}
	if(p!=NULL) {
		p->pts=0;
		free(p);	
		p=NULL;
	}
}

MediaParser * add_media_parser(void) 
{
	MediaParserType * parser_type;
	MediaParser *p;
	if((parser_type=(MediaParserType *)malloc(sizeof(MediaParserType)))==NULL)
		return NULL;
	
	if((p=(MediaParser *)malloc(sizeof(MediaParser)))==NULL) {
		free(parser_type);
		return NULL;
	}
	p->parser_type=parser_type;
	p->pts=0;

	return p;
}

int set_media_entity(MediaParserType *pt, char *encoding_name)
{

	if ((strcmp(encoding_name,"H26L")!=0) || (strcmp(encoding_name,"MPV")!=0) || \
	    (strcmp(encoding_name,"MP2T")!=0) || (strcmp(encoding_name,"MP4V-ES")!=0) ) {
		strcpy(pt->media_entity,"video");
	}
		
	else if ((strcmp(encoding_name,"MPA")!=0) || (strcmp(encoding_name,"L16")!=0) || (strcmp(encoding_name,"GSM")!=0)) {
		strcpy(pt->media_entity,"audio");
	}
	else
		return ERR_GENERIC; /*unknown*/
	/*TODO: text*/

	return	register_mediatype_functions(pt,encoding_name);
}
