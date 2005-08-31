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

#include <string.h>

#include <fenice/demuxer.h>
#include <fenice/utils.h>
#include <fenice/fnc_log.h>

typedef struct __TRACK_LIST {
	Track *track;
	struct __TRACK_LIST *next;
} TrackList;

static TrackList *ex_tracks=NULL;

static TrackList *ex_track_search(Track *);
static void ex_track_remove(Track *);
static int ex_tracks_save(Track *[], uint32);
// static void ex_tracks_free(Track *[], uint32);

void free_track(Track *t)
{
	close_is(t->i_stream);
	if(t->track_info!=NULL)
		free(t->track_info);
	// t->track_info=NULL;
	if(t->parser!=NULL)
		free_parser(t->parser);
	if(t->buffer!=NULL)
		OMSbuff_free(t->buffer);
	if(t->private_data!=NULL) {
		free(t->private_data);
		// t->private_data=NULL;
	}
	// t->calculate_timestamp=NULL; /*TODO*/
	if ( t->i_stream && IS_ISEXCLUSIVE(t->i_stream) )
		ex_track_remove(t);

	free(t);
}

Resource *r_open(resource_name n)
{
	Resource *r;
	InputStream *i_stream;
	
	if((i_stream=create_inputstream(n))==NULL) {
		fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
		return NULL;
	}
	if((r=(Resource *)malloc(sizeof(Resource)))==NULL) {
		fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
		istream_close(i_stream);
		return NULL;
	}
	if((r->info=(ResourceInfo *)malloc(sizeof(ResourceInfo)))==NULL) {
		fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
		free(r);
		r=NULL;
		istream_close(i_stream);
		return NULL;
	}
	r->i_stream=i_stream;
	r->private_data=NULL;
	r->format=NULL; /*use register_format*/
	/*initializate tracks[MAX_TRACKS]??? TODO*/
	// temporary track initialization:
	r->num_tracks=0;
	// search for exclusive tracks: should be done track per track?
	ex_tracks_save(r->tracks, r->num_tracks);
	
	return r;
}

void r_close(Resource *r)
{
	uint32 i;

	if(r!=NULL) {
		close_is(r->i_stream);
		free(r->info);
		r->info=NULL;
		if(r->private_data!=NULL) {
			free(r->private_data);
		}
		for(i=0;i<r->num_tracks;i++) 
			if(r->tracks[i]!=NULL)	/*r_close_track ??? TODO*/
				free_track(r->tracks[i]);
		free(r);
	}
}

msg_error get_resource_info(resource_name n, ResourceInfo *r)
{
	//...
	return RESOURCE_OK;
}

Selector *r_open_tracks(Resource *r, char *track_name, Capabilities *capabilities)
{
	Selector *s;
	uint32 i=0,j;
	Track *tracks[MAX_SEL_TRACKS];

	/*Capabilities aren't used yet. TODO*/

	for(j=0;j<r->num_tracks;j++)
		if( !strcmp((r->tracks[j])->track_name, track_name) && (i<MAX_SEL_TRACKS) ) {
			tracks[i]=r->tracks[j];
			i++;
		}
	if(i==0)
		return NULL;
	
	if((s=(Selector*)malloc(sizeof(Selector)))==NULL) {
		fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
		return NULL;
	}

	for(j=0;j<i;j++)
		s->tracks[j]=tracks[j];
	s->total=i;
	s->default_index=0;/*TODO*/
	s->selected_index=0;/*TODO*/
	//...
	return s;
}

void r_close_tracks(Selector *s)
{
	/*see r_close, what i have to do???*/
	// free_track(s->tracks[s->selected_index]);
	/*shawill: probably we must free only selector data*/
	free(s);
}

inline msg_error r_seek(Resource *r, long int time_sec)
{
	return r->format->read_seek(r,time_sec);
}

/*
Resource *init_resource(resource_name name)
{
	Resource *r;
	//...
	return r;
}
*/

Track *add_track(Resource *r/*, char * filename*/)
{
	Track *t;
 	//InputStream *i_stream;
	TrackInfo *track_info;
	MediaParser *parser;
	OMSBuffer *buffer;

	if(r->num_tracks>=MAX_TRACKS)
		return NULL;
	if((t=(Track *)malloc(sizeof(Track)))==NULL)
		return NULL;
	
	/*
	 if(!strcmp(r->i_stream->name,filename)) {
		if((i_stream=create_inputstream(filename))==NULL) {
			fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
			free_track(t);
			return NULL;
		}
	}
	else
		i_stream = r->i_stream;
	*/
	if((track_info = (TrackInfo *)malloc(sizeof(TrackInfo)))==NULL) {
		fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
		free_track(t);
		return NULL;
	}

	if((parser=add_media_parser())==NULL) {
		fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
		free(track_info);
		free_track(t);
		return NULL;
	}
	if((buffer=OMSbuff_new(OMSBUFFER_DEFAULT_DIM))==NULL) {
		fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
		free_parser(parser);
		free(track_info);
		free_track(t);
		return NULL;
	}

	t->track_name[0]='\0';
	t->track_info=track_info;
	t->parser=parser;
	t->buffer=buffer;
	t->i_stream=NULL;
	/*t->i_stream=i_stream*/
	
	r->tracks[r->num_tracks]=t;
	r->num_tracks++;
	
	return t;
}

// static functions
static TrackList *ex_track_search(Track *track)
{
	TrackList *i;

	for (i=ex_tracks; i && (i->track != track); i=i->next);

	return i;
}

static void ex_track_remove(Track *track)
{
	TrackList *i, *prev;

	for (i=prev=ex_tracks; i && (i->track != track); prev=i, i=i->next);

	if (i) {
		if (i == ex_tracks) {
			ex_tracks = ex_tracks->next;
		} else {
			prev->next = i->next;
		}
		free(i);
	}
}

static int ex_tracks_save(Track *tracks[], uint32 num_tracks)
{
	uint32 i;
	TrackList *track_item;
	
	for (i=0; i<num_tracks; i++) {
		if ( tracks[i]->i_stream && IS_ISEXCLUSIVE(tracks[i]->i_stream) && !ex_track_search(tracks[i]) ) {
			if ( !(track_item = malloc(sizeof(TrackList))) ) {
				fnc_log(FNC_LOG_FATAL,"Could not alloc memory.\n");
				return ERR_ALLOC;
			}
			// insert at the beginning of the list
			track_item->next = ex_tracks;
			ex_tracks = track_item;
		}
	}

	return ERR_NOERROR;
}

#if 0
static void ex_tracks_free(Track *tracks[], uint32 num_tracks)
{
	uint32 i;
	TrackList *track_item;
	
	for (i=0; i<num_tracks; i++) {
		if ( tracks[i]->i_stream && IS_ISEXCLUSIVE(tracks[i]->i_stream) ) {
			// remove from list (if present)
			ex_track_remove(tracks[i]);
		}
	}
}
#endif

