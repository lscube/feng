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

#include <config.h>

#include <stdbool.h>
#include <time.h>
#include <mqueue.h>
#include <fcntl.h> /* for mq_open's O_* options */
#include <errno.h>
#include <unistd.h> /* for usleep() */
#include <string.h>
#include <math.h>

#include "feng.h"
#include "fnc_log.h"

#include "media/media.h"

#define REQUIRED_FLUX_PROTOCOL_VERSION 4

struct flux_msg {
    uint32_t proto_version;
    double start_time;
    uint32_t dts;
    uint32_t start_dts;
    double duration;
    double insertion_time;

    uint8_t discard1;
    uint8_t marker;
    uint16_t seq_no;
    uint32_t timestamp;
    uint32_t discard2;

    uint8_t data[];
} ATTR_PACKED;

static gpointer flux_read_messages(gpointer ptr);

/**
 * @brief Uninitialisation function for the demuxer_sd fake parser
 *
 * This function simply frees the slice in Track::private_data as a
 * mqd_t object.
 */
static void live_track_uninit(Track *tr) {
    g_free(tr->live.mq_path);
}

/**
 * @defgroup sd2keys SD2 format keys
 *
 * @note These are defined as constant arrays of characters rather
 *       than as literal macros. This is because they are never
 *       injected directly in other literals, and having them this way
 *       can warn if support for them is mistakenly removed from the
 *       logic code.
 *
 * @{
 */
static const char SD2_KEY_MRL            [] = "mrl";
static const char SD2_KEY_CLOCK_RATE     [] = "clock_rate";
static const char SD2_KEY_PAYLOAD_TYPE   [] = "payload_type";
static const char SD2_KEY_ENCODING_NAME  [] = "encoding_name";
static const char SD2_KEY_MEDIA_TYPE     [] = "media_type";
static const char SD2_KEY_AUDIO_CHANNELS [] = "audio_channels";
static const char SD2_KEY_FMTP           [] = "fmtp";

static const char SD2_KEY_LICENSE        [] = "license";
static const char SD2_KEY_RDF_PAGE       [] = "rdf_page";
static const char SD2_KEY_TITLE          [] = "title";
static const char SD2_KEY_CREATOR        [] = "creator";
/**
 * @}
 */

/**
 * @brief Sets the default track values for common encodings.
 *
 * @param track The track to set the defaults to
 *
 * @TODO It would be possible to optimize this by making sure that the
 *       strings are in strcmp order, so that if the table has an
 *       entry bigger than the track's encoding, it bails out right
 *       away.

 * @TODO Dynamic payload types' defaults could have some parameters
 *       set by using -1 as payload.
 */
static void set_encoding_defaults(Track *track)
{
    static const struct {
        char encoding_name[8];
        int32_t payload;
        int32_t clock_rate;
    } encoding_defaults[] = {
        {"PCMU"   , 0, 8000  },
        {"G726_32", 2, 8000  },
        {"GSM"    , 3, 8000  },
        {"G723"   , 4, 8000  },
        {"DVI4"   , 5, 8000  },
        {"DVI4"   , 6, 16000 },
        {"LPC"    , 7, 8000  },
        {"PCMA"   , 8, 8000  },
        {"G722"   , 9, 8000  },
        {"L16"    ,10, 44100 },
        {"L16"    ,11, 44100 },
        {"QCELP"  ,12, 8000  },
        {"MPA"    ,14, 90000 },
        {"G728"   ,15, 8000  },
        {"DVI4"   ,16, 11025 },
        {"DVI4"   ,17, 22050 },
        {"G729"   ,18, 8000  },
        {"CelB"   ,25, 90000 },
        {"JPEG"   ,26, 90000 },
        {"nv"     ,28, 90000 },
        {"H261"   ,31, 90000 },
        {"MPV"    ,32, 90000 },
        {"MP2T"   ,33, 90000 },
        {"H263"   ,34, 90000 }
    };

    size_t i;

    for (i = 0; i < sizeof(encoding_defaults)/sizeof(encoding_defaults[0]); i++) {
        if ( strcmp(encoding_defaults[i].encoding_name, track->encoding_name) == 0) {
            track->payload_type = encoding_defaults[i].payload;
            track->clock_rate = encoding_defaults[i].clock_rate;
        }
    }
}

