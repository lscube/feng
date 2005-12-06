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

#ifndef __DEMUXER_H
#define __DEMUXER_H

#include <glib.h>

#include <fenice/types.h>
#include <fenice/utils.h>
#include <fenice/mediautils.h>
#include <fenice/InputStream.h>
#include <fenice/MediaParser.h>
#include <fenice/bufferpool.h>

#define resource_name char *
/*
 * a resource_name can be a mkv, sd, program stream, avi, device ... 
 * syntax could be: 
 * 	udp://ip:port
 * 	file://path/filename
 * 	dev://device:driver
 */

#define msg_error int
/*msg_error:*/
#define	RESOURCE_OK 0 
#define	RESOURCE__NOT_FOUND -1 
#define	RESOURCE_DAMAGED -2
#define	RESOURCE_NOT_SEEKABLE -3
#define	RESOURCE_TRACK_NOT_FOUND -4
/*...*/
#define MAX_TRACKS 20	
#define MAX_SEL_TRACKS 5

//! Macros that take the data part of a GList element and cast to correct type
#define RESOURCE(x) ((Resource *)x->data)
#define TRACK(x) ((Track *)x->data)
#define RESOURCE_DESCR(x) ((ResourceDescr *)x->data)
#define MEDIA_DESCR(x) ((MediaDescr *)x->data)

typedef struct __CAPABILITIES {

} Capabilities;

#if 0 // define MObject with MObject_def
typedef struct __TRACK_INFO {
	MOBJECT_COMMONS; // MObject commons MUST be the first field
#endif
MObject_def(__TRACK_INFO)
	char *mrl;
	char name[255];
	char *sdp_private;
	//start CC
	char commons_deed[255]; 
	char rdf_page[255];
	char title[80];
	char author[80];	
	// int tag_dim;    
	//end CC
} TrackInfo;

//typedef enum {stored=0,live} media_source;
	
typedef struct __TRACK {
	InputStream *i_stream;
	TrackInfo *info;
	// char name[255];
	long int timestamp;
	MediaParser *parser;
	/*bufferpool*/
	OMSBuffer *buffer;
	media_source msource;
	MediaProperties *properties; /* track properties */
	/* private data is managed by specific media parser: from allocation to deallocation
	 * track MUST NOT do anything on this pointer! */ 
	void *private_data;
	void *parser_private; /* private data of media parser */
} Track;

typedef struct __SELECTOR {
	// Track *tracks[MAX_SEL_TRACKS];	
	GList *tracks;
	uint32 default_index;
	uint32 selected_index;/**/
	uint32 total; /*total tracks in selector*/
} Selector;

#if 0 // define MObject with MObject_def
typedef struct __RESOURCE_INFO {
	MOBJECT_COMMONS; // MObject commons MUST be the first field
#endif
MObject_def(__RESOURCE_INFO)
	char *mrl;
	char *name;
	char *description;
	char *descrURI;
	char *email;
	char *phone;
	char *sdp_private;
	// char mrl[255];
	char twin[255];
	char multicast[16];
	char ttl[4];
} ResourceInfo;

typedef struct __RESOURCE {
	InputStream *i_stream;
	struct __DEMUXER *demuxer;
	ResourceInfo *info;
	// Track *tracks[MAX_TRACKS];
	GList *tracks;
	uint32 num_tracks;
	void *private_data; /* private data of demuxer */
} Resource;

typedef struct {
	/*name of demuxer module*/
	const char *name;
	/* short name (for config strings) (e.g.:"sd") */
	const char *short_name;
	/* author ("Author name & surname <mail>") */
	const char *author;
	/* any additional comments */
	const char *comment;
	/* served file extensions */
	const char *extensions; // coma separated list of extensions (w/o '.')
} DemuxerInfo;

typedef struct __DEMUXER {
	DemuxerInfo *info;
	int (*probe)(InputStream *);
	int (*init)(Resource *);
	int (*read_header)(Resource *);
	int (*read_packet)(Resource *);
	int (*seek)(Resource *, long int time_sec);
	int (*uninit)(Resource *);
	//...
} Demuxer;

typedef struct {
	time_t last_change;
	ResourceInfo *info;
	GList *media; // GList of MediaDescr elements
} ResourceDescr;

typedef struct {
	time_t last_change;
	TrackInfo *info;
	MediaProperties *properties;
} MediaDescr;

// --- functions --- //

// Resounces
Resource *r_open(resource_name);/*open the resource: mkv, sd ...*/
void r_close(Resource *);
msg_error get_resource_info(resource_name, ResourceInfo *);
Selector *r_open_tracks(Resource *, char *track_name, Capabilities *capabilities);/*open the right tracks*/
void r_close_tracks(Selector *);/*close all tracks*/ // shawill: XXX do we need it?
inline msg_error r_seek(Resource *, long int /*time_sec*/ );/*seeks the resource: mkv, sd ...*/
int r_changed(ResourceDescr *);

// TrackList handling functions

// Tracks
// Track *add_track(Resource *);
Track *add_track(Resource *, TrackInfo *, MediaProperties *);
void free_track(Track *, Resource *);

// Resources and Media descriptions
ResourceDescr *r_descr_get(resource_name);
// ResourceDescr *r_descr_new(Resource *);
// void r_descr_free(ResourceDescr *);
/* --- functions implemented in descriptionAPI.c --- */
/*! the functions that return pointers do not allocate new memory, simply return
 * the pointer of the description resource. So,l there is no need to free
 * anything. 
 * The functions that return pointers return NULL if the value is not set.
 * */
inline time_t r_descr_last_change(ResourceDescr *);
inline char *r_descr_mrl(ResourceDescr *);
inline char *r_descr_twin(ResourceDescr *);
inline char *r_descr_multicast(ResourceDescr *);
inline char *r_descr_ttl(ResourceDescr *);
inline char *r_descr_name(ResourceDescr *);
inline char *r_descr_description(ResourceDescr *);
inline char *r_descr_descrURI(ResourceDescr *);
inline char *r_descr_email(ResourceDescr *);
inline char *r_descr_phone(ResourceDescr *);
/*-------------------------------------------*/

#endif // __DEMUXER_H
