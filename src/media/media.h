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

#include <config.h>

#include <glib.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

struct feng;
struct RTP_session;

#define RESOURCE_OK 0
#define RESOURCE_ERR -1
#define RESOURCE_EOF -2
#define DEFAULT_MTU 1440

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

typedef struct Resource Resource;
typedef struct Track Track;

/**
 * @brief Descriptor structure of a resource
 * @ingroup resources
 *
 * This structure contains the basic parameters used by the media
 * backend code to access a resource; it connects to the demuxer used,
 * to the tracks found on the resource, and can be either private to a
 * client or shared among many (live streaming).
 */
struct Resource {
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

    /**
     * @brief End-of-resource indication
     *
     * @note Do note change this to gboolean because it is used
     *       through g_atomic_int_get/g_atomic_int_set.
     */
    int eor;

    MediaSource source;

    char *mrl;
    time_t mtime;
    double duration;

   /**
     * @brief Pool of one thread for filling up data for the session
     *
     * This is a pool consisting of exactly one thread that is used to
     * fill up the resource's tracks' @ref Track with data when it's
     * running low.
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

    int (*read_packet)(Resource *);
    int (*seek)(Resource *, double time_sec);
    GDestroyNotify uninit;

    /* Multiformat related things */
    TrackList tracks;
    void *private_data; /* Demuxer private data */
};

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

#define SDP_F_NAME         "n=%s\r\n"
#define SDP_F_DESCRURI     "u=%s\r\n"
#define SDP_F_EMAIL        "e=%s\r\n"
#define SDP_F_PHONE        "p=%s\r\n"

#define SDP_F_COMMONS_DEED "a=uriLicense:%s\r\n"
#define SDP_F_RDF_PAGE     "a=urimetadata:%s\r\n"
#define SDP_F_TITLE        "a=title:%s\r\n"
#define SDP_F_AUTHOR       "a=author:%s\r\n"

/**
 * @}
 */

struct Track {
    GMutex *lock;
    double start_time;

    /**
     * @brief The actual buffer queue
     *
     * This double-ended queue contains the actual buffer elements
     * that the BufferQueue framework deals with.
     */
    GQueue *queue;


    /**
     * @brief Stopped flag
     *
     * When this value is set to true, the consumer is stopped and no
     * further elements can be added. To set this flag call the
     * @ref bq_producer_unref function.
     *
     * A stopped producer cannot accept any new consumer, and will
     * wait for all the currently-connected consumers to return before
     * it is deleted.
     *
     * @note gint is used to be able to use g_atomic_int_get function.
     */
    gint stopped;

    /**
     * @brief Next serial to use for the added elements
     *
     * This is the next value for @ref struct MParserBuffer::serial; it
     * starts from zero and it's reset to zero each time the queue is
     * reset; each element added to the queue gets this value before
     * getting incremented; it is used by @ref bq_consumer_unseen().
     */
    uint16_t next_serial;

    /**
     * @brief Serial number for the queue
     *
     * This is the serial number of the queue inside the producer,
     * starts from zero and is increased each time the queue is reset.
     */
    gulong queue_serial;

    /**
     * @brief Count of registered consumers
     *
     * This attribute keeps the updated number of registered
     * consumers; each buffer needs to reach the same count before it
     * gets removed from the queue.
     */
    gulong consumers;

    /**
     * @brief Last consumer exited condition
     *
     * This condition is signalled once the last consumer of a stopped
     * producer (see @ref stopped) exits.
     *
     * @see bq_consumer_unref
     */
    GCond *last_consumer;

    Resource *parent;

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
     * @brief SDP description for the track
     *
     * This string is used to append attributes that have to be sent
     * with the SDP description of the track by the DESCRIBE method
     * (see @ref sdp_track_descr).
     *
     * Simply append them, newline-terminated, to this string and
     * they'll be copied straight to the SDP description.
     */
    GString *sdp_description;

    /**
     * @defgroup track_methods
     *
     * @{ */

    /**
     * @brief Actual parser callback
     *
     * Take a single elementary unit of the codec stream and prepare
     * the rtp payload out of it.
     *
     * @param track Track object on which to work
     * @param data Packet coming from the demuxer layer,
     * @param len Length of @p data byte array
     *
     * @return: 0 on success, non zero otherwise.
     */
    int (*parse)(Track *track, uint8_t *data, ssize_t len);

    /**
     * @brief Uninit function to free the private data
     */
    void (*uninit)(Track *track);