Resource *sd2_open(const char *url)
{
    Resource *r = NULL;
    char *mrl;
    GKeyFile *file = g_key_file_new();
    gchar **tracknames = NULL, **trackgroups = NULL, *currtrack = NULL;
    int next_dynamic_payload = 96;
    TrackList tracks = NULL;

    fnc_log(FNC_LOG_DEBUG,
            "using live streaming demuxer for resource '%s'",
            url);

    mrl = g_strdup_printf("%s/%s.sd2",
                          feng_default_vhost->virtuals_root,
                          url);

    if ( !g_key_file_load_from_file(file, mrl, G_KEY_FILE_NONE, NULL) )
        goto error;

    if ( (tracknames = g_key_file_get_groups(file, NULL)) == NULL )
        goto error;

    trackgroups = tracknames;

    while ( (currtrack = *tracknames++) != NULL ) {
        Track *track = NULL;

        gchar *track_mrl, *media_type, *tmpstr;

        if ( !feng_str_is_unreserved(currtrack) ) {
            fnc_log(FNC_LOG_ERR, "[sd2] invalid track name '%s' for '%s'",
                    currtrack, mrl);
            goto corrupted_track;
        }

        track = track_new(currtrack);

        track->uninit = live_track_uninit;

        track_mrl = g_key_file_get_string(file, currtrack,
                                          SD2_KEY_MRL,
                                          NULL);

        /* This section might have to be changed if we end up
           supporting multiple source protocoles. */
        if ( track_mrl == NULL || !g_str_has_prefix(track_mrl, "mq://") ) {
            fnc_log(FNC_LOG_ERR, "[sd2] invalid mrl '%s' for '%s'",
                    track_mrl, mrl);
            goto corrupted_track;
        }

        track->live.mq_path = strdup(track_mrl + strlen("mq://"));

        if ( (track->encoding_name = g_strdup(g_key_file_get_string(file, currtrack,
                                                                    SD2_KEY_ENCODING_NAME,
                                                                    NULL))) == NULL ) {
            fnc_log(FNC_LOG_ERR, "[live] %s: missing encoding name",
                    mrl);
            goto corrupted_track;
        }

        set_encoding_defaults(track);

        if ( g_key_file_has_key(file, currtrack, SD2_KEY_PAYLOAD_TYPE, NULL) )
            track->payload_type = g_key_file_get_integer(file, currtrack,
                                                         SD2_KEY_PAYLOAD_TYPE,
                                                         NULL);

        if ( track->payload_type == -1 )
            track->payload_type = next_dynamic_payload++;
        else if ( track->payload_type >= next_dynamic_payload )
            next_dynamic_payload = track->payload_type + 1;

        if (track->payload_type >= 0 && track->payload_type < 24)
            track->media_type = MP_audio;

        if (track->payload_type > 23 && track->payload_type < 96)
            track->media_type = MP_video;

        media_type = g_key_file_get_string(file, currtrack,
                                           SD2_KEY_MEDIA_TYPE,
                                           NULL);

        if ( media_type == NULL && track->media_type == MP_undef ) {
            fnc_log(FNC_LOG_ERR, "[live] %s: media type undefined for dynamic payload %d",
                    mrl, track->payload_type);
        } else if ( g_strcmp0(media_type, "audio") == 0 )
            track->media_type = MP_audio;
        else if ( g_strcmp0(media_type, "video") == 0 )
            track->media_type = MP_video;
        else {
            fnc_log(FNC_LOG_ERR, "[live] %s: invalid media type string '%s'",
                    mrl, media_type);
            goto corrupted_track;
        }

        if ( track->media_type == MP_audio ) {
            track->audio_channels = g_key_file_get_integer(file, currtrack,
                                                                SD2_KEY_AUDIO_CHANNELS,
                                                                NULL);

            if ( track->audio_channels >= INT32_MAX || track->audio_channels <= 0 ) {
                fnc_log(FNC_LOG_ERR, "[sd2] invalid audio_channels '%d' for '%s'",
                        track->audio_channels, mrl);
                goto corrupted_track;
            }
        }

        if ( g_key_file_has_key(file, currtrack, SD2_KEY_CLOCK_RATE, NULL) ) {
            track->clock_rate = g_key_file_get_integer(file, currtrack,
                                                       SD2_KEY_CLOCK_RATE,
                                                       NULL);
            if ( track->clock_rate >= INT32_MAX || track->clock_rate <= 0 ) {
                fnc_log(FNC_LOG_ERR, "[sd2] invalid clock_rate '%d' for '%s'",
                        track->clock_rate, mrl);
                goto corrupted_track;
            }
        }

        if ( (tmpstr = g_key_file_get_string(file, currtrack,
                                             SD2_KEY_LICENSE,
                                             NULL)) )
             g_string_append_printf(track->sdp_description,
                                    SDP_F_COMMONS_DEED,
                                    tmpstr);

        if ( (tmpstr = g_key_file_get_string(file, currtrack,
                                             SD2_KEY_RDF_PAGE,
                                             NULL)) )
             g_string_append_printf(track->sdp_description,
                                    SDP_F_RDF_PAGE,
                                    tmpstr);

        if ( (tmpstr = g_key_file_get_string(file, currtrack,
                                             SD2_KEY_TITLE,
                                             NULL)) )
             g_string_append_printf(track->sdp_description,
                                    SDP_F_TITLE,
                                    tmpstr);

        if ( (tmpstr = g_key_file_get_string(file, currtrack,
                                             SD2_KEY_CREATOR,
                                             NULL)) )
             g_string_append_printf(track->sdp_description,
                                    SDP_F_AUTHOR,
                                    tmpstr);

        if (track->payload_type >= 96)
            sdp_descr_append_rtpmap(track);

        /* This goes _after_ rtpmap for compatibility with older
           FFmpeg */
        if ( (tmpstr = g_key_file_get_string(file, currtrack,
                                             SD2_KEY_FMTP,
                                             NULL)) )
            g_string_append_printf(track->sdp_description,
                                   "a=fmtp:%u %s",
                                   track->payload_type,
                                   tmpstr);

        tracks = g_list_append(tracks, track);
        continue;

    corrupted_track:
        fnc_log(FNC_LOG_ERR, "[sd2] corrupted track '%s' from '%s'",
                currtrack, mrl);
        track_free(track);
    }

    if ( tracks == NULL )
        goto error;

    r = g_slice_new0(Resource);
    r->mrl = mrl;
    r->lock = g_mutex_new();

    r->source = LIVE_SOURCE;
    r->duration = HUGE_VAL;
    r->tracks = tracks;

    for (tracks = g_list_first(r->tracks); tracks != NULL; tracks = g_list_next(tracks)) {
        Track *track = tracks->data;

        track->parent = r;
        g_thread_create(flux_read_messages, track, false, NULL);
    }

    return r;

 error:
    g_slice_free(Resource, r);
    g_strfreev(trackgroups);
    g_key_file_free(file);
    g_free(mrl);
    return NULL;
}

