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

/*! \brief The exclusive tracks.
 * The Media Thread keeps a list of the tracks that can be opened only once.
 * */
static GList *ex_tracks=NULL;

/*! Private functions for exclusive tracks.
 * */
static inline void ex_tracks_save(GList *);
static void ex_track_save(Track *, gpointer *);
static inline void ex_track_remove(Track *);
// static void ex_tracks_free(Track *[], uint32);

// private funcions for specific demuxer
static int find_demuxer(InputStream *);

Resource *r_open(resource_name n)
{
	Resource *r;
	int dmx_idx;
	InputStream *i_stream;

	if( !(i_stream=istream_open(n)) )
		return NULL;

	// shawill: MUST go away!!!
	if ( (dmx_idx=find_demuxer(i_stream))<0 ) {
		fnc_log(FNC_LOG_DEBUG, "[MT] Could not find a valid demuxer for resource %s\n", n);
		return NULL;
	}

	fnc_log(FNC_LOG_DEBUG, "[MT] registrered demuxer \"%s\" for resource \"%s\"\n", demuxers[dmx_idx]->info->name, n);

	// ----------- allocation of all data structures ---------------------------//
	if( !(r = calloc(1, sizeof(Resource))) ) {
		fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
		istream_close(i_stream);
		return NULL;
	}

	if( !(r->info=calloc(1, sizeof(ResourceInfo))) ) { // init all infos
		fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
		istream_close(i_stream);
		free(r);
		return NULL;
	}
	// -------------------------------------------------------------------------//
	// ----------------------- initializations ---------------------------------//
	/* initialization non needed 'cause we use calloc
	r->private_data=NULL;
	r->demuxer=NULL;
	//initializate tracks??? TODO
	// temporary track initialization:
	r->num_tracks=0;
	*/
	r->i_stream = i_stream;
	r->demuxer=demuxers[dmx_idx];
	// ------------------------------------------------------------------------//

	r->demuxer->init(r);

	// search for exclusive tracks: should be done track per track?
	ex_tracks_save(r->tracks);
	
	return r;
}

void r_close(Resource *r)
{
	if(r) {
		istream_close(r->i_stream);
		free(r->info);
		r->info=NULL;
		if(r->private_data!=NULL)
			free(r->private_data);

		g_list_foreach(r->tracks, (GFunc)free_track, r);

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
	Track *tracks[MAX_SEL_TRACKS];
	GList *track, *sel_tracks=NULL;

	/*Capabilities aren't used yet. TODO*/

	for (track=g_list_first(r->tracks); track; track=g_list_next(track))
		if( !strcmp(((Track *)track->data)->track_name, track_name) )
			sel_tracks=g_list_prepend(sel_tracks, track);

	if (!sel_tracks)
		return NULL;
	// now we reverse the order of the list to rebuild the resource tracks order
	// Probably this is not so useful: I feel free to remove the instruction sooner or later...
	g_list_reverse(sel_tracks);
	
	if((s=(Selector*)malloc(sizeof(Selector)))==NULL) {
		fnc_log(FNC_LOG_FATAL,"Memory allocation problems.\n");
		return NULL;
	}

	s->tracks = sel_tracks;
	s->total = g_list_length(sel_tracks);
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
		free_track(t, r); \
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

	/* parser allocation no more needed.
	 * we should now find the right media parser and link t->parse to the correspondig index
	if( !(t->parser=add_media_parser()) )
		ADD_TRACK_ERROR(FNC_LOG_FATAL,"Memory allocation problems.\n");
	*/

	if( !(t->buffer=OMSbuff_new(OMSBUFFER_DEFAULT_DIM)) )
		ADD_TRACK_ERROR(FNC_LOG_FATAL,"Memory allocation problems.\n");

	/* using calloc this initializzation is not needed.
	t->track_name[0]='\0';
	t->i_stream=NULL;
	*/
	/*t->i_stream=i_stream*/
	
	r->tracks = g_list_append(r->tracks, t);
	r->num_tracks++;
	
	return t;
}
#undef ADD_TRACK_ERROR

void free_track(Track *t, Resource *r)
{
	if (!t)
		return;

	istream_close(t->i_stream);
	free(t->track_info);
	mparser_unreg(t->parser, t->private_data);
	OMSbuff_free(t->buffer);
#if 0 // private data is not under Track jurisdiction!
	free(t->private_data);
#endif
	// t->calculate_timestamp=NULL; /*TODO*/
	if ( t->i_stream && IS_ISEXCLUSIVE(t->i_stream) )
		ex_track_remove(t);

	r->tracks = g_list_remove(r->tracks, t);
	free(t);
}

// static functions
static inline void ex_tracks_save(GList *tracks)
{
	g_list_foreach(tracks, (GFunc)ex_track_save, NULL);
}

static void ex_track_save(Track *track, gpointer *user_data)
{
	if ( track->i_stream && IS_ISEXCLUSIVE(track->i_stream) && !g_list_find(ex_tracks, track) )
			ex_tracks = g_list_prepend(ex_tracks, track);
}

static inline void ex_track_remove(Track *track)
{
	ex_tracks = g_list_remove(ex_tracks, track);
}

// private funcions for specific demuxer

/*! This function finds the correct demuxer for the given
 * <tt>resource_name</tt>.
 * First of all, it tries to match the <tt>resource_name</tt>'s extension with
 * one of those served by the demuxers and, if found, probes that demuxer.  If
 * no demuxer can be found this way, then it tries everyone from demuxer list.
 * \param resource_name the name of the resource.
 * \return the index of the valid demuxer in the list or -1 if it could not be
 * found.
 * */
// static int find_demuxer(resource_name n)
static int find_demuxer(InputStream *i_stream)
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
	if ( (/* find resource name extension: */res_ext=strrchr(i_stream->name, '.')) && (res_ext++) ) {
		// extension present
		for (i=0; demuxers[i]; i++) {
			strncpy(exts, demuxers[i]->info->extensions, sizeof(exts));
			for (tkn=strtok(exts, ","); tkn; tkn=strtok(NULL, ",")) {
				if (!strcmp(tkn, res_ext)) {
					fnc_log(FNC_LOG_DEBUG, "[MT] probing demuxer: extension \"%s\" matches \"%s\" demuxer\n", res_ext, demuxers[i]->info->name);
					if (demuxers[i]->probe(i_stream) == RESOURCE_OK) {
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
	if (!found) {
		for (i=0; demuxers[i]; i++) {
			if ( (i!=probed) && (demuxers[i]->probe(i_stream) == RESOURCE_OK) ) {
				fnc_log(FNC_LOG_DEBUG, "[MT] probing demuxer: demuxer found\n", res_ext, demuxers[i]->info->name);
				found = 1;
				break;
			}
		}
	}

	return found ? i: -1;
}

