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

/**
 * @brief Descriptor structure of a resource
 * @ingroup resources
 *
 * This structure contains the basic parameters used by the media
 * backend code to access a resource; it connects to the demuxer used,
 * to the tracks found on the resource, and can be either private to a
 * client or shared among many (live streaming).
 */
typedef struct Resource {
    GMutex *lock;

    /**
     * @brief Reference counter for the clients using the resource
     *
     * This variable keeps count of the number of clients that are
     * connected to a given resource, it is supposed to keep at 1 for
     * non-live resources, and to vary between 0 and the number of
     * clients when it is a live resource.
     */
    gint count;

    const struct Demuxer *demuxer;
    ResourceInfo *info;

   /**
     * @brief Pool of one thread for filling up data for the session
     *
     * This is a pool consisting of exactly one thread that is used to
     * fill up the resource's tracks' @ref BufferQueue_Producer with
     * data when it's running low.
     *
     * Since we do want to do this asynchronously but we don't really
     * want race conditions (and they would anyway just end up waiting
     * on the same lock), there is no need to allow multiple threads
     * to do the same thing here.
     *
     * Please note that this is created, for non-live resources,
     * during the resume phase (@ref r_resume), and stopped during
     * either the pause phase (@ref r_pause) or during the final free
     * (@ref r_free_cb). For live resources, this will be created by
     * @ref r_open_hashed when the first client connects, and
     * destroyed by @ref r_close when the last client disconnects.
     */
    GThreadPool *fill_pool;

    /* Timescale fixer callback function for meta-demuxers */
    double (*timescaler)(struct Resource *, double);
    /* EDL specific data */
    struct Resource *edl;
    /* Multiformat related things */
    TrackList tracks;
    int num_tracks;
    int eor;
    void *private_data; /* Demuxer private data */
} Resource;

/**
 * @defgroup SDP_FORMAT_MACROS SDP attributes format macros
 *
 * This is a list of standard format macros that can be used to append
 * standard extra attributes to an SDP reply. They are listed all
 * together so that the code uses them consistently; they are macros
 * rather than string arrays because they can be combined into a
 * single format at once.
 *
 * @{
 */

#define SDP_F_COMMONS_DEED "a=uriLicense:%s\r\n"
#define SDP_F_RDF_PAGE     "a=urimetadata:%s\r\n"
#define SDP_F_TITLE        "a=title:%s\r\n"
#define SDP_F_AUTHOR       "a=author:%s\r\n"

/**
 * @}
 */

typedef struct Trackinfo_s {
    char *mrl;
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

    /**
     * @brief Track name
     *
     * This string is used to access the track within the resource,
     * and is reported to the client for identification.
     *
     * It should be set to an instance-bound string (duplicated) to be
     * freed by g_free.
     */
    char *name;

    /**
     * @brief SDP attributes for the track
     *
     * This string is used to append attributes that have to be sent
     * with the SDP description of the track by the DESCRIBE method
     * (see @ref sdp_track_descr).
     *
     * Simply append them, newline-terminated, to this string and
     * they'll be copied straight to the SDP description.
     */
    GString *attributes;

    MediaProperties properties;
} Track;

#define FENG_DEMUXER(shortname, source)                                   \
    const Demuxer fnc_demuxer_##shortname = {                             \
        shortname##_name,                                                 \
        source,                                                           \
        shortname##_probe,                                                \
        shortname##_init,                                                 \
        shortname##_read_packet,                                          \
        shortname##_seek,                                                 \
        shortname##_uninit                                                \
    }

typedef struct Demuxer {
   /** name of demuxer module*/
    const char *name;

    /** demuxer source type */
    MediaSource source;

    gboolean (*probe)(const char *filename);
    int (*init)(Resource *);
    int (*read_packet)(Resource *);
    int (*seek)(Resource *, double time_sec);
    GDestroyNotify uninit;
} Demuxer;


// --- functions --- //

Resource *r_open(const char *inner_path);

int r_read(Resource *resource);
int r_seek(Resource *resource, double time);

void r_close(Resource *resource);
void r_pause(Resource *resource);
void r_resume(Resource *resource);
void r_fill(Resource *resource, BufferQueue_Consumer *consumer);

Track *r_find_track(Resource *, const char *);

// Tracks
Track *add_track(Resource *, TrackInfo *, MediaProperties *);
void free_track(gpointer element, gpointer user_data);

BufferQueue_Producer *track_get_producer(Track *tr);

#endif // FN_DEMUXER_H
