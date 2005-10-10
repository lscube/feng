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

typedef struct {
	char *mrl;
	time_t last_change;
	GList *media; // GList of MediaDescr elements
	char *sdp;
	// here we will put other description formats that may be supported.
} SessionDescr;

typedef struct {
	char *mrl;
	time_t last_change;
	char *sdp;
	// here we will put other description formats that may be supported.
} MediaDescr;

typedef struct __CAPABILITIES {

} Capabilities;

typedef struct __TRACK_INFO {
	//start CC
	char commons_deed[255]; 
	char rdf_page[255];
	char title[80];
	char author[80];	
	int tag_dim;    
	//end CC
} TrackInfo;

//typedef enum {stored=0,live} media_source;
	
typedef struct __TRACK {
	InputStream *i_stream;
	TrackInfo *track_info;
	char name[255];
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

typedef struct __RESOURCE_INFO {
	char twin[255];
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

/*infos: 
	char* track-name, 
	char* filename, 
	char *encoding_name, 
	uint32 bit_rate,	
	uint32 clock_rate,	 
	float sample_rate,
	float OutputSamplingFrequency,
	short audio_channels,
	uint32 bit_per_sample, 
	uint32 FlagInterlaced,
	uint32 PixelWidth;
	uint32 PixelHeight,
	uint32 DisplayWidth,
	uint32 DisplayHeight,
	uint32 DisplayUnit,
	uint32 AspectRatio,
	uint8 *ColorSpace,
	float GammaValue,
*/

// Resounces
Resource *r_open(resource_name);/*open the resource: mkv, sd ...*/
void r_close(Resource *);
msg_error get_resource_info(resource_name, ResourceInfo *);
Selector *r_open_tracks(Resource *, char *track_name, Capabilities *capabilities);/*open the right tracks*/
void r_close_tracks(Selector *);/*close all tracks*/ // shawill: XXX do we need it?
inline msg_error r_seek(Resource *, long int /*time_sec*/ );/*seeks the resource: mkv, sd ...*/

// TrackList handling functions

// Tracks
Track *add_track(Resource *);
void free_track(Track *, Resource *);
/*-------------------------------------------*/

#endif // __DEMUXER_H
