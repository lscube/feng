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
#include <stdint.h>

#include "bufferqueue.h"

struct feng;

#define RESOURCE_OK 0
#define RESOURCE_NOT_FOUND -1
#define RESOURCE_DAMAGED -2
#define RESOURCE_AGAIN -3
#define RESOURCE_TRACK_NOT_FOUND -4
#define RESOURCE_NOT_PARSEABLE -5
#define RESOURCE_EOF -6

#define MAX_TRACKS 20
#define MAX_SEL_TRACKS 5

typedef enum {
    MP_undef = -1,
    MP_audio,
    MP_video,
    MP_application,
    MP_data,
    MP_control
} MediaType;

typedef enum {
    STORED_SOURCE=0,
    LIVE_SOURCE
} MediaSource;

//! typedefs that give convenient names to GLists used
typedef GList *TrackList;
typedef GList *SelList;

struct MediaParser;

typedef enum {empty_field, fmtp, rtpmap} sdp_field_type;

typedef struct {
	sdp_field_type type;
	char *field;
} sdp_field;

typedef struct ResourceInfo_s {
    char *mrl;
    char *name;
    char *description;
    char *descrURI;
    char *email;
    char *phone;
    time_t mtime;
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
     * resource (@ref ResourceInfo::media_source set to @ref LIVE_SOURCE)
     * or when the demuxer provides no @ref Demuxer::seek function.
     *
     * In the future this can be extended to be set to false if the
     * user disable seeking in a particular stored (and otherwise
     * seekable) resource.
     */
    gboolean seekable;
} ResourceInfo;

typedef struct Resource {
    GMutex *lock;
    guint count;
    const struct Demuxer *demuxer;
    ResourceInfo *info;

    /* Timescale fixer callback function for meta-demuxers */
    double (*timescaler)(struct Resource *, double);
    /* EDL specific data */
    struct Resource *edl;
    /* Multiformat related things */
    TrackList tracks;
    int num_tracks;
    int eor;
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

typedef struct MediaProperties {
    int bit_rate; /*!< average if VBR or -1 is not useful*/
    int payload_type;
    unsigned int clock_rate;
    char encoding_name[11];
    MediaType media_type;
    MediaSource media_source;
    int codec_id; /*!< Codec ID as defined by ffmpeg */
    int codec_sub_id; /*!< Subcodec ID as defined by ffmpeg */
    double pts;             //time is in seconds
    double dts;             //time is in seconds
    double frame_duration;  //time is in seconds
    float sample_rate;/*!< SamplingFrequency*/
    float OutputSamplingFrequency;
    int audio_channels;
    int bit_per_sample;/*!< BitDepth*/
    float frame_rate;
    int FlagInterlaced;
    unsigned int PixelWidth;
    unsigned int PixelHeight;
    unsigned int DisplayWidth;
    unsigned int DisplayHeight;
    unsigned int DisplayUnit;
    unsigned int AspectRatio;
    uint8_t *ColorSpace;
    float GammaValue;
    uint8_t *extradata;
    size_t extradata_len;
} MediaProperties;

typedef struct Track {
    GMutex *lock;
    TrackInfo *info;
    double start_time;
    const struct MediaParser *parser;
    /*feng*/
    BufferQueue_Producer *producer;
    Resource *parent;

    void *private_data; /* private data of media parser */

    GSList *sdp_fields;

    MediaProperties properties;
} Track;

typedef struct {
    /** name of demuxer module*/
    const char *name;
    /** short name (for config strings) (e.g.:"sd") */
    const char *short_name;
    /** author ("Author name & surname <mail>") */
    const char *author;
    /** any additional comments */
    const char *comment;
    /** served file extensions */
    const char *extensions; // coma separated list of extensions (w/o '.')
    /** demuxer source type */
    MediaSource source;
} DemuxerInfo;

typedef struct Demuxer {
    const DemuxerInfo *info;
    int (*probe)(const char *filename);
    int (*init)(Resource *);
    int (*read_packet)(Resource *);
    int (*seek)(Resource *, double time_sec);
    GDestroyNotify uninit;
} Demuxer;


// --- functions --- //

Resource *r_open(struct feng *srv, const char *inner_path);

int r_read(Resource *resource);
int r_seek(Resource *resource, double time);

void r_close(Resource *resource);

Track *r_find_track(Resource *, const char *);

// Tracks
Track *add_track(Resource *, TrackInfo *, MediaProperties *);
void free_track(gpointer element, gpointer user_data);

void track_add_sdp_field(Track *track, sdp_field_type type, char *value);

BufferQueue_Producer *track_get_producer(Track *tr);

#endif // FN_DEMUXER_H
