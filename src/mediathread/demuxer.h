/* *
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 *
 * feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * feng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with feng; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * */

#ifndef FN_DEMUXER_H
#define FN_DEMUXER_H

#include <glib.h>

#include <fenice/utils.h>
#include "mediautils.h"
#include "InputStream.h"
#include "mediaparser.h"
#include "bufferqueue.h"
#include <fenice/sdp_grammar.h>
#include <fenice/server.h>

#define RESOURCE_OK 0
#define RESOURCE_NOT_FOUND -1
#define RESOURCE_DAMAGED -2
#define RESOURCE_NOT_SEEKABLE -3
#define RESOURCE_TRACK_NOT_FOUND -4
#define RESOURCE_NOT_PARSEABLE -5
#define RESOURCE_EOF -6

#define MAX_TRACKS 20
#define MAX_SEL_TRACKS 5

//! Macros that take the data part of a GList element and cast to correct type
#define RESOURCE(x) ((Resource *)x->data)
#define TRACK(x) ((Track *)x->data)
#define RESOURCE_DESCR(x) ((ResourceDescr *)x->data)
#define MEDIA_DESCR(x) ((MediaDescr *)x->data)

//! typedefs that give convenient names to GLists used
typedef GList *TrackList;
typedef GList *MediaDescrList;
typedef GList *SelList;
typedef GPtrArray *MediaDescrListArray;

//! Some macros to wrap GList functions
#define list_first(x) g_list_first(x)
#define list_next(x) g_list_next(x)

//! Some mecros to wrap GPtrArray functions
#define array_data(x) x->pdata
#define array_index(x, y) x->pdata[y]

MObject_def(ResourceInfo_s)
    char *mrl;
    char *name;
    char *description;
    char *descrURI;
    char *email;
    char *phone;
    sdp_field_list sdp_private;
    double duration;
    MediaSource media_source;
    char twin[256];
    char multicast[16];
    int port;
    char ttl[4];
} ResourceInfo;

typedef struct Resource {
    GMutex *lock;
    InputStream *i_stream;
    struct Demuxer *demuxer;
    ResourceInfo *info;
    // Metadata begin
#ifdef HAVE_METADATA
    void *metadata;
#endif
    // Metadata end

    /* Timescale fixer callback function for meta-demuxers */
    double (*timescaler)(struct Resource *, double);
    /* EDL specific data */
    struct Resource *edl;
    /* Multiformat related things */
    SelList sel;
    TrackList tracks;
    int num_sel;
    int num_tracks;
    void *private_data; /* Demuxer private data */
    int eos; //!< signals the end of stream
    feng *srv;
} Resource;

MObject_def(Trackinfo_s)
    char *mrl;
    char name[256];
    int id; // should it more generic?
    int rtp_port;
    //start CC
    char commons_deed[256];
    char rdf_page[256];
    char title[256];
    char author[256];
    //end CC
} TrackInfo;

typedef struct Track {
    InputStream *i_stream;
    TrackInfo *info;
    long int timestamp;
    MediaParser *parser;
    /*feng*/
    BufferQueue_Producer *producer;
    MediaProperties *properties; /* track properties */
    Resource *parent;
    /* private data is managed by specific media parser: from allocation to deallocation
     * track MUST NOT do anything on this pointer! */
    void *private_data;
    void *parser_private; /* private data of media parser */
} Track;

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

typedef struct Demuxer {
    const DemuxerInfo *info;
    int (*probe)(InputStream *);
    int (*init)(Resource *);
    int (*read_packet)(Resource *);
    int (*seek)(Resource *, double time_sec);
    int (*uninit)(Resource *);
    //...
} Demuxer;

typedef struct {
    time_t last_change;
    ResourceInfo *info;
    MediaDescrList media; // GList of MediaDescr elements
} ResourceDescr;

typedef struct {
    time_t last_change;
    TrackInfo *info;
    MediaProperties *properties;
} MediaDescr;

// --- functions --- //

// Resources

Resource *r_open(feng *srv, const char *inner_path);

void r_close(Resource *);

int r_seek(Resource *resource, double time);

Track *r_find_track(Resource *, const char *);

// Tracks
Track *add_track(Resource *, TrackInfo *, MediaProperties *);

// Resources and Media descriptions
ResourceDescr *r_descr_get(feng *srv, const char *inner_path);
MediaDescrListArray r_descr_get_media(ResourceDescr *r_descr);

#endif // FN_DEMUXER_H