    /** @} */

    /**
     * @defgroup MediaProperties
     *
     * @brief Track's media properties
     *
     * @{ */
    int payload_type;
    unsigned int clock_rate;
    char *encoding_name;
    MediaType media_type;
    int audio_channels;
    double pts;             //time is in seconds
    double dts;             //time is in seconds
    double frame_duration;  //time is in seconds
    uint8_t *extradata;
    size_t extradata_len;
    /** @} */

    union {
        struct {
            uint8_t ident[3];
        } xiph;

        struct {
            bool is_avc;
            uint8_t nal_length_size; // used in avc
        } h264;

        struct {
            char *mq_path;
        } live;
    } private_data;
};

/**
 * @brief Buffer passed between parsers and RTP sessions
 *
 * This is what is being encapsulated by @ref BufferQueue_Element.
 */
struct MParserBuffer {
    /**
     * @brief Seen count
     *
     * Reverse reference counter, that tells how many consumers have
     * seen the buffer already. Once the buffer has been seen by all
     * the consumers of its producer, the buffer is deleted and the
     * queue is shifted further on.
     *
     * @note Since once the counter reaches the number of consumers
     *       the element is deleted and freed, before increasing the
     *       counter it is necessary to hold the @ref
     *       BufferQueue_Producer lock.
     */
    gulong seen;

    double timestamp;   /*!< presentation time of packet */
    double delivery;    /*!< decoding time of packet */
    double duration;    /*!< packet duration */

    gboolean marker;    /*!< marker bit, set if we are sending the last frag */
    uint32_t rtp_timestamp; /*!< RTP version of the presenation time, used only by live */
    uint16_t seq_no;    /*!< Packet sequence number, used only by live */

    size_t data_size;   /*!< packet size */
    uint8_t *data;      /*!< actual packet data */
};

// --- functions --- //

Resource *r_open(const char *inner_path);

int r_read(Resource *resource);
int r_seek(Resource *resource, double time);

void r_close(Resource *resource);
void r_pause(Resource *resource);
void r_resume(Resource *resource);
void r_fill(Resource *resource, struct RTP_session *consumer);

Track *r_find_track(Resource *, const char *);

Track *track_new(char *name);
void track_free(Track *track);
void track_reset_queue(struct Track *);
void track_write(Track *tr, struct MParserBuffer *buffer);

struct MParserBuffer *bq_consumer_get(struct RTP_session *consumer);
gulong bq_consumer_unseen(struct RTP_session *consumer);
gboolean bq_consumer_move(struct RTP_session *consumer);
gboolean bq_consumer_stopped(struct RTP_session *consumer);
void bq_consumer_free(struct RTP_session *consumer);

void sdp_descr_append_config(Track *track);
void sdp_descr_append_rtpmap(Track *track);

void bq_init();
void ffmpeg_init(void);

/**
 * @defgroup parsers
 *
 * @brief Parser init methods
 *
 * Declaration of the functions used by the libavformat demuxer to
 * initialize the parsers and to set @ref Track::parse and @ref
 * Track::uninit.
 *
 * @{ */

int aac_init(Track *track);
int aac_parse(Track *track, uint8_t *data, ssize_t len);

int amr_init(Track *track);
int amr_parse(Track *track, uint8_t *data, ssize_t len);

int h263_init(Track *track);
int h263_parse(Track *track, uint8_t *data, ssize_t len);

int h264_init(Track *track);
int h264_parse(Track *track, uint8_t *data, ssize_t len);

int mp4ves_init(Track *track);
int mp4ves_parse(Track *track, uint8_t *data, ssize_t len);

int mpa_parse(Track *track, uint8_t *data, ssize_t len);

int mpv_parse(Track *track, uint8_t *data, ssize_t len);

int speex_parse(Track *track, uint8_t *data, ssize_t len);

int vp8_init(Track *track);
int vp8_parse(Track *track, uint8_t *data, ssize_t len);

/**
 * @defgroup parsers_xiph
 *
 * @brief Xiph parsers
 *
 * Theora and Vorbis share the same parser and uninit function, and a
 * shared private data structure that is filled by the initialization.
 * @{ */
int theora_init(Track *track);
int vorbis_init(Track *track);

int xiph_parse(Track *tr, uint8_t *data, ssize_t len);
void xiph_uninit(Track *tr);
/** @} */

/** @} */

#endif // FN_DEMUXER_H