static gpointer flux_read_messages(gpointer ptr) {
    Track *tr = ptr;
    struct flux_msg *message = NULL;

    while ( tr ) {
        mqd_t queue = (mqd_t)-1;
        struct mq_attr attr;

        struct MParserBuffer *buffer;

        if ( (queue = mq_open(tr->live.mq_path, O_RDONLY, S_IRWXU, NULL)) == (mqd_t)-1) {
            fnc_perror("mq_open");
            goto error;
        }

        if ( mq_getattr(queue, &attr) < 0 ) {
            fnc_perror("mq_getattr");
            goto error;
        }

        message = g_realloc(message, attr.mq_msgsize);

        while ( 1 ) {
            uint32_t package_timestamp;
            double timestamp;
            double delivery;
            double delta;

            ssize_t msg_len;
            int marker;
            uint16_t seq_no;

            if ( (msg_len = mq_receive(queue, (char*)message,
                                       attr.mq_msgsize, NULL)) < 0 ) {
                fnc_log(FNC_LOG_ERR, "Unable to read from '%s', %s",
                        tr->live.mq_path, strerror(errno));

                break;
            }

            if (message->proto_version != REQUIRED_FLUX_PROTOCOL_VERSION) {
                fnc_log(FNC_LOG_FATAL, "[%s] Invalid Flux Protocol Version, expecting %d got %d",
                        tr->live.mq_path, REQUIRED_FLUX_PROTOCOL_VERSION, message->proto_version);
                break;
            }

            /* Don't bother queuing buffers if there are no clients
             * connected, keep reading the messages from the queue
             * though.
             *
             * Note that we don't need to use atomic operations
             * because, even if there are no consumers but we did keep
             * the loop running, we'd just be creating extra objects.
             */
            if ( tr->consumers == 0 )
                continue;

            delta = ev_time() - message->insertion_time;

#if 0
            fprintf(stderr, "[%s] read (%5.4f) BEGIN:%5.4f START_DTS:%u DTS:%u\n",
                    tr->live.mq_path, delta, message->start_time, message->start_dts, message->dts);
#endif

            if (delta > 0.5f) {
                fnc_log(FNC_LOG_INFO, "[%s] late mq packet %f/%f, discarding..",
                        tr->live.mq_path, message->insertion_time, delta);
                continue;
            }

            package_timestamp = ntohl(message->timestamp);
            delivery = (message->dts - message->start_dts)/((double)tr->clock_rate);

            tr->frame_duration = message->duration/((double)tr->clock_rate);
            timestamp = package_timestamp/((double)tr->clock_rate);
            marker = message->marker >> 7;
            seq_no = ntohs(message->seq_no);

            // calculate the duration while consuming stale packets.
            // This is an HACK that must be moved to Flux, here just to quick fix live problems
            if (!tr->frame_duration) {
                if (tr->dts) {
                    tr->frame_duration = (timestamp - tr->dts);
                } else {
                    tr->dts = timestamp;
                }
            }

            buffer = g_slice_new0(struct MParserBuffer);

            buffer->timestamp = timestamp;
            buffer->delivery = message->start_time + delivery;
            buffer->duration = tr->frame_duration * 3;

            buffer->marker = marker;
            buffer->seq_no = seq_no;
            buffer->rtp_timestamp = package_timestamp;

            buffer->data_size = msg_len - sizeof(struct flux_msg);;
            buffer->data = g_memdup(message->data, buffer->data_size);

#if 0
            fprintf(stderr, "[%s] packet TS:%5.4f DELIVERY:%5.4f -> %5.4f (%5.4f)\n",
                    tr->live.mq_path,
                    timestamp,
                    delivery,
                    buffer->delivery,
                    ev_time() - buffer->delivery);
#endif

            track_write(tr, buffer);
        }

    error:
        mq_close(queue);
        queue = (mqd_t)-1;

        usleep(30);
    }

    return NULL;
}
