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
        encoding,                                           \
        type,                                               \
        shortname##_init,                                   \
        shortname##_parse,                                  \
        shortname##_uninit                                  \
    }

typedef struct MediaParser {
    /** Encoding name */
    const char *encoding_name;

    const MediaType media_type;

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

const MediaParser *mparser_find(const char *);

char *extradata2config(MediaProperties *properties);

/**
 * @brief Buffer passed between parsers and RTP sessions
 *
 * This is what is being encapsulated by @ref BufferQueue_Element.
 */
typedef struct {
    double timestamp;   /*!< presentation time of packet */
    uint32_t rtp_timestamp; /*!< RTP version of the presenation time, used only by live */
    double delivery;    /*!< decoding time of packet */
    double duration;    /*!< packet duration */
    gboolean marker;    /*!< marker bit, set if we are sending the last frag */
    uint16_t seq_no;    /*!< Packet sequence number, used only by live */
    size_t data_size;   /*!< packet size */
    uint8_t data[];     /*!< actual packet data */
} MParserBuffer;

void mparser_buffer_write(struct Track *tr,
                          double presentation,
                          double delivery,
                          double duration,
                          gboolean marker,
                          uint8_t *data, size_t data_size);

void mparser_live_buffer_write(struct Track *tr,
                          double presentation,
                          uint32_t rtp_timestamp,
                          double delivery,
                          double duration,
                          uint16_t seq_no,
                          gboolean marker,
                          uint8_t *data, size_t data_size);


#define DEFAULT_MTU 1440

#endif // FN_MEDIAPARSER_H
