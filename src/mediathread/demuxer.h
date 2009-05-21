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

#include "mediautils.h"
#include "InputStream.h"
#include "mediaparser.h"
#include "bufferqueue.h"
#include "sdp_grammar.h"

struct feng;

#define RESOURCE_OK 0
#define RESOURCE_NOT_FOUND -1
#define RESOURCE_DAMAGED -2
#define RESOURCE_TRACK_NOT_FOUND -4
#define RESOURCE_NOT_PARSEABLE -5
#define RESOURCE_EOF -6

#define MAX_TRACKS 20
#define MAX_SEL_TRACKS 5

//! typedefs that give convenient names to GLists used
typedef GList *TrackList;
typedef GList *SelList;

typedef struct ResourceInfo_s {
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

    /**
     * @brief Seekable resource flag
     *
     * Right now this is only false when the resource is a live
     * resource (@ref ResourceInfo::media_source set to @ref MS_live)
     * or when the demuxer provides no @ref Demuxer::seek function.
     *
     * In the future this can be extended to be set to false if the
     * user disable seeking in a particular stored (and otherwise
     * seekable) resource.
     */
    gboolean seekable;
} ResourceInfo;

ResourceInfo *resinfo_new();

typedef struct Resource {
    GMutex *lock;
    GThreadPool *fill_pool;
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
    TrackList tracks;
    int num_tracks;
    gboolean eor;
    void *private_data; /* Demuxer private data */
    struct feng *srv;
} Resource;

typedef struct Trackinfo_s {
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
    GMutex *lock;
    InputStream *i_stream;
    TrackInfo *info;
    double start_time;
    MediaParser *parser;
    /*feng*/
    BufferQueue_Producer *producer;
    MediaProperties *properties; /* track properties */
    Resource *parent;
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


// --- functions --- //

Demuxer *find_demuxer(InputStream *i_stream);

Resource *r_open(struct feng *srv, const char *inner_path);

void r_fill(Resource *resource, BufferQueue_Consumer *consumer);
int r_seek(Resource *resource, double time);

void r_close(Resource *resource);

Track *r_find_track(Resource *, const char *);

// Tracks
Track *add_track(Resource *, TrackInfo *, MediaProperties *);
void free_track(gpointer element, gpointer user_data);

#endif // FN_DEMUXER_H
