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

// global demuxer modules:
extern Demuxer fnc_demuxer_sd;

// static array containing all the available demuxers:
static Demuxer *demuxers[] = {
	&fnc_demuxer_sd,
	NULL
};

/*! \brief a list for some tracks.
 * It is useful for <em>exclusive tracks</em> list.
 * The Media Thread keeps a list of the tracks that can be opened only once.
 * */
typedef struct __TRACK_LIST {
	Track *track;
	struct __TRACK_LIST *next;
} TrackList;

/*! \brief The exclusive tracks.
 * The Media Thread keeps a list of the tracks that can be opened only once.
 * */
static TrackList *ex_tracks=NULL;

/*! Private functions for exclusive tracks.
 * */
static TrackList *ex_track_search(Track *);
static void ex_track_remove(Track *);
static int ex_tracks_save(Track *[], uint32);
// static void ex_tracks_free(Track *[], uint32);

// private funcions for specific demuxer
static int find_demuxer(resource_name);

Resource *r_open(resource_name n)
{
	Resource *r;
	int dmx_idx;

	// shawill: MUST go away!!!
	fnc_log(FNC_LOG_DEBUG, "[MT] Resource requested: %s\n", n);
	if ( (dmx_idx=find_demuxer(n))<0 ) {
		fnc_log(FNC_LOG_DEBUG, "[MT] Could not find a valid demuxer for resource %s\n", n);
		return NULL;
	}

	fnc_log(FNC_LOG_DEBUG, "[MT] Demuxer found: \"%s\"\n", demuxers[dmx_idx]->info->name);

	if( !(r = calloc(1, sizeof(Resource))) ) {
		fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
		return NULL;
	}
	
	if( !(r->i_stream=istream_open(n)) ) {
		free(r);
		return NULL;
	}

	if( !(r->info=calloc(1, sizeof(ResourceInfo))) ) { // init all infos
		fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
		istream_close(r->i_stream);
		free(r);
		return NULL;
	}
	/* initialization non needed 'cause we use calloc
	r->private_data=NULL;
	r->demuxer=NULL;
	//initializate tracks[MAX_TRACKS]??? TODO
	// temporary track initialization:
	r->num_tracks=0;
	*/
	// TODO decomment when ready
	// demuxers[dmx_idx]->init(r);

	// search for exclusive tracks: should be done track per track?
	ex_tracks_save(r->tracks, r->num_tracks);
	
	return r;
}

void r_close(Resource *r)
{
	uint32 i;

	if(r!=NULL) {
		istream_close(r->i_stream);
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
	return r->demuxer->seek(r,time_sec);
}

/*
Resource *init_resource(resource_name name)
{
	Resource *r;
	//...
	return r;
}
*/

/*! Add track to resource tree.  This function adds a new track data struct to
 * resource tree. It used by specific demuxer function in order to obtain the
 * struct to fill.
 * \param r pointer to resource.
 * \return pointer to newly allocated track struct.
 * */
#define ADD_TRACK_ERROR(level, ...) \
	{ \
		fnc_log(level, __VA_ARGS__); \
		free_track(t); \
		return NULL; \
	}
Track *add_track(Resource *r)
{
	Track *t;

	if(r->num_tracks>=MAX_TRACKS)
		return NULL;
	if( !(t=(Track *)calloc(1, sizeof(Track))) ) 
		ADD_TRACK_ERROR(FNC_LOG_FATAL,"Memory allocation problems.\n");
	
	if( !(t->track_info = malloc(sizeof(TrackInfo))) )
		ADD_TRACK_ERROR(FNC_LOG_FATAL,"Memory allocation problems.\n");

	if( !(t->properties = malloc(sizeof(MediaProperties))) )
		ADD_TRACK_ERROR(FNC_LOG_FATAL,"Memory allocation problems.\n");

	if( !(t->parser=add_media_parser()) )
		ADD_TRACK_ERROR(FNC_LOG_FATAL,"Memory allocation problems.\n");

	if( !(t->buffer=OMSbuff_new(OMSBUFFER_DEFAULT_DIM)) )
		ADD_TRACK_ERROR(FNC_LOG_FATAL,"Memory allocation problems.\n");

	/* using calloc this initializzation is not needed.
	t->track_name[0]='\0';
	t->i_stream=NULL;
	*/
	/*t->i_stream=i_stream*/
	
	r->tracks[r->num_tracks]=t;
	r->num_tracks++;
	
	return t;
}
#undef ADD_TRACK_ERROR

void free_track(Track *t)
{
	if (!t)
		return;

	istream_close(t->i_stream);
	free(t->track_info);
	free_parser(t->parser);
	OMSbuff_free(t->buffer);
#if 0 // private data is not under Track jurisdiction!
	free(t->private_data);
#endif
	// t->calculate_timestamp=NULL; /*TODO*/
	if ( t->i_stream && IS_ISEXCLUSIVE(t->i_stream) )
		ex_track_remove(t);

	free(t);
}

// static functions
// Exclusive Tracks handling functions
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

// private funcions for specific demuxer
static int find_demuxer(resource_name n)
{
	// this int will contain the index of the demuxer already probed second
	// the extension suggestion, in order to not probe twice the same
	// demuxer that was proved to be not usable.
	int probed=-1;
	char found=0; // flag for demuxer found.
	int i; // index
	char exts[128], *res_ext, *tkn; // temp string containing extensione served by probing demuxer.

	// First of all try that with matching extension: we use extension as a
	// suggestion of resource type.
	// find resource name extension:
	if ( (res_ext=strrchr(n, '.')) && (res_ext++) ) {
		// extension present
		for (i=0; demuxers[i]; i++) {
			strncpy(exts, demuxers[i]->info->extensions, sizeof(exts));
			for (tkn=strtok(exts, ","); tkn; tkn=strtok(NULL, ",")) {
				if (!strcmp(tkn, res_ext)) {
					fnc_log(FNC_LOG_DEBUG, "[MT] probing demuxer: extension \"%s\" matches \"%s\" demuxer\n", res_ext, demuxers[i]->info->name);
					if (demuxers[i]->probe(n) == RESOURCE_OK) {
						fnc_log(FNC_LOG_DEBUG, "[MT] probing demuxer: demuxer found\n", res_ext, demuxers[i]->info->name);
						found = 1;
						break;
					}
				}
			}
			if (found)
				break;
		}
	}

	return found ? i: -1;
}

