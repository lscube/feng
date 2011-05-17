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
#ifndef FN_MEDIAPARSER_H
#define FN_MEDIAPARSER_H

#include <glib.h>
#include <stdint.h>

#include "demuxer.h"

struct Track;

// return errors
#define MP_PKT_TOO_SMALL -101
#define MP_NOT_FULL_FRAME -102

#define FENG_MEDIAPARSER(shortname, encoding, type)         \
    const MediaParser fnc_mediaparser_##shortname = {       \
        shortname##_init,                                   \
        shortname##_parse,                                  \
        shortname##_uninit                                  \
    }

typedef struct MediaParser {
    /**
     * @brief Initialisation callback for the parser
     */
    int (*init)(Track *track);

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
    int (*parse)(Track *track, uint8_t *data, size_t len);

    /**
     * @brief Uninit function to free the private data
     */
    void (*uninit)(Track *track);
} MediaParser;

extern const MediaParser fnc_mediaparser_mpv;
extern const MediaParser fnc_mediaparser_mpa;
extern const MediaParser fnc_mediaparser_h264;
extern const MediaParser fnc_mediaparser_aac;
extern const MediaParser fnc_mediaparser_mp4ves;
extern const MediaParser fnc_mediaparser_vorbis;
extern const MediaParser fnc_mediaparser_theora;
extern const MediaParser fnc_mediaparser_speex;
extern const MediaParser fnc_mediaparser_mp2t;
extern const MediaParser fnc_mediaparser_h263;
extern const MediaParser fnc_mediaparser_amr;
extern const MediaParser fnc_mediaparser_vp8;

char *extradata2config(MediaProperties *properties);

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
    uint16_t seq_no;    /*!< Packet sequence number, used only by live */
    uint32_t rtp_timestamp; /*!< RTP version of the presenation time, used only by live */

    size_t data_size;   /*!< packet size */
    uint8_t *data;      /*!< actual packet data */
};

void mparser_buffer_write(struct Track *tr, struct MParserBuffer *buffer);

#define DEFAULT_MTU 1440

#endif // FN_MEDIAPARSER_H
